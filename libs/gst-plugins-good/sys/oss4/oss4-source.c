/* GStreamer OSS4 audio source
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
 * SECTION:element-oss4src
 * @title: oss4src
 *
 * This element lets you record sound using the Open Sound System (OSS)
 * version 4.
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v oss4src ! queue ! audioconvert ! vorbisenc ! oggmux ! filesink location=mymusic.ogg
 * ]| will record sound from your sound card using OSS4 and encode it to an
 * Ogg/Vorbis file (this will only work if your mixer settings are right
 * and the right inputs areenabled etc.)
 *
 */

/* FIXME: make sure we're not doing ioctls from the app thread (e.g. via the
 * mixer interface) while recording */

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

#define NO_LEGACY_MIXER
#include "oss4-audio.h"
#include "oss4-source.h"
#include "oss4-property-probe.h"
#include "oss4-soundcard.h"

#define GST_OSS4_SOURCE_IS_OPEN(src)  (GST_OSS4_SOURCE(src)->fd != -1)

GST_DEBUG_CATEGORY_EXTERN (oss4src_debug);
#define GST_CAT_DEFAULT oss4src_debug

#define DEFAULT_DEVICE       NULL
#define DEFAULT_DEVICE_NAME  NULL

enum
{
  PROP_0,
  PROP_DEVICE,
  PROP_DEVICE_NAME
};

#define gst_oss4_source_parent_class parent_class
G_DEFINE_TYPE (GstOss4Source, gst_oss4_source, GST_TYPE_AUDIO_SRC);

static void gst_oss4_source_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_oss4_source_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_oss4_source_dispose (GObject * object);
static void gst_oss4_source_finalize (GstOss4Source * osssrc);

static GstCaps *gst_oss4_source_getcaps (GstBaseSrc * bsrc, GstCaps * filter);

static gboolean gst_oss4_source_open (GstAudioSrc * asrc,
    gboolean silent_errors);
static gboolean gst_oss4_source_open_func (GstAudioSrc * asrc);
static gboolean gst_oss4_source_close (GstAudioSrc * asrc);
static gboolean gst_oss4_source_prepare (GstAudioSrc * asrc,
    GstAudioRingBufferSpec * spec);
static gboolean gst_oss4_source_unprepare (GstAudioSrc * asrc);
static guint gst_oss4_source_read (GstAudioSrc * asrc, gpointer data,
    guint length, GstClockTime * timestamp);
static guint gst_oss4_source_delay (GstAudioSrc * asrc);
static void gst_oss4_source_reset (GstAudioSrc * asrc);

static void
gst_oss4_source_class_init (GstOss4SourceClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstAudioSrcClass *gstaudiosrc_class;
  GstPadTemplate *templ;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstaudiosrc_class = (GstAudioSrcClass *) klass;

  gobject_class->dispose = gst_oss4_source_dispose;
  gobject_class->finalize = (GObjectFinalizeFunc) gst_oss4_source_finalize;
  gobject_class->get_property = gst_oss4_source_get_property;
  gobject_class->set_property = gst_oss4_source_set_property;

  gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_oss4_source_getcaps);

  gstaudiosrc_class->open = GST_DEBUG_FUNCPTR (gst_oss4_source_open_func);
  gstaudiosrc_class->prepare = GST_DEBUG_FUNCPTR (gst_oss4_source_prepare);
  gstaudiosrc_class->unprepare = GST_DEBUG_FUNCPTR (gst_oss4_source_unprepare);
  gstaudiosrc_class->close = GST_DEBUG_FUNCPTR (gst_oss4_source_close);
  gstaudiosrc_class->read = GST_DEBUG_FUNCPTR (gst_oss4_source_read);
  gstaudiosrc_class->delay = GST_DEBUG_FUNCPTR (gst_oss4_source_delay);
  gstaudiosrc_class->reset = GST_DEBUG_FUNCPTR (gst_oss4_source_reset);

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device",
          "OSS4 device (e.g. /dev/oss/hdaudio0/pcm0 or /dev/dspN) "
          "(NULL = use first available device)",
          DEFAULT_DEVICE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Human-readable name of the sound device", DEFAULT_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "OSS v4 Audio Source", "Source/Audio",
      "Capture from a sound card via OSS version 4",
      "Tim-Philipp Müller <tim centricular net>");

  templ = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
      gst_oss4_audio_get_template_caps ());
  gst_element_class_add_pad_template (gstelement_class, templ);
}

