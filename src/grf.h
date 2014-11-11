#ifndef __GRF_H__
#define __GRF_H__

#define RETURN_ON_ERROR(__func__) \
{\
    int __retval__ = (__func__);\
    if ((__retval__)) return (__retval__); \
}

#define GRF_BAUDRATE            B9600

#define GRF_NUL                 0x00
#define GRF_STX                 0x02
#define GRF_ETX                 0x03
#define GRF_ACK                 0x06
#define GRF_NAK                 0x15

#define GRF_INIT_TEST           "%c01TESTA1%c"      /* Set RF module to command mode */
#define GRF_INIT_SV             "%cSV%c"            /* Request sending the firmware version */
#define GRF_SCAN_GA             "%cGA%c"            /* Request scanning group adress */
#define GRF_REQUEST_DA_START    "%cDA:%04x:05%c"    /* Request starting data acquisition */
#define GRF_REQUEST_DA_STOP     "%cDA:%04x:01%c"    /* Request stopping data acquisition */
#define GRF_REQUEST_SEND        "%cSD:%04x%c"       /* Request sending data */

#define GRF_ANSWER_TIMEOUT      "Timeout"           /* Also used for end of transmission ??? */
#define GRF_ANSWER_DONE         "Done"              /* Expected answer to indicate completion of command */
#define GRF_ANSWER_REC          "REC"               /* Expected to indicate that data recording is in process ??? */
#define GRF_ANSWER_VERSION      "GI_RM_V00.70"      /* Expected version string */

#define GRF_DATATYPE_ERROR      -1
#define GRF_DATATYPE_CONTROL     0
#define GRF_DATATYPE_ACK         1
#define GRF_DATATYPE_DATA       10
#define GRF_DATATYPE_VERSION    11
#define GRF_DATATYPE_TIMEOUT    12


/* UART functions */
int  grf_uart_open(const char *dev);
int  grf_uart_setup(int fd);
void grf_uart_close(int fd);
void grf_uart_set_timeout(int fd, unsigned int timeout);

/* Communication functions */
int grf_comm_init(int fd, char **firmware_version);
int grf_comm_scan_groups(int fd, char **groups);


#endif /* __GRF_H__ */