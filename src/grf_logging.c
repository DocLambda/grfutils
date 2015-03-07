/*
 * Logging helper implementation
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>
#include <time.h>

#include "grf_logging.h"

/* Logging level */
static int log_consolelevel = GRF_LOGGING_WARN;

static void grf_logging_print(const char *msg)
{
    int len;
    int i;

    if (!msg)
    {
        printf("<0x00>\n");
        return;
    }
    
    len = strlen(msg);
    
    for (i = 0; i < len; i++)
    {
        if (isprint(msg[i]))
            printf("%c", msg[i]);
        else
            printf("<0x%02x>", msg[i]);
    }
    printf("\n");
}

static void grf_logging_print_hex(const char *msg, const char *hexstr, size_t hexlen)
{
	int len;
	int i;

	if (!msg)
	{
		printf("<0x00>\n");
		return;
	}

	len = strlen(msg);
	for (i = 0; i < len; i++)
	{
		if (isprint(msg[i]))
		printf("%c", msg[i]);
		else
		printf("<0x%02x>", msg[i]);
	}

	printf(" (");
	for (i = 0; i < hexlen; i++)
	{
		if (i == 0)
		printf("%02x", hexstr[i]);
		else
		printf(" %02x", hexstr[i]);
	}
	printf(")\n");
}

void grf_logging_setlevel(int level)
{
	log_consolelevel = level;
}

void grf_logging_log(int level, const char *fmt, ...)
{
	va_list          arglist;
	struct timespec  logtime;
	const char      *loglevel;
	char            *msg;
	
	/* We should not log this message... */
	if (level > log_consolelevel)
		return;

	/* Determine the point in time the logging occures */
	clock_gettime(CLOCK_REALTIME, &logtime);

	switch(level)
	{
		case GRF_LOGGING_DEBUG_IO:
			loglevel = "I/O:   ";
			break;
		case GRF_LOGGING_DEBUG:
			loglevel = "DEBUG: ";
			break;
		case GRF_LOGGING_INFO:
			loglevel = "INFO:  ";
			break;
		case GRF_LOGGING_WARN:
			loglevel = "WARN:  ";
			break;
		case GRF_LOGGING_ERR:
			loglevel = "ERROR: ";
			break;
		default:
			loglevel = "UNKNOWN: ";
			break;
	}

	printf("%.3f: %s", logtime.tv_sec + (float)logtime.tv_nsec*10E-9, loglevel);
	va_start(arglist, fmt);
	vasprintf(&msg, fmt, arglist);
	va_end(arglist);
	grf_logging_print(msg);
	if (msg)
		free(msg);
	fflush(stdout);
}

void grf_logging_log_hex(int level, const char *hexstr, size_t hexlen, const char *fmt, ...)
{
	va_list          arglist;
	struct timespec  logtime;
	const char      *loglevel;
	char            *msg;

	/* We should not log this message... */
	if (level > log_consolelevel)
		return;

	/* Determine the point in time the logging occures */
	clock_gettime(CLOCK_REALTIME, &logtime);

	switch(level)
	{
		case GRF_LOGGING_DEBUG_IO:
			loglevel = "I/O:   ";
			break;
		case GRF_LOGGING_DEBUG:
			loglevel = "DEBUG: ";
			break;
		case GRF_LOGGING_INFO:
			loglevel = "INFO:  ";
			break;
		case GRF_LOGGING_WARN:
			loglevel = "WARN:  ";
			break;
		case GRF_LOGGING_ERR:
			loglevel = "ERROR: ";
			break;
		default:
			loglevel = "UNKNOWN: ";
			break;
	}

	printf("%.3f: %s", logtime.tv_sec + (float)logtime.tv_nsec*10E-9, loglevel);
	va_start(arglist, fmt);
	vasprintf(&msg, fmt, arglist);
	va_end(arglist);
	grf_logging_print_hex(msg, hexstr, hexlen);
	free(msg);
}
