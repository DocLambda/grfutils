/*
 * Radio module interface include file
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

/*! \ingroup comm
 *  \file grf_radio.h
 *  \brief Data structures and functions to interface with the radio module
 *
 * This file defines the API and corresponding data structures for
 * interfacing the original Gira radio device.
 *
 * @{
 */

#ifndef __GRF_RADIO_H__
#define __GRF_RADIO_H__

#include <stdint.h>
#include <termios.h>

#define GRF_BAUDRATE            B9600	/*!< Baudrate of the serial device (9600 8N1) */

#define GRF_NUL                 0x00	/*!< Definition of `<NUL>` - zero value */
#define GRF_STX                 0x02	/*!< Definition of `<STX>` - start of transmission */
#define GRF_ETX                 0x03	/*!< Definition of `<ETX>` - end of transmission */
#define GRF_CONT                0x0a	/*!< Definition of `<CONT>`- a continued message*/
#define GRF_ACK                 0x06	/*!< Definition of `<ACK>` - an acknowledged message */
#define GRF_NAK                 0x15	/*!< Definition of `<NAK>` - a not acknowledged message */

#define grf_radio_is_valid(__r__) ((__r__) && (__r__)->is_initialized && (__r__)->fd >= 0) /*!< Macro to check if a radio device is initialized and sane */

/*! Data structure representing a radio device */
struct grf_radio
{
	char           *dev;			/*!< Path to the serial device attached to the radio */
	int             fd;				/*!< File descriptor of the serial device attached to the radio */
	bool            is_initialized;	/*!< Status flag if the initialization of the serial device is complete */

	uint32_t        timeout_user;	/*!< Timeout in seconds as specified by the user */
	uint8_t         timeout_tty;	/*!< Internal timeout in 1/10 seconds to reproduce the user timeout in combination with \ref timeout_repeats */
	uint8_t         timeout_repeats;/*!< Number of repetitions required to reproduce the user timeout with the limited TTY timeout \ref timeout_tty */

	struct termios  tty_attr;		/*!< Setting actually used for the serial device */
	struct termios  tty_attr_saved;	/*!< Saved setting of the serial device to restore on exit */

	char           *firmware_version;/*!< Firmware version of the radio device */
};

/*! \brief Initialization and setup of the radio device.
 *
 *  This function initializes the radio data structure and
 *  perform the setup of the serial interface to communicate
 *  with the radio device.
 *
 *  \param radio	radio device structure to initialize
 *  \param dev		path to the serial device attached to the radio
 *  \param timeout	communication timeout in seconds (0 for infinite)
 *  \returns		0 on success and an error code otherwise
 */
int grf_radio_init(struct grf_radio *radio, const char *dev, unsigned int timeout);

/*! \brief Deinitialization of the radio device.
 *
 *  This function ends the communication with the radio device,
 *  closes the serial interface and resets the radio data structure.
 *
 *  \param radio	radio device to deinitialize
 *  \returns		0 on success and an error code otherwise
 */
int grf_radio_exit(struct grf_radio *radio);

/*! \brief Read a message from the radio device.
 *
 *  This function receives a message from the radio device. It blocks
 *  either until the message is received or the timeout specified
 *  at \ref grf_radio_init() is reached.
 *
 *  \param radio	radio device to read from
 *  \param message	buffer of size *size* to store the received data in
 *  \param len		buffer to store the length of the received message
 *  \param size		capacity of the *message* buffer
 *  \returns		0 on success and an error code otherwise
 */
int grf_radio_read(struct grf_radio *radio, char *message, size_t *len, size_t size);

/*! \brief Write a message to the radio device.
 *
 *  This function sends a message to the radio device. It blocks
 *  either until the message is sent or the timeout specified
 *  at \ref grf_radio_init() is reached.
 *
 *  \param radio	radio device to read from
 *  \param message	message of length *len* to send
 *  \param len		length of the message to send
 *  \returns		0 on success and an error code otherwise
 */
int grf_radio_write(struct grf_radio *radio, const char *message, size_t len);

/*! \brief Write a single (control) character to the radio device.
 *
 *  This function sends a single (control) character to the radio device.
 *  It blocks either until the message is sent or the timeout specified
 *  at \ref grf_radio_init() is reached.
 *
 *  \param radio	radio device to read from
 *  \param ctrl		control character to send
 *  \returns		0 on success and an error code otherwise
 */
int grf_radio_write_ctrl(struct grf_radio *radio, char ctrl);

#endif /* __GRF_RADIO_H__ */
/* @} */
