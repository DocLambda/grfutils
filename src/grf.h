/*
 * Communication include file
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
/*! \defgroup comm    Communication */
/*! \defgroup logging Logging */

/*! \ingroup comm
 *  \file grf.h
 *  \brief Data structures and high-level functions to communicate with smoke detector devices
 *
 * This file defines the high-level API and the corresponding data structures for
 * communicating with Gira smoke detector devices using the original radio module.
 *
 * For more information about the hardware or protocols:
 * \see http://knx-user-forum.de/forum/%C3%B6ffentlicher-bereich/knx-eib-forum/diy-do-it-yourself/15657-gira-dual-rauchmelder-vernetzung-einzelauswertung-fernausl%C3%B6sung
 * \see http://github.com/selfbus/software/tree/master/Rauchmelder/Doc
 *
 * @{
 */

#ifndef __GRF_H__
#define __GRF_H__

#include <time.h>
#include <stdint.h>

#include "grf_radio.h"

#define RETURN_ON_ERROR(__func__) \
{\
    int __retval__ = (__func__);\
    if ((__retval__)) return (__retval__); \
}									/*!< Check return value and return on error */

#define GRF_UNKNOWN_REGISTER_INDEX(__regid__) ((__regid__) - 0x14)	/*!< Determine register array index from register ID */

#define GRF_MAXDEVICES          40		/*!< Maximum number of devices in one group */

/*! Data structure representing a single smoke detector device */
struct grf_device
{
	char    *id;						/*!< 4-character ID of the smoke detector */
	time_t   timestamp;					/*!< Timestamp of data reception */

	/* Device properties */
	uint32_t serial_number;				/*!< Serial number of the smoke detector */
	float    operation_time;			/*!< Time of operation in seconds */
	uint8_t  smoke_chamber_pollution;	/*!< Smoke chamber pollution in percent? */ /*FIXME*/
	float    battery_voltage;			/*!< Battery voltage in Volt */
	float    temperature1;				/*!< Measured temperture in degree celcius */
	float    temperature2;				/*!< Measured temperture in degree celcius */
	/* Alerts */
	uint8_t  local_smoke_alerts;		/*!< Number of smoke alerts occured on the local detector */
	uint8_t  local_temperature_alerts;	/*!< Number of temperature alerts occured on the local detector */
	uint8_t  local_test_alerts;			/*!< Number of test alerts occured on the local detector */
	uint8_t  remote_cable_alerts;		/*!< Number of remote alerts transmitted via wire */
	uint8_t  remote_radio_alerts;		/*!< Number of remote alerts transmitted wireless via radio */
	uint8_t  remote_cable_test_alerts;	/*!< Number of remote test alerts transmitted via wire */
	uint8_t  remote_radio_test_alerts;	/*!< Number of remote test alerts transmitted wireless via radio */
	/* Unknown data */
	uint16_t smoke_chamber_value;		/*!< Unknown data propably related to smoke chamber */ /* FIXME */
	uint32_t unknown_02;				/*!< Unknown data (register 0x20) */ /* FIXME */
	uint32_t unknown_registers[40];		/*!< Unknown data (registers 0x14 to 0x3B) */ /* FIXME */
	uint32_t unknown_64;				/*!< Unknown data (register 0x64) */ /* FIXME */
};

/*! Data structure representing a list of smoke detector devices */
struct grf_devicelist
{
	struct grf_device  devices[GRF_MAXDEVICES];	/*!< Array of smoke detector devices*/
	uint8_t            len;						/*!< Number of valid smoke detector devices in the array */
};

/*! \brief Initialization of the communication with radio.
 *
 *  This function initializes the communication with the radio device
 *  and retrieves its firmware version via
    \code
     <NUL><STX>01TESTA1<ETX>   -->
                               <-- <ACK>
     <STX>SV<ETX>              -->
                               <-- firmware version
    \endcode
 *
 *  \param radio	radio device structure initialized by \ref grf_radio_init()
 *  \returns		0 on success and an error code otherwise
 */
int grf_comm_init(struct grf_radio *radio);