static void
gst_oss4_source_init (GstOss4Source * osssrc)
{
  const gchar *device;

  device = g_getenv ("AUDIODEV");
  if (device == NULL)
    device = DEFAULT_DEVICE;

  osssrc->fd = -1;
  osssrc->device = g_strdup (device);
  osssrc->device_name = g_strdup (DEFAULT_DEVICE_NAME);
  osssrc->device_name = NULL;
}

static void
gst_oss4_source_finalize (GstOss4Source * oss)
{
  g_free (oss->device);
  oss->device = NULL;

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (oss));
}

static void
gst_oss4_source_dispose (GObject * object)
{
  GstOss4Source *oss = GST_OSS4_SOURCE (object);

  if (oss->probed_caps) {
    gst_caps_unref (oss->probed_caps);
    oss->probed_caps = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_oss4_source_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOss4Source *oss;

  oss = GST_OSS4_SOURCE (object);

  switch (prop_id) {
    case PROP_DEVICE:
      GST_OBJECT_LOCK (oss);
      if (oss->fd == -1) {
        g_free (oss->device);
        oss->device = g_value_dup_string (value);
        g_free (oss->device_name);
        oss->device_name = NULL;
      } else {
        g_warning ("%s: can't change \"device\" property while audio source "
            "is open", GST_OBJECT_NAME (oss));
      }
      GST_OBJECT_UNLOCK (oss);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_oss4_source_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstOss4Source *oss;

  oss = GST_OSS4_SOURCE (object);

  switch (prop_id) {
    case PROP_DEVICE:
      GST_OBJECT_LOCK (oss);
      g_value_set_string (value, oss->device);
      GST_OBJECT_UNLOCK (oss);
      break;
    case PROP_DEVICE_NAME:
      GST_OBJECT_LOCK (oss);
      /* If device is set, try to retrieve the name even if we're not open */
      if (oss->fd == -1 && oss->device != NULL) {
        if (gst_oss4_source_open (GST_AUDIO_SRC (oss), TRUE)) {
          g_value_set_string (value, oss->device_name);
          gst_oss4_source_close (GST_AUDIO_SRC (oss));
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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstCaps *
gst_oss4_source_getcaps (GstBaseSrc * bsrc, GstCaps * filter)
{
  GstOss4Source *oss;
  GstCaps *caps;

  oss = GST_OSS4_SOURCE (bsrc);

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
gst_oss4_source_open (GstAudioSrc * asrc, gboolean silent_errors)
{
  GstOss4Source *oss;
  gchar *device;
  int mode;

  oss = GST_OSS4_SOURCE (asrc);

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
  oss->fd = open (device, O_RDONLY | O_NONBLOCK, 0);
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

  GST_INFO_OBJECT (oss, "Opened device");

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
    gst_oss4_source_close (asrc);
    if ((oss->fd = open (device, O_RDONLY, 0)) == -1)
      goto non_block;
  }

  oss->open_device = device;

  /* not using ENGINEINFO here because it sometimes returns a different and
   * less useful name than AUDIOINFO for the same device */
  if (!gst_oss4_property_probe_find_device_name (GST_OBJECT (oss), oss->fd,
          oss->open_device, &oss->device_name)) {
    oss->device_name = NULL;
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
      GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_READ,
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
      GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_READ,
          (_("Could not open audio device for playback.")), GST_ERROR_SYSTEM);
    }
    g_free (device);
    return FALSE;
  }
legacy_oss:
  {
    gst_oss4_source_close (asrc);
    if (!silent_errors) {
      GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_READ,
          (_("Could not open audio device for playback. "
                  "This version of the Open Sound System is not supported by this "
                  "element.")), ("Try the 'osssink' element instead"));
    }
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
gst_oss4_source_open_func (GstAudioSrc * asrc)
{
  return gst_oss4_source_open (asrc, FALSE);
}

static gboolean
gst_oss4_source_close (GstAudioSrc * asrc)
{
  GstOss4Source *oss;

  oss = GST_OSS4_SOURCE (asrc);

  if (oss->fd != -1) {
    GST_DEBUG_OBJECT (oss, "closing device");
    close (oss->fd);
    oss->fd = -1;
  }

  oss->bytes_per_sample = 0;

  gst_caps_replace (&oss->probed_caps, NULL);

  g_free (oss->open_device);
  oss->open_device = NULL;

  g_free (oss->device_name);
  oss->device_name = NULL;

  return TRUE;
}

static gboolean
gst_oss4_source_prepare (GstAudioSrc * asrc, GstAudioRingBufferSpec * spec)
{
  GstOss4Source *oss;

  oss = GST_OSS4_SOURCE (asrc);

  if (!gst_oss4_audio_set_format (GST_OBJECT_CAST (oss), oss->fd, spec)) {
    GST_WARNING_OBJECT (oss, "Couldn't set requested format %" GST_PTR_FORMAT,
        spec->caps);
    return FALSE;
  }

  oss->bytes_per_sample = GST_AUDIO_INFO_BPF (&spec->info);

  return TRUE;
}

static gboolean
gst_oss4_source_unprepare (GstAudioSrc * asrc)
{
  /* could do a SNDCTL_DSP_HALT, but the OSS manual recommends a close/open,
   * since HALT won't properly reset some devices, apparently */

  if (!gst_oss4_source_close (asrc))
    goto couldnt_close;

  if (!gst_oss4_source_open_func (asrc))
    goto couldnt_reopen;

  return TRUE;

  /* ERRORS */
couldnt_close:
  {
    GST_DEBUG_OBJECT (asrc, "Couldn't close the audio device");
    return FALSE;
  }
couldnt_reopen:
  {
    GST_DEBUG_OBJECT (asrc, "Couldn't reopen the audio device");
    return FALSE;
  }
}

static guint
gst_oss4_source_read (GstAudioSrc * asrc, gpointer data, guint length,
    GstClockTime * timestamp)
{
  GstOss4Source *oss;
  int n;

  oss = GST_OSS4_SOURCE_CAST (asrc);

  n = read (oss->fd, data, length);
  GST_LOG_OBJECT (asrc, "%u bytes, %u samples", n, n / oss->bytes_per_sample);

  if (G_UNLIKELY (n < 0)) {
    switch (errno) {
      case ENOTSUP:
      case EACCES:{
        /* This is the most likely cause, I think */
        GST_ELEMENT_ERROR (asrc, RESOURCE, READ,
            (_("Recording is not supported by this audio device.")),
            ("read: %s (device: %s) (maybe this is an output-only device?)",
                g_strerror (errno), oss->open_device));
        break;
      }
      default:{
        GST_ELEMENT_ERROR (asrc, RESOURCE, READ,
            (_("Error recording from audio device.")),
            ("read: %s (device: %s)", g_strerror (errno), oss->open_device));
        break;
      }
    }
  }

  return (guint) n;
}

static guint
gst_oss4_source_delay (GstAudioSrc * asrc)
{
  audio_buf_info info = { 0, };
  GstOss4Source *oss;
  guint delay;

  oss = GST_OSS4_SOURCE_CAST (asrc);

  if (ioctl (oss->fd, SNDCTL_DSP_GETISPACE, &info) == -1) {
    GST_LOG_OBJECT (oss, "GETISPACE failed: %s", g_strerror (errno));
    return 0;
  }

  delay = (info.fragstotal * info.fragsize) - info.bytes;
  GST_LOG_OBJECT (oss, "fragstotal:%d, fragsize:%d, bytes:%d, delay:%d",
      info.fragstotal, info.fragsize, info.bytes, delay);
  return delay;
}

static void
gst_oss4_source_reset (GstAudioSrc * asrc)
{
  /* There's nothing we can do here really: OSS can't handle access to the
   * same device/fd from multiple threads and might deadlock or blow up in
   * other ways if we try an ioctl SNDCTL_DSP_HALT or similar */
}
