/* GStreamer OSS4 audio plugin
 * Copyright (C) 2007-2008 Tim-Philipp Müller <tim centricular net>
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

#ifndef GST_OSS4_AUDIO_H
#define GST_OSS4_AUDIO_H

#include <gst/gst.h>
#include <gst/audio/audio.h>

/* This is the minimum version we require */
#define GST_MIN_OSS4_VERSION  0x040003

int       gst_oss4_audio_get_version (GstObject * obj, int fd);

gboolean  gst_oss4_audio_check_version (GstObject * obj, int fd);

GstCaps * gst_oss4_audio_probe_caps  (GstObject * obj, int fd);

gboolean  gst_oss4_audio_set_format  (GstObject * obj, int fd, GstAudioRingBufferSpec * spec);

GstCaps * gst_oss4_audio_get_template_caps (void);

gchar   * gst_oss4_audio_find_device (GstObject * oss);

#endif /* GST_OSS4_AUDIO_H */


