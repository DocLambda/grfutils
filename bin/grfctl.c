/*
 * Tool for communicating with smoke detectors from command line
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

#include <getopt.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

#include "grf.h"

#include "grf_logging.h"

#define GRF_VERSION         	"0.1.0"
#define GRF_DEFAULT_DEVICE	"/dev/ttyUSB0"
#define GRF_DEFAULT_TIMEOUT	60 /* seconds */
#define GRF_DEFAULT_LOGLEVEL	GRF_LOGGING_WARN

static void on_exit_handler(void);

extern int grf_scan_group(struct grf_radio *radio, char **groupid);
extern int grf_scan_devices(struct grf_radio *radio, const char *groupid, struct grf_devicelist *devices);
extern int grf_read_data(struct grf_radio *radio, const char *deviceid, struct grf_device *device);
extern void grf_print_data(struct grf_device *device);
extern int grf_switch_signal(struct grf_radio *radio, const char *deviceid, bool on);

static struct grf_radio radio;

static void on_exit_handler(void)
{
	grf_radio_exit(&radio);
}

static void usage(const char *progname)
{
	printf("Usage: %s [options] <command> [command arguments]\n", progname);
	printf("\n");
	printf("  options:\n"
		"    -d  --device <device>                    use the given device (default: %s)\n"
		"    -t  --timeout <timeout>                  use the timeout in seconds while executing the command (default: %d)\n"
		"    -v  --verbose <level>                    set debug level to one of {error, warn, info, debug, debugio}\n"
		"    -h  --help                               show this help\n",
		GRF_DEFAULT_DEVICE, GRF_DEFAULT_TIMEOUT
		);
	printf("\n");
	printf("  commands:\n"
		"    show-version                             show the program version\n"
		"    show-firmware-version                    show the firmware version of the device\n"
		"    scan-groups                              scan for detector groups\n"
		"    scan-devices <group>                     scan for all devices in the given group\n"
		"    request-data <device>                    read the data of the given device\n"
		"    activate-signal <device>                 activate the accustic signal of the given device\n"
		"    deactivate-signal <device>               deactivate the accustic signal of the given device\n"
		);
	printf("\n");
	
	fflush(stdout);
}

static const char *get_cmd_param(char **argv, int argc, int oidx)
{
	if (argc - optind < 2)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	return argv[oidx+1];
}

