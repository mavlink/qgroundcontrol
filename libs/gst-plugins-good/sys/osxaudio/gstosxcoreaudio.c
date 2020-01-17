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
#include "gstosxcoreaudiocommon.h"

GST_DEBUG_CATEGORY_STATIC (osx_audio_debug);
#define GST_CAT_DEFAULT osx_audio_debug

G_DEFINE_TYPE (GstCoreAudio, gst_core_audio, G_TYPE_OBJECT);

#ifdef HAVE_IOS
#include "gstosxcoreaudioremoteio.c"
#else
#include "gstosxcoreaudiohal.c"
#endif


static void
gst_core_audio_class_init (GstCoreAudioClass * klass)
{
}

static void
gst_core_audio_init (GstCoreAudio * core_audio)
{
  core_audio->is_passthrough = FALSE;
  core_audio->device_id = kAudioDeviceUnknown;
  core_audio->is_src = FALSE;
  core_audio->audiounit = NULL;
  core_audio->cached_caps = NULL;
  core_audio->cached_caps_valid = FALSE;
#ifndef HAVE_IOS
  core_audio->hog_pid = -1;
  core_audio->disabled_mixing = FALSE;
#endif
}

static gboolean
_is_outer_scope (AudioUnitScope scope, AudioUnitElement element)
{
  return
      (scope == kAudioUnitScope_Input && element == 1) ||
      (scope == kAudioUnitScope_Output && element == 0);
}

static void
_audio_unit_property_listener (void *inRefCon, AudioUnit inUnit,
    AudioUnitPropertyID inID, AudioUnitScope inScope,
    AudioUnitElement inElement)
{
  GstCoreAudio *core_audio;

  core_audio = GST_CORE_AUDIO (inRefCon);
  g_assert (inUnit == core_audio->audiounit);

  switch (inID) {
    case kAudioUnitProperty_AudioChannelLayout:
    case kAudioUnitProperty_StreamFormat:
      if (_is_outer_scope (inScope, inElement)) {
        /* We don't push gst_event_new_caps here (for src),
         * nor gst_event_new_reconfigure (for sink), since Core Audio continues
         * to happily function with the old format, doing conversion/resampling
         * as needed.
         * This merely "refreshes" our PREFERRED caps. */

        /* This function is called either from a Core Audio thread
         * or as a result of a Core Audio API (e.g. AudioUnitInitialize)
         * from our own thread. In the latter case, osxbuf can be
         * already locked (GStreamer's mutex is not recursive).
         * For this reason we use a boolean flag instead of nullifying
         * cached_caps. */
        core_audio->cached_caps_valid = FALSE;
      }
      break;
  }
}

/**************************
 *       Public API       *
 *************************/

GstCoreAudio *
gst_core_audio_new (GstObject * osxbuf)
{
  GstCoreAudio *core_audio;

  core_audio = g_object_new (GST_TYPE_CORE_AUDIO, NULL);
  core_audio->osxbuf = osxbuf;
  core_audio->cached_caps = NULL;
  return core_audio;
}

gboolean
gst_core_audio_close (GstCoreAudio * core_audio)
{
  OSStatus status;

  /* Uninitialize the AudioUnit */
  status = AudioUnitUninitialize (core_audio->audiounit);
  if (status) {
    GST_ERROR_OBJECT (core_audio, "Failed to uninitialize AudioUnit: %d",
        (int) status);
    return FALSE;
  }

  AudioUnitRemovePropertyListenerWithUserData (core_audio->audiounit,
      kAudioUnitProperty_AudioChannelLayout, _audio_unit_property_listener,
      core_audio);
  AudioUnitRemovePropertyListenerWithUserData (core_audio->audiounit,
      kAudioUnitProperty_StreamFormat, _audio_unit_property_listener,
      core_audio);

  /* core_audio->osxbuf is already locked at this point */
  core_audio->cached_caps_valid = FALSE;
  gst_caps_replace (&core_audio->cached_caps, NULL);

  AudioComponentInstanceDispose (core_audio->audiounit);
  core_audio->audiounit = NULL;
  return TRUE;
}

