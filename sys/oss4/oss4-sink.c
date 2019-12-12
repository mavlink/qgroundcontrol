/* GStreamer OSS4 audio sink
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
/**
 * SECTION:element-oss4sink
 * @title: oss4sink
 *
 * This element lets you output sound using the Open Sound System (OSS)
 * version 4.
 *
 * Note that you should almost always use generic audio conversion elements
 * like audioconvert and audioresample in front of an audiosink to make sure
 * your pipeline works under all circumstances (those conversion elements will
 * act in passthrough-mode if no conversion is necessary).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v audiotestsrc ! audioconvert ! volume volume=0.1 ! oss4sink
 * ]| will output a sine wave (continuous beep sound) to your sound card (with
 * a very low volume as precaution).
 * |[
 * gst-launch-1.0 -v filesrc location=music.ogg ! decodebin ! audioconvert ! audioresample ! oss4sink
 * ]| will play an Ogg/Vorbis audio file and output it using the Open Sound System
 * version 4.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <gst/gst-i18n-plugin.h>
#include <gst/audio/streamvolume.h>

#define NO_LEGACY_MIXER
#include "oss4-audio.h"
#include "oss4-sink.h"
#include "oss4-property-probe.h"
#include "oss4-soundcard.h"

GST_DEBUG_CATEGORY_EXTERN (oss4sink_debug);
#define GST_CAT_DEFAULT oss4sink_debug

static void gst_oss4_sink_dispose (GObject * object);
static void gst_oss4_sink_finalize (GObject * object);

static void gst_oss4_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_oss4_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static GstCaps *gst_oss4_sink_getcaps (GstBaseSink * bsink, GstCaps * filter);
static gboolean gst_oss4_sink_open (GstAudioSink * asink,
    gboolean silent_errors);
static gboolean gst_oss4_sink_open_func (GstAudioSink * asink);
static gboolean gst_oss4_sink_close (GstAudioSink * asink);
static gboolean gst_oss4_sink_prepare (GstAudioSink * asink,
    GstAudioRingBufferSpec * spec);
static gboolean gst_oss4_sink_unprepare (GstAudioSink * asink);
static gint gst_oss4_sink_write (GstAudioSink * asink, gpointer data,
    guint length);
static guint gst_oss4_sink_delay (GstAudioSink * asink);
static void gst_oss4_sink_reset (GstAudioSink * asink);

#define DEFAULT_DEVICE      NULL
#define DEFAULT_DEVICE_NAME NULL
#define DEFAULT_MUTE        FALSE
#define DEFAULT_VOLUME      1.0
#define MAX_VOLUME          10.0

enum
{
  PROP_0,
  PROP_DEVICE,
  PROP_DEVICE_NAME,
  PROP_VOLUME,
  PROP_MUTE,
  PROP_LAST
};

#define gst_oss4_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstOss4Sink, gst_oss4_sink,
    GST_TYPE_AUDIO_SINK, G_IMPLEMENT_INTERFACE (GST_TYPE_STREAM_VOLUME, NULL));

static void
gst_oss4_sink_dispose (GObject * object)
{
  GstOss4Sink *osssink = GST_OSS4_SINK (object);

  if (osssink->probed_caps) {
    gst_caps_unref (osssink->probed_caps);
    osssink->probed_caps = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_oss4_sink_class_init (GstOss4SinkClass * klass)
{
  GstAudioSinkClass *audiosink_class = (GstAudioSinkClass *) klass;
  GstBaseSinkClass *basesink_class = (GstBaseSinkClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstPadTemplate *templ;

  gobject_class->dispose = gst_oss4_sink_dispose;
  gobject_class->finalize = gst_oss4_sink_finalize;
  gobject_class->get_property = gst_oss4_sink_get_property;
  gobject_class->set_property = gst_oss4_sink_set_property;

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device",
          "OSS4 device (e.g. /dev/oss/hdaudio0/pcm0 or /dev/dspN) "
          "(NULL = use first available playback device)",
          DEFAULT_DEVICE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Human-readable name of the sound device", DEFAULT_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_VOLUME,
      g_param_spec_double ("volume", "Volume",
          "Linear volume of this stream, 1.0=100%", 0.0, MAX_VOLUME,
          DEFAULT_VOLUME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_MUTE,
      g_param_spec_boolean ("mute", "Mute",
          "Mute state of this stream", DEFAULT_MUTE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  basesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_oss4_sink_getcaps);

  audiosink_class->open = GST_DEBUG_FUNCPTR (gst_oss4_sink_open_func);
  audiosink_class->close = GST_DEBUG_FUNCPTR (gst_oss4_sink_close);
  audiosink_class->prepare = GST_DEBUG_FUNCPTR (gst_oss4_sink_prepare);
  audiosink_class->unprepare = GST_DEBUG_FUNCPTR (gst_oss4_sink_unprepare);
  audiosink_class->write = GST_DEBUG_FUNCPTR (gst_oss4_sink_write);
  audiosink_class->delay = GST_DEBUG_FUNCPTR (gst_oss4_sink_delay);
  audiosink_class->reset = GST_DEBUG_FUNCPTR (gst_oss4_sink_reset);

  gst_element_class_set_static_metadata (gstelement_class,
      "OSS v4 Audio Sink", "Sink/Audio",
      "Output to a sound card via OSS version 4",
      "Tim-Philipp Müller <tim centricular net>");

  templ = gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
      gst_oss4_audio_get_template_caps ());
  gst_element_class_add_pad_template (gstelement_class, templ);
}

static void
gst_oss4_sink_init (GstOss4Sink * osssink)
{
  const gchar *device;

  device = g_getenv ("AUDIODEV");
  if (device == NULL)
    device = DEFAULT_DEVICE;
  osssink->device = g_strdup (device);
  osssink->fd = -1;
  osssink->bytes_per_sample = 0;
  osssink->probed_caps = NULL;
  osssink->device_name = NULL;
  osssink->mute_volume = 100 | (100 << 8);
}

static void
gst_oss4_sink_finalize (GObject * object)
{
  GstOss4Sink *osssink = GST_OSS4_SINK (object);

  g_free (osssink->device);
  osssink->device = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_oss4_sink_set_volume (GstOss4Sink * oss, gdouble volume)
{
  int ivol;

  volume = volume * 100.0;
  ivol = (int) volume | ((int) volume << 8);
  GST_OBJECT_LOCK (oss);
  if (ioctl (oss->fd, SNDCTL_DSP_SETPLAYVOL, &ivol) < 0) {
    GST_LOG_OBJECT (oss, "SETPLAYVOL failed");
  }
  GST_OBJECT_UNLOCK (oss);
}

static gdouble
gst_oss4_sink_get_volume (GstOss4Sink * oss)
{
  int ivol, lvol, rvol;
  gdouble dvol = DEFAULT_VOLUME;

  GST_OBJECT_LOCK (oss);
  if (ioctl (oss->fd, SNDCTL_DSP_GETPLAYVOL, &ivol) < 0) {
    GST_LOG_OBJECT (oss, "GETPLAYVOL failed");
  } else {
    /* Return the higher of the two volume channels, if different */
    lvol = ivol & 0xff;
    rvol = (ivol >> 8) & 0xff;
    dvol = MAX (lvol, rvol) / 100.0;
  }
  GST_OBJECT_UNLOCK (oss);

  return dvol;
}

