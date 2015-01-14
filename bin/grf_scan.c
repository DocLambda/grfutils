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

int grf_scan_devices(int fd, int timeout, char **groupid, struct grf_devicelist *devices)
{
	/* Initialize the variables */
	int ret;

	/* Set the timeout for scanning devices */
	grf_uart_set_timeout(fd, timeout);

	/* Scan for group IDs */
	printf("Scanning for group...\n");
	printf( "    Please activate sending the group ID at your smoke detector by\n"
		"    pressing the \"programming\" button until the programming LED\n"
		"    flashes once per second. Afterwards press the smoke detector\n"
		"    button until you hear a beep sound!\n");
	ret = grf_comm_scan_groups(fd, groupid);
	if (ret)
		return ret;

	printf("Found group %s!\n", *groupid);
	sleep(1);

	/* Scan for devices */
	printf("Scanning for devices of group %s...\n", *groupid);
	printf("    Please deactive the programming mode of your smoke detector by\n"
	       "    pressing the \"programming\" button untile the programming LED\n"
	       "    stops flashing and press ENTER!\n");
	getchar();
	sleep(5);

	return grf_comm_scan_devices(fd, *groupid, devices);
}