int main(int argc, char **argv)
{
	const char    *cmd;
	char          *dev = strdup(GRF_DEFAULT_DEVICE);
	int            timeout = GRF_DEFAULT_TIMEOUT;
	int            loglevel = GRF_DEFAULT_LOGLEVEL;
	int            index;
	int            ret;
	char           c;

	static struct option options[] =
	{
		{"device",  required_argument, 0, 'd'},
		{"timeout", required_argument, 0, 't'},
		{"verbose", required_argument, 0, 'v'},
		{"help",    no_argument,       0, 'h'},
		{0, 0, 0, 0}
	};

	/* Parse the command line options */
	while ((c = getopt_long(argc, argv, "d:t:v:h", options, &index)) > -1)
	{
		switch (c)
		{
			case 'd':
				dev = strdup(optarg);
				printf("Using device %s...\n", dev);
				break;
			case 't':
				timeout = atoi(optarg);
				printf("Using a %u second timeout...\n", timeout);
				break;
			case 'v':
				printf("Using log-level %s...\n", optarg);
				if (strcmp(optarg, "error") == 0)
				{
					loglevel = GRF_LOGGING_ERR;
				} else if (strcmp(optarg, "warn") == 0)
				{
					loglevel = GRF_LOGGING_WARN;
				} else if (strcmp(optarg, "info") == 0)
				{
					loglevel = GRF_LOGGING_INFO;
				} else if (strcmp(optarg, "debug") == 0)
				{
					loglevel = GRF_LOGGING_DEBUG;
				} else if (strcmp(optarg, "debugio") == 0)
				{
					loglevel = GRF_LOGGING_DEBUG_IO;
				} else {
					fprintf(stderr, "Unknown log-level %s!\n", optarg);
					fprintf(stderr, "Use one of the following levels: error, warn, info, debug, debugio.\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'h':
				usage(argv[0]);
				exit(EXIT_SUCCESS);
			default:
				fprintf(stderr, "Unknown option -%c!\n", c);
				exit(EXIT_FAILURE);
		}
	}

	/* Check the command line parameters */
	if (argc - optind < 1)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	
	/* Adjust the log-level to the given level */
	grf_logging_setlevel(loglevel);

	/* Parse the command and handle all commands that do not require the radio to be set up */
	cmd = argv[optind];
	if(strcasecmp(cmd, "show-version") == 0)
	{
		printf("grfctl version %s\n", GRF_VERSION);
		exit(EXIT_SUCCESS);
	}

	/* Setup the radio and register the on exit handler in case we die suddenly */
	memset(&radio, 0, sizeof(struct grf_radio));
	radio.is_initialized = false;
	atexit(on_exit_handler);
	ret = grf_radio_init(&radio, dev, timeout);
	if (ret)
	{
		fprintf(stderr, "ERROR: Initialization of radio device failed: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	/* Initialize Gira RF module */
	ret = grf_comm_init(&radio);
	if (ret)
	{
		fprintf(stderr, "ERROR: Initializing communication failed: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	/* Parse the remaining command that require the radio */
	if(strcasecmp(cmd, "show-firmware-version") == 0)
	{
		printf("Firmware version: %s\n", radio.firmware_version);
	}
	else if(strcasecmp(cmd, "scan-groups") == 0)
	{
		char *groupid;
		
		ret = grf_scan_group(&radio, &groupid);
		if (ret)
		{
			fprintf(stderr, "ERROR: Scanning group IDs failed: %s\n", strerror(ret));
			exit(EXIT_FAILURE);
		}

		/* Output the result of the scan */
		if (!groupid)
			printf("No group found!\n");

		printf("Found the following groups:\n");
		printf("    %s\n", groupid);
	}
	else if(strcasecmp(cmd, "scan-devices") == 0)
	{
		const char            *groupid = get_cmd_param(argv, argc, optind);
		struct grf_devicelist  devices;
		int                    i;
		
		ret = grf_scan_devices(&radio, groupid, &devices);
		if (ret)
		{
			fprintf(stderr, "ERROR: Scanning devices of group %s failed: %s\n", groupid, strerror(ret));
			exit(EXIT_FAILURE);
		}

		/* Output the result of the scan */
		if (devices.len < 1)
			printf("No devices found!\n");

		printf("Found %d devices in group %s:\n", devices.len, groupid);
		for (i = 0; i < devices.len; i++)
		{
			printf("    %s\n", devices.devices[i].id);
		}
	}
	else if(strcasecmp(cmd, "request-data") == 0)
	{
		const char            *deviceid = get_cmd_param(argv, argc, optind);
		struct grf_device      device;

		ret = grf_read_data(&radio, deviceid, &device);
		if (ret)
		{
			fprintf(stderr, "ERROR: Requesting data of device %s failed: %s\n", deviceid, strerror(ret));
			exit(EXIT_FAILURE);
		}

		/* Output the result of the request */
		printf("Data of %s:\n", deviceid);
		grf_print_data(&device);
	}
	else if(strcasecmp(cmd, "activate-signal") == 0)
	{
		const char *deviceid = get_cmd_param(argv, argc, optind);

		ret = grf_switch_signal(&radio, deviceid, true);
		if (ret)
		{
			fprintf(stderr, "ERROR: Activating signal of device %s failed: %s\n", deviceid, strerror(ret));
			exit(EXIT_FAILURE);
		}
	}
	else if(strcasecmp(cmd, "deactivate-signal") == 0)
	{
		const char *deviceid = get_cmd_param(argv, argc, optind);

		ret = grf_switch_signal(&radio, deviceid, false);
		if (ret)
		{
			fprintf(stderr, "ERROR: Deactivating signal of device %s failed: %s\n", deviceid, strerror(ret));
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		fprintf(stderr, "Unknown command \"%s\"\n", cmd);
		exit(EXIT_FAILURE);
	}

	/* Close the radio */
	ret = grf_radio_exit(&radio);
	if (ret)
	{
		fprintf(stderr, "ERROR: Closing the radio device failed: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