static void
gst_oss4_sink_set_mute (GstOss4Sink * oss, gboolean mute)
{
  int ivol;

  if (mute) {
    /*
     * OSSv4 does not have a per-channel mute, so simulate by setting
     * the value to 0.  Save the volume before doing a mute so we can
     * reset the value when the user un-mutes.
     */
    ivol = 0;

    GST_OBJECT_LOCK (oss);
    if (ioctl (oss->fd, SNDCTL_DSP_GETPLAYVOL, &oss->mute_volume) < 0) {
      GST_LOG_OBJECT (oss, "GETPLAYVOL failed");
    }
    if (ioctl (oss->fd, SNDCTL_DSP_SETPLAYVOL, &ivol) < 0) {
      GST_LOG_OBJECT (oss, "SETPLAYVOL failed");
    }
    GST_OBJECT_UNLOCK (oss);
  } else {
    /*
     * If the saved volume is 0, then reset it to 100.  Otherwise the mute
     * can get stuck.  This can happen, for example, due to rounding
     * errors in converting from the float to an integer.
     */
    if (oss->mute_volume == 0) {
      oss->mute_volume = 100 | (100 << 8);
    }
    GST_OBJECT_LOCK (oss);
    if (ioctl (oss->fd, SNDCTL_DSP_SETPLAYVOL, &oss->mute_volume) < 0) {
      GST_LOG_OBJECT (oss, "SETPLAYVOL failed");
    }
    GST_OBJECT_UNLOCK (oss);
  }
}

