/*
 * Radio module interface
 *
 * This file is part of the grfutils project.
 *
 * Copyright (c) 2014-2015 Sven Rebhan <odinshorse@googlemail.com>
 *
 * grfutils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * grfutils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with grfutils.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <termios.h>
#include <fcntl.h>

#include "grf.h"
#include "grf_radio.h"
#include "grf_logging.h"

/*---------------------------------------------------------------------------*/
static int grf_uart_set_timeout(int fd, cc_t timeout)
{
	assert(fd >= 0);

	struct termios tty_attr;

	grf_logging_info("Setting timeout of %d to %.1f seconds...", fd, timeout/10.0f);

	/* In case no timeout is given, we always block to retrieve at
	 * least one character.
	 */
	if (tcgetattr(fd, &tty_attr))
	{
		grf_logging_err("Getting TTY attributes of %d failed: %s", fd, strerror(errno));
		return errno;
	}

	if (timeout == 0)
		tty_attr.c_cc[VMIN] = 1;
	else
		tty_attr.c_cc[VMIN] = 0;

	tty_attr.c_cc[VTIME] = timeout;

	grf_logging_dbg("  vmin  = %u...", tty_attr.c_cc[VMIN]);
	grf_logging_dbg("  vtime = %u...", tty_attr.c_cc[VTIME]);

	if (tcsetattr(fd, TCSANOW, &tty_attr))
	{
		grf_logging_err("Setting timeout of %d to %u failed: %s", fd, timeout, strerror(errno));
		return errno;
	}

	/* Make sure settings are actually transmitted */
	if (tcdrain(fd))
	{
		grf_logging_err("Draining data of TTY %d failed: %s", fd, strerror(errno));
		return errno;
	}

	return 0;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static int grf_uart_open(const char *dev)
{
	assert(dev);

	int fd;

	/* Open the device for read and write and prevent it from
	 * becoming a control TTY.
	 */
	grf_logging_info("Opening %s...", dev);
	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0)
		return fd;

	grf_logging_dbg("    got fd=%d...", fd);

	/* Make sure the given device is a tty. */
	if (!isatty(fd))
	{
		errno = ENOTTY;
		return -1;
	}

	return fd;
}

static int grf_uart_setup(int fd)
{
	assert(fd >= 0);

	struct termios tty_attr;

	grf_logging_info("Setting up UART %d...", fd);

	/* Setup the port for communication. */
	if (tcgetattr(fd, &tty_attr))
	{
		grf_logging_err("Getting TTY attributes of %d failed: %s", fd, strerror(errno));
		return errno;
	}

	if (tcflush(fd, TCIOFLUSH))
	{
		grf_logging_err("Flushing of %d failed: %s", fd, strerror(errno));
		return errno;
	}

	if (cfsetspeed(&tty_attr, GRF_BAUDRATE))
	{
		grf_logging_err("Setting speed of TTY %d failed: %s", fd, strerror(errno));
		return errno;
	}

	cfmakeraw(&tty_attr);
	if (tcflush(fd, TCIOFLUSH))
	{
		grf_logging_err("Flushing of %d failed: %s", fd, strerror(errno));
		return errno;
	}

	/* Set port to blocking */
	tty_attr.c_cc[VMIN]  = 0;
	tty_attr.c_cc[VTIME] = 10;

	/* Actually set the new configuration */
	if (tcsetattr(fd, TCSANOW, &tty_attr))
	{
		grf_logging_err("Setting TTY attributes of %d failed: %s", fd, strerror(errno));
		return errno;
	}

	/* Make sure settings are actually transmitted */
	return tcdrain(fd);
}

