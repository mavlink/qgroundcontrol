/*
 * GStreamer
 * Copyright (C) 2012-2013 Fluendo S.A. <support@fluendo.com>
 *   Authors: Josep Torra Vallès <josep@fluendo.com>
 *            Andoni Morales Alastruey <amorales@fluendo.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "gstosxcoreaudio.h"
#include <gst/audio/audio-channels.h>

typedef struct
{
  GMutex lock;
  GCond cond;
} PropertyMutex;

gboolean gst_core_audio_bind_device                       (GstCoreAudio *core_audio);

void gst_core_audio_dump_channel_layout                   (AudioChannelLayout * channel_layout);

void gst_core_audio_remove_render_callback                (GstCoreAudio * core_audio);

gboolean gst_core_audio_io_proc_start                     (GstCoreAudio * core_audio);

gboolean gst_core_audio_io_proc_stop                      (GstCoreAudio * core_audio);

AudioBufferList * buffer_list_alloc                       (UInt32 channels, UInt32 size, gboolean interleaved);

void buffer_list_free                                     (AudioBufferList * list);

gboolean gst_core_audio_set_format                        (GstCoreAudio * core_audio,
                                                           AudioStreamBasicDescription format);

gboolean gst_core_audio_set_channel_layout                (GstCoreAudio * core_audio,
                                                           gint channels, GstCaps * caps);

gboolean gst_core_audio_open_device                       (GstCoreAudio *core_audio,
                                                           OSType sub_type,
                                                           const gchar *adesc);

OSStatus gst_core_audio_render_notify                     (GstCoreAudio * core_audio,
                                                           AudioUnitRenderActionFlags * ioActionFlags,
                                                           const AudioTimeStamp * inTimeStamp,
                                                           unsigned int inBusNumber,
                                                           unsigned int inNumberFrames,
                                                           AudioBufferList * ioData);

AudioChannelLabel gst_audio_channel_position_to_core_audio (GstAudioChannelPosition position, int channel);

GstAudioChannelPosition gst_core_audio_channel_label_to_gst (AudioChannelLabel label, int channel, gboolean warn);