/*! \brief Scan for the group ID of a smoke detector.
 *
 *  This function initiates a scan for the group ID of a smoke detector.
 *  __NOTE:__ This requires that sending the group ID is activated manually
 *  in the smoke detector by pressing the *programming* button until the
 *  programming LED flashes once per second. Afterwards the smoke
 *  detector button has to be pressed until you hear a beep sound.
 *
 *  The group scan is performed via
    \code
     <NUL><STX>01TESTA1<ETX>   -->
                               <-- <ACK>
     <STX>GA<ETX>              -->
                               <-- 4-digit group ID
    \endcode
 *
 *  \param radio	radio device structure initialized by \ref grf_comm_init()
 *  \param groups	newly allocated storage containing the retrieved group ID. __This memory should be free'd by the user to avoid memory leaks.__
 *  \returns		0 on success and an error code otherwise
 */
int grf_comm_scan_groups(struct grf_radio *radio, char **groups);

/*! \brief Scan for smoke detector devices within a group.
 *
 *  This function initiates a scan for smoke detector devices associated with the given group ID.
 *
 *  The device scan is performed via
    \code
     <NUL><STX>01TESTA1<ETX>   -->
                               <-- <ACK>
     <STX>GD:$GROUPID<ETX>     -->
                               <-- list of device IDs
                               <-- <STX>Timeout<ETX>
    \endcode
 * 
 *  \param radio	radio device structure initialized by \ref grf_comm_init()
 *  \param group	group ID to be scanned e.g. retrieved using \ref grf_comm_scan_groups()
 *  \param devices	list of devices belonging to *group*
 *  \returns		0 on success and an error code otherwise
 */
int grf_comm_scan_devices(struct grf_radio *radio, const char *group, struct grf_devicelist *devices);

/*! \brief Retrieve the data of a smoke detector device
 *
 *  This function requests all available data from the smoke detector device with the given ID.
 *  The type of data that can be retrieved is documented in \ref grf_device.
 *
 *  The device data request is performed via
    \code
     <NUL><STX>01TESTA1<ETX>   -->
                               <-- <ACK>
     <STX>DA:$DEVICEID:05<ETX> -->
                               <-- <ACK>
                               <-- <STX>Done<ETX>
     in case we receive a TIMEOUT we need to start the diagnosis mode first:
         <STX>SD:$DEVICEID<ETX>    -->
                                   <-- <ACK>
                                   <-- <STX>REC<ETX>
                                   <-- <STX>Done<ETX>
     end
     <STX>DA:$DEVICEID:01<ETX> -->
                               <-- <ACK>
                               <-- data of the device
                               <-- <STX>Timeout<ETX>
     <STX>DA:$DEVICEID:04<ETX> -->
                               <-- <ACK>
                               <-- <STX>Done<ETX>
    \endcode
 *
 *  \param radio	radio device structure initialized by \ref grf_comm_init()
 *  \param deviceid	ID of the device to be read-out e.g. retrieved using \ref grf_comm_scan_devices()
 *  \param device	device data structure containing the retrieved information
 *  \returns		0 on success and an error code otherwise
 */
int grf_comm_read_data(struct grf_radio *radio, const char *deviceid, struct grf_device *device);

/*! \brief Switch accustic signal of the smoke detector device ON or OFF
 *
 *  This function allows to switch the accustig alert of the smoke detector
 *  ON or OFF for the given device ID.
 *
 *  The switching is performed via
    \code
     <NUL><STX>01TESTA1<ETX>   -->
                               <-- <ACK>
     <STX>DA:$DEVICEID:05<ETX> -->
                               <-- <ACK>
                               <-- <STX>Done<ETX>
     in case we receive a Timeout we need to start the diagnosis mode first:
         <STX>SD:$DEVICEID<ETX>    -->
                                   <-- <ACK>
                                   <-- <STX>REC<ETX>
                                   <-- <STX>Done<ETX>
     end
     in case we want to switch the signal ON:
         <STX>DA:$DEVICEID:03<ETX> -->
                                  <-- <ACK>
     in case we want to switch the signal OFF:
         <STX>DA:$DEVICEID:06<ETX> -->
                                  <-- <ACK>
     end
     <STX>DA:$DEVICEID:04<ETX> -->
                               <-- <ACK>
                               <-- <STX>Done<ETX>
    \endcode
 *
 *  \param radio	radio device structure initialized by \ref grf_comm_init()
 *  \param deviceid	ID of the device to be switched e.g. retrieved using \ref grf_comm_scan_devices()
 *  \param on		switches alert ON if true and OFF if false
 *  \returns		0 on success and an error code otherwise
 */
int grf_comm_switch_signal(struct grf_radio *radio, const char *deviceid, bool on);

#endif /* __GRF_H__ */
/* @} */
