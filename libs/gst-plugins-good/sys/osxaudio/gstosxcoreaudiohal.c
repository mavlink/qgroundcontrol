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

#include <unistd.h>             /* for getpid */
#include "gstosxaudiosink.h"

static inline gboolean
_audio_system_set_runloop (CFRunLoopRef runLoop)
{
  OSStatus status = noErr;

  gboolean res = FALSE;

  AudioObjectPropertyAddress runloopAddress = {
    kAudioHardwarePropertyRunLoop,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectSetPropertyData (kAudioObjectSystemObject,
      &runloopAddress, 0, NULL, sizeof (CFRunLoopRef), &runLoop);
  if (status == noErr) {
    res = TRUE;
  } else {
    GST_ERROR ("failed to set runloop to %p: %d", runLoop, (int) status);
  }

  return res;
}

static inline AudioDeviceID
_audio_system_get_default_device (gboolean output)
{
  OSStatus status = noErr;
  UInt32 propertySize = sizeof (AudioDeviceID);
  AudioDeviceID device_id = kAudioDeviceUnknown;
  AudioObjectPropertySelector prop_selector;

  prop_selector = output ? kAudioHardwarePropertyDefaultOutputDevice :
      kAudioHardwarePropertyDefaultInputDevice;

  AudioObjectPropertyAddress defaultDeviceAddress = {
    prop_selector,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyData (kAudioObjectSystemObject,
      &defaultDeviceAddress, 0, NULL, &propertySize, &device_id);
  if (status != noErr) {
    GST_ERROR ("failed getting default output device: %d", (int) status);
  }

  GST_DEBUG ("Default device id: %u", (unsigned) device_id);

  return device_id;
}

static inline AudioDeviceID *
_audio_system_get_devices (gint * ndevices)
{
  OSStatus status = noErr;
  UInt32 propertySize = 0;
  AudioDeviceID *devices = NULL;

  AudioObjectPropertyAddress audioDevicesAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyDataSize (kAudioObjectSystemObject,
      &audioDevicesAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    GST_WARNING ("failed getting number of devices: %d", (int) status);
    return NULL;
  }

  *ndevices = propertySize / sizeof (AudioDeviceID);

  devices = (AudioDeviceID *) g_malloc (propertySize);
  if (devices) {
    status = AudioObjectGetPropertyData (kAudioObjectSystemObject,
        &audioDevicesAddress, 0, NULL, &propertySize, devices);
    if (status != noErr) {
      GST_WARNING ("failed getting the list of devices: %d", (int) status);
      g_free (devices);
      *ndevices = 0;
      return NULL;
    }
  }
  return devices;
}

static inline gboolean
_audio_device_is_alive (AudioDeviceID device_id, gboolean output)
{
  OSStatus status = noErr;
  int alive = FALSE;
  UInt32 propertySize = sizeof (alive);
  AudioObjectPropertyScope prop_scope;

  prop_scope = output ? kAudioDevicePropertyScopeOutput :
      kAudioDevicePropertyScopeInput;

  AudioObjectPropertyAddress audioDeviceAliveAddress = {
    kAudioDevicePropertyDeviceIsAlive,
    prop_scope,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyData (device_id,
      &audioDeviceAliveAddress, 0, NULL, &propertySize, &alive);
  if (status != noErr) {
    alive = FALSE;
  }

  return alive;
}

static inline guint
_audio_device_get_latency (AudioDeviceID device_id)
{
  OSStatus status = noErr;
  UInt32 latency = 0;
  UInt32 propertySize = sizeof (latency);

  AudioObjectPropertyAddress audioDeviceLatencyAddress = {
    kAudioDevicePropertyLatency,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyData (device_id,
      &audioDeviceLatencyAddress, 0, NULL, &propertySize, &latency);
  if (status != noErr) {
    GST_ERROR ("failed to get latency: %d", (int) status);
    latency = -1;
  }

  return latency;
}

static inline pid_t
_audio_device_get_hog (AudioDeviceID device_id)
{
  OSStatus status = noErr;
  pid_t hog_pid;
  UInt32 propertySize = sizeof (hog_pid);

  AudioObjectPropertyAddress audioDeviceHogModeAddress = {
    kAudioDevicePropertyHogMode,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyData (device_id,
      &audioDeviceHogModeAddress, 0, NULL, &propertySize, &hog_pid);
  if (status != noErr) {
    GST_ERROR ("failed to get hog: %d", (int) status);
    hog_pid = -1;
  }

  return hog_pid;
}

static inline gboolean
_audio_device_set_hog (AudioDeviceID device_id, pid_t hog_pid)
{
  OSStatus status = noErr;
  UInt32 propertySize = sizeof (hog_pid);
  gboolean res = FALSE;

  AudioObjectPropertyAddress audioDeviceHogModeAddress = {
    kAudioDevicePropertyHogMode,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectSetPropertyData (device_id,
      &audioDeviceHogModeAddress, 0, NULL, propertySize, &hog_pid);

  if (status == noErr) {
    res = TRUE;
  } else {
    GST_ERROR ("failed to set hog: %d", (int) status);
  }

  return res;
}

static inline gboolean
_audio_device_set_mixing (AudioDeviceID device_id, gboolean enable_mix)
{
  OSStatus status = noErr;
  UInt32 propertySize = 0, can_mix = enable_mix;
  Boolean writable = FALSE;
  gboolean res = FALSE;

  AudioObjectPropertyAddress audioDeviceSupportsMixingAddress = {
    kAudioDevicePropertySupportsMixing,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  if (AudioObjectHasProperty (device_id, &audioDeviceSupportsMixingAddress)) {
    /* Set mixable to false if we are allowed to */
    status = AudioObjectIsPropertySettable (device_id,
        &audioDeviceSupportsMixingAddress, &writable);
    if (status) {
      GST_DEBUG ("AudioObjectIsPropertySettable: %d", (int) status);
    }
    status = AudioObjectGetPropertyDataSize (device_id,
        &audioDeviceSupportsMixingAddress, 0, NULL, &propertySize);
    if (status) {
      GST_DEBUG ("AudioObjectGetPropertyDataSize: %d", (int) status);
    }
    status = AudioObjectGetPropertyData (device_id,
        &audioDeviceSupportsMixingAddress, 0, NULL, &propertySize, &can_mix);
    if (status) {
      GST_DEBUG ("AudioObjectGetPropertyData: %d", (int) status);
    }

    if (status == noErr && writable) {
      can_mix = enable_mix;
      status = AudioObjectSetPropertyData (device_id,
          &audioDeviceSupportsMixingAddress, 0, NULL, propertySize, &can_mix);
      res = TRUE;
    }

    if (status != noErr) {
      GST_ERROR ("failed to set mixmode: %d", (int) status);
    }
  } else {
    GST_DEBUG ("property not found, mixing coudln't be changed");
  }

  return res;
}

static inline gchar *
_audio_device_get_name (AudioDeviceID device_id, gboolean output)
{
  OSStatus status = noErr;
  UInt32 propertySize = 0;
  gchar *device_name = NULL;
  AudioObjectPropertyScope prop_scope;

  prop_scope = output ? kAudioDevicePropertyScopeOutput :
      kAudioDevicePropertyScopeInput;

  AudioObjectPropertyAddress deviceNameAddress = {
    kAudioDevicePropertyDeviceName,
    prop_scope,
    kAudioObjectPropertyElementMaster
  };

  /* Get the length of the device name */
  status = AudioObjectGetPropertyDataSize (device_id,
      &deviceNameAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    goto beach;
  }

  /* Get the name of the device */
  device_name = (gchar *) g_malloc (propertySize);
  status = AudioObjectGetPropertyData (device_id,
      &deviceNameAddress, 0, NULL, &propertySize, device_name);
  if (status != noErr) {
    g_free (device_name);
    device_name = NULL;
  }

beach:
  return device_name;
}

static inline gboolean
_audio_device_has_output (AudioDeviceID device_id)
{
  OSStatus status = noErr;
  UInt32 propertySize;

  AudioObjectPropertyAddress streamsAddress = {
    kAudioDevicePropertyStreams,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyDataSize (device_id,
      &streamsAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    return FALSE;
  }
  if (propertySize == 0) {
    return FALSE;
  }

  return TRUE;
}

#ifdef GST_CORE_AUDIO_DEBUG
static AudioChannelLayout *
gst_core_audio_audio_device_get_channel_layout (AudioDeviceID device_id,
    gboolean output)
{
  OSStatus status = noErr;
  UInt32 propertySize = 0;
  AudioChannelLayout *layout = NULL;
  AudioObjectPropertyScope prop_scope;

  prop_scope = output ? kAudioDevicePropertyScopeOutput :
      kAudioDevicePropertyScopeInput;

  AudioObjectPropertyAddress channelLayoutAddress = {
    kAudioDevicePropertyPreferredChannelLayout,
    prop_scope,
    kAudioObjectPropertyElementMaster
  };

  /* Get the length of the default channel layout structure */
  status = AudioObjectGetPropertyDataSize (device_id,
      &channelLayoutAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    GST_ERROR ("failed to get preferred layout: %d", (int) status);
    goto beach;
  }

  /* Get the default channel layout of the device */
  layout = (AudioChannelLayout *) g_malloc (propertySize);
  status = AudioObjectGetPropertyData (device_id,
      &channelLayoutAddress, 0, NULL, &propertySize, layout);
  if (status != noErr) {
    GST_ERROR ("failed to get preferred layout: %d", (int) status);
    goto failed;
  }

  if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap) {
    /* bitmap defined channellayout */
    status =
        AudioFormatGetProperty (kAudioFormatProperty_ChannelLayoutForBitmap,
        sizeof (UInt32), &layout->mChannelBitmap, &propertySize, layout);
    if (status != noErr) {
      GST_ERROR ("failed to get layout for bitmap: %d", (int) status);
      goto failed;
    }
  } else if (layout->mChannelLayoutTag !=
      kAudioChannelLayoutTag_UseChannelDescriptions) {
    /* layouttags defined channellayout */
    status = AudioFormatGetProperty (kAudioFormatProperty_ChannelLayoutForTag,
        sizeof (AudioChannelLayoutTag), &layout->mChannelLayoutTag,
        &propertySize, layout);
    if (status != noErr) {
      GST_ERROR ("failed to get layout for tag: %d", (int) status);
      goto failed;
    }
  }

  gst_core_audio_dump_channel_layout (layout);

beach:
  return layout;

failed:
  g_free (layout);
  return NULL;
}
#endif

static inline AudioStreamID *
_audio_device_get_streams (AudioDeviceID device_id, gint * nstreams)
{
  OSStatus status = noErr;
  UInt32 propertySize = 0;
  AudioStreamID *streams = NULL;

  AudioObjectPropertyAddress streamsAddress = {
    kAudioDevicePropertyStreams,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyDataSize (device_id,
      &streamsAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    GST_WARNING ("failed getting number of streams: %d", (int) status);
    return NULL;
  }

  *nstreams = propertySize / sizeof (AudioStreamID);
  streams = (AudioStreamID *) g_malloc (propertySize);

  if (streams) {
    status = AudioObjectGetPropertyData (device_id,
        &streamsAddress, 0, NULL, &propertySize, streams);
    if (status != noErr) {
      GST_WARNING ("failed getting the list of streams: %d", (int) status);
      g_free (streams);
      *nstreams = 0;
      return NULL;
    }
  }

  return streams;
}

static inline guint
_audio_stream_get_latency (AudioStreamID stream_id)
{
  OSStatus status = noErr;
  UInt32 latency;
  UInt32 propertySize = sizeof (latency);

  AudioObjectPropertyAddress latencyAddress = {
    kAudioStreamPropertyLatency,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyData (stream_id,
      &latencyAddress, 0, NULL, &propertySize, &latency);
  if (status != noErr) {
    GST_ERROR ("failed to get latency: %d", (int) status);
    latency = -1;
  }

  return latency;
}

static inline gboolean
_audio_stream_get_current_format (AudioStreamID stream_id,
    AudioStreamBasicDescription * format)
{
  OSStatus status = noErr;
  UInt32 propertySize = sizeof (AudioStreamBasicDescription);

  AudioObjectPropertyAddress formatAddress = {
    kAudioStreamPropertyPhysicalFormat,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyData (stream_id,
      &formatAddress, 0, NULL, &propertySize, format);
  if (status != noErr) {
    GST_ERROR ("failed to get current format: %d", (int) status);
    return FALSE;
  }

  return TRUE;
}

static inline gboolean
_audio_stream_set_current_format (AudioStreamID stream_id,
    AudioStreamBasicDescription format)
{
  OSStatus status = noErr;
  UInt32 propertySize = sizeof (AudioStreamBasicDescription);

  AudioObjectPropertyAddress formatAddress = {
    kAudioStreamPropertyPhysicalFormat,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectSetPropertyData (stream_id,
      &formatAddress, 0, NULL, propertySize, &format);
  if (status != noErr) {
    GST_ERROR ("failed to set current format: %d", (int) status);
    return FALSE;
  }

  return TRUE;
}

static inline AudioStreamRangedDescription *
_audio_stream_get_formats (AudioStreamID stream_id, gint * nformats)
{
  OSStatus status = noErr;
  UInt32 propertySize = 0;
  AudioStreamRangedDescription *formats = NULL;

  AudioObjectPropertyAddress formatsAddress = {
    kAudioStreamPropertyAvailablePhysicalFormats,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyDataSize (stream_id,
      &formatsAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    GST_WARNING ("failed getting number of stream formats: %d", (int) status);
    return NULL;
  }

  *nformats = propertySize / sizeof (AudioStreamRangedDescription);

  formats = (AudioStreamRangedDescription *) g_malloc (propertySize);
  if (formats) {
    status = AudioObjectGetPropertyData (stream_id,
        &formatsAddress, 0, NULL, &propertySize, formats);
    if (status != noErr) {
      GST_WARNING ("failed getting the list of stream formats: %d",
          (int) status);
      g_free (formats);
      *nformats = 0;
      return NULL;
    }
  }
  return formats;
}

static inline gboolean
_audio_stream_is_spdif_avail (AudioStreamID stream_id)
{
  AudioStreamRangedDescription *formats;
  gint i, nformats = 0;
  gboolean res = FALSE;

  formats = _audio_stream_get_formats (stream_id, &nformats);
  GST_DEBUG ("found %d stream formats", nformats);

  if (formats) {
    GST_DEBUG ("formats supported on stream ID: %u", (unsigned) stream_id);

    for (i = 0; i < nformats; i++) {
      GST_DEBUG ("  " CORE_AUDIO_FORMAT,
          CORE_AUDIO_FORMAT_ARGS (formats[i].mFormat));

      if (CORE_AUDIO_FORMAT_IS_SPDIF (formats[i])) {
        res = TRUE;
      }
    }
    g_free (formats);
  }

  return res;
}

static OSStatus
_audio_stream_format_listener (AudioObjectID inObjectID,
    UInt32 inNumberAddresses,
    const AudioObjectPropertyAddress inAddresses[], void *inClientData)
{
  OSStatus status = noErr;
  guint i;
  PropertyMutex *prop_mutex = inClientData;

  for (i = 0; i < inNumberAddresses; i++) {
    if (inAddresses[i].mSelector == kAudioStreamPropertyPhysicalFormat) {
      g_mutex_lock (&prop_mutex->lock);
      g_cond_signal (&prop_mutex->cond);
      g_mutex_unlock (&prop_mutex->lock);
      break;
    }
  }
  return (status);
}

static gboolean
_audio_stream_change_format (AudioStreamID stream_id,
    AudioStreamBasicDescription format)
{
  OSStatus status = noErr;
  gint i;
  gboolean ret = FALSE;
  AudioStreamBasicDescription cformat;
  PropertyMutex prop_mutex;

  AudioObjectPropertyAddress formatAddress = {
    kAudioStreamPropertyPhysicalFormat,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  GST_DEBUG ("setting stream format: " CORE_AUDIO_FORMAT,
      CORE_AUDIO_FORMAT_ARGS (format));

  /* Condition because SetProperty is asynchronous */
  g_mutex_init (&prop_mutex.lock);
  g_cond_init (&prop_mutex.cond);

  g_mutex_lock (&prop_mutex.lock);

  /* Install the property listener to serialize the operations */
  status = AudioObjectAddPropertyListener (stream_id, &formatAddress,
      _audio_stream_format_listener, (void *) &prop_mutex);
  if (status != noErr) {
    GST_ERROR ("AudioObjectAddPropertyListener failed: %d", (int) status);
    goto done;
  }

  /* Change the format */
  if (!_audio_stream_set_current_format (stream_id, format)) {
    goto done;
  }

  /* The AudioObjectSetProperty is not only asynchronous
   * it is also not atomic in its behaviour.
   * Therefore we check 4 times before we really give up. */
  for (i = 0; i < 4; i++) {
    GTimeVal timeout;

    g_get_current_time (&timeout);
    g_time_val_add (&timeout, 250000);

    if (!g_cond_wait_until (&prop_mutex.cond, &prop_mutex.lock, timeout.tv_sec)) {
      GST_LOG ("timeout...");
    }

    if (_audio_stream_get_current_format (stream_id, &cformat)) {
      GST_DEBUG ("current stream format: " CORE_AUDIO_FORMAT,
          CORE_AUDIO_FORMAT_ARGS (cformat));

      if (cformat.mSampleRate == format.mSampleRate &&
          cformat.mFormatID == format.mFormatID &&
          cformat.mFramesPerPacket == format.mFramesPerPacket) {
        /* The right format is now active */
        break;
      }
    }
  }

  if (cformat.mSampleRate != format.mSampleRate ||
      cformat.mFormatID != format.mFormatID ||
      cformat.mFramesPerPacket != format.mFramesPerPacket) {
    goto done;
  }

  ret = TRUE;

done:
  /* Removing the property listener */
  status = AudioObjectRemovePropertyListener (stream_id,
      &formatAddress, _audio_stream_format_listener, (void *) &prop_mutex);
  if (status != noErr) {
    GST_ERROR ("AudioObjectRemovePropertyListener failed: %d", (int) status);
  }
  /* Destroy the lock and condition */
  g_mutex_unlock (&prop_mutex.lock);
  g_mutex_clear (&prop_mutex.lock);
  g_cond_clear (&prop_mutex.cond);

  return ret;
}

static OSStatus
_audio_stream_hardware_changed_listener (AudioObjectID inObjectID,
    UInt32 inNumberAddresses,
    const AudioObjectPropertyAddress inAddresses[], void *inClientData)
{
  OSStatus status = noErr;
  guint i;
  GstCoreAudio *core_audio = inClientData;

  for (i = 0; i < inNumberAddresses; i++) {
    if (inAddresses[i].mSelector == kAudioDevicePropertyDeviceHasChanged) {
      if (!gst_core_audio_audio_device_is_spdif_avail (core_audio->device_id)) {
        GstOsxAudioSink *sink =
            GST_OSX_AUDIO_SINK (GST_OBJECT_PARENT (core_audio->osxbuf));
        GST_ELEMENT_ERROR (sink, RESOURCE, FAILED,
            ("SPDIF output no longer available"),
            ("Audio device is reporting that SPDIF output isn't available"));
      }
      break;
    }
  }
  return (status);
}

static inline gboolean
_monitorize_spdif (GstCoreAudio * core_audio)
{
  OSStatus status = noErr;
  gboolean ret = TRUE;

  AudioObjectPropertyAddress propAddress = {
    kAudioDevicePropertyDeviceHasChanged,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  /* Install the property listener */
  status = AudioObjectAddPropertyListener (core_audio->device_id,
      &propAddress, _audio_stream_hardware_changed_listener,
      (void *) core_audio);
  if (status != noErr) {
    GST_ERROR_OBJECT (core_audio->osxbuf,
        "AudioObjectAddPropertyListener failed: %d", (int) status);
    ret = FALSE;
  }

  return ret;
}

static inline gboolean
_unmonitorize_spdif (GstCoreAudio * core_audio)
{
  OSStatus status = noErr;
  gboolean ret = TRUE;

  AudioObjectPropertyAddress propAddress = {
    kAudioDevicePropertyDeviceHasChanged,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  /* Remove the property listener */
  status = AudioObjectRemovePropertyListener (core_audio->device_id,
      &propAddress, _audio_stream_hardware_changed_listener,
      (void *) core_audio);
  if (status != noErr) {
    GST_ERROR_OBJECT (core_audio->osxbuf,
        "AudioObjectRemovePropertyListener failed: %d", (int) status);
    ret = FALSE;
  }

  return ret;
}

static inline gboolean
_open_spdif (GstCoreAudio * core_audio)
{
  gboolean res = FALSE;
  pid_t hog_pid, own_pid = getpid ();

  /* We need the device in exclusive and disable the mixing */
  hog_pid = _audio_device_get_hog (core_audio->device_id);

  if (hog_pid != -1 && hog_pid != own_pid) {
    GST_DEBUG_OBJECT (core_audio,
        "device is currently in use by another application");
    goto done;
  }

  if (_audio_device_set_hog (core_audio->device_id, own_pid)) {
    core_audio->hog_pid = own_pid;
  }

  if (_audio_device_set_mixing (core_audio->device_id, FALSE)) {
    GST_DEBUG_OBJECT (core_audio, "disabled mixing on the device");
    core_audio->disabled_mixing = TRUE;
  }

  res = TRUE;
done:
  return res;
}

static inline gboolean
_close_spdif (GstCoreAudio * core_audio)
{
  pid_t hog_pid;

  _unmonitorize_spdif (core_audio);

  if (core_audio->revert_format) {
    if (!_audio_stream_change_format (core_audio->stream_id,
            core_audio->original_format)) {
      GST_WARNING_OBJECT (core_audio->osxbuf, "Format revert failed");
    }
    core_audio->revert_format = FALSE;
  }

  if (core_audio->disabled_mixing) {
    _audio_device_set_mixing (core_audio->device_id, TRUE);
    core_audio->disabled_mixing = FALSE;
  }

  if (core_audio->hog_pid != -1) {
    hog_pid = _audio_device_get_hog (core_audio->device_id);
    if (hog_pid == getpid ()) {
      if (_audio_device_set_hog (core_audio->device_id, -1)) {
        core_audio->hog_pid = -1;
      }
    }
  }

  return TRUE;
}

static OSStatus
_io_proc_spdif (AudioDeviceID inDevice,
    const AudioTimeStamp * inNow,
    const void *inInputData,
    const AudioTimeStamp * inTimestamp,
    AudioBufferList * bufferList,
    const AudioTimeStamp * inOutputTime, GstCoreAudio * core_audio)
{
  OSStatus status;

  status = core_audio->element->io_proc (core_audio->osxbuf, NULL, inTimestamp,
      0, 0, bufferList);

  return status;
}

static inline gboolean
_acquire_spdif (GstCoreAudio * core_audio, AudioStreamBasicDescription format)
{
  AudioStreamID *streams = NULL;
  gint i, j, nstreams = 0;
  gboolean ret = FALSE;

  if (!_open_spdif (core_audio))
    goto done;

  streams = _audio_device_get_streams (core_audio->device_id, &nstreams);

  for (i = 0; i < nstreams; i++) {
    AudioStreamRangedDescription *formats = NULL;
    gint nformats = 0;

    formats = _audio_stream_get_formats (streams[i], &nformats);

    if (formats) {
      gboolean is_spdif = FALSE;

      /* Check if one of the supported formats is a digital format */
      for (j = 0; j < nformats; j++) {
        if (CORE_AUDIO_FORMAT_IS_SPDIF (formats[j])) {
          is_spdif = TRUE;
          break;
        }
      }

      if (is_spdif) {
        /* if this stream supports a digital (cac3) format,
         * then go set it. */
        gint requested_rate_format = -1;
        gint current_rate_format = -1;
        gint backup_rate_format = -1;

        core_audio->stream_id = streams[i];
        core_audio->stream_idx = i;

        if (!core_audio->revert_format) {
          if (!_audio_stream_get_current_format (core_audio->stream_id,
                  &core_audio->original_format)) {
            GST_WARNING_OBJECT (core_audio->osxbuf,
                "format could not be saved");
            g_free (formats);
            continue;
          }
          core_audio->revert_format = TRUE;
        }

        for (j = 0; j < nformats; j++) {
          if (CORE_AUDIO_FORMAT_IS_SPDIF (formats[j])) {
            GST_LOG_OBJECT (core_audio->osxbuf,
                "found stream format: " CORE_AUDIO_FORMAT,
                CORE_AUDIO_FORMAT_ARGS (formats[j].mFormat));

            if (formats[j].mFormat.mSampleRate == format.mSampleRate) {
              requested_rate_format = j;
              break;
            } else if (formats[j].mFormat.mSampleRate ==
                core_audio->original_format.mSampleRate) {
              current_rate_format = j;
            } else {
              if (backup_rate_format < 0 ||
                  formats[j].mFormat.mSampleRate >
                  formats[backup_rate_format].mFormat.mSampleRate) {
                backup_rate_format = j;
              }
            }
          }
        }

        if (requested_rate_format >= 0) {
          /* We prefer to output at the rate of the original audio */
          core_audio->stream_format = formats[requested_rate_format].mFormat;
        } else if (current_rate_format >= 0) {
          /* If not possible, we will try to use the current rate */
          core_audio->stream_format = formats[current_rate_format].mFormat;
        } else {
          /* And if we have to, any digital format will be just
           * fine (highest rate possible) */
          core_audio->stream_format = formats[backup_rate_format].mFormat;
        }
      }
      g_free (formats);
    }
  }
  g_free (streams);

  GST_DEBUG_OBJECT (core_audio,
      "original stream format: " CORE_AUDIO_FORMAT,
      CORE_AUDIO_FORMAT_ARGS (core_audio->original_format));

  if (!_audio_stream_change_format (core_audio->stream_id,
          core_audio->stream_format))
    goto done;

  ret = TRUE;

done:
  return ret;
}

static inline void
_remove_render_spdif_callback (GstCoreAudio * core_audio)
{
  OSStatus status;

  /* Deactivate the render callback by calling
   * AudioDeviceDestroyIOProcID */
  status =
      AudioDeviceDestroyIOProcID (core_audio->device_id, core_audio->procID);
  if (status != noErr) {
    GST_ERROR_OBJECT (core_audio->osxbuf,
        "AudioDeviceDestroyIOProcID failed: %d", (int) status);
  }

  GST_DEBUG_OBJECT (core_audio,
      "osx ring buffer removed ioproc ID: %p device_id %lu",
      core_audio->procID, (gulong) core_audio->device_id);

  /* We're deactivated.. */
  core_audio->procID = 0;
  core_audio->io_proc_needs_deactivation = FALSE;
  core_audio->io_proc_active = FALSE;
}

static inline gboolean
_io_proc_spdif_start (GstCoreAudio * core_audio)
{
  OSErr status;

  GST_DEBUG_OBJECT (core_audio,
      "osx ring buffer start ioproc ID: %p device_id %lu",
      core_audio->procID, (gulong) core_audio->device_id);

  if (!core_audio->io_proc_active) {
    /* Add IOProc callback */
    status = AudioDeviceCreateIOProcID (core_audio->device_id,
        (AudioDeviceIOProc) _io_proc_spdif,
        (void *) core_audio, &core_audio->procID);
    if (status != noErr) {
      GST_ERROR_OBJECT (core_audio->osxbuf,
          ":AudioDeviceCreateIOProcID failed: %d", (int) status);
      return FALSE;
    }
    core_audio->io_proc_active = TRUE;
  }

  core_audio->io_proc_needs_deactivation = FALSE;

  /* Start device */
  status = AudioDeviceStart (core_audio->device_id, core_audio->procID);
  if (status != noErr) {
    GST_ERROR_OBJECT (core_audio->osxbuf,
        "AudioDeviceStart failed: %d", (int) status);
    return FALSE;
  }
  return TRUE;
}

static inline gboolean
_io_proc_spdif_stop (GstCoreAudio * core_audio)
{
  OSErr status;

  /* Stop device */
  status = AudioDeviceStop (core_audio->device_id, core_audio->procID);
  if (status != noErr) {
    GST_ERROR_OBJECT (core_audio->osxbuf,
        "AudioDeviceStop failed: %d", (int) status);
  }

  GST_DEBUG_OBJECT (core_audio,
      "osx ring buffer stop ioproc ID: %p device_id %lu",
      core_audio->procID, (gulong) core_audio->device_id);

  if (core_audio->io_proc_active) {
    _remove_render_spdif_callback (core_audio);
  }

  _close_spdif (core_audio);

  return TRUE;
}


/***********************
 *   Implementation    *
 **********************/

static gboolean
gst_core_audio_open_impl (GstCoreAudio * core_audio)
{
  gboolean ret;

  /* The following is needed to instruct HAL to create their own
   * thread to handle the notifications. */
  _audio_system_set_runloop (NULL);

  /* Create a HALOutput AudioUnit.
   * This is the lowest-level output API that is actually sensibly
   * usable (the lower level ones require that you do
   * channel-remapping yourself, and the CoreAudio channel mapping
   * is sufficiently complex that doing so would be very difficult)
   *
   * Note that for input we request an output unit even though
   * we will do input with it.
   * http://developer.apple.com/technotes/tn2002/tn2091.html
   */
  ret = gst_core_audio_open_device (core_audio, kAudioUnitSubType_HALOutput,
      "HALOutput");
  if (!ret) {
    GST_DEBUG ("Could not open device");
    goto done;
  }

  ret = gst_core_audio_bind_device (core_audio);
  if (!ret) {
    GST_DEBUG ("Could not bind device");
    goto done;
  }

done:
  return ret;
}

static gboolean
gst_core_audio_start_processing_impl (GstCoreAudio * core_audio)
{
  if (core_audio->is_passthrough) {
    return _io_proc_spdif_start (core_audio);
  } else {
    return gst_core_audio_io_proc_start (core_audio);
  }
}

static gboolean
gst_core_audio_pause_processing_impl (GstCoreAudio * core_audio)
{
  if (core_audio->is_passthrough) {
    GST_DEBUG_OBJECT (core_audio,
        "osx ring buffer pause ioproc ID: %p device_id %lu",
        core_audio->procID, (gulong) core_audio->device_id);

    if (core_audio->io_proc_active) {
      _remove_render_spdif_callback (core_audio);
    }
  } else {
    GST_DEBUG_OBJECT (core_audio,
        "osx ring buffer pause ioproc: %p device_id %lu",
        core_audio->element->io_proc, (gulong) core_audio->device_id);
    if (core_audio->io_proc_active) {
      /* CoreAudio isn't threadsafe enough to do this here;
       * we must deactivate the render callback elsewhere. See:
       * http://lists.apple.com/archives/Coreaudio-api/2006/Mar/msg00010.html
       */
      core_audio->io_proc_needs_deactivation = TRUE;
    }
  }
  return TRUE;
}

static gboolean
gst_core_audio_stop_processing_impl (GstCoreAudio * core_audio)
{
  if (core_audio->is_passthrough) {
    _io_proc_spdif_stop (core_audio);
  } else {
    gst_core_audio_io_proc_stop (core_audio);
  }

  return TRUE;
}

static gboolean
gst_core_audio_get_samples_and_latency_impl (GstCoreAudio * core_audio,
    gdouble rate, guint * samples, gdouble * latency)
{
  OSStatus status;
  UInt32 size = sizeof (double);

  if (core_audio->is_passthrough) {
    *samples = _audio_device_get_latency (core_audio->device_id);
    *samples += _audio_stream_get_latency (core_audio->stream_id);
    *latency = (double) *samples / rate;
  } else {
    status = AudioUnitGetProperty (core_audio->audiounit, kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0,        /* N/A for global */
        latency, &size);

    if (status) {
      GST_WARNING_OBJECT (core_audio->osxbuf, "Failed to get latency: %d",
          (int) status);
      *samples = 0;
      return FALSE;
    }

    *samples = *latency * rate;
  }
  return TRUE;
}

static gboolean
gst_core_audio_initialize_impl (GstCoreAudio * core_audio,
    AudioStreamBasicDescription format, GstCaps * caps,
    gboolean is_passthrough, guint32 * frame_size)
{
  gboolean ret = FALSE;
  OSStatus status;

  /* Uninitialize the AudioUnit before changing formats */
  status = AudioUnitUninitialize (core_audio->audiounit);
  if (status) {
    GST_ERROR_OBJECT (core_audio, "Failed to uninitialize AudioUnit: %d",
        (int) status);
    return FALSE;
  }

  core_audio->is_passthrough = is_passthrough;
  if (is_passthrough) {
    if (!_acquire_spdif (core_audio, format))
      goto done;
    _monitorize_spdif (core_audio);
  } else {
    OSStatus status;
    UInt32 propertySize;

    core_audio->stream_idx = 0;
    if (!gst_core_audio_set_format (core_audio, format))
      goto done;

    if (!gst_core_audio_set_channel_layout (core_audio,
            format.mChannelsPerFrame, caps))
      goto done;

    if (core_audio->is_src) {
      propertySize = sizeof (*frame_size);
      status = AudioUnitGetProperty (core_audio->audiounit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0,     /* N/A for global */
          frame_size, &propertySize);

      if (status) {
        GST_WARNING_OBJECT (core_audio->osxbuf, "Failed to get frame size: %d",
            (int) status);
        goto done;
      }
    }
  }

  ret = TRUE;

done:
  /* Format changed, initialise the AudioUnit again */
  status = AudioUnitInitialize (core_audio->audiounit);
  if (status) {
    GST_ERROR_OBJECT (core_audio, "Failed to initialize AudioUnit: %d",
        (int) status);
    ret = FALSE;
  }

  if (ret) {
    GST_DEBUG_OBJECT (core_audio, "osxbuf ring buffer acquired");
  }

  return ret;
}

static gboolean
gst_core_audio_select_device_impl (GstCoreAudio * core_audio)
{
  AudioDeviceID *devices = NULL;
  AudioDeviceID device_id = core_audio->device_id;
  AudioDeviceID default_device_id = 0;
  gint i, ndevices = 0;
  gboolean output = !core_audio->is_src;
  gboolean res = FALSE;
#ifdef GST_CORE_AUDIO_DEBUG
  AudioChannelLayout *channel_layout;
#endif

  devices = _audio_system_get_devices (&ndevices);

  if (ndevices < 1) {
    GST_ERROR ("no audio output devices found");
    goto done;
  }

  GST_DEBUG ("found %d audio device(s)", ndevices);

#ifdef GST_CORE_AUDIO_DEBUG
  for (i = 0; i < ndevices; i++) {
    gchar *device_name;

    if ((device_name = _audio_device_get_name (devices[i], output))) {
      if (!_audio_device_has_output (devices[i])) {
        GST_DEBUG ("Input Device ID: %u Name: %s",
            (unsigned) devices[i], device_name);
      } else {
        GST_DEBUG ("Output Device ID: %u Name: %s",
            (unsigned) devices[i], device_name);

        channel_layout =
            gst_core_audio_audio_device_get_channel_layout (devices[i], output);
        if (channel_layout) {
          gst_core_audio_dump_channel_layout (channel_layout);
          g_free (channel_layout);
        }
      }

      g_free (device_name);
    }
  }
#endif

  /* Find the ID of the default output device */
  default_device_id = _audio_system_get_default_device (output);

  /* Here we decide if selected device is valid or autoselect
   * the default one when required */
  if (device_id == kAudioDeviceUnknown) {
    if (default_device_id != kAudioDeviceUnknown) {
      device_id = default_device_id;
      res = TRUE;
    } else {
      GST_ERROR ("No device of required type available");
      res = FALSE;
    }
  } else {
    for (i = 0; i < ndevices; i++) {
      if (device_id == devices[i]) {
        res = TRUE;
        break;
      }
    }

    if (res && !_audio_device_is_alive (device_id, output)) {
      GST_ERROR ("Requested device not usable");
      res = FALSE;
      goto done;
    }
  }

  if (res)
    core_audio->device_id = device_id;

done:
  g_free (devices);
  return res;
}

static gboolean
gst_core_audio_audio_device_is_spdif_avail_impl (AudioDeviceID device_id)
{
  AudioStreamID *streams = NULL;
  gint i, nstreams = 0;
  gboolean res = FALSE;

  streams = _audio_device_get_streams (device_id, &nstreams);
  GST_DEBUG ("found %d streams", nstreams);
  if (streams) {
    for (i = 0; i < nstreams; i++) {
      if (_audio_stream_is_spdif_avail (streams[i])) {
        res = TRUE;
      }
    }

    g_free (streams);
  }

  return res;
}
