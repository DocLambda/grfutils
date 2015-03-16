/*
 * High-level communication protocol implementation
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

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <math.h>

#include "grf.h"
#include "grf_radio.h"
#include "grf_logging.h"


#define GRF_ANSWER_TIMEOUT      "Timeout"           /* Also used for end of transmission */
#define GRF_ANSWER_DONE         "Done"              /* Expected answer to indicate completion of command */
#define GRF_ANSWER_REC          "REC"               /* Expected to indicate that data recording is in process */
#define GRF_ANSWER_VERSION      "GI_RM_V00.70"      /* Expected version string */

#define GRF_DATATYPE_ERROR      -1
#define GRF_DATATYPE_CONTROL     0
#define GRF_DATATYPE_ACK         1
#define GRF_DATATYPE_DATA       10
#define GRF_DATATYPE_VERSION    11
#define GRF_DATATYPE_REC        12
#define GRF_DATATYPE_DONE       13
#define GRF_DATATYPE_TIMEOUT    19

#define MSGBUFSIZE		255

/*****************************************************************************/
static int generate_command(char *msg, size_t *len, size_t size, const char *fmt, ...)
{
	assert(msg);
	assert(len);
	assert(fmt);

	va_list arglist;
	int     count;

	/* Empty messages are considered invalid */
	if (size < 1)
		return EINVAL;

	/* Initialize the returned len */
	*len = 0;

	/* Construct the output string */
	va_start(arglist, fmt);
	count = vsnprintf(msg, size, fmt, arglist);
	va_end(arglist);

	/* Check if the message fits the message buffer */
	if (count >= size || count < 0 /* handle GNU C Library prior to 2.1*/)
		return ENOBUFS;

	/* Check if the resulting command has content at all */
	if (count  < 1)
		return EINVAL;

	*len = count;

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
static int get_data(const char *msg, size_t len, char *data)
{
	assert(msg);
	assert(data);

	int datatype = GRF_DATATYPE_ERROR;

	/* Empty messages are considered invalid */
	if (len < 1)
		return GRF_DATATYPE_ERROR;
	
	/* Clear the data field */
	memset(data, '\0', len*sizeof(char));

	/* Check for control characters */
	if (len == 1)
	{
		data[0] = msg[0];

		/* Determine the datatype */
		switch(data[0])
		{
			case GRF_ACK:
				datatype = GRF_DATATYPE_ACK;
				break;
			default:
				datatype = GRF_DATATYPE_CONTROL;
				break;
		}
	}
	else
	{
		/* Check the leading STX and trailing ETX */
		if (msg[0] != GRF_STX || msg[len-1] != GRF_ETX)
			return GRF_DATATYPE_ERROR;

		/* Copy only the real content of sent data and
		 * make sure we have a terminating \0 character.
		 */
		strncpy(data, msg+1, (len-2));

		/* Determine the datatype */
		if (strncmp(data, GRF_ANSWER_VERSION, 7) == 0)
			datatype = GRF_DATATYPE_VERSION;
		else if (strcmp(data, GRF_ANSWER_TIMEOUT) == 0)
			datatype = GRF_DATATYPE_TIMEOUT;
		else if (strcmp(data, GRF_ANSWER_REC) == 0)
			datatype = GRF_DATATYPE_REC;
		else if (strcmp(data, GRF_ANSWER_DONE) == 0)
			datatype = GRF_DATATYPE_DONE;
		else
			datatype = GRF_DATATYPE_DATA;
	}

	return datatype;
}
/*****************************************************************************/

/*****************************************************************************/
static bool msg_is_ack(char *msg, size_t len)
{
	assert(msg);
	assert(len > 0);

	char data[len];

	return (get_data(msg, len, data) == GRF_DATATYPE_ACK);
}

static bool msg_is_timeout(char *msg, size_t len)
{
	assert(msg);
	assert(len > 0);

	char data[len];

	return (get_data(msg, len, data) == GRF_DATATYPE_TIMEOUT);
}

static bool msg_is_done(char *msg, size_t len)
{
	assert(msg);
	assert(len > 0);

	char data[len];

	return (get_data(msg, len, data) == GRF_DATATYPE_DONE);
}

static bool msg_is_rec(char *msg, size_t len)
{
	assert(msg);
	assert(len > 0);

	char data[len];

	return (get_data(msg, len, data) == GRF_DATATYPE_REC);
}
/*****************************************************************************/

/*****************************************************************************/
static int send_init_sequence(struct grf_radio *radio)
{
	assert(grf_radio_is_valid(radio));

	char    msg[MSGBUFSIZE];
	size_t  len;

	/* Write the initialization sequence:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 */
	RETURN_ON_ERROR(grf_radio_write_ctrl(radio, GRF_NUL));
	RETURN_ON_ERROR(generate_command(msg, &len, MSGBUFSIZE, GRF_INIT_TEST, GRF_STX, GRF_ETX));
	RETURN_ON_ERROR(grf_radio_write(radio, msg, len));
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;

	return 0;
}

static int send_request_firmware_version(struct grf_radio *radio)
{
	assert(grf_radio_is_valid(radio));

	char    msg[MSGBUFSIZE];
	char    data[MSGBUFSIZE];
	size_t  len;

	/* Get firmware version:
	 *    <STX>SV<ETX>              -->
	 *                              <-- Version string
	 */
	RETURN_ON_ERROR(generate_command(msg, &len, MSGBUFSIZE, GRF_INIT_SV, GRF_STX, GRF_ETX));
	RETURN_ON_ERROR(grf_radio_write(radio, msg, len));
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (get_data(msg, len, data) != GRF_DATATYPE_VERSION)
		return EIO;

	radio->firmware_version = strdup(data);
	if (!radio->firmware_version)
		return ENOMEM;

	return 0;
}

static int send_request_groups(struct grf_radio *radio, char **groups)
{
	assert(grf_radio_is_valid(radio));
	assert(groups);

	char    msg[MSGBUFSIZE];
	char    data[MSGBUFSIZE];
	size_t  len;

	/* Start group scanning:
	 *    <STX>GA<ETX>              -->
	 *                              <-- <ACK>
	 *                              <-- Group IDs
	 */
	RETURN_ON_ERROR(generate_command(msg, &len, MSGBUFSIZE, GRF_SCAN_GA, GRF_STX, GRF_ETX));
	RETURN_ON_ERROR(grf_radio_write(radio, msg, len));
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (get_data(msg, len, data) != GRF_DATATYPE_DATA)
		return EIO;

	*groups = strdup(data);
	if (!*groups)
		return ENOMEM;

	return 0;
}

static int send_request_devices(struct grf_radio *radio, const char *group, struct grf_devicelist *devices)
{
	assert(grf_radio_is_valid(radio));
	assert(group);
	assert(devices);

	char    msg[MSGBUFSIZE];
	char    data[MSGBUFSIZE];
	size_t  len;

	/* Start group scanning:
	 *    <STX>GD:$GROUPID<ETX>     -->
	 *                              <-- <ACK>
	 *                              <-- <STX>REC<ETX>
	 *                              <-- Device IDs
	 */
	RETURN_ON_ERROR(generate_command(msg, &len, MSGBUFSIZE, GRF_SCAN_GD, GRF_STX, group, GRF_ETX));
	RETURN_ON_ERROR(grf_radio_write(radio, msg, len));
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;

	/* Expect the REC answer */
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_rec(msg, len))
		return EIO;

	/* Receive devices until we get a timout */
	devices->len = 0;
	while (true) 
	{
		RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
		if (msg_is_timeout(msg, len))
			break;
                if (get_data(msg, len, data) != GRF_DATATYPE_DATA)
			return EIO;
		grf_logging_dbg("Received device ID: %s", data);

		/* Check if there is still some room to store the devices */
		if (devices->len >= GRF_MAXDEVICES)
			return ENOBUFS;

		/* Add device to the device array and set the update
		 * time to invalid (-1) to mark that the device was
		 * not yet updated.
		 */
		devices->devices[devices->len].id        = strdup(data);
		devices->devices[devices->len].timestamp = -1;
		devices->len++;

		if (!devices->devices[devices->len].id)
			return ENOMEM;
	}

	return 0;
}

