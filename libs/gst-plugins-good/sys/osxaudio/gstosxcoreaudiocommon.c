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

#include "gstosxcoreaudiocommon.h"

void
gst_core_audio_remove_render_callback (GstCoreAudio * core_audio)
{
  AURenderCallbackStruct input;
  OSStatus status;

  /* Deactivate the render callback by calling SetRenderCallback
   * with a NULL inputProc.
   */
  input.inputProc = NULL;
  input.inputProcRefCon = NULL;

  status = AudioUnitSetProperty (core_audio->audiounit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0,        /* N/A for global */
      &input, sizeof (input));

  if (status) {
    GST_WARNING_OBJECT (core_audio->osxbuf,
        "Failed to remove render callback %d", (int) status);
  }

  /* Remove the RenderNotify too */
  status = AudioUnitRemoveRenderNotify (core_audio->audiounit,
      (AURenderCallback) gst_core_audio_render_notify, core_audio);

  if (status) {
    GST_WARNING_OBJECT (core_audio->osxbuf,
        "Failed to remove render notify callback %d", (int) status);
  }

  /* We're deactivated.. */
  core_audio->io_proc_needs_deactivation = FALSE;
  core_audio->io_proc_active = FALSE;
}

OSStatus
gst_core_audio_render_notify (GstCoreAudio * core_audio,
    AudioUnitRenderActionFlags * ioActionFlags,
    const AudioTimeStamp * inTimeStamp,
    unsigned int inBusNumber,
    unsigned int inNumberFrames, AudioBufferList * ioData)
{
  /* Before rendering a frame, we get the PreRender notification.
   * Here, we detach the RenderCallback if we've been paused.
   *
   * This is necessary (rather than just directly detaching it) to
   * work around some thread-safety issues in CoreAudio
   */
  if ((*ioActionFlags) & kAudioUnitRenderAction_PreRender) {
    if (core_audio->io_proc_needs_deactivation) {
      gst_core_audio_remove_render_callback (core_audio);
    }
  }

  return noErr;
}

gboolean
gst_core_audio_io_proc_start (GstCoreAudio * core_audio)
{
  OSStatus status;
  AURenderCallbackStruct input;
  AudioUnitPropertyID callback_type;

  GST_DEBUG_OBJECT (core_audio->osxbuf,
      "osx ring buffer start ioproc: %p device_id %lu",
      core_audio->element->io_proc, (gulong) core_audio->device_id);
  if (!core_audio->io_proc_active) {
    callback_type = core_audio->is_src ?
        kAudioOutputUnitProperty_SetInputCallback :
        kAudioUnitProperty_SetRenderCallback;

    input.inputProc = (AURenderCallback) core_audio->element->io_proc;
    input.inputProcRefCon = core_audio->osxbuf;

    status = AudioUnitSetProperty (core_audio->audiounit, callback_type, kAudioUnitScope_Global, 0,     /* N/A for global */
        &input, sizeof (input));

    if (status) {
      GST_ERROR_OBJECT (core_audio->osxbuf,
          "AudioUnitSetProperty failed: %d", (int) status);
      return FALSE;
    }
    // ### does it make sense to do this notify stuff for input mode?
    status = AudioUnitAddRenderNotify (core_audio->audiounit,
        (AURenderCallback) gst_core_audio_render_notify, core_audio);

    if (status) {
      GST_ERROR_OBJECT (core_audio->osxbuf,
          "AudioUnitAddRenderNotify failed %d", (int) status);
      return FALSE;
    }
    core_audio->io_proc_active = TRUE;
  }

  core_audio->io_proc_needs_deactivation = FALSE;

  status = AudioOutputUnitStart (core_audio->audiounit);
  if (status) {
    GST_ERROR_OBJECT (core_audio->osxbuf, "AudioOutputUnitStart failed: %d",
        (int) status);
    return FALSE;
  }
  return TRUE;
}

