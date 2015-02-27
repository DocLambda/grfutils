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

#ifndef __GRF_H__
#define __GRF_H__

#include <time.h>
#include <stdint.h>

#include "grf_radio.h"

#define RETURN_ON_ERROR(__func__) \
{\
    int __retval__ = (__func__);\
    if ((__retval__)) return (__retval__); \
}

#define GRF_MAXDEVICES          40                  /* Maximum number of devices in one group */

#define GRF_INIT_TEST           "%c01TESTA1%c"      /* Set RF module to command mode */
#define GRF_INIT_SV             "%cSV%c"            /* Request sending the firmware version */
#define GRF_SCAN_GA             "%cGA%c"            /* Request scanning group adress */
#define GRF_SCAN_GD             "%cGD:%s%c"         /* Request scanning of all devices of a group adress */
#define GRF_REQUEST_DA_TMPL     "%cDA:%s:%02d%c"    /* Template for data acquisition request */
#define GRF_DA_TYPE_START       5                   /* Request starting data acquisition */
#define GRF_DA_TYPE_SIGNAL_ON   3                   /* Request switching on accustic signal */
#define GRF_DA_TYPE_SIGNAL_OFF  6                   /* Request switching off accustic signal */
#define GRF_DA_TYPE_SEND        1                   /* Request sending the aquired data */
#define GRF_DA_TYPE_STOP        4                   /* Request stopping data acquisition */
#define GRF_REQUEST_DIAG        "%cSD:%s%c"         /* Request sending diagnosis data */

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

#define GRF_UNKNOWN_REGISTER_INDEX(__regid__) ((__regid__) - 0x14)

/* Data structures */
struct grf_device
{
	char    *id;
	time_t   timestamp;

	/* Device properties */
	uint32_t serial_number;
	float    operation_time;          /* seconds */
	uint8_t  smoke_chamber_pollution; /* FIXME: percent? */
	float    battery_voltage;         /* Volt */
	float    temperature1;            /* degree celcius */
	float    temperature2;            /* degree celcius */
	/* Alerts */
	uint8_t  local_smoke_alerts;
	uint8_t  local_temperature_alerts;
	uint8_t  local_test_alerts;
	uint8_t  remote_cable_alerts;
	uint8_t  remote_radio_alerts;
	uint8_t  remote_cable_test_alerts;
	uint8_t  remote_radio_test_alerts;
	/* Unknown data */
	uint16_t smoke_chamber_value;     /* FIXME: unknown */
	uint32_t unknown_02;              /* FIXME: unknown */
	uint32_t unknown_registers[40];   /* FIXME: unknown registers 0x14 to 0x3B */
	uint32_t unknown_64;              /* FIXME: unknown */
};

struct grf_devicelist
{
	struct grf_device  devices[GRF_MAXDEVICES];
	uint8_t            len;
};

/* Communication functions */
int grf_comm_init(struct grf_radio *radio);
int grf_comm_scan_groups(struct grf_radio *radio, char **groups);
int grf_comm_scan_devices(struct grf_radio *radio, const char *group, struct grf_devicelist *devices);
int grf_comm_read_data(struct grf_radio *radio, const char *deviceid, struct grf_device *device);
int grf_comm_switch_signal(struct grf_radio *radio, const char *deviceid, bool on);

#endif /* __GRF_H__ */