static void grf_uart_close(int fd)
{
	assert(fd >= 0);

	/* Clear all remaining data on port */
	tcflush(fd, TCIOFLUSH);
	close(fd);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
int grf_radio_init(struct grf_radio *radio, const char *dev, unsigned int timeout)
{
	assert(radio);
	assert(dev);

	uint32_t t_user = timeout * 10;
	uint32_t i;
	int      ret;

	grf_logging_info("Initializing device %s", dev);

	/* Reset the radio structure */
	memset(radio, 0, sizeof(struct grf_radio));
	radio->is_initialized = false;

	/* Copy the given data to the radio */
	radio->dev          = strdup(dev);
	radio->timeout_user = t_user;
	if (!radio->dev)
		return ENOMEM;

	/* The TTY layer handles the timeout as unsigned char, thus limiting the
	 * maximal timeout to 25.5 seconds. We sometimes, especially during
	 * read-out of devices, require longer timeouts up to 60 seconds.
	 * Therefore, we have to split the timeout into multiple smaller ones
	 * and repeat reading.
	 */
	if (timeout <= 25)
	{
		radio->timeout_tty     = t_user;
		radio->timeout_repeats = 1;
	}
	else
	{
		/* Calculate the greatest common divisor between the user specified
		 * timeout and the maximum timeout of the TTY layer (255). We use
		 * trial divisions in this case due to the limited number of trials.
		 * NOTE: This loop is quaranteed to end latest at i=10 due to the
		 *       multiplication t_user = timeout * 10.
		 */
		i = 256;
		while (t_user % --i != 0);
		radio->timeout_tty     = i;
		radio->timeout_repeats = t_user / i;
	}
	grf_logging_dbg("init: timeout %u 1/10s --> %u 1/10s * %u", t_user, radio->timeout_tty, radio->timeout_repeats);

	/* Open UART and store the current UART setting to later restore them */
	radio->fd = grf_uart_open(dev);
	if (radio->fd < 0)
	{
		grf_logging_err("Opening radio device %s failed: %s", dev, strerror(errno));
		return errno;
	}
	if (tcgetattr(radio->fd, &radio->tty_attr_saved))
	{
		grf_logging_err("Getting TTY attributes of radio device %s failed: %s", dev, strerror(errno));
		return errno;
	}
	radio->is_initialized = true;

	/* Setup the port settings to allow communication with the radio. */
	memcpy(&radio->tty_attr, &radio->tty_attr_saved, sizeof(struct termios));
	ret = grf_uart_setup(radio->fd);
	if (ret)
	{
		grf_logging_err("Setting up radio device %s failed: %s", dev, strerror(ret));
		return ret;
	}

	/* Set the timeout for request / response communication */
	return grf_uart_set_timeout(radio->fd, radio->timeout_tty);
}

int grf_radio_exit(struct grf_radio *radio)
{
	assert(radio);

	if (!radio->is_initialized)
		return 0;

	grf_logging_info("Closing communication at device %s", radio->dev);

	if (radio->fd >= 0)
	{
		/* Clean all remaining data on the device */
		tcflush(radio->fd, TCIOFLUSH);

		/* Restore UART settings */
		tcsetattr(radio->fd, TCSANOW, &radio->tty_attr_saved);

		/* Close UART */
		grf_uart_close(radio->fd);
	}

	/* Free the device name */
	if (radio->dev)
		free(radio->dev);

	/* Reset the radio structure */
	memset(radio, 0, sizeof(struct grf_radio));
	radio->is_initialized = false;

	return 0;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
int grf_radio_read(struct grf_radio *radio, char *message, size_t *len, size_t size)
{
	assert(grf_radio_is_valid(radio));
	assert(message);
	assert(len);
	assert(size > 0);

	char    c;
	bool    msgstarted = false;
	bool    stop       = false;
	int     repeats    = radio->timeout_repeats;
	ssize_t count;
	int     retval     = ETIMEDOUT;

	/* Clear message buffer */
	memset(message, '\0', size * sizeof(char));
	*len = 0;

	/* Wait for data and respect the retries calculated to arrive at the
	 * user specified timeout.
	 */
	errno = 0;
	while ((count = read(radio->fd, &c, 1*sizeof(char))) > 0 || (!errno && --repeats > 0))
	{
		if (count < 1)
		{
			grf_logging_dbg("read: No data received. Retrying %d more time(s)...", repeats);
			continue;
		}
		grf_logging_log(GRF_LOGGING_DEBUG_IO, "read: 0x%02x", c);

		/* Maintain state machine for parsing */
		if (!msgstarted)
		{
			/* We either expect a control character such as ACK/NAK or
			 * a begin-of-message tag. All other characters are treated
			 * as errors!
			 */
			switch(c)
			{
				case GRF_NUL:
				case GRF_ACK:
				case GRF_NAK:
					message[0] = c;
					*len       = 1;
					retval     = 0;
					stop       = true;
					break;
				case GRF_STX:
					message[0] = c;
					*len       = 1;
					msgstarted = true;
					break;
				case GRF_CONT:
					/* Just digest the continuation of the
					 * previous message.
					 */
					break;
				default:
					grf_logging_err("State invalid (INITIAL and got x%02x)!", c);
					retval     = EINVAL;
					stop       = true;
					break;
			}
		}
		else	/* msgstarted */
		{
			/* We either expect a data character or and end-of-message tag.
			 * All other characters are treated as errors!
			 */
			switch(c)
			{
				case GRF_ETX:
					message[*len] = c;
					*len += 1;
					retval     = 0;
					stop       = true;
					break;
				case GRF_STX:
					grf_logging_warn("Missed ETX! (STARTED and got x%02x)!", c);
					grf_logging_warn_hex(message, *len, "Incomplete message was: %s", message);
					message[0] = c;
					*len       = 1;
					break;
				case GRF_NUL:
				case GRF_ACK:
				case GRF_NAK:
					grf_logging_err("State invalid (STARTED and got x%02x)!", c);
					retval     = EINVAL;
					stop       = true;
					break;
				default:
					message[*len]   = c;
					*len += 1;
					break;
			}
		}

		/* Check if we exceed the message buffer size */
		if (*len >= size)
		{
			retval = EMSGSIZE;
			stop   = true;
		}

		/* Stop if requested by the state machine */
		if (stop)
			break;
	}

	if (*len < 1)
	{
		grf_logging_dbg("recv: %s", "Timeout! No data received.");
		retval = ETIMEDOUT;
	}
	else
	{
		grf_logging_dbg_hex(message, *len, "recv: %s (len=%zu)", message, *len);
	}

	return retval;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
int grf_radio_write(struct grf_radio *radio, const char *message, size_t len)
{
	assert(grf_radio_is_valid(radio));
	assert(message);

	int     repeats = radio->timeout_repeats;
	ssize_t count;

	grf_logging_dbg_hex(message, len, "send: %s", message);

	errno = 0;
	while(len > 0) {
		count = write(radio->fd, message, len*sizeof(char));
		if (count < 0)
		{
			return errno;
		}
		else if (count == 0)
		{
			if (--repeats > 0)
			{
				grf_logging_dbg("write: No data written. Retrying %d more time(s)...", repeats);
				continue;
			}
			else
			{
				return ETIMEDOUT;
			}
		}
		message += count;
		len     -= count;
	}

	if (tcflush(radio->fd, TCOFLUSH))
	{
		grf_logging_err("Calling tcflush on %d failed: %s", radio->fd, strerror(errno));
		return errno;
	}

	return 0;
}

int grf_radio_write_ctrl(struct grf_radio *radio, char ctrl)
{
	assert(grf_radio_is_valid(radio));

	grf_logging_dbg("sctl: 0x%02x", ctrl);
	if (write(radio->fd, &ctrl, sizeof(char)) < 0)
		return errno;

	if (tcflush(radio->fd, TCOFLUSH))
	{
		grf_logging_err("Calling tcflush on %d failed: %s", radio->fd, strerror(errno));
		return errno;
	}

	return 0;
}
/*---------------------------------------------------------------------------*/
