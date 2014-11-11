#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>

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



void grf_logging_setlevel(int level)
{
	log_consolelevel = level;
}

void grf_logging_log(int level, const char *fmt, ...)
{
	va_list     arglist;
	const char *loglevel;
	char       *msg;
	
	/* We should not log this message... */
	if (level > log_consolelevel)
		return;

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

	printf("%s", loglevel);
	va_start(arglist, fmt);
	vasprintf(&msg, fmt, arglist);
	va_end(arglist);
	grf_logging_print(msg);
	free(msg);
}