gboolean
gst_core_audio_io_proc_stop (GstCoreAudio * core_audio)
{
  OSErr status;

  GST_DEBUG_OBJECT (core_audio->osxbuf,
      "osx ring buffer stop ioproc: %p device_id %lu",
      core_audio->element->io_proc, (gulong) core_audio->device_id);

  status = AudioOutputUnitStop (core_audio->audiounit);
  if (status) {
    GST_WARNING_OBJECT (core_audio->osxbuf,
        "AudioOutputUnitStop failed: %d", (int) status);
  }
  // ###: why is it okay to directly remove from here but not from pause() ?
  if (core_audio->io_proc_active) {
    gst_core_audio_remove_render_callback (core_audio);
  }
  return TRUE;
}

AudioBufferList *
buffer_list_alloc (UInt32 channels, UInt32 size, gboolean interleaved)
{
  AudioBufferList *list;
  gsize list_size;
  UInt32 num_buffers, n;

  num_buffers = interleaved ? 1 : channels;
  /* AudioBufferList member mBuffers is variable-length array */
  list_size = G_STRUCT_OFFSET (AudioBufferList, mBuffers[num_buffers]);
  list = (AudioBufferList *) g_malloc (list_size);

  list->mNumberBuffers = num_buffers;
  for (n = 0; n < num_buffers; ++n) {
    /* See http://lists.apple.com/archives/coreaudio-api/2015/Feb/msg00027.html */
    list->mBuffers[n].mNumberChannels = interleaved ? channels : 1;
    /* AudioUnitRender will keep overwriting mDataByteSize */
    list->mBuffers[n].mDataByteSize = size;
    list->mBuffers[n].mData = g_malloc (size);
  }

  return list;
}

void
buffer_list_free (AudioBufferList * list)
{
  UInt32 n;

  if (list == NULL)
    return;

  for (n = 0; n < list->mNumberBuffers; ++n) {
    g_free (list->mBuffers[n].mData);
  }

  g_free (list);
}

gboolean
gst_core_audio_bind_device (GstCoreAudio * core_audio)
{
  OSStatus status;

  /* Specify which device we're using. */
  GST_DEBUG_OBJECT (core_audio->osxbuf, "Bind AudioUnit to device %d",
      (int) core_audio->device_id);
  status = AudioUnitSetProperty (core_audio->audiounit,
      kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0,
      &core_audio->device_id, sizeof (AudioDeviceID));
  if (status) {
    GST_ERROR_OBJECT (core_audio->osxbuf, "Failed binding to device: %d",
        (int) status);
    goto audiounit_error;
  }
  return TRUE;

audiounit_error:
  if (core_audio->recBufferList) {
    buffer_list_free (core_audio->recBufferList);
    core_audio->recBufferList = NULL;
  }
  return FALSE;
}

static gboolean
_core_audio_set_property (GstCoreAudio * core_audio, AudioUnitPropertyID inID,
    void *inData, UInt32 inDataSize)
{
  OSStatus status;
  AudioUnitScope scope;
  AudioUnitElement element;

  scope = CORE_AUDIO_INNER_SCOPE (core_audio);
  element = CORE_AUDIO_ELEMENT (core_audio);

  status =
      AudioUnitSetProperty (core_audio->audiounit, inID, scope, element, inData,
      inDataSize);

  if (status != noErr) {
    GST_WARNING_OBJECT (core_audio->osxbuf,
        "Failed to set Audio Unit property: %d", (int) status);
    return FALSE;;
  }

  return TRUE;
}

