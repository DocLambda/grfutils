#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>

#include "grf.h"

#include "grf_logging.h"

int grf_scan_group(int fd, int timeout, char **groupid)
{
	/* Initialize the groupid */
	*groupid = NULL;

	/* Set the timeout for scanning devices */
	grf_uart_set_timeout(fd, timeout);

	/* Scan for group IDs */
	printf("Scanning for group...\n");
	printf( "    Please activate sending the group ID at your smoke detector by\n"
		"    pressing the \"programming\" button until the programming LED\n"
		"    flashes once per second. Afterwards press the smoke detector\n"
		"    button until you hear a beep sound!\n");

	return grf_comm_scan_groups(fd, groupid);
}

int grf_scan_devices(int fd, int timeout, const char *groupid, struct grf_devicelist *devices)
{
	/* Initialize the variables */

	/* Set the timeout for scanning devices */
	grf_uart_set_timeout(fd, timeout);

	/* Scan for devices */
	printf("Scanning for devices of group %s...\n", groupid);

	return grf_comm_scan_devices(fd, groupid, devices);
}
