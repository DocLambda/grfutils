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
	struct termios  tty_attr;
	struct termios  tty_attr_saved;
	uint32_t        timeout_user;
	uint8_t         timeout_tty;
	uint8_t         timeout_repeats;
};

/* UART functions */
int  grf_uart_open(const char *dev);
int  grf_uart_setup(int fd);
void grf_uart_close(int fd);
void grf_uart_set_timeout(int fd, unsigned int timeout);
int  grf_uart_read_message(int fd, char *message, size_t *len);
int  grf_uart_write_message(int fd, const char *message, size_t len);
int  grf_uart_write_ctl(int fd, char ctl);

#endif /* __GRF_RADIO_H__ */