/* The AudioUnit must be uninitialized before calling this */
gboolean
gst_core_audio_set_channel_layout (GstCoreAudio * core_audio,
    gint channels, GstCaps * caps)
{
  AudioChannelLayout *layout = NULL;
  gboolean ret;
  gsize layoutSize;
  gint i;
  GstStructure *structure;
  GstAudioChannelPosition positions[GST_OSX_AUDIO_MAX_CHANNEL];
  guint64 channel_mask;

  g_return_val_if_fail (channels <= GST_OSX_AUDIO_MAX_CHANNEL, FALSE);

  /* Determine the channel positions */
  structure = gst_caps_get_structure (caps, 0);
  channel_mask = 0;
  gst_structure_get (structure, "channel-mask", GST_TYPE_BITMASK, &channel_mask,
      NULL);

  if (channel_mask != 0)
    gst_audio_channel_positions_from_mask (channels, channel_mask, positions);

  /* AudioChannelLayout member mChannelDescriptions is variable-length array */
  layoutSize =
      G_STRUCT_OFFSET (AudioChannelLayout, mChannelDescriptions[channels]);
  layout = g_malloc (layoutSize);

  /* Fill out the AudioChannelLayout */
  layout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
  layout->mChannelBitmap = 0;   /* Not used */
  layout->mNumberChannelDescriptions = channels;
  for (i = 0; i < channels; i++) {
    if (channel_mask != 0) {
      layout->mChannelDescriptions[i].mChannelLabel =
          gst_audio_channel_position_to_core_audio (positions[i], i);
    } else {
      /* Discrete channel numbers are ORed into this */
      layout->mChannelDescriptions[i].mChannelLabel =
          kAudioChannelLabel_Discrete_0 | i;
    }

    /* Others unused */
    layout->mChannelDescriptions[i].mChannelFlags = kAudioChannelFlags_AllOff;
    layout->mChannelDescriptions[i].mCoordinates[0] = 0.f;
    layout->mChannelDescriptions[i].mCoordinates[1] = 0.f;
    layout->mChannelDescriptions[i].mCoordinates[2] = 0.f;
  }

  /* Sets GStreamer-ordered channel layout on the inner scope.
   * Reordering between the inner scope and outer scope is handled
   * by the Audio Unit itself. */
  ret = _core_audio_set_property (core_audio,
      kAudioUnitProperty_AudioChannelLayout, layout, layoutSize);

  g_free (layout);
  return ret;
}

/* The AudioUnit must be uninitialized before calling this */
gboolean
gst_core_audio_set_format (GstCoreAudio * core_audio,
    AudioStreamBasicDescription format)
{
  GST_DEBUG_OBJECT (core_audio->osxbuf, "Setting format for AudioUnit");

  return _core_audio_set_property (core_audio, kAudioUnitProperty_StreamFormat,
      &format, sizeof (AudioStreamBasicDescription));
}

gboolean
gst_core_audio_open_device (GstCoreAudio * core_audio, OSType sub_type,
    const gchar * adesc)
{
  AudioComponentDescription desc;
  AudioComponent comp;
  OSStatus status;
  AudioUnit unit;
  UInt32 enableIO;

  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = sub_type;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  comp = AudioComponentFindNext (NULL, &desc);

  if (comp == NULL) {
    GST_WARNING_OBJECT (core_audio->osxbuf, "Couldn't find %s component",
        adesc);
    return FALSE;
  }

  status = AudioComponentInstanceNew (comp, &unit);

  if (status) {
    GST_ERROR_OBJECT (core_audio->osxbuf, "Couldn't open %s component %d",
        adesc, (int) status);
    return FALSE;
  }

  if (core_audio->is_src) {
    /* enable input */
    enableIO = 1;
    status = AudioUnitSetProperty (unit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1,   /* 1 = input element */
        &enableIO, sizeof (enableIO));

    if (status) {
      AudioComponentInstanceDispose (unit);
      GST_WARNING_OBJECT (core_audio->osxbuf, "Failed to enable input: %d",
          (int) status);
      return FALSE;
    }

    /* disable output */
    enableIO = 0;
    status = AudioUnitSetProperty (unit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0,  /* 0 = output element */
        &enableIO, sizeof (enableIO));

    if (status) {
      AudioComponentInstanceDispose (unit);
      GST_WARNING_OBJECT (core_audio->osxbuf, "Failed to disable output: %d",
          (int) status);
      return FALSE;
    }
  }

  GST_DEBUG_OBJECT (core_audio->osxbuf, "Created %s AudioUnit: %p", adesc,
      unit);
  core_audio->audiounit = unit;
  return TRUE;
}

