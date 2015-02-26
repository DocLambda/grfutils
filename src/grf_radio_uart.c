#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include <termios.h>
#include <fcntl.h>

#include "grf.h"
#include "grf_radio.h"
#include "grf_logging.h"

/*****************************************************************************/
static void grf_uart_set_timeout(int fd, cc_t timeout)
{
	struct termios tty_attr;

	grf_logging_info("Setting timeout of %d to %.1f seconds...", fd, timeout/10.0f);

	/* In case no timeout is given, we always block to retrieve at
	 * least one character.
	 */
	tcgetattr(fd, &tty_attr);
	if (timeout == 0)
		tty_attr.c_cc[VMIN] = 1;
	else
		tty_attr.c_cc[VMIN] = 0;

	tty_attr.c_cc[VTIME] = timeout;

	grf_logging_dbg("  vmin  = %u...", tty_attr.c_cc[VMIN]);
	grf_logging_dbg("  vtime = %u...", tty_attr.c_cc[VTIME]);

	if (tcsetattr(fd, TCSANOW, &tty_attr))
		grf_logging_err("Setting timeout of %d to %u failed: %s", fd, timeout, strerror(errno));

	/* Make sure settings are actually transmitted */
	tcdrain(fd);
}
/*****************************************************************************/

/*****************************************************************************/
static int grf_uart_open(const char *dev)
{
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
	struct termios tty_attr;

	grf_logging_info("Setting up UART %d...", fd);

	/* Setup the port for communication. */
	tcgetattr(fd, &tty_attr);
	tcflush(fd, TCIOFLUSH);
	cfsetspeed(&tty_attr, GRF_BAUDRATE);
	cfmakeraw(&tty_attr);
	tcflush(fd, TCIOFLUSH);

	/* Set port to blocking */
	tty_attr.c_cc[VMIN]  = 0;
	tty_attr.c_cc[VTIME] = 10;

	/* Actually set the new configuration */
	if (tcsetattr(fd, TCSANOW, &tty_attr))
		return errno;

	/* Make sure settings are actually transmitted */
	return tcdrain(fd);
}

static void grf_uart_close(int fd)
{
	/* Clear all remaining data on port */
	tcflush(fd, TCIOFLUSH);
	close(fd);
}
/*****************************************************************************/

/*****************************************************************************/
int grf_radio_init(struct grf_radio *radio, const char *dev, unsigned int timeout)
{
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
	tcgetattr(radio->fd, &radio->tty_attr_saved);
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
	grf_uart_set_timeout(radio->fd, radio->timeout_tty);

	return 0;
}

int grf_radio_exit(struct grf_radio *radio)
{
	if (!radio->is_initialized)
		return 0;

	grf_logging_info("Closing communication at device %s", radio->dev);

	/* Clean all remaining data on the device */
	tcflush(radio->fd, TCIOFLUSH);

	/* Restore UART settings */
	tcsetattr(radio->fd, TCSANOW, &radio->tty_attr_saved);

	/* Close UART */
	if (radio->fd > 0)
		grf_uart_close(radio->fd);

	/* Free the device name */
	if (radio->dev)
		free(radio->dev);

	/* Reset the device structure */
	/* Reset the radio structure */
	memset(radio, 0, sizeof(struct grf_radio));
	radio->is_initialized = false;

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_radio_read(struct grf_radio *radio, char *message, size_t *len)
{
	char    c;
	bool    msgstarted = false;
	bool    stop       = false;
	int     repeats    = radio->timeout_repeats;
	ssize_t count;
	int     retval     = ETIMEDOUT;

	/* Clear message buffer */
	/* FIXME: Use a common define for 255 in both uart and caller side. */
	memset(message, '\0', 255 * sizeof(char));
	*len = 0;

	/* Wait for data and respect the retries calculated to arrive at the
	 * user specified timeout.
	 */
	errno = 0;
	while ((count = read(radio->fd, &c, 1*sizeof(char))) > 0 || (!errno && --repeats > 0))
	{
		if (count < 1)
		{
			grf_logging_log(GRF_LOGGING_DEBUG, "read: No data received. Retrying %d more time(s)...", repeats);
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
					grf_logging_err("State invalid (INITIAL and got \'%c\')!", c);
					retval     = EINVAL;
					stop       = true;
					break;
			}
		}
		else
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
					grf_logging_err("State invalid (STARTED and got \'%c\')!", c);
					retval     = EINTR;
					stop       = true;
					break;
				case GRF_NUL:
				case GRF_ACK:
				case GRF_NAK:
					grf_logging_err("State invalid (STARTED and got \'%c\')!", c);
					retval     = EINVAL;
					stop       = true;
					break;
				default:
					message[*len]   = c;
					*len += 1;
					break;
			}
		}

		if (stop)
			break;
	}
	
	if (*len < 1)
	{
		grf_logging_dbg("recv: %s", "No data received!");
		retval = errno;
	}
	else
		grf_logging_dbg_hex(message, *len, "recv: %s (len=%zu)", message, *len);

	return retval;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_radio_write(struct grf_radio *radio, const char *message, size_t len)
{
	ssize_t count;
	size_t  written = 0;

	grf_logging_dbg_hex(message, len, "send: %s", message);

	do {
		count = write(radio->fd, message, len*sizeof(char));
		if (count < 0)
			return errno;
		written += count;
	} while (written < len);

	fsync(radio->fd);

	return 0;
}

int grf_radio_write_ctrl(struct grf_radio *radio, char ctrl)
{
	grf_logging_dbg("sctl: 0x%02x", ctrl);
	if (write(radio->fd, &ctrl, sizeof(char)) < 0)
		return errno;

	fsync(radio->fd);

	return 0;
}
/*****************************************************************************/
