/*
 * Data reading command implementation
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

#include <string.h>
#include <errno.h>

#include <math.h>

#include "grf.h"
#include "grf_logging.h"

void grf_print_data(struct grf_device *device)
{
	int i;

	printf("--------------------------------------------\n");
	printf("    serial number:               %08X\n", device->serial_number);
	printf("    operation time:              %d days %d hours %d minutes %.2f seconds\n",
	       (int)(device->operation_time / (24.0f * 3600.0f)),
	       (int)(fmodf(device->operation_time, 24.0f * 3600.0f) / 3600.0f),
	       (int)(fmodf(device->operation_time, 3600.0f) / 60.0f),
	       fmodf(device->operation_time, 60.0f));
	printf("    smoke chamber pollution:     %d\n", device->smoke_chamber_pollution);
	printf("    battery voltage:             %.2f V\n", device->battery_voltage);
	printf("    temperature 1:               %.1f degree celcius\n", device->temperature1);
	printf("    temperature 2:               %.1f degree celcius\n", device->temperature2);
	printf("    local smoke alerts:          %d\n", device->local_smoke_alerts);
	printf("    local temperature alerts:    %d\n", device->local_temperature_alerts);
	printf("    remote wired alerts:         %d\n", device->remote_cable_alerts);
	printf("    remote wireless alerts:      %d\n", device->remote_radio_alerts);
	printf("    local test alerts:           %d\n", device->local_test_alerts);
	printf("    remote wired test alerts:    %d\n", device->remote_cable_test_alerts);
	printf("    remote wireless test alerts: %d\n", device->remote_radio_test_alerts);
	printf("--------------------------------------------\n");
	printf("    unknown smoke chamber value: %d\n", device->smoke_chamber_value);
	printf("    unknown data id=0002:        0x%08X\n", device->unknown_02);
	for (i = 0; i < 40; i++)
	{
		printf("    unknown data id=%04X:        0x%08X\n", i+0x14, device->unknown_registers[i]);
	}
	printf("    unknown data id=0064:        0x%08X\n", device->unknown_64);
	printf("--------------------------------------------\n");
}


int grf_read_data(struct grf_radio *radio, const char *deviceid, struct grf_device *device)
{
	/* Request data from devices */
	printf("Requesting data of device %s...\n", deviceid);

	return grf_comm_read_data(radio, deviceid, device);
}

int grf_switch_signal(struct grf_radio *radio, const char *deviceid, bool on)
{
	/* Request switching accustic signal on or off */
	printf("Switching signal of %s to %s...\n", deviceid, on ? "on" : "off");

	return grf_comm_switch_signal(radio, deviceid, on);
}