AudioChannelLabel
gst_audio_channel_position_to_core_audio (GstAudioChannelPosition
    position, int channel)
{
  switch (position) {
    case GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT:
      return kAudioChannelLabel_Left;
    case GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT:
      return kAudioChannelLabel_Right;
    case GST_AUDIO_CHANNEL_POSITION_REAR_CENTER:
      return kAudioChannelLabel_CenterSurround;
    case GST_AUDIO_CHANNEL_POSITION_REAR_LEFT:
      return kAudioChannelLabel_LeftSurround;
    case GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT:
      return kAudioChannelLabel_RightSurround;
    case GST_AUDIO_CHANNEL_POSITION_LFE1:
      return kAudioChannelLabel_LFEScreen;
    case GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER:
      return kAudioChannelLabel_Center;
    case GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT:
      return kAudioChannelLabel_LeftSurroundDirect;
    case GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT:
      return kAudioChannelLabel_RightSurroundDirect;
    case GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER:
      return kAudioChannelLabel_LeftCenter;
    case GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER:
      return kAudioChannelLabel_RightCenter;
    case GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT:
      return kAudioChannelLabel_TopBackLeft;
    case GST_AUDIO_CHANNEL_POSITION_TOP_REAR_CENTER:
      return kAudioChannelLabel_TopBackCenter;
    case GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT:
      return kAudioChannelLabel_TopBackRight;
    case GST_AUDIO_CHANNEL_POSITION_WIDE_LEFT:
      return kAudioChannelLabel_LeftWide;
    case GST_AUDIO_CHANNEL_POSITION_WIDE_RIGHT:
      return kAudioChannelLabel_RightWide;
    case GST_AUDIO_CHANNEL_POSITION_LFE2:
      return kAudioChannelLabel_LFE2;
    case GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT:
      return kAudioChannelLabel_VerticalHeightLeft;
    case GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT:
      return kAudioChannelLabel_VerticalHeightRight;
    case GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_CENTER:
      return kAudioChannelLabel_VerticalHeightCenter;

      /* Special position values */
    case GST_AUDIO_CHANNEL_POSITION_NONE:
      return kAudioChannelLabel_Discrete_0 | channel;
    case GST_AUDIO_CHANNEL_POSITION_MONO:
      return kAudioChannelLabel_Mono;

      /* Following positions are unmapped --
       * i.e. mapped to kAudioChannelLabel_Unknown: */
    case GST_AUDIO_CHANNEL_POSITION_TOP_CENTER:
    case GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_LEFT:
    case GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_RIGHT:
    case GST_AUDIO_CHANNEL_POSITION_BOTTOM_FRONT_CENTER:
    case GST_AUDIO_CHANNEL_POSITION_BOTTOM_FRONT_LEFT:
    case GST_AUDIO_CHANNEL_POSITION_BOTTOM_FRONT_RIGHT:
    case GST_AUDIO_CHANNEL_POSITION_SURROUND_LEFT:
    case GST_AUDIO_CHANNEL_POSITION_SURROUND_RIGHT:
    default:
      return kAudioChannelLabel_Unknown;
  }
}

