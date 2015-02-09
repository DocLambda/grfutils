#ifndef __GRF_H__
#define __GRF_H__

#include <time.h>
#include <stdint.h>

#define RETURN_ON_ERROR(__func__) \
{\
    int __retval__ = (__func__);\
    if ((__retval__)) return (__retval__); \
}

#define GRF_MAXDEVICES          40                  /* Maximum number of devices in one group */

#define GRF_BAUDRATE            B9600

#define GRF_NUL                 0x00
#define GRF_STX                 0x02
#define GRF_ETX                 0x03
#define GRF_CONT                0x0a
#define GRF_ACK                 0x06
#define GRF_NAK                 0x15

#define GRF_INIT_TEST           "%c01TESTA1%c"      /* Set RF module to command mode */
#define GRF_INIT_SV             "%cSV%c"            /* Request sending the firmware version */
#define GRF_SCAN_GA             "%cGA%c"            /* Request scanning group adress */
#define GRF_SCAN_GD             "%cGD:%s%c"         /* Request scanning of all devices of a group adress */
#define GRF_REQUEST_DA_TMPL     "%cDA:%s:%02d%c"    /* Template for data acquisition request */
#define GRF_DA_TYPE_START       5                   /* Request starting data acquisition ??? */
#define GRF_DA_TYPE_03          3                   /* Request ??? */
#define GRF_DA_TYPE_06          6                   /* Request ??? */
#define GRF_DA_TYPE_SEND        1                   /* Request sending the aquired data ??? */
#define GRF_DA_TYPE_STOP        4                   /* Request stopping data acquisition ??? */
#define GRF_REQUEST_DIAG        "%cSD:%s%c"         /* Request sending diagnosis data */

#define GRF_ANSWER_TIMEOUT      "Timeout"           /* Also used for end of transmission ??? */
#define GRF_ANSWER_DONE         "Done"              /* Expected answer to indicate completion of command */
#define GRF_ANSWER_REC          "REC"               /* Expected to indicate that data recording is in process ??? */
#define GRF_ANSWER_VERSION      "GI_RM_V00.70"      /* Expected version string */

#define GRF_DATATYPE_ERROR      -1
#define GRF_DATATYPE_CONTROL     0
#define GRF_DATATYPE_ACK         1
#define GRF_DATATYPE_DATA       10
#define GRF_DATATYPE_VERSION    11
#define GRF_DATATYPE_REC        12
#define GRF_DATATYPE_DONE       13
#define GRF_DATATYPE_TIMEOUT    19

/* Data structures */
struct grf_device
{
	char    *id;
	time_t   timestamp;
	/* TODO: Device properties */
};

struct grf_devicelist
{
	struct grf_device  devices[GRF_MAXDEVICES];
	uint8_t            len;
};

/* UART functions */
int  grf_uart_open(const char *dev);
int  grf_uart_setup(int fd);
void grf_uart_close(int fd);
void grf_uart_set_timeout(int fd, unsigned int timeout);

/* Communication functions */
int grf_comm_init(int fd, char **firmware_version);
int grf_comm_scan_groups(int fd, char **groups);
int grf_comm_scan_devices(int fd, const char *group, struct grf_devicelist *devices);
int grf_comm_read_data(int fd, const char *deviceid, struct grf_device *device);

#endif /* __GRF_H__ */
