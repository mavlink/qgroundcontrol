/* GStreamer DTMF plugin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_RTP_DTMF_COMMON_H__
#define __GST_RTP_DTMF_COMMON_H__

#include <gst/math-compat.h>

#define MIN_INTER_DIGIT_INTERVAL 100     /* ms */
#define MIN_PULSE_DURATION       250     /* ms */

#define MIN_VOLUME               0
#define MAX_VOLUME               36

#define MIN_EVENT                0
#define MAX_EVENT                15
#define MIN_EVENT_STRING         "0"
#define MAX_EVENT_STRING         "15"

typedef struct
{
  unsigned event:8;             /* Current DTMF event */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  unsigned volume:6;            /* power level of the tone, in dBm0 */
  unsigned r:1;                 /* Reserved-bit */
  unsigned e:1;                 /* End-bit */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
  unsigned e:1;                 /* End-bit */
  unsigned r:1;                 /* Reserved-bit */
  unsigned volume:6;            /* power level of the tone, in dBm0 */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
  unsigned duration:16;         /* Duration of digit, in timestamp units */
} GstRTPDTMFPayload;

#endif /* __GST_RTP_DTMF_COMMON_H__ */
