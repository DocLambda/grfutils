#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include "grf.h"
#include "grf_logging.h"

/* Announce low-level uart read/write functions */
extern int grf_uart_read_message(int fd, char *message, size_t *len);
extern int grf_uart_write_message(int fd, const char *message, size_t len);
extern int grf_uart_write_ctl(int fd, char ctl);

/*****************************************************************************/
static int get_data(const char *msg, size_t len, char *data)
{
	int datatype = GRF_DATATYPE_ERROR;
	
	/* Clear the data field */
	memset(data, '\0', len*sizeof(char));

	/* Check for control characters */
	if (len <= 1)
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
	char data[len];

	return (get_data(msg, len, data) == GRF_DATATYPE_ACK);
}

static bool msg_is_timeout(char *msg, size_t len)
{
	char data[len];

	return (get_data(msg, len, data) == GRF_DATATYPE_TIMEOUT);
}
/*****************************************************************************/

/*****************************************************************************/
static int send_init_sequence(int fd)
{
	char    msg[255];
	size_t  len;

	/* Write the initialization sequence:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 */
	RETURN_ON_ERROR(grf_uart_write_ctl(fd, GRF_NUL));
	len = sprintf(msg, GRF_INIT_TEST, GRF_STX, GRF_ETX);
	RETURN_ON_ERROR(grf_uart_write_message(fd, msg, len));
	RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;

	return 0;
}

static int send_request_firmware_version(int fd, char **firmware_version)
{
	char    msg[255];
	char    data[255];
	int     datatype;
	size_t  len;

	/* Get firmware version:
	 *    <STX>SV<ETX>              -->
	 *                              <-- Version string
	 */
	len = sprintf(msg, GRF_INIT_SV, GRF_STX, GRF_ETX);
	RETURN_ON_ERROR(grf_uart_write_message(fd, msg, len));
	RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	datatype = get_data(msg, len, data);
	if (datatype != GRF_DATATYPE_VERSION)
		return EIO;

	if (firmware_version)
		*firmware_version = strdup(data);

	return 0;
}

static int send_request_groups(int fd, char **groups)
{
	char    msg[255];
	char    data[255];
	int     datatype;
	size_t  len;

	/* Start group scanning:
	 *    <STX>GA<ETX>              -->
	 *                              <-- <ACK>
	 *                              <-- Group IDs
	 */
	len = sprintf(msg, GRF_SCAN_GA, GRF_STX, GRF_ETX);
	RETURN_ON_ERROR(grf_uart_write_message(fd, msg, len));
	RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;
	RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	datatype = get_data(msg, len, data);
	if (datatype != GRF_DATATYPE_DATA)
		return EIO;
	if (groups)
		*groups = strdup(data);

	return 0;
}

static int send_request_devices(int fd, const char *group, struct grf_devicelist *devices)
{
	char    msg[255];
	char    data[255];
	int     datatype;
	size_t  len;

	/* Start group scanning:
	 *    <STX>GD:$GROUPID<ETX>     -->
	 *                              <-- <ACK>
	 *                              <-- <STX>REC<ETX>
	 *                              <-- Device IDs
	 */
	len = sprintf(msg, GRF_SCAN_GD, GRF_STX, group, GRF_ETX);
	RETURN_ON_ERROR(grf_uart_write_message(fd, msg, len));
	RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;

	/* Expect the REC answer */
	RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	datatype = get_data(msg, len, data);
	if (datatype != GRF_DATATYPE_REC)
		return EIO;

	/* Receive devices until we get a timout */
	devices->len = 0;
	while (true) 
	{
		RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
		if (msg_is_timeout(msg, len))
			break;
		datatype = get_data(msg, len, data);
		if (datatype != GRF_DATATYPE_DATA)
			return EIO;
		printf("Received device ID: %s\n", data);

		/* Add device to the device array and set the update
		 * time to invalid (-1) to mark that the device was
		 * not yet updated.
		 */
		devices->devices[devices->len].id        = strdup(data);
		devices->devices[devices->len].timestamp = -1;
		devices->len++;
	}

	return 0;
}

