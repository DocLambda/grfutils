/*
 * Logging helper include file
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

#ifndef __GRF_LOGGING_H
#define __GRF_LOGGING_H

#define GRF_LOGGING_DEBUG_IO	4
#define GRF_LOGGING_DEBUG	3
#define GRF_LOGGING_INFO	2
#define GRF_LOGGING_WARN	1
#define GRF_LOGGING_ERR		0

#define grf_logging_dbg(_fmt_, ...)	grf_logging_log(GRF_LOGGING_DEBUG, (_fmt_), __VA_ARGS__)
#define grf_logging_info(_fmt_, ...)	grf_logging_log(GRF_LOGGING_INFO,  (_fmt_), __VA_ARGS__)
#define grf_logging_warn(_fmt_, ...)	grf_logging_log(GRF_LOGGING_WARN,  (_fmt_), __VA_ARGS__)
#define grf_logging_err(_fmt_, ...)	grf_logging_log(GRF_LOGGING_ERR,   (_fmt_), __VA_ARGS__)

#define grf_logging_dbg_hex(_hexstr_, _hexlen_, _fmt_, ...) grf_logging_log_hex(GRF_LOGGING_DEBUG, (_hexstr_), (_hexlen_), (_fmt_), __VA_ARGS__)
#define grf_logging_warn_hex(_hexstr_, _hexlen_, _fmt_, ...) grf_logging_log_hex(GRF_LOGGING_DEBUG, (_hexstr_), (_hexlen_), (_fmt_), __VA_ARGS__)

void grf_logging_setlevel(int level);
void grf_logging_log(int level, const char *fmt, ...);
void grf_logging_log_hex(int level, const char *hexstr, size_t hexlen, const char *fmt, ...);

#endif /* __GRF_LOGGING_H */
