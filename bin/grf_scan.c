/*
 * Scanning command implementation
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

#include <string.h>
#include <errno.h>

#include "grf.h"

#include "grf_logging.h"

int grf_scan_group(struct grf_radio *radio, char **groupid)
{
	/* Initialize the groupid */
	*groupid = NULL;

	/* Scan for group IDs */
	printf("Scanning for group...\n");
	printf( "    Please activate sending the group ID at your smoke detector by\n"
		"    pressing the \"programming\" button until the programming LED\n"
		"    flashes once per second. Afterwards press the smoke detector\n"
		"    button until you hear a beep sound!\n");

	return grf_comm_scan_groups(radio, groupid);
}

int grf_scan_devices(struct grf_radio *radio, const char *groupid, struct grf_devicelist *devices)
{
	/* Scan for devices */
	printf("Scanning for devices of group %s...\n", groupid);

	return grf_comm_scan_devices(radio, groupid, devices);
}
