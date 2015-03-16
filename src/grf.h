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

#define GRF_UNKNOWN_REGISTER_INDEX(__regid__) ((__regid__) - 0x14)

#define GRF_MAXDEVICES          40                  /* Maximum number of devices in one group */

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
