/*
 *  GStreamer pulseaudio plugin
 *
 *  Copyright (c) 2004-2008 Lennart Poettering
 *
 *  gst-pulse is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  gst-pulse is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with gst-pulse; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 *  USA.
 */

#ifndef __GST_PULSEUTIL_H__
#define __GST_PULSEUTIL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <pulse/pulseaudio.h>
#include <gst/audio/gstaudioringbuffer.h>
#include <gst/audio/gstaudiosink.h>


#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
# define _PULSE_FORMATS   "{ S16LE, S16BE, F32LE, F32BE, S32LE, S32BE, " \
                     "S24LE, S24BE, S24_32LE, S24_32BE, U8 }"
#else
# define _PULSE_FORMATS   "{ S16BE, S16LE, F32BE, F32LE, S32BE, S32LE, " \
                     "S24BE, S24LE, S24_32BE, S24_32LE, U8 }"
#endif

/* NOTE! that we do NOT actually support rate=MAX. This must be fixed up using
 * gst_pulse_fix_pcm_caps() before being used. */
#define _PULSE_CAPS_LINEAR \
    "audio/x-raw, " \
      "format = (string) " _PULSE_FORMATS ", " \
      "layout = (string) interleaved, " \
      "rate = (int) [ 1, MAX ], " \
      "channels = (int) [ 1, 32 ]; "
#define _PULSE_CAPS_ALAW \
    "audio/x-alaw, " \
      "rate = (int) [ 1, MAX], " \
      "channels = (int) [ 1, 32 ]; "
#define _PULSE_CAPS_MULAW \
    "audio/x-mulaw, " \
      "rate = (int) [ 1, MAX], " \
      "channels = (int) [ 1, 32 ]; "

#define _PULSE_CAPS_AC3 "audio/x-ac3, framed = (boolean) true; "
#define _PULSE_CAPS_EAC3 "audio/x-eac3, framed = (boolean) true; "
#define _PULSE_CAPS_DTS "audio/x-dts, framed = (boolean) true, " \
    "block-size = (int) { 512, 1024, 2048 }; "
#define _PULSE_CAPS_MP3 "audio/mpeg, mpegversion = (int) 1, " \
    "mpegaudioversion = (int) [ 1, 3 ], parsed = (boolean) true;"
#define _PULSE_CAPS_AAC "audio/mpeg, mpegversion = (int) { 2, 4 }, " \
    "framed = (boolean) true, stream-format = (string) adts;"

#define _PULSE_CAPS_PCM \
  _PULSE_CAPS_LINEAR \
  _PULSE_CAPS_ALAW \
  _PULSE_CAPS_MULAW


gboolean gst_pulse_fill_sample_spec (GstAudioRingBufferSpec * spec,
    pa_sample_spec * ss);
gboolean gst_pulse_fill_format_info (GstAudioRingBufferSpec * spec,
    pa_format_info ** f, guint * channels);
const char * gst_pulse_sample_format_to_caps_format (pa_sample_format_t sf);

gchar *gst_pulse_client_name (void);

pa_channel_map *gst_pulse_gst_to_channel_map (pa_channel_map * map,
    const GstAudioRingBufferSpec * spec);

GstAudioRingBufferSpec *gst_pulse_channel_map_to_gst (const pa_channel_map * map,
    GstAudioRingBufferSpec * spec);

void gst_pulse_cvolume_from_linear (pa_cvolume *v, unsigned channels, gdouble volume);

pa_proplist *gst_pulse_make_proplist (const GstStructure *properties);
GstStructure *gst_pulse_make_structure (pa_proplist *properties);

GstCaps * gst_pulse_format_info_to_caps (pa_format_info * format);
GstCaps * gst_pulse_fix_pcm_caps (GstCaps * incaps);

#endif