gboolean
gst_core_audio_open (GstCoreAudio * core_audio)
{
  OSStatus status;

  /* core_audio->osxbuf is already locked at this point */
  core_audio->cached_caps_valid = FALSE;
  gst_caps_replace (&core_audio->cached_caps, NULL);

  if (!gst_core_audio_open_impl (core_audio))
    return FALSE;

  /* Add property listener */
  status = AudioUnitAddPropertyListener (core_audio->audiounit,
      kAudioUnitProperty_AudioChannelLayout, _audio_unit_property_listener,
      core_audio);
  if (status != noErr) {
    GST_ERROR_OBJECT (core_audio, "Failed to add audio channel layout property "
        "listener for AudioUnit: %d", (int) status);
  }
  status = AudioUnitAddPropertyListener (core_audio->audiounit,
      kAudioUnitProperty_StreamFormat, _audio_unit_property_listener,
      core_audio);
  if (status != noErr) {
    GST_ERROR_OBJECT (core_audio, "Failed to add stream format property "
        "listener for AudioUnit: %d", (int) status);
  }

  /* Initialize the AudioUnit. We keep the audio unit initialized early so that
   * we can probe the underlying device. */
  status = AudioUnitInitialize (core_audio->audiounit);
  if (status) {
    GST_ERROR_OBJECT (core_audio, "Failed to initialize AudioUnit: %d",
        (int) status);
    return FALSE;
  }

  return TRUE;
}

gboolean
gst_core_audio_start_processing (GstCoreAudio * core_audio)
{
  return gst_core_audio_start_processing_impl (core_audio);
}

gboolean
gst_core_audio_pause_processing (GstCoreAudio * core_audio)
{
  return gst_core_audio_pause_processing_impl (core_audio);
}

gboolean
gst_core_audio_stop_processing (GstCoreAudio * core_audio)
{
  return gst_core_audio_stop_processing_impl (core_audio);
}

gboolean
gst_core_audio_get_samples_and_latency (GstCoreAudio * core_audio,
    gdouble rate, guint * samples, gdouble * latency)
{
  return gst_core_audio_get_samples_and_latency_impl (core_audio, rate,
      samples, latency);
}

gboolean
gst_core_audio_initialize (GstCoreAudio * core_audio,
    AudioStreamBasicDescription format, GstCaps * caps, gboolean is_passthrough)
{
  guint32 frame_size;

  GST_DEBUG_OBJECT (core_audio,
      "Initializing: passthrough:%d caps:%" GST_PTR_FORMAT, is_passthrough,
      caps);

  if (!gst_core_audio_initialize_impl (core_audio, format, caps,
          is_passthrough, &frame_size)) {
    return FALSE;
  }

  if (core_audio->is_src) {
    /* create AudioBufferList needed for recording */
    core_audio->recBufferSize = frame_size * format.mBytesPerFrame;
    core_audio->recBufferList =
        buffer_list_alloc (format.mChannelsPerFrame, core_audio->recBufferSize,
        /* Currently always TRUE (i.e. interleaved) */
        !(format.mFormatFlags & kAudioFormatFlagIsNonInterleaved));
  }

  return TRUE;
}

void
gst_core_audio_uninitialize (GstCoreAudio * core_audio)
{
  buffer_list_free (core_audio->recBufferList);
  core_audio->recBufferList = NULL;
}

void
gst_core_audio_set_volume (GstCoreAudio * core_audio, gfloat volume)
{
  AudioUnitSetParameter (core_audio->audiounit, kHALOutputParam_Volume,
      kAudioUnitScope_Global, 0, (float) volume, 0);
}

gboolean
gst_core_audio_select_device (GstCoreAudio * core_audio)
{
  return gst_core_audio_select_device_impl (core_audio);
}

void
gst_core_audio_init_debug (void)
{
  GST_DEBUG_CATEGORY_INIT (osx_audio_debug, "osxaudio", 0,
      "OSX Audio Elements");
}

gboolean
gst_core_audio_audio_device_is_spdif_avail (AudioDeviceID device_id)
{
  return gst_core_audio_audio_device_is_spdif_avail_impl (device_id);
}

/* Does the channel have at least one positioned channel?
 * (GStreamer is more strict than Core Audio, in that it requires either
 * all channels to be positioned, or all unpositioned.) */
