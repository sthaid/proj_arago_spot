#ifndef __UTIL_MISC_H__
#define __UTIL_MISC_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

// -----------------  LOGGING  -----------------------------------------------

#define INFO(fmt, args...) \
    do { \
        logmsg("INFO", __func__, fmt, ## args); \
    } while (0)
#define WARN(fmt, args...) \
    do { \
        logmsg("WARN", __func__, fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        logmsg("ERROR", __func__, fmt, ## args); \
    } while (0)
#define DEBUG(fmt, args...) \
    do { \
        if (debug_enabled) { \
            logmsg("DEBUG", __func__, fmt, ## args); \
        } \
    } while (0)
#define FATAL(fmt, args...) \
    do { \
        logmsg("FATAL", __func__, fmt, ## args); \
        exit(1); \
    } while (0)

extern bool debug_enabled;
void logmsg(char * lvl, const char * func, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));

// -----------------  TIME UTILS  --------------------------------------------

uint64_t microsec_timer(void);
uint64_t get_real_time_us(void);
char * time2str(char * str, int64_t us, bool gmt, bool display_ms, bool display_date);

#endif
