#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <termios.h>

#include <string.h>
#include <errno.h>

#include "grf.h"

#include "grf_logging.h"

#define GRF_TIMEOUT_IO      20 /* [1/10 s] -->  2 seconds */
#define GRF_TIMEOUT_SCAN  0 /* [1/10 s] --> 60 seconds */

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
    char                *group_ids        = NULL;
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
    
    
    grf_logging_setlevel(GRF_LOGGING_DEBUG);
    
    printf("Initializing communication at %s...\n", dev);

    
    /* Open UART and store the current UART setting to later restore them */
    fd = grf_uart_open(dev);
    if (fd < 0)
    {
        fprintf(stderr, "ERROR: Opening device %s failed: %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }
    tcgetattr(fd, &exit_args.tty_attr);
    ret = grf_uart_setup(fd);
    if (ret)
    {
        fprintf(stderr, "ERROR: Setting up UART device %s failed: %s\n", dev, strerror(ret));
        exit(EXIT_FAILURE);
    }

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
    
    /* Set the timeout for scanning devices */
    grf_uart_set_timeout(fd, GRF_TIMEOUT_SCAN);

    /* Scan for group IDs */
    printf("Scanning group IDs...\n");
    printf("    Please activate sending the group ID at your smoke detector by\n"
           "    pressing the \"programming\" button until the programming LED\n"
	   "    flashes once per second. Afterwards press the smoke detector\n"
	   "    button until you hear a beep sound!\n");
    ret = grf_comm_scan_groups(fd, &group_ids);
    if (ret)
    {
        fprintf(stderr, "ERROR: Scanning group IDs failed: %s\n", strerror(ret));
        if (group_ids)
            free(group_ids);
        exit(EXIT_FAILURE);
    }
    if (!group_ids)
    {
	printf("No group found!\n");
	goto bailout_success;
    }
    
    printf("Found group: %s\n", group_ids);

    /* TODO: Scan for smoke detectors in the groups */

    
bailout_success:    
    /* Free allocated space */
    free(firmware_version);
    free(group_ids);

    exit(EXIT_SUCCESS);
}