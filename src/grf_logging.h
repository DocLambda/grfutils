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

/*! \ingroup logging
 *  \file grf_logging.h
 *  \brief Data structures and functions to log information for different logging levels
 *
 * This file defines the API and the corresponding data structures for logging
 * processing information for different logging levels. The API defined here is
 * mostly used internally.
 *
 * @{
 */

#ifndef __GRF_LOGGING_H
#define __GRF_LOGGING_H

#define GRF_LOGGING_DEBUG_IO	4	/*!< Lowest defined log level to log raw I/O data passed from and to the radio module */
#define GRF_LOGGING_DEBUG		3	/*!< Log level to debug problems when communicating with the smoke detector */
#define GRF_LOGGING_INFO		2	/*!< Log level to show processing information */
#define GRF_LOGGING_WARN		1	/*!< Log level to show warnings about problems occured during communication with the smoke detector */
#define GRF_LOGGING_ERR			0	/*!< Log level to only show errors occured when communicating with the smoke detector */

#define grf_logging_dbg(_fmt_, ...)		grf_logging_log(GRF_LOGGING_DEBUG, (_fmt_), __VA_ARGS__)	/*!< Macro to log debug information */
#define grf_logging_info(_fmt_, ...)	grf_logging_log(GRF_LOGGING_INFO,  (_fmt_), __VA_ARGS__)	/*!< Macro to log processing information */
#define grf_logging_warn(_fmt_, ...)	grf_logging_log(GRF_LOGGING_WARN,  (_fmt_), __VA_ARGS__)	/*!< Macro to log warnings */
#define grf_logging_err(_fmt_, ...)		grf_logging_log(GRF_LOGGING_ERR,   (_fmt_), __VA_ARGS__)	/*!< Macro to log errors */

#define grf_logging_dbg_hex(_hexstr_, _hexlen_, _fmt_, ...)		grf_logging_log_hex(GRF_LOGGING_DEBUG, (_hexstr_), (_hexlen_), (_fmt_), __VA_ARGS__)	/*!< Macro to log debug information including a HEX output of the data */
#define grf_logging_warn_hex(_hexstr_, _hexlen_, _fmt_, ...)	grf_logging_log_hex(GRF_LOGGING_WARN,  (_hexstr_), (_hexlen_), (_fmt_), __VA_ARGS__)	/*!< Macro to log warnings including a HEX output of the data */

/*! \brief Set the level of output that should be shown.
 *
 *  This function sets the log-level which influences which
 *  logging information is shown to the user.
 *
 *  \param level	log-level which should be one of GRF_LOGGING_*
 */
void grf_logging_setlevel(int level);

/*! \brief Log information with the given level.
 *
 *  This function logs the given message with the given log-level.
 *  If the information is shown or not is influences by the log-level
 *  set with \ref grf_logging_setlevel().
 *  __For easier usage use one of the grf_logging_* macros!__
 *
 *  \param level	log-level of the message
 *  \param fmt		formating string
 *  \param ...		varadic data for the formating string
 */
void grf_logging_log(int level, const char *fmt, ...);

/*! \brief Log information with the given level and the given HEX data.
 *
 *  This function logs the given message with the given log-level just as
 *  \ref grf_logging_setlevel(), but will additionally show the given HEX
 *  information. If the information is shown or not is influences by the
 *  log-level set with \ref grf_logging_setlevel().
 *  __For easier usage use one of the grf_logging_*_hex macros!__
 *
 *  \param level	log-level of the message
 *  \param hexstr	data to show as hexadecimal
 *  \param hexlen	length of the hexadecimal *hexstr*
 *  \param fmt		formating string
 *  \param ...		varadic data for the formating string
 */
void grf_logging_log_hex(int level, const char *hexstr, size_t hexlen, const char *fmt, ...);

#endif /* __GRF_LOGGING_H */
/* @} */
