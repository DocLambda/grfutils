#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include <termios.h>
#include <fcntl.h>

#include "grf.h"

#include "grf_logging.h"

/*****************************************************************************/
void grf_uart_set_timeout(int fd, unsigned int timeout)
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
    
    grf_logging_dbg("  vmin  = %d...", tty_attr.c_cc[VMIN]);
    grf_logging_dbg("  vtime = %d...", tty_attr.c_cc[VTIME]);

    if (tcsetattr(fd, TCSANOW, &tty_attr))
	    grf_logging_err("Setting timeout of %d to %u failed: %s", fd, timeout, strerror(errno));
}
/*****************************************************************************/

/*****************************************************************************/
int grf_uart_open(const char *dev)
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

int grf_uart_setup(int fd)
{
    struct termios tty_attr;

    grf_logging_info("Setting up UART %d...", fd);

    /* Setup the port for communication. */
    tcgetattr(fd, &tty_attr);
    cfmakeraw(&tty_attr);
    cfsetspeed(&tty_attr, GRF_BAUDRATE);
    /* Set port to blocking */
    tty_attr.c_cc[VMIN]  = 0;
    tty_attr.c_cc[VTIME] = 10;

    /* Actually set the new configuration */
    if (tcsetattr(fd, TCSANOW, &tty_attr))
	return errno;
    
    return 0;
}

void grf_uart_close(int fd)
{
    /* Clear all remaining data on port */
    tcflush(fd, TCIOFLUSH);
    close(fd);
}
/*****************************************************************************/

/*****************************************************************************/
int grf_uart_read_message(int fd, char *message, size_t *len)
{
    char c;
    bool msgstarted = false;
    bool stop       = false;
    int  retval     = ETIMEDOUT;
    
    /* Clear message buffer */
    memset(message, '\0', *len * sizeof(char));
    *len = 0;

    /* Wait for begin of message */
    while (read(fd, &c, 1*sizeof(char)) > 0)
    {
	grf_logging_log(GRF_LOGGING_DEBUG_IO, "read: 0x%02x\n", c);
	
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
                default:
                    grf_logging_err("State invalid (INITIAL and got \'%c\')!\n", c);
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
                    grf_logging_err("State invalid (STARTED and got \'%c\')!\n", c);
		    retval     = EINTR;
		    stop       = true;
                case GRF_NUL:
                case GRF_ACK:
                case GRF_NAK:
                    grf_logging_err("State invalid (STARTED and got \'%c\')!\n", c);
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
	grf_logging_dbg("recv: %s", "NONE");
    else
	grf_logging_dbg_hex(message, *len, "recv: %s (len=%zu)", message, *len);
    
    return retval;
}

int grf_uart_write_message(int fd, const char *message, size_t len)
{
    ssize_t count;
    size_t  written = 0;
    
    grf_logging_dbg_hex(message, len, "send: %s", message);
    
    do {
        count = write(fd, message, len*sizeof(char));
        if (count < 0)
            return errno;
        written += count;
    } while (written < len);

    fsync(fd);

    return 0;
}

int grf_uart_write_ctl(int fd, char ctl)
{
	grf_logging_dbg("sctl: 0x%02x", ctl);
	if (write(fd, &ctl, sizeof(char)) < 0)
		return errno;
	return 0;
}
/*****************************************************************************/
