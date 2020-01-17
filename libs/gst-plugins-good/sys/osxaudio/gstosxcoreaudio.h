/*
 * GStreamer
 * Copyright (C) 2012 Fluendo S.A. <support@fluendo.com>
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
 *
 */

#ifndef __GST_CORE_AUDIO_H__
#define __GST_CORE_AUDIO_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/audio/audio-channels.h>
#ifdef HAVE_IOS
  #include <CoreAudio/CoreAudioTypes.h>
  #define AudioDeviceID gint
  #define kAudioDeviceUnknown 0
#else
  #include <CoreAudio/CoreAudio.h>
  #include <AudioToolbox/AudioToolbox.h>
  #if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
    #include <CoreServices/CoreServices.h>
    #define AudioComponentFindNext FindNextComponent
    #define AudioComponentInstanceNew OpenAComponent
    #define AudioComponentInstanceDispose CloseComponent
    #define AudioComponent Component
    #define AudioComponentDescription ComponentDescription
  #endif
#endif
#include <AudioUnit/AudioUnit.h>
#include "gstosxaudioelement.h"


G_BEGIN_DECLS

#define GST_TYPE_CORE_AUDIO \
  (gst_core_audio_get_type())
#define GST_CORE_AUDIO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CORE_AUDIO,GstCoreAudio))
#define GST_CORE_AUDIO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CORE_AUDIO,GstCoreAudioClass))
#define GST_CORE_AUDIO_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_CORE_AUDIO,GstCoreAudioClass))
#define GST_IS_CORE_AUDIO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CORE_AUDIO))
#define GST_IS_CORE_AUDIO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CORE_AUDIO))

/* TODO: Consider raising to 64 */
#define GST_OSX_AUDIO_MAX_CHANNEL (9)

#define CORE_AUDIO_FORMAT_IS_SPDIF(f) ((f).mFormat.mFormatID == 'IAC3' || (f).mFormat.mFormatID == 'iac3' || (f).mFormat.mFormatID == kAudioFormat60958AC3 || (f).mFormat.mFormatID == kAudioFormatAC3)

#define CORE_AUDIO_FORMAT "FormatID: %" GST_FOURCC_FORMAT " rate: %f flags: 0x%x BytesPerPacket: %u FramesPerPacket: %u BytesPerFrame: %u ChannelsPerFrame: %u BitsPerChannel: %u"
#define CORE_AUDIO_FORMAT_ARGS(f) GST_FOURCC_ARGS((unsigned int)(f).mFormatID),(f).mSampleRate,(unsigned int)(f).mFormatFlags,(unsigned int)(f).mBytesPerPacket,(unsigned int)(f).mFramesPerPacket,(unsigned int)(f).mBytesPerFrame,(unsigned int)(f).mChannelsPerFrame,(unsigned int)(f).mBitsPerChannel

#define CORE_AUDIO_INNER_SCOPE(core_audio) ((core_audio)->is_src ? kAudioUnitScope_Output : kAudioUnitScope_Input)
#define CORE_AUDIO_OUTER_SCOPE(core_audio) ((core_audio)->is_src ? kAudioUnitScope_Input : kAudioUnitScope_Output)
#define CORE_AUDIO_ELEMENT(core_audio) ((core_audio)->is_src ? 1 : 0)

typedef struct _GstCoreAudio GstCoreAudio;
typedef struct _GstCoreAudioClass GstCoreAudioClass;

struct _GstCoreAudio
{
  GObject object;

  GstObject *osxbuf;
  GstOsxAudioElementInterface *element;

  gboolean is_src;
  gboolean is_passthrough;
  AudioDeviceID device_id;
  gboolean cached_caps_valid; /* thread-safe flag */
  GstCaps *cached_caps;
  gint stream_idx;
  gboolean io_proc_active;
  gboolean io_proc_needs_deactivation;

  /* For LPCM in/out */
  AudioUnit audiounit;
  UInt32 recBufferSize; /* AudioUnitRender clobbers mDataByteSize */
  AudioBufferList *recBufferList;

#ifndef HAVE_IOS
  /* For SPDIF out */
  pid_t hog_pid;
  gboolean disabled_mixing;
  AudioStreamID stream_id;
  gboolean revert_format;
  AudioStreamBasicDescription original_format, stream_format;
  AudioDeviceIOProcID procID;
#endif
};

struct _GstCoreAudioClass
{
  GObjectClass parent_class;
};

GType gst_core_audio_get_type                                (void);

void gst_core_audio_init_debug (void);

GstCoreAudio * gst_core_audio_new                            (GstObject *osxbuf);

gboolean gst_core_audio_open                                 (GstCoreAudio *core_audio);

gboolean gst_core_audio_close                                (GstCoreAudio *core_audio);

gboolean gst_core_audio_initialize                           (GstCoreAudio *core_audio,
                                                              AudioStreamBasicDescription format,
                                                              GstCaps *caps,
                                                              gboolean is_passthrough);

void gst_core_audio_uninitialize                             (GstCoreAudio *core_audio);

gboolean gst_core_audio_start_processing                     (GstCoreAudio *core_audio);

gboolean gst_core_audio_pause_processing                     (GstCoreAudio *core_audio);

gboolean gst_core_audio_stop_processing                      (GstCoreAudio *core_audio);

gboolean gst_core_audio_get_samples_and_latency              (GstCoreAudio * core_audio,
                                                              gdouble rate,
                                                              guint *samples,
                                                              gdouble *latency);

void  gst_core_audio_set_volume                              (GstCoreAudio *core_audio,
                                                              gfloat volume);

gboolean gst_core_audio_audio_device_is_spdif_avail          (AudioDeviceID device_id);


gboolean gst_core_audio_select_device                        (GstCoreAudio * core_audio);

GstCaps *
gst_core_audio_probe_caps (GstCoreAudio * core_audio, GstCaps * in_caps);

AudioChannelLayout *
gst_core_audio_get_channel_layout (GstCoreAudio * core_audio, gboolean outer);

gboolean gst_core_audio_parse_channel_layout (AudioChannelLayout * layout,
    guint * channels, guint64 * channel_mask, GstAudioChannelPosition * pos);
GstCaps * gst_core_audio_asbd_to_caps (AudioStreamBasicDescription * asbd,
    AudioChannelLayout * layout);

G_END_DECLS

#endif /* __GST_CORE_AUDIO_H__ */