/* Performs a best-effort conversion. 'channel' is used for warnings only. */
GstAudioChannelPosition
gst_core_audio_channel_label_to_gst (AudioChannelLabel label,
    int channel, gboolean warn)
{
  switch (label) {
    case kAudioChannelLabel_Left:
      return GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
    case kAudioChannelLabel_Right:
      return GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
    case kAudioChannelLabel_Center:
      return GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER;
    case kAudioChannelLabel_LFEScreen:
      return GST_AUDIO_CHANNEL_POSITION_LFE1;
    case kAudioChannelLabel_LeftSurround:
      return GST_AUDIO_CHANNEL_POSITION_REAR_LEFT;
    case kAudioChannelLabel_RightSurround:
      return GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT;
    case kAudioChannelLabel_LeftSurroundDirect:
      return GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT;
    case kAudioChannelLabel_RightSurroundDirect:
      return GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT;
    case kAudioChannelLabel_CenterSurround:
      return GST_AUDIO_CHANNEL_POSITION_REAR_CENTER;
    case kAudioChannelLabel_LeftCenter:
      return GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER;
    case kAudioChannelLabel_RightCenter:
      return GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER;
    case kAudioChannelLabel_TopBackLeft:
      return GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT;
    case kAudioChannelLabel_TopBackCenter:
      return GST_AUDIO_CHANNEL_POSITION_TOP_REAR_CENTER;
    case kAudioChannelLabel_TopBackRight:
      return GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT;
    case kAudioChannelLabel_LeftWide:
      return GST_AUDIO_CHANNEL_POSITION_WIDE_LEFT;
    case kAudioChannelLabel_RightWide:
      return GST_AUDIO_CHANNEL_POSITION_WIDE_RIGHT;
    case kAudioChannelLabel_LFE2:
      return GST_AUDIO_CHANNEL_POSITION_LFE2;
    case kAudioChannelLabel_VerticalHeightLeft:
      return GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT;
    case kAudioChannelLabel_VerticalHeightRight:
      return GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT;
    case kAudioChannelLabel_VerticalHeightCenter:
      return GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_CENTER;

      /* Special position values */

    case kAudioChannelLabel_Mono:
      /* GST_AUDIO_CHANNEL_POSITION_MONO is only for 1-channel layouts */
      return GST_AUDIO_CHANNEL_POSITION_INVALID;
    case kAudioChannelLabel_Discrete:
      return GST_AUDIO_CHANNEL_POSITION_NONE;

      /*
         Following labels are unmapped --
         i.e. mapped to GST_AUDIO_CHANNEL_POSITION_INVALID:
       */
    case kAudioChannelLabel_RearSurroundLeft:
    case kAudioChannelLabel_RearSurroundRight:
    case kAudioChannelLabel_TopCenterSurround:
    case kAudioChannelLabel_LeftTotal:
    case kAudioChannelLabel_RightTotal:
    case kAudioChannelLabel_HearingImpaired:
    case kAudioChannelLabel_Narration:
    case kAudioChannelLabel_DialogCentricMix:
    case kAudioChannelLabel_CenterSurroundDirect:
    case kAudioChannelLabel_Haptic:
    default:
      if (label >> 16 != 0) {   /* kAudioChannelLabel_Discrete_N */
        /* no way to store discrete channel order */
        if (warn)
          GST_WARNING
              ("Core Audio channel %u labeled kAudioChannelLabel_Discrete_%u -- discrete order will be lost",
              channel, ((unsigned int) label) & 0xFFFF);
        return GST_AUDIO_CHANNEL_POSITION_NONE;
      } else {
        if (warn)
          GST_WARNING
              ("Core Audio channel %u has unsupported label %d and will be skipped",
              channel, (int) label);
        return GST_AUDIO_CHANNEL_POSITION_INVALID;
      }
  }
}

void
gst_core_audio_dump_channel_layout (AudioChannelLayout * channel_layout)
{
  UInt32 i;

  GST_DEBUG ("mChannelLayoutTag: 0x%lx",
      (unsigned long) channel_layout->mChannelLayoutTag);
  GST_DEBUG ("mChannelBitmap: 0x%lx",
      (unsigned long) channel_layout->mChannelBitmap);
  GST_DEBUG ("mNumberChannelDescriptions: %lu",
      (unsigned long) channel_layout->mNumberChannelDescriptions);
  for (i = 0; i < channel_layout->mNumberChannelDescriptions; i++) {
    AudioChannelDescription *channel_desc =
        &channel_layout->mChannelDescriptions[i];
    GST_DEBUG ("  mChannelLabel: 0x%lx mChannelFlags: 0x%lx "
        "mCoordinates[0]: %f mCoordinates[1]: %f "
        "mCoordinates[2]: %f",
        (unsigned long) channel_desc->mChannelLabel,
        (unsigned long) channel_desc->mChannelFlags,
        channel_desc->mCoordinates[0], channel_desc->mCoordinates[1],
        channel_desc->mCoordinates[2]);
  }
}
