#ifndef __GRF_RADIO_H__
#define __GRF_RADIO_H__

#include <stdint.h>
#include <termios.h>

#define GRF_BAUDRATE            B9600

#define GRF_NUL                 0x00
#define GRF_STX                 0x02
#define GRF_ETX                 0x03
#define GRF_CONT                0x0a
#define GRF_ACK                 0x06
#define GRF_NAK                 0x15

/* Data structures */
struct grf_radio
{
	char           *dev;
	int             fd;
	bool            is_initialized;

	uint32_t        timeout_user;
	uint8_t         timeout_tty;
	uint8_t         timeout_repeats;

	struct termios  tty_attr;
	struct termios  tty_attr_saved;

	char           *firmware_version;
};

/* Radio functions */
int grf_radio_init(struct grf_radio *radio, const char *dev, unsigned int timeout);
int grf_radio_exit(struct grf_radio *radio);
int grf_radio_read(struct grf_radio *radio, char *message, size_t *len);
int grf_radio_write(struct grf_radio *radio, const char *message, size_t len);
int grf_radio_write_ctrl(struct grf_radio *radio, char ctrl);

#endif /* __GRF_RADIO_H__ */
