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


int grf_read_data(int fd, const char *deviceid, struct grf_device *device)
{
	/* Scan for devices */
	printf("Requesting data of device %s...\n", deviceid);

	return grf_comm_read_data(fd, deviceid, device);
}
