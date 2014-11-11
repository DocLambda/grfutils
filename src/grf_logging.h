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

void grf_logging_setlevel(int level);
void grf_logging_log(int level, const char *fmt, ...);

#endif /* __GRF_LOGGING_H */