static gboolean
_is_core_audio_layout_positioned (AudioChannelLayout * layout)
{
  guint i;

  g_assert (layout->mChannelLayoutTag ==
      kAudioChannelLayoutTag_UseChannelDescriptions);

  for (i = 0; i < layout->mNumberChannelDescriptions; ++i) {
    GstAudioChannelPosition p =
        gst_core_audio_channel_label_to_gst
        (layout->mChannelDescriptions[i].mChannelLabel, i, FALSE);

    if (p >= 0)                 /* not special positition */
      return TRUE;
  }

  return FALSE;
}

static void
_core_audio_parse_channel_descriptions (AudioChannelLayout * layout,
    guint * channels, guint64 * channel_mask, GstAudioChannelPosition * pos)
{
  gboolean positioned;
  guint i;

  g_assert (layout->mChannelLayoutTag ==
      kAudioChannelLayoutTag_UseChannelDescriptions);

  positioned = _is_core_audio_layout_positioned (layout);
  *channel_mask = 0;

  /* Go over all labels, either taking only positioned or only
   * unpositioned channels, up to GST_OSX_AUDIO_MAX_CHANNEL channels.
   *
   * The resulting 'pos' array will contain either:
   *  - only regular (>= 0) positions
   *  - only GST_AUDIO_CHANNEL_POSITION_NONE positions
   * in a compact form, skipping over all unsupported positions.
   */
  *channels = 0;
  for (i = 0; i < layout->mNumberChannelDescriptions; ++i) {
    GstAudioChannelPosition p =
        gst_core_audio_channel_label_to_gst
        (layout->mChannelDescriptions[i].mChannelLabel, i, TRUE);

    /* In positioned layouts, skip all unpositioned channels.
     * In unpositioned layouts, skip all invalid channels. */
    if ((positioned && p >= 0) ||
        (!positioned && p == GST_AUDIO_CHANNEL_POSITION_NONE)) {

      if (pos)
        pos[*channels] = p;
      *channel_mask |= G_GUINT64_CONSTANT (1) << p;
      ++(*channels);

      if (*channels == GST_OSX_AUDIO_MAX_CHANNEL)
        break;                  /* not to overflow */
    }
  }
}

