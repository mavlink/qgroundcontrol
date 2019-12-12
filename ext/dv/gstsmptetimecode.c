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

/*
 * Utility functions for handing SMPTE Time Codes, as described in
 * SMPTE Standard 12M-1999.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstsmptetimecode.h"

#define NTSC_FRAMES_PER_10_MINS (10*60*30 - 10*2 + 2)
#define NTSC_FRAMES_PER_HOUR (6*NTSC_FRAMES_PER_10_MINS)

/**
 * gst_smpte_time_code_from_frame_number:
 * @system: SMPTE Time Code system
 * @time_code: pointer to time code structure
 * @frame_number: integer frame number
 *
 * Converts a frame number to a time code.
 *
 * Returns: TRUE if the conversion was successful
 */
gboolean
gst_smpte_time_code_from_frame_number (GstSMPTETimeCodeSystem system,
    GstSMPTETimeCode * time_code, int frame_number)
{
  int ten_mins;
  int n;

  g_return_val_if_fail (time_code != NULL, FALSE);
  g_return_val_if_fail (GST_SMPTE_TIME_CODE_SYSTEM_IS_VALID (system), FALSE);

  time_code->hours = 99;
  time_code->minutes = 99;
  time_code->seconds = 99;
  time_code->frames = 99;

  if (frame_number < 0)
    return FALSE;

  switch (system) {
    case GST_SMPTE_TIME_CODE_SYSTEM_30:
      if (frame_number >= 24 * NTSC_FRAMES_PER_HOUR)
        return FALSE;

      ten_mins = frame_number / NTSC_FRAMES_PER_10_MINS;
      frame_number -= ten_mins * NTSC_FRAMES_PER_10_MINS;

      time_code->hours = ten_mins / 6;
      time_code->minutes = 10 * (ten_mins % 6);

      if (frame_number < 2) {
        /* treat the first two frames of each ten minutes specially */
        time_code->seconds = 0;
        time_code->frames = frame_number;
      } else {
        n = (frame_number - 2) / (60 * 30 - 2);
        time_code->minutes += n;
        frame_number -= n * (60 * 30 - 2);

        time_code->seconds = frame_number / 30;
        time_code->frames = frame_number % 30;
      }
      break;
    case GST_SMPTE_TIME_CODE_SYSTEM_25:
      if (frame_number >= 24 * 60 * 60 * 25)
        return FALSE;

      time_code->frames = frame_number % 25;
      frame_number /= 25;
      time_code->seconds = frame_number % 60;
      frame_number /= 60;
      time_code->minutes = frame_number % 60;
      frame_number /= 60;
      time_code->hours = frame_number;
      break;
    case GST_SMPTE_TIME_CODE_SYSTEM_24:
      if (frame_number >= 24 * 60 * 60 * 24)
        return FALSE;

      time_code->frames = frame_number % 24;
      frame_number /= 24;
      time_code->seconds = frame_number % 60;
      frame_number /= 60;
      time_code->minutes = frame_number % 60;
      frame_number /= 60;
      time_code->hours = frame_number;
      break;
  }

  return TRUE;
}

/**
 * gst_smpte_time_code_is_valid:
 * @system: SMPTE Time Code system
 * @time_code: pointer to time code structure
 *
 * Checks that the time code represents a valid time code.
 *
 * Returns: TRUE if the time code is valid
 */
gboolean
gst_smpte_time_code_is_valid (GstSMPTETimeCodeSystem system,
    GstSMPTETimeCode * time_code)
{
  g_return_val_if_fail (time_code != NULL, FALSE);
  g_return_val_if_fail (GST_SMPTE_TIME_CODE_SYSTEM_IS_VALID (system), FALSE);

  if (time_code->hours < 0 || time_code->hours >= 24)
    return FALSE;
  if (time_code->minutes < 0 || time_code->minutes >= 60)
    return FALSE;
  if (time_code->seconds < 0 || time_code->seconds >= 60)
    return FALSE;
  if (time_code->frames < 0)
    return FALSE;

  switch (system) {
    case GST_SMPTE_TIME_CODE_SYSTEM_30:
      if (time_code->frames >= 30)
        return FALSE;
      if (time_code->frames >= 2 || time_code->seconds > 0)
        return TRUE;
      if (time_code->minutes % 10 != 0)
        return FALSE;
      break;
    case GST_SMPTE_TIME_CODE_SYSTEM_25:
      if (time_code->frames >= 25)
        return FALSE;
      break;
    case GST_SMPTE_TIME_CODE_SYSTEM_24:
      if (time_code->frames >= 24)
        return FALSE;
      break;
  }
  return TRUE;
}

/**
 * gst_smpte_time_get_frame_number:
 * @system: SMPTE Time Code system
 * @frame_number: pointer to frame number
 * @time_code: pointer to time code structure
 *
 * Converts the time code structure to a linear frame number.
 *
 * Returns: TRUE if the time code could be converted
 */
gboolean
gst_smpte_time_code_get_frame_number (GstSMPTETimeCodeSystem system,
    int *frame_number, GstSMPTETimeCode * time_code)
{
  int frame = 0;

  g_return_val_if_fail (GST_SMPTE_TIME_CODE_SYSTEM_IS_VALID (system), FALSE);
  g_return_val_if_fail (time_code != NULL, FALSE);

  if (!gst_smpte_time_code_is_valid (system, time_code)) {
    return FALSE;
  }

  switch (system) {
    case GST_SMPTE_TIME_CODE_SYSTEM_30:
      frame = time_code->hours * NTSC_FRAMES_PER_HOUR;
      frame += (time_code->minutes / 10) * NTSC_FRAMES_PER_10_MINS;
      frame += (time_code->minutes % 10) * (30 * 60 - 2);
      frame += time_code->seconds * 30;
      break;
    case GST_SMPTE_TIME_CODE_SYSTEM_25:
      time_code->frames =
          25 * ((time_code->hours * 60 + time_code->minutes) * 60 +
          time_code->seconds);
      break;
    case GST_SMPTE_TIME_CODE_SYSTEM_24:
      time_code->frames =
          24 * ((time_code->hours * 60 + time_code->minutes) * 60 +
          time_code->seconds);
      break;
  }
  frame += time_code->frames;

  if (frame_number) {
    *frame_number = frame;
  }

  return TRUE;
}

/**
 * gst_smpte_time_get_timestamp:
 * @system: SMPTE Time Code system
 * @time_code: pointer to time code structure
 *
 * Converts the time code structure to a timestamp.
 *
 * Returns: Time stamp for time code, or GST_CLOCK_TIME_NONE if time
 *   code is invalid.
 */
GstClockTime
gst_smpte_time_code_get_timestamp (GstSMPTETimeCodeSystem system,
    GstSMPTETimeCode * time_code)
{
  int frame_number;

  g_return_val_if_fail (GST_SMPTE_TIME_CODE_SYSTEM_IS_VALID (system),
      GST_CLOCK_TIME_NONE);
  g_return_val_if_fail (time_code != NULL, GST_CLOCK_TIME_NONE);

  if (gst_smpte_time_code_get_frame_number (system, &frame_number, time_code)) {
    static const int framerate_n[3] = { 3000, 25, 24 };
    static const int framerate_d[3] = { 1001, 1, 1 };

    return gst_util_uint64_scale (frame_number,
        GST_SECOND * framerate_d[system], framerate_n[system]);
  }

  return GST_CLOCK_TIME_NONE;
}