static int send_start_diagnosis(struct grf_radio *radio, const char *deviceid)
{
	assert(grf_radio_is_valid(radio));
	assert(deviceid);

	char    msg[MSGBUFSIZE];
	size_t  len;

	/* Request data acquisition:
	 *    <STX>SD:$DEVICEID<ETX>	      -->
	 *                                   <-- <ACK>
	 *                                   <-- <STX>REC<ETX>
	 *                                   <-- <STX>Done<ETX>
	 */
	RETURN_ON_ERROR(generate_command(msg, &len, MSGBUFSIZE, GRF_REQUEST_DIAG, GRF_STX, deviceid, GRF_ETX));
	RETURN_ON_ERROR(grf_radio_write(radio, msg, len));
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;

	/* Expect the REC answer */
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_rec(msg, len))
		return EIO;

	/* Expect the Done answer */
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_done(msg, len))
		return EIO;

	return 0;
}

static int send_data_request(struct grf_radio *radio, const char *deviceid, uint8_t reqtype)
{
	assert(grf_radio_is_valid(radio));
	assert(deviceid);

	char    msg[MSGBUFSIZE];
	size_t  len;

	/* Request data acquisition:
	 *    <STX>DA:$DEVICEID:$REQTYPE<ETX> -->
	 *                                   <-- <ACK>
	 * if request type is 1:
	 *                                   <-- DATA
	 * else:
	 *                                   <-- <STX>Done<ETX>
	 */
	RETURN_ON_ERROR(generate_command(msg, &len, MSGBUFSIZE, GRF_REQUEST_DA_TMPL, GRF_STX, deviceid, reqtype, GRF_ETX));
	RETURN_ON_ERROR(grf_radio_write(radio, msg, len));
	RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;

	/* Expect the actual data for the send request */
	if (reqtype != GRF_DA_TYPE_SEND)
	{
		RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
		if (msg_is_timeout(msg, len))
			return ETIMEDOUT;
		if (!msg_is_done(msg, len))
			return EIO;
	}

	return 0;
}

