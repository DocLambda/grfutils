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
#define GRF_DEFAULT_TIMEOUT	600 /* [1/10 s] --> 60 seconds */
#define GRF_DEFAULT_LOGLEVEL	GRF_LOGGING_WARN

#define GRF_TIMEOUT_SCAN    	0 /* [1/10 s] --> infinite  */

static void on_exit_handler(void);

extern int grf_scan_group(int fd, int timeout, char **groupid);
extern int grf_scan_devices(int fd, int timeout, char **groupid, struct grf_devicelist *devices);

struct ctlparams {
	char          *dev;
	int            fd;
	bool           tty_restore;
	struct termios tty_attr;
};

static struct ctlparams params;

static int init_device(const char *dev, int timeout, char **firmware_version)
{
	int fd = -1;
	int ret;
	
	printf("Initializing communication at %s...\n", dev);

	/* Setup the exit handler */
	params.dev         = (char *) dev;
	params.fd          = -1;
	params.tty_restore = false;
	atexit(on_exit_handler);
	
	/* Open UART and store the current UART setting to later restore them */
	fd = grf_uart_open(dev);
	if (fd < 0)
	{
		fprintf(stderr, "ERROR: Opening device %s failed: %s\n", dev, strerror(errno));
		exit(EXIT_FAILURE);
	}
	params.fd = fd;
	tcgetattr(fd, &params.tty_attr);
	params.tty_restore = true;
	ret = grf_uart_setup(fd);
	if (ret)
	{
		fprintf(stderr, "ERROR: Setting up UART device %s failed: %s\n", dev, strerror(ret));
		exit(EXIT_FAILURE);
	}

	/* Set the timeout for request / response communication */
	grf_uart_set_timeout(fd, timeout);

	/* Initialize Gira RF module */
	ret = grf_comm_init(fd, firmware_version);
	if (ret)
	{
		fprintf(stderr, "ERROR: Initializing device %s failed: %s\n", dev, strerror(ret));
		if (firmware_version)
			free(firmware_version);
		exit(EXIT_FAILURE);
	}
	
	return fd;
}

static void destroy_device(void)
{    
	printf("Closing communication at device %s...\n", params.dev);

	/* Free the device name */
	if (params.dev)
		free(params.dev);

	/* Restore UART settings */
	if (params.tty_restore)
		tcsetattr(params.fd, TCSANOW, &(params.tty_attr));
	
	/* Close UART */
	if (params.fd > 0)
		grf_uart_close(params.fd);
	
	/* Make the exit handler do nothing */
	params.fd          = -1;
	params.tty_restore = false;
}

static void on_exit_handler(void)
{
	destroy_device();
}

static void usage(const char *progname)
{
	printf("Usage: %s [options] <command> [command arguments]\n", progname);
	printf("\n");
	printf("  options:\n"
		"    -d  --device <device>                    use the given device (default: %s)\n"
		"    -t  --timeout <timeout>                  use the timeout while executing the command (default: %d)\n"
		"    -v  --verbose <level>                    set debug level to one of {error, warn, info, debug, debugio}\n"
		"    -h  --help                               show this help\n",
		GRF_DEFAULT_DEVICE, GRF_DEFAULT_TIMEOUT
		);
	printf("\n");
	printf("  commands:\n"
		"    show-version                             show the program version\n"
		"    show-firmware-version                    show the firmware version of the device\n"
		"    scan-groups                              scan for detector groups\n"
		"    scan-devices                             scan for all devices in a group\n"
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
				printf("Using a %.1f second timeout...\n", 0.1*timeout);
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

	/* Parse the command and do the processing */
	cmd = argv[optind];
	if(strcasecmp(cmd, "show-version") == 0)
	{
		printf("grfctl version %s\n", GRF_VERSION);
	} else if(strcasecmp(cmd, "show-firmware-version") == 0)
	{
		char *firmware_version = NULL;
		
		init_device(dev, timeout, &firmware_version);
		if (firmware_version)
			printf("firmware version %s\n", firmware_version);
		free(firmware_version);
		destroy_device();
	} else if(strcasecmp(cmd, "scan-groups") == 0)
	{
		char *groupid;
		
		init_device(dev, timeout, NULL);
		ret = grf_scan_group(params.fd, GRF_TIMEOUT_SCAN, &groupid);
		if (ret)
		{
			fprintf(stderr, "ERROR: Scanning group IDs failed: %s\n", strerror(ret));
			exit(EXIT_FAILURE);
		}
		destroy_device();

		/* Output the result of the scan */
		if (!groupid)
			printf("No group found!\n");

		printf("Found the following groups:\n");
		printf("    %s\n", groupid);
	} else if(strcasecmp(cmd, "scan-devices") == 0)
	{
		char                  *groupid = NULL;
		struct grf_devicelist  devices;
		int                    i;
		
		init_device(dev, timeout, NULL);
		ret = grf_scan_devices(params.fd, GRF_TIMEOUT_SCAN, groupid, &devices);
		if (ret)
		{
			if (!groupid)
				fprintf(stderr, "ERROR: Scanning for group failed: %s\n", strerror(ret));
			else
				fprintf(stderr, "ERROR: Scanning devices of group %s failed: %s\n", groupid, strerror(ret));
				exit(EXIT_FAILURE);
		}
		destroy_device();

		/* Output the result of the scan */
		if (devices.len < 1)
			printf("No devices found!\n");

		printf("Found the %d devices in group %s:\n", devices.len, groupid);
		for (i = 0; i < devices.len; i++)
		{
			printf("    %s\n", devices.devices[i].id);
		}
	} else {
		fprintf(stderr, "Unknown command \"%s\"\n", cmd);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
