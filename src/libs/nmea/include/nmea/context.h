/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: context.h 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifndef __NMEA_CONTEXT_H__
#define __NMEA_CONTEXT_H__

#include "config.h"

#define NMEA_DEF_PARSEBUFF  (1024)
#define NMEA_MIN_PARSEBUFF  (256)

#ifdef  __cplusplus
extern "C" {
#endif

typedef void (*nmeaTraceFunc)(const char *str, int str_size);
typedef void (*nmeaErrorFunc)(const char *str, int str_size);

typedef struct _nmeaPROPERTY
{
    nmeaTraceFunc   trace_func;
    nmeaErrorFunc   error_func;
    int             parse_buff_size;

} nmeaPROPERTY;

nmeaPROPERTY * nmea_property();

void nmea_trace(const char *str, ...);
void nmea_trace_buff(const char *buff, int buff_size);
void nmea_error(const char *str, ...);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_CONTEXT_H__ */