gboolean
gst_core_audio_parse_channel_layout (AudioChannelLayout * layout,
    guint * channels, guint64 * channel_mask, GstAudioChannelPosition * pos)
{
  g_assert (channels != NULL);
  g_assert (channel_mask != NULL);
  g_assert (layout != NULL);

  if (layout->mChannelLayoutTag ==
      kAudioChannelLayoutTag_UseChannelDescriptions) {

    switch (layout->mNumberChannelDescriptions) {
      case 0:
        if (pos)
          pos[0] = GST_AUDIO_CHANNEL_POSITION_NONE;
        *channels = 0;
        *channel_mask = 0;
        return TRUE;
      case 1:
        if (pos)
          pos[0] = GST_AUDIO_CHANNEL_POSITION_MONO;
        *channels = 1;
        *channel_mask = 0;
        return TRUE;
      case 2:
        if (pos) {
          pos[0] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
          pos[1] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
        }
        *channels = 2;
        *channel_mask =
            GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_LEFT) |
            GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_RIGHT);
        return TRUE;
      default:
        _core_audio_parse_channel_descriptions (layout, channels, channel_mask,
            pos);
        return TRUE;
    }
  } else if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_Mono) {
    if (pos)
      pos[0] = GST_AUDIO_CHANNEL_POSITION_MONO;
    *channels = 1;
    *channel_mask = 0;
    return TRUE;
  } else if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_Stereo ||
      layout->mChannelLayoutTag == kAudioChannelLayoutTag_StereoHeadphones ||
      layout->mChannelLayoutTag == kAudioChannelLayoutTag_Binaural) {
    if (pos) {
      pos[0] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
      pos[1] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
    }
    *channels = 2;
    *channel_mask =
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_RIGHT);
    return TRUE;
  } else if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_Quadraphonic) {
    if (pos) {
      pos[0] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
      pos[1] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
      pos[2] = GST_AUDIO_CHANNEL_POSITION_SURROUND_LEFT;
      pos[3] = GST_AUDIO_CHANNEL_POSITION_SURROUND_RIGHT;
    }
    *channels = 4;
    *channel_mask =
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_RIGHT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (SURROUND_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (SURROUND_RIGHT);
    return TRUE;
  } else if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_Pentagonal) {
    if (pos) {
      pos[0] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
      pos[1] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
      pos[2] = GST_AUDIO_CHANNEL_POSITION_SURROUND_LEFT;
      pos[3] = GST_AUDIO_CHANNEL_POSITION_SURROUND_RIGHT;
      pos[4] = GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER;
    }
    *channels = 5;
    *channel_mask =
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_RIGHT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (SURROUND_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (SURROUND_RIGHT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_CENTER);
    return TRUE;
  } else if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_Cube) {
    if (pos) {
      pos[0] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
      pos[1] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
      pos[2] = GST_AUDIO_CHANNEL_POSITION_REAR_LEFT;
      pos[3] = GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT;
      pos[4] = GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT;
      pos[5] = GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT;
      pos[6] = GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT;
      pos[7] = GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT;

    }
    *channels = 8;
    *channel_mask =
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_RIGHT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (REAR_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (REAR_RIGHT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (TOP_FRONT_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (TOP_FRONT_RIGHT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (TOP_REAR_LEFT) |
        GST_AUDIO_CHANNEL_POSITION_MASK (TOP_REAR_RIGHT);
    return TRUE;
  } else {
    GST_WARNING
        ("AudioChannelLayoutTag: %u not yet supported",
        layout->mChannelLayoutTag);
    *channels = 0;
    *channel_mask = 0;
    return FALSE;
  }
}

/* Converts an AudioStreamBasicDescription to preferred caps.
 *
 * These caps will indicate the AU element's canonical format, which won't
 * make Core Audio resample nor convert.
 *
 * NOTE ON MULTI-CHANNEL AUDIO:
 *
 * If layout is not NULL, resulting caps will only include the subset
 * of channels supported by GStreamer. If the Core Audio layout contained
 * ANY positioned channels, then ONLY positioned channels will be included
 * in the resulting caps. Otherwise, resulting caps will be unpositioned,
 * and include only unpositioned channels.
 * (Channels with unsupported AudioChannelLabel will be skipped either way.)
 *
 * Naturally, the number of channels indicated by 'channels' can be lower
 * than the AU element's total number of channels.
 */
GstCaps *
gst_core_audio_asbd_to_caps (AudioStreamBasicDescription * asbd,
    AudioChannelLayout * layout)
{
  GstAudioInfo info;
  GstAudioFormat format = GST_AUDIO_FORMAT_UNKNOWN;
  guint rate, channels, bps, endianness;
  guint64 channel_mask;
  gboolean sign, interleaved;
  GstAudioChannelPosition pos[GST_OSX_AUDIO_MAX_CHANNEL];

  if (asbd->mFormatID != kAudioFormatLinearPCM) {
    GST_WARNING ("Only linear PCM is supported");
    goto error;
  }

  if (!(asbd->mFormatFlags & kAudioFormatFlagIsPacked)) {
    GST_WARNING ("Only packed formats supported");
    goto error;
  }

  if (asbd->mFormatFlags & kLinearPCMFormatFlagsSampleFractionMask) {
    GST_WARNING ("Fixed point audio is unsupported");
    goto error;
  }

  rate = asbd->mSampleRate;
  if (rate == kAudioStreamAnyRate) {
    GST_WARNING ("No sample rate");
    goto error;
  }

  bps = asbd->mBitsPerChannel;
  endianness = asbd->mFormatFlags & kAudioFormatFlagIsBigEndian ?
      G_BIG_ENDIAN : G_LITTLE_ENDIAN;
  sign = asbd->mFormatFlags & kAudioFormatFlagIsSignedInteger ? TRUE : FALSE;
  interleaved = asbd->mFormatFlags & kAudioFormatFlagIsNonInterleaved ?
      TRUE : FALSE;

  if (asbd->mFormatFlags & kAudioFormatFlagIsFloat) {
    if (bps == 32) {
      if (endianness == G_LITTLE_ENDIAN)
        format = GST_AUDIO_FORMAT_F32LE;
      else
        format = GST_AUDIO_FORMAT_F32BE;

    } else if (bps == 64) {
      if (endianness == G_LITTLE_ENDIAN)
        format = GST_AUDIO_FORMAT_F64LE;
      else
        format = GST_AUDIO_FORMAT_F64BE;
    }
  } else {
    format = gst_audio_format_build_integer (sign, endianness, bps, bps);
  }

  if (format == GST_AUDIO_FORMAT_UNKNOWN) {
    GST_WARNING ("Unsupported sample format");
    goto error;
  }

  if (layout) {
    if (!gst_core_audio_parse_channel_layout (layout, &channels, &channel_mask,
            pos)) {
      GST_WARNING
          ("Failed to parse channel layout, best effort channels layout mapping will be used");
      layout = NULL;
    }
  }

  if (layout) {
    /* The AU can have arbitrary channel order, but we're using GstAudioInfo
     * which supports only the GStreamer channel order.
     * Also, we're eventually producing caps, which only have channel-mask
     * (whose implied order is the GStreamer channel order). */
    gst_audio_channel_positions_to_valid_order (pos, channels);

    gst_audio_info_set_format (&info, format, rate, channels, pos);
  } else {
    channels = MIN (asbd->mChannelsPerFrame, GST_OSX_AUDIO_MAX_CHANNEL);
    gst_audio_info_set_format (&info, format, rate, channels, NULL);
  }

  return gst_audio_info_to_caps (&info);

error:
  return NULL;
}

static gboolean
_core_audio_get_property (GstCoreAudio * core_audio, gboolean outer,
    AudioUnitPropertyID inID, void *inData, UInt32 * inDataSize)
{
  OSStatus status;
  AudioUnitScope scope;
  AudioUnitElement element;

  scope = outer ?
      CORE_AUDIO_OUTER_SCOPE (core_audio) : CORE_AUDIO_INNER_SCOPE (core_audio);
  element = CORE_AUDIO_ELEMENT (core_audio);

  status =
      AudioUnitGetProperty (core_audio->audiounit, inID, scope, element, inData,
      inDataSize);

  return status == noErr;
}

static gboolean
_core_audio_get_stream_format (GstCoreAudio * core_audio,
    AudioStreamBasicDescription * asbd, gboolean outer)
{
  UInt32 size;

  size = sizeof (AudioStreamBasicDescription);
  return _core_audio_get_property (core_audio, outer,
      kAudioUnitProperty_StreamFormat, asbd, &size);
}

AudioChannelLayout *
gst_core_audio_get_channel_layout (GstCoreAudio * core_audio, gboolean outer)
{
  UInt32 size;
  AudioChannelLayout *layout;

  if (core_audio->is_src) {
    GST_WARNING_OBJECT (core_audio,
        "gst_core_audio_get_channel_layout not supported on source.");
    return NULL;
  }

  if (!_core_audio_get_property (core_audio, outer,
          kAudioUnitProperty_AudioChannelLayout, NULL, &size)) {
    GST_WARNING_OBJECT (core_audio, "unable to get channel layout");
    return NULL;
  }

  layout = g_malloc (size);
  if (!_core_audio_get_property (core_audio, outer,
          kAudioUnitProperty_AudioChannelLayout, layout, &size)) {
    GST_WARNING_OBJECT (core_audio, "unable to get channel layout");
    g_free (layout);
    return NULL;
  }

  return layout;
}

#define STEREO_CHANNEL_MASK \
  (GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_LEFT) | \
   GST_AUDIO_CHANNEL_POSITION_MASK (FRONT_RIGHT))

GstCaps *
gst_core_audio_probe_caps (GstCoreAudio * core_audio, GstCaps * in_caps)
{
  guint i, channels;
  gboolean spdif_allowed;
  AudioChannelLayout *layout;
  AudioStreamBasicDescription outer_asbd;
  gboolean got_outer_asbd;
  GstCaps *caps = NULL;
  guint64 channel_mask;

  /* Get the ASBD of the outer scope (i.e. input scope of Input,
   * output scope of Output).
   * This ASBD indicates the hardware format. */
  got_outer_asbd =
      _core_audio_get_stream_format (core_audio, &outer_asbd, TRUE);

  /* Collect info about the HW capabilities and preferences */
  spdif_allowed =
      gst_core_audio_audio_device_is_spdif_avail (core_audio->device_id);
  if (!core_audio->is_src)
    layout = gst_core_audio_get_channel_layout (core_audio, TRUE);
  else
    layout = NULL;              /* no supported for sources */

  GST_DEBUG_OBJECT (core_audio, "Selected device ID: %u SPDIF allowed: %d",
      (unsigned) core_audio->device_id, spdif_allowed);

  if (layout) {
    if (!gst_core_audio_parse_channel_layout (layout, &channels, &channel_mask,
            NULL)) {
      GST_WARNING_OBJECT (core_audio, "Failed to parse channel layout");
      channel_mask = 0;
    }

    /* If available, start with the preferred caps. */
    if (got_outer_asbd)
      caps = gst_core_audio_asbd_to_caps (&outer_asbd, layout);

    g_free (layout);
  } else if (got_outer_asbd) {
    channels = outer_asbd.mChannelsPerFrame;
    channel_mask = 0;
    /* If available, start with the preferred caps */
    caps = gst_core_audio_asbd_to_caps (&outer_asbd, NULL);
  } else {
    GST_ERROR_OBJECT (core_audio,
        "Unable to get any information about hardware");
    return NULL;
  }

  /* Append the allowed subset based on the template caps  */
  if (!caps)
    caps = gst_caps_new_empty ();
  for (i = 0; i < gst_caps_get_size (in_caps); i++) {
    GstStructure *in_s;

    in_s = gst_caps_get_structure (in_caps, i);

    if (gst_structure_has_name (in_s, "audio/x-ac3") ||
        gst_structure_has_name (in_s, "audio/x-dts")) {
      if (spdif_allowed) {
        gst_caps_append_structure (caps, gst_structure_copy (in_s));
      }
    } else {
      GstStructure *out_s;

      out_s = gst_structure_copy (in_s);
      gst_structure_set (out_s, "channels", G_TYPE_INT, channels, NULL);
      if (channel_mask != 0) {
        /* positioned layout */
        gst_structure_set (out_s,
            "channel-mask", GST_TYPE_BITMASK, channel_mask, NULL);
      } else {
        /* unpositioned layout */
        gst_structure_remove_field (out_s, "channel-mask");
      }

#ifndef HAVE_IOS
      if (core_audio->is_src && got_outer_asbd
          && outer_asbd.mSampleRate != kAudioStreamAnyRate) {
        /* According to Core Audio engineer, AUHAL does not support sample rate conversion.
         * on sources. Therefore, we fixate the sample rate.
         *
         * "You definitely cannot do rate conversion as part of getting input from AUHAL.
         *  That's the most common cause of those "cannot do in current context" errors."
         * http://lists.apple.com/archives/coreaudio-api/2006/Sep/msg00088.html
         */
        gst_structure_set (out_s, "rate", G_TYPE_INT,
            (gint) outer_asbd.mSampleRate, NULL);
      }
#endif

      /* Special cases for upmixing and downmixing.
       * Other than that, the AUs don't upmix or downmix multi-channel audio,
       * e.g. if you push 5.1-surround audio to a stereo configuration,
       * the left and right channels will be played accordingly,
       * and the rest will be dropped. */
      if (channels == 1) {
        /* If have mono, then also offer stereo since CoreAudio downmixes to it */
        GstStructure *stereo = gst_structure_copy (out_s);
        gst_structure_remove_field (out_s, "channel-mask");
        gst_structure_set (stereo, "channels", G_TYPE_INT, 2,
            "channel-mask", GST_TYPE_BITMASK, STEREO_CHANNEL_MASK, NULL);
        gst_caps_append_structure (caps, stereo);
        gst_caps_append_structure (caps, out_s);
      } else if (channels == 2 && (channel_mask == 0
              || channel_mask == STEREO_CHANNEL_MASK)) {
        /* If have stereo channels, then also offer mono since CoreAudio
         * upmixes it. */
        GstStructure *mono = gst_structure_copy (out_s);
        gst_structure_set (mono, "channels", G_TYPE_INT, 1, NULL);
        gst_structure_remove_field (mono, "channel-mask");
        gst_structure_set (out_s, "channel-mask", GST_TYPE_BITMASK,
            STEREO_CHANNEL_MASK, NULL);

        gst_caps_append_structure (caps, out_s);
        gst_caps_append_structure (caps, mono);
      } else {
        /* Otherwise just add the caps */
        gst_caps_append_structure (caps, out_s);
      }
    }
  }

  GST_DEBUG_OBJECT (core_audio, "Probed caps:%" GST_PTR_FORMAT, caps);
  return caps;
}
