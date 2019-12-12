/*
 * GStreamer
 * Copyright (C) 2012 Fluendo S.A. <support@fluendo.com>
 *   Authors: Andoni Morales Alastruey <amorales@fluendo.com>
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

#import <AudioUnit/AUComponent.h>

static gboolean
gst_core_audio_open_impl (GstCoreAudio * core_audio)
{
  return gst_core_audio_open_device (core_audio, kAudioUnitSubType_RemoteIO,
      "RemoteIO");
}

static gboolean
gst_core_audio_start_processing_impl (GstCoreAudio * core_audio)
{
  return gst_core_audio_io_proc_start (core_audio);
}

static gboolean
gst_core_audio_pause_processing_impl (GstCoreAudio * core_audio)
{
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
  return TRUE;
}

static gboolean
gst_core_audio_stop_processing_impl (GstCoreAudio * core_audio)
{
  gst_core_audio_io_proc_stop (core_audio);
  return TRUE;
}

static gboolean
gst_core_audio_get_samples_and_latency_impl (GstCoreAudio * core_audio,
    gdouble rate, guint * samples, gdouble * latency)
{
  OSStatus status;
  UInt32 size = sizeof (double);

  status = AudioUnitGetProperty (core_audio->audiounit, kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0,  /* N/A for global */
      latency, &size);

  if (status) {
    GST_WARNING_OBJECT (core_audio, "Failed to get latency: %d", (int) status);
    *samples = 0;
    return FALSE;
  }

  *samples = *latency * rate;
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
  core_audio->stream_idx = 0;

  if (!gst_core_audio_set_format (core_audio, format))
    goto done;

  /* FIXME: Use kAudioSessionProperty_CurrentHardwareSampleRate and
   * kAudioSessionProperty_CurrentHardwareIOBufferDuration with property
   * listeners to detect changes in screen locks.
   * For now use the maximum value */
  *frame_size = 4196;

  GST_DEBUG_OBJECT (core_audio, "osxbuf ring buffer acquired");
  ret = TRUE;

done:
  /* Format changed, initialise the AudioUnit again */
  status = AudioUnitInitialize (core_audio->audiounit);
  if (status) {
    GST_ERROR_OBJECT (core_audio, "Failed to initialize AudioUnit: %d",
        (int) status);
    ret = FALSE;
  }

  return ret;
}

static gboolean
gst_core_audio_select_device_impl (GstCoreAudio * core_audio)
{
  /* No device selection in iOS */
  return TRUE;
}

static gboolean
gst_core_audio_audio_device_is_spdif_avail_impl (AudioDeviceID device_id)
{
  /* No SPDIF in iOS */
  return FALSE;
}
