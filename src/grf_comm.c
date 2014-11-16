#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include "grf.h"

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

	*firmware_version = strdup(data);

	return 0;

}

static int send_request_groups(int fd, char **groups)
{
	char    msg[255];
	char    data[255];
	int     datatype;
	size_t  len;
	
	/* Get firmware version:
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
	if (firmware_version)
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
