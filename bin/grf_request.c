#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <termios.h>

#include <string.h>
#include <errno.h>

#include "grf.h"

#define GRF_TIMEOUT_IO     20 /* [1/10 s] --> 2 seconds */

struct on_exit_args {
    int            fd;
    bool           tty_restore;
    struct termios tty_attr;
};

static struct on_exit_args exit_args;

void on_exit_handler(void)
{
    /* Restore UART settings */
    if (exit_args.tty_restore)
        tcsetattr(exit_args.fd, TCSANOW, &(exit_args.tty_attr));
    
    /* Close UART */
    if (exit_args.fd > 0)
        grf_uart_close(exit_args.fd);
}

void usage(const char *progname)
{
    printf("Usage: %s <device>\n", progname);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    const char          *dev;
    int                  fd               = -1;
    char                *firmware_version = NULL;
    int                  ret;

    /* Parse command-line options */
    if (argc < 2)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    dev = argv[1];

    /* Setup the exit handler */
    exit_args.fd          = -1;
    exit_args.tty_restore = false;
    atexit(on_exit_handler);
    
    /* Open UART and store the current UART setting to later restore them */
    fd = grf_uart_open(dev);
    if (fd < 0)
    {
        fprintf(stderr, "ERROR: Opening device %s failed: %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }
    tcgetattr(fd, &exit_args.tty_attr);
    grf_uart_setup(fd);

    /* Set the timeout for request / response communication */
    grf_uart_set_timeout(fd, GRF_TIMEOUT_IO);

    /* Initialize Gira RF module */
    ret = grf_comm_init(fd, &firmware_version);
    if (ret)
    {
        fprintf(stderr, "ERROR: Initializing device %s failed: %s\n", dev, strerror(ret));
        if (firmware_version)
            free(firmware_version);
        exit(EXIT_FAILURE);
    }
    if (firmware_version)
    {
        printf("Firmware version: %s\n", firmware_version);
        free(firmware_version);
    }
    
    /* TODO: Handle the request */

    /* TODO: Print response to the given request */
    
    /* Close UART */
    grf_uart_close(fd);
    fd = -1;


    exit(EXIT_SUCCESS);
}