static int send_data_request(int fd, const char *deviceid, uint8_t reqtype)
{
	char    msg[255];
	char    data[255];
	int     datatype;
	size_t  len;

	/* Request data acquisition:
	 *    <STX>DA:$DEVICEID:$REQTYPE<ETX> -->
	 *                                   <-- <ACK>
	 * if request type is 1:
	 *                                   <-- DATA
	 * else:
	 *                                   <-- <STX>Done<ETX>
	 */
	len = sprintf(msg, GRF_REQUEST_DA_TMPL, GRF_STX, deviceid, reqtype, GRF_ETX);
	RETURN_ON_ERROR(grf_uart_write_message(fd, msg, len));
	RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
	if (msg_is_timeout(msg, len))
		return ETIMEDOUT;
	if (!msg_is_ack(msg, len))
		return EIO;

	/* Expect the Done answer */
	if (reqtype != 1)
	{
		RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
		if (msg_is_timeout(msg, len))
			return ETIMEDOUT;
		datatype = get_data(msg, len, data);
		if (datatype != GRF_DATATYPE_DONE)
			return EIO;
	}
	return 0;
}

static int recv_data(int fd, struct grf_device *device)
{
	char    msg[255];
	char    data[255];
	int     datatype;
	size_t  len;

	while (true)
	{
		RETURN_ON_ERROR(grf_uart_read_message(fd, msg, &len));
		if (msg_is_timeout(msg, len))
			break;
		datatype = get_data(msg, len, data);
		if (datatype != GRF_DATATYPE_DATA)
			return EIO;
		grf_logging_dbg("    data: %s", data);

		/* TODO: Interprete the received data.*/
	}

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_init(int fd, char **firmware_version)
{
	/* Write the initialization sequence and get firmware version:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>SV<ETX>              -->
	 *                              <-- Version string
	 */
	RETURN_ON_ERROR(send_init_sequence(fd));
	RETURN_ON_ERROR(send_request_firmware_version(fd, firmware_version));

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_scan_groups(int fd, char **groups)
{
	/* Write the initialization sequence and get firmware version:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>GA<ETX>              -->
	 *                              <-- Group IDs
	 */
	RETURN_ON_ERROR(send_init_sequence(fd));
	RETURN_ON_ERROR(send_request_groups(fd, groups));

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_scan_devices(int fd, const char *group, struct grf_devicelist *devices)
{
	/* Initialize the device list */
	devices->len = 0;

	/* Write the initialization sequence and get firmware version:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>GD:$GROUPID<ETX>     -->
	 *                              <-- Group IDs
	 */
	RETURN_ON_ERROR(send_init_sequence(fd));
	RETURN_ON_ERROR(send_request_devices(fd, group, devices));

	return 0;
}
/*****************************************************************************/

/*****************************************************************************/
int grf_comm_read_data(int fd, const char *deviceid, struct grf_device *device)
{
	/* Initialize the device data */
	device->id = strdup(deviceid);

	/* Write the initialization sequence and receive acquired data:
	 *    <NUL><STX>01TESTA1<ETX>   -->
	 *                              <-- <ACK>
	 *    <STX>DA:$DEVICEID:05<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- <STX>Done<ETX>
	 *    <STX>DA:$DEVICEID:01<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- DATA
	 *    <STX>DA:$DEVICEID:04<ETX> -->
	 *                              <-- <ACK>
	 *                              <-- <STX>Done<ETX>
	 */
	RETURN_ON_ERROR(send_init_sequence(fd));
	RETURN_ON_ERROR(send_data_request(fd, deviceid, GRF_DA_TYPE_START));
	RETURN_ON_ERROR(send_data_request(fd, deviceid, GRF_DA_TYPE_SEND));
	RETURN_ON_ERROR(recv_data(fd, device));
	RETURN_ON_ERROR(send_data_request(fd, deviceid, GRF_DA_TYPE_STOP));

	return 0;
}
/*****************************************************************************/
