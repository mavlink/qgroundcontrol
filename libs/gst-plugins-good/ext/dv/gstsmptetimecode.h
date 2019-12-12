/* GStreamer
 * Copyright (C) 2009 David A. Schleef <ds@schleef.org>
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

#ifndef _GST_SMPTE_TIME_CODE_H_
#define _GST_SMPTE_TIME_CODE_H_

#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _GstSMPTETimeCode GstSMPTETimeCode;

/**
 * GstSMPTETimeCode:
 * @GST_SMPTE_TIME_CODE_SYSTEM_30: 29.97 frame per second system (NTSC)
 * @GST_SMPTE_TIME_CODE_SYSTEM_25: 25 frame per second system (PAL)
 * @GST_SMPTE_TIME_CODE_SYSTEM_24: 24 frame per second system
 *
 * Enum value representing SMPTE Time Code system.
 */
typedef enum {
  GST_SMPTE_TIME_CODE_SYSTEM_30 = 0,
  GST_SMPTE_TIME_CODE_SYSTEM_25,
  GST_SMPTE_TIME_CODE_SYSTEM_24
} GstSMPTETimeCodeSystem;

struct _GstSMPTETimeCode {
  int hours;
  int minutes;
  int seconds;
  int frames;
};

#define GST_SMPTE_TIME_CODE_SYSTEM_IS_VALID(x) \
  ((x) >= GST_SMPTE_TIME_CODE_SYSTEM_30 && (x) <= GST_SMPTE_TIME_CODE_SYSTEM_24)

#define GST_SMPTE_TIME_CODE_FORMAT "02d:%02d:%02d:%02d"
#define GST_SMPTE_TIME_CODE_ARGS(timecode) \
  (timecode)->hours, (timecode)->minutes, \
  (timecode)->seconds, (timecode)->frames

gboolean gst_smpte_time_code_is_valid (GstSMPTETimeCodeSystem system,
    GstSMPTETimeCode *time_code);
gboolean gst_smpte_time_code_from_frame_number (GstSMPTETimeCodeSystem system,
    GstSMPTETimeCode *time_code, int frame_number);
gboolean gst_smpte_time_code_get_frame_number (GstSMPTETimeCodeSystem system,
    int *frame_number, GstSMPTETimeCode *time_code);
GstClockTime gst_smpte_time_code_get_timestamp (GstSMPTETimeCodeSystem system,
    GstSMPTETimeCode *time_code);

G_END_DECLS

#endif