static int recv_data(struct grf_radio *radio, struct grf_device *device)
{
	assert(grf_radio_is_valid(radio));
	assert(device);

	char      msg[MSGBUFSIZE];
	char      data[MSGBUFSIZE];
	size_t    len;
	uint32_t  key;
	uint32_t  value;

	while (true)
	{
		RETURN_ON_ERROR(grf_radio_read(radio, msg, &len, MSGBUFSIZE));
		if (msg_is_timeout(msg, len))
			break;
		if (get_data(msg, len, data) != GRF_DATATYPE_DATA)
			return EIO;
		grf_logging_dbg("    data: %s", data);

		/* Interprete the received data.*/
		sscanf(data, "%04x:%08x", &key, &value);
		switch(key)
		{
			case 0x0001:	/* Serial number */
				device->serial_number = value;
				break;
			case 0x0002:	/* FIXME: Unknown */
				device->unknown_02 = value;
				break;
			case 0x0003:	/* Operation time */
				device->operation_time = (float)value * 0.25f;
				break;
			case 0x0004:	/* Smoke chamber state */
				device->smoke_chamber_value = (value >> 16) & 0xFFFF; /* FIXME: unknown */
				device->local_smoke_alerts  = (value >> 8)  & 0xFF;
				device->smoke_chamber_pollution = value & 0xFF;
				break;
			case 0x0005:	/* Battery and temperature */
				device->battery_voltage = (float)((value >> 16) & 0xFFFF) * 9.184f / 500.0f;
				device->temperature1    = (float)((value >> 8)  & 0xFF) * 0.50f - 20.0f;
				device->temperature2    = (float)(value         & 0xFF) * 0.50f - 20.0f;
				break;
			case 0x0006:	/* Alert count */
				device->local_temperature_alerts = (value >> 24) & 0xFF;
				device->local_test_alerts        = (value >> 16) & 0xFF;
				device->remote_cable_alerts      = (value >> 8)  & 0xFF;
				device->remote_radio_alerts      =  value        & 0xFF;
				break;
			case 0x0007:	/* Remote test alert count */
				/* FIXME: upper two bytes unknown. Always zero? */
				device->remote_cable_test_alerts = (value >> 8)  & 0xFF;
				device->remote_radio_test_alerts =  value        & 0xFF;
				break;
			case 0x0014:	/* FIXME: Unknown register */
			case 0x0015:	/* FIXME: Unknown register */
			case 0x0016:	/* FIXME: Unknown register */
			case 0x0017:	/* FIXME: Unknown register */
			case 0x0018:	/* FIXME: Unknown register */
			case 0x0019:	/* FIXME: Unknown register */
			case 0x001A:	/* FIXME: Unknown register */
			case 0x001B:	/* FIXME: Unknown register */
			case 0x001C:	/* FIXME: Unknown register */
			case 0x001D:	/* FIXME: Unknown register */
			case 0x001E:	/* FIXME: Unknown register */
			case 0x001F:	/* FIXME: Unknown register */
			case 0x0020:	/* FIXME: Unknown register */
			case 0x0021:	/* FIXME: Unknown register */
			case 0x0022:	/* FIXME: Unknown register */
			case 0x0023:	/* FIXME: Unknown register */
			case 0x0024:	/* FIXME: Unknown register */
			case 0x0025:	/* FIXME: Unknown register */
			case 0x0026:	/* FIXME: Unknown register */
			case 0x0027:	/* FIXME: Unknown register */
			case 0x0028:	/* FIXME: Unknown register */
			case 0x0029:	/* FIXME: Unknown register */
			case 0x002A:	/* FIXME: Unknown register */
			case 0x002B:	/* FIXME: Unknown register */
			case 0x002C:	/* FIXME: Unknown register */
			case 0x002D:	/* FIXME: Unknown register */
			case 0x002E:	/* FIXME: Unknown register */
			case 0x002F:	/* FIXME: Unknown register */
			case 0x0030:	/* FIXME: Unknown register */
			case 0x0031:	/* FIXME: Unknown register */
			case 0x0032:	/* FIXME: Unknown register */
			case 0x0033:	/* FIXME: Unknown register */
			case 0x0034:	/* FIXME: Unknown register */
			case 0x0035:	/* FIXME: Unknown register */
			case 0x0036:	/* FIXME: Unknown register */
			case 0x0037:	/* FIXME: Unknown register */
			case 0x0038:	/* FIXME: Unknown register */
			case 0x0039:	/* FIXME: Unknown register */
			case 0x003A:	/* FIXME: Unknown register */
			case 0x003B:	/* FIXME: Unknown register */
				device->unknown_registers[GRF_UNKNOWN_REGISTER_INDEX(key)] = value;
				break;
			case 0x0064:	/* FIXME: Unknown */
				device->unknown_64 = value;
				break;
			default:
				grf_logging_dbg("    UNKNOWN KEY:  key = %u    value = %u", key, value);
				break;
		}
	}

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_init(struct grf_radio *radio)
{
	assert(grf_radio_is_valid(radio));

	/* Write the initialization sequence and get firmware version:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>SV<ETX>              -->
	 *                              <-- Version string
	 */
	RETURN_ON_ERROR(send_init_sequence(radio));
	RETURN_ON_ERROR(send_request_firmware_version(radio));

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_scan_groups(struct grf_radio *radio, char **groups)
{
	assert(grf_radio_is_valid(radio));
	assert(groups);

	/* Write the initialization sequence and get firmware version:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>GA<ETX>              -->
	 *                              <-- Group IDs
	 */
	RETURN_ON_ERROR(send_init_sequence(radio));
	RETURN_ON_ERROR(send_request_groups(radio, groups));

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_scan_devices(struct grf_radio *radio, const char *group, struct grf_devicelist *devices)
{
	assert(grf_radio_is_valid(radio));
	assert(group);
	assert(devices);

	/* Initialize the device list */
	devices->len = 0;

	/* Write the initialization sequence and get firmware version:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>GD:$GROUPID<ETX>     -->
	 *                              <-- Group IDs
	 */
	RETURN_ON_ERROR(send_init_sequence(radio));
	RETURN_ON_ERROR(send_request_devices(radio, group, devices));

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_read_data(struct grf_radio *radio, const char *deviceid, struct grf_device *device)
{
	assert(grf_radio_is_valid(radio));
	assert(deviceid);
	assert(device);

	/* Variable declaration */
	int retval;

	/* Initialize the device data */
	device->id = strdup(deviceid);
	if (!device->id)
		return ENOMEM;

	/* Write the initialization sequence and receive acquired data:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>DA:$DEVICEID:05<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- <STX>Done<ETX>
	 *  in case we receive a TIMEOUT we need to start the diagnosis mode first:
	 *    <STX>SD:$DEVICEID<ETX>    -->
	 *                              <-- <ACK>
	 *                              <-- <STX>REC<ETX>
	 *                              <-- <STX>Done<ETX>
	 *  end
	 *    <STX>DA:$DEVICEID:01<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- DATA
	 *    <STX>DA:$DEVICEID:04<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- <STX>Done<ETX>
	 */
	RETURN_ON_ERROR(send_init_sequence(radio));
	retval = send_data_request(radio, deviceid, GRF_DA_TYPE_START);
	if (retval == ETIMEDOUT)
		retval = send_start_diagnosis(radio, deviceid);
	RETURN_ON_ERROR(retval);
	RETURN_ON_ERROR(send_data_request(radio, deviceid, GRF_DA_TYPE_SEND));
	RETURN_ON_ERROR(recv_data(radio, device));
	RETURN_ON_ERROR(send_data_request(radio, deviceid, GRF_DA_TYPE_STOP));

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_switch_signal(struct grf_radio *radio, const char *deviceid, bool on)
{
	assert(grf_radio_is_valid(radio));
	assert(deviceid);

	/* Variable declaration */
	int retval;

	/* Write the initialization sequence and switch the signal on or off:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>DA:$DEVICEID:05<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- <STX>Done<ETX>
	 *  in case we receive a TIMEOUT we need to start the diagnosis mode first:
	 *    <STX>SD:$DEVICEID<ETX>    -->
	 *                              <-- <ACK>
	 *                              <-- <STX>REC<ETX>
	 *                              <-- <STX>Done<ETX>
	 *  end
	 *  in case we want to switch the signal on:
	 *    <STX>DA:$DEVICEID:03<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- DATA
	 *  in case we want to switch the signal off:
	 *    <STX>DA:$DEVICEID:06<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- DATA
	 *  end
	 *    <STX>DA:$DEVICEID:04<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- <STX>Done<ETX>
	 */
	RETURN_ON_ERROR(send_init_sequence(radio));
	retval = send_data_request(radio, deviceid, GRF_DA_TYPE_START);
	if (retval == ETIMEDOUT)
		retval = send_start_diagnosis(radio, deviceid);
	RETURN_ON_ERROR(retval);
	RETURN_ON_ERROR(send_data_request(radio, deviceid, on ? GRF_DA_TYPE_SIGNAL_ON : GRF_DA_TYPE_SIGNAL_OFF));
	RETURN_ON_ERROR(send_data_request(radio, deviceid, GRF_DA_TYPE_STOP));

	return 0;
}
/*****************************************************************************/