static gboolean
gst_oss4_sink_get_mute (GstOss4Sink * oss)
{
  int ivol, lvol, rvol;

  GST_OBJECT_LOCK (oss);
  if (ioctl (oss->fd, SNDCTL_DSP_GETPLAYVOL, &ivol) < 0) {
    GST_LOG_OBJECT (oss, "GETPLAYVOL failed");
    lvol = rvol = 100;
  } else {
    lvol = ivol & 0xff;
    rvol = (ivol >> 8) & 0xff;
  }
  GST_OBJECT_UNLOCK (oss);

  return (lvol == 0 && rvol == 0);
}

static void
gst_oss4_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOss4Sink *oss = GST_OSS4_SINK (object);

  switch (prop_id) {
    case PROP_DEVICE:
      GST_OBJECT_LOCK (oss);
      if (oss->fd == -1) {
        g_free (oss->device);
        oss->device = g_value_dup_string (value);
        if (oss->probed_caps) {
          gst_caps_unref (oss->probed_caps);
          oss->probed_caps = NULL;
        }
        g_free (oss->device_name);
        oss->device_name = NULL;
      } else {
        g_warning ("%s: can't change \"device\" property while audio sink "
            "is open", GST_OBJECT_NAME (oss));
      }
      GST_OBJECT_UNLOCK (oss);
      break;
    case PROP_VOLUME:
      gst_oss4_sink_set_volume (oss, g_value_get_double (value));
      break;
    case PROP_MUTE:
      gst_oss4_sink_set_mute (oss, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_oss4_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstOss4Sink *oss = GST_OSS4_SINK (object);

  switch (prop_id) {
    case PROP_DEVICE:
      GST_OBJECT_LOCK (oss);
      g_value_set_string (value, oss->device);
      GST_OBJECT_UNLOCK (oss);
      break;
    case PROP_DEVICE_NAME:
      GST_OBJECT_LOCK (oss);
      if (oss->fd == -1 && oss->device != NULL) {
        /* If device is set, try to retrieve the name even if we're not open */
        if (gst_oss4_sink_open (GST_AUDIO_SINK (oss), TRUE)) {
          g_value_set_string (value, oss->device_name);
          gst_oss4_sink_close (GST_AUDIO_SINK (oss));
        } else {
          gchar *name = NULL;

          gst_oss4_property_probe_find_device_name_nofd (GST_OBJECT (oss),
              oss->device, &name);
          g_value_set_string (value, name);
          g_free (name);
        }
      } else {
        g_value_set_string (value, oss->device_name);
      }
      GST_OBJECT_UNLOCK (oss);
      break;
    case PROP_VOLUME:
      g_value_set_double (value, gst_oss4_sink_get_volume (oss));
      break;
    case PROP_MUTE:
      g_value_set_boolean (value, gst_oss4_sink_get_mute (oss));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstCaps *
gst_oss4_sink_getcaps (GstBaseSink * bsink, GstCaps * filter)
{
  GstOss4Sink *oss;
  GstCaps *caps;

  oss = GST_OSS4_SINK (bsink);

  if (oss->fd == -1) {
    caps = gst_oss4_audio_get_template_caps ();
  } else if (oss->probed_caps) {
    caps = gst_caps_copy (oss->probed_caps);
  } else {
    caps = gst_oss4_audio_probe_caps (GST_OBJECT (oss), oss->fd);
    if (caps != NULL && !gst_caps_is_empty (caps)) {
      oss->probed_caps = gst_caps_copy (caps);
    }
  }

  if (filter && caps) {
    GstCaps *intersection;

    intersection =
        gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    return intersection;
  } else {
    return caps;
  }
}

/* note: we must not take the object lock here unless we fix up get_property */
static gboolean
gst_oss4_sink_open (GstAudioSink * asink, gboolean silent_errors)
{
  GstOss4Sink *oss;
  gchar *device;
  int mode;

  oss = GST_OSS4_SINK (asink);

  if (oss->device)
    device = g_strdup (oss->device);
  else
    device = gst_oss4_audio_find_device (GST_OBJECT_CAST (oss));

  /* desperate times, desperate measures */
  if (device == NULL)
    device = g_strdup ("/dev/dsp0");

  GST_INFO_OBJECT (oss, "Trying to open OSS4 device '%s'", device);

  /* we open in non-blocking mode even if we don't really want to do writes
   * non-blocking because we can't be sure that this is really a genuine
   * OSS4 device with well-behaved drivers etc. We really don't want to
   * hang forever under any circumstances. */
  oss->fd = open (device, O_WRONLY | O_NONBLOCK, 0);
  if (oss->fd == -1) {
    switch (errno) {
      case EBUSY:
        goto busy;
      case EACCES:
        goto no_permission;
      default:
        goto open_failed;
    }
  }

  GST_INFO_OBJECT (oss, "Opened device '%s'", device);

  /* Make sure it's OSS4. If it's old OSS, let osssink handle it */
  if (!gst_oss4_audio_check_version (GST_OBJECT_CAST (oss), oss->fd))
    goto legacy_oss;

  /* now remove the non-blocking flag. */
  mode = fcntl (oss->fd, F_GETFL);
  mode &= ~O_NONBLOCK;
  if (fcntl (oss->fd, F_SETFL, mode) < 0) {
    /* some drivers do no support unsetting the non-blocking flag, try to
     * close/open the device then. This is racy but we error out properly. */
    GST_WARNING_OBJECT (oss, "failed to unset O_NONBLOCK (buggy driver?), "
        "will try to re-open device now");
    gst_oss4_sink_close (asink);
    if ((oss->fd = open (device, O_WRONLY, 0)) == -1)
      goto non_block;
  }

  oss->open_device = device;

  /* not using ENGINEINFO here because it sometimes returns a different and
   * less useful name than AUDIOINFO for the same device */
  if (!gst_oss4_property_probe_find_device_name (GST_OBJECT (oss), oss->fd,
          oss->open_device, &oss->device_name)) {
    oss->device_name = NULL;
  }

  /* list output routings, for informational purposes only so far */
  {
    oss_mixer_enuminfo routings = { 0, };
    guint i;

    if (ioctl (oss->fd, SNDCTL_DSP_GET_PLAYTGT_NAMES, &routings) != -1) {
      GST_LOG_OBJECT (oss, "%u output routings (static list: %d)",
          routings.nvalues, ! !(routings.version == 0));
      for (i = 0; i < routings.nvalues; ++i) {
        GST_LOG_OBJECT (oss, "  output routing %d: %s", i,
            &routings.strings[routings.strindex[i]]);
      }
    }
  }

  return TRUE;

  /* ERRORS */
busy:
  {
    if (!silent_errors) {
      GST_ELEMENT_ERROR (oss, RESOURCE, BUSY,
          (_("Could not open audio device for playback. "
                  "Device is being used by another application.")), (NULL));
    }
    g_free (device);
    return FALSE;
  }
no_permission:
  {
    if (!silent_errors) {
      GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_WRITE,
          (_("Could not open audio device for playback. "
                  "You don't have permission to open the device.")),
          GST_ERROR_SYSTEM);
    }
    g_free (device);
    return FALSE;
  }
open_failed:
  {
    if (!silent_errors) {
      GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_WRITE,
          (_("Could not open audio device for playback.")), GST_ERROR_SYSTEM);
    }
    g_free (device);
    return FALSE;
  }
legacy_oss:
  {
    if (!silent_errors) {
      GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_WRITE,
          (_("Could not open audio device for playback. "
                  "This version of the Open Sound System is not supported by this "
                  "element.")), ("Try the 'osssink' element instead"));
    }
    gst_oss4_sink_close (asink);
    g_free (device);
    return FALSE;
  }
non_block:
  {
    if (!silent_errors) {
      GST_ELEMENT_ERROR (oss, RESOURCE, SETTINGS, (NULL),
          ("Unable to set device %s into non-blocking mode: %s",
              oss->device, g_strerror (errno)));
    }
    g_free (device);
    return FALSE;
  }
}

