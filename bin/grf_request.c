#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <errno.h>

#include "grf.h"
#include "grf_logging.h"

int grf_read_data(int fd, int timeout, const char *deviceid, struct grf_device *device)
{
	/* Initialize the variables */

	/* Set the timeout for scanning devices */
	grf_uart_set_timeout(fd, timeout);

	/* Scan for devices */
	printf("Requesting data of device %s...\n", deviceid);

	return grf_comm_read_data(fd, deviceid, device);
}