static gboolean
gst_oss4_sink_open_func (GstAudioSink * asink)
{
  if (!gst_oss4_sink_open (asink, FALSE))
    return FALSE;

  /* the initial volume might not be the property default, so notify
   * application to make it get a reading of the current volume */
  g_object_notify (G_OBJECT (asink), "volume");
  return TRUE;
}

static gboolean
gst_oss4_sink_close (GstAudioSink * asink)
{
  GstOss4Sink *oss = GST_OSS4_SINK (asink);

  if (oss->fd != -1) {
    GST_DEBUG_OBJECT (oss, "closing device");
    close (oss->fd);
    oss->fd = -1;
  }

  oss->bytes_per_sample = 0;
  /* we keep the probed caps cached, at least until the device changes */

  g_free (oss->open_device);
  oss->open_device = NULL;

  g_free (oss->device_name);
  oss->device_name = NULL;

  if (oss->probed_caps) {
    gst_caps_unref (oss->probed_caps);
    oss->probed_caps = NULL;
  }

  return TRUE;
}

static gboolean
gst_oss4_sink_prepare (GstAudioSink * asink, GstAudioRingBufferSpec * spec)
{
  GstOss4Sink *oss;

  oss = GST_OSS4_SINK (asink);

  if (!gst_oss4_audio_set_format (GST_OBJECT_CAST (oss), oss->fd, spec)) {
    GST_WARNING_OBJECT (oss, "Couldn't set requested format %" GST_PTR_FORMAT,
        spec->caps);
    return FALSE;
  }

  oss->bytes_per_sample = GST_AUDIO_INFO_BPF (&spec->info);

  return TRUE;
}

static gboolean
gst_oss4_sink_unprepare (GstAudioSink * asink)
{
  /* could do a SNDCTL_DSP_HALT, but the OSS manual recommends a close/open,
   * since HALT won't properly reset some devices, apparently */

  if (!gst_oss4_sink_close (asink))
    goto couldnt_close;

  if (!gst_oss4_sink_open_func (asink))
    goto couldnt_reopen;

  return TRUE;

  /* ERRORS */
couldnt_close:
  {
    GST_DEBUG_OBJECT (asink, "Couldn't close the audio device");
    return FALSE;
  }
couldnt_reopen:
  {
    GST_DEBUG_OBJECT (asink, "Couldn't reopen the audio device");
    return FALSE;
  }
}

static gint
gst_oss4_sink_write (GstAudioSink * asink, gpointer data, guint length)
{
  GstOss4Sink *oss;
  int n;

  oss = GST_OSS4_SINK_CAST (asink);

  n = write (oss->fd, data, length);
  GST_LOG_OBJECT (asink, "wrote %d/%d samples, %d bytes",
      n / oss->bytes_per_sample, length / oss->bytes_per_sample, n);

  if (G_UNLIKELY (n < 0)) {
    switch (errno) {
      case ENOTSUP:
      case EACCES:{
        /* This is the most likely cause, I think */
        GST_ELEMENT_ERROR (asink, RESOURCE, WRITE,
            (_("Playback is not supported by this audio device.")),
            ("write: %s (device: %s) (maybe this is an input-only device?)",
                g_strerror (errno), oss->open_device));
        break;
      }
      default:{
        GST_ELEMENT_ERROR (asink, RESOURCE, WRITE,
            (_("Audio playback error.")),
            ("write: %s (device: %s)", g_strerror (errno), oss->open_device));
        break;
      }
    }
  }

  return n;
}

static guint
gst_oss4_sink_delay (GstAudioSink * asink)
{
  GstOss4Sink *oss;
  gint delay = -1;

  oss = GST_OSS4_SINK_CAST (asink);

  GST_OBJECT_LOCK (oss);
  if (ioctl (oss->fd, SNDCTL_DSP_GETODELAY, &delay) < 0 || delay < 0) {
    GST_LOG_OBJECT (oss, "GETODELAY failed");
  }
  GST_OBJECT_UNLOCK (oss);

  if (G_UNLIKELY (delay < 0))   /* error case */
    return 0;

  return delay / oss->bytes_per_sample;
}

static void
gst_oss4_sink_reset (GstAudioSink * asink)
{
  /* There's nothing we can do here really: OSS can't handle access to the
   * same device/fd from multiple threads and might deadlock or blow up in
   * other ways if we try an ioctl SNDCTL_DSP_HALT or similar */
}
