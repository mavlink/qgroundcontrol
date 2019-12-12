/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *               2000,2005 Wim Taymans <wim@fluendo.com>
 *
 * gstosssink.c: 
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
 * SECTION:element-osssink
 * @title: osssink
 *
 * This element lets you output sound using the Open Sound System (OSS).
 *
 * Note that you should almost always use generic audio conversion elements
 * like audioconvert and audioresample in front of an audiosink to make sure
 * your pipeline works under all circumstances (those conversion elements will
 * act in passthrough-mode if no conversion is necessary).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v audiotestsrc ! audioconvert ! volume volume=0.1 ! osssink
 * ]| will output a sine wave (continuous beep sound) to your sound card (with
 * a very low volume as precaution).
 * |[
 * gst-launch-1.0 -v filesrc location=music.ogg ! decodebin ! audioconvert ! audioresample ! osssink
 * ]| will play an Ogg/Vorbis audio file and output it using the Open Sound System.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_OSS_INCLUDE_IN_SYS
# include <sys/soundcard.h>
#else
# ifdef HAVE_OSS_INCLUDE_IN_ROOT
#  include <soundcard.h>
# else
#  ifdef HAVE_OSS_INCLUDE_IN_MACHINE
#   include <machine/soundcard.h>
#  else
#   error "What to include?"
#  endif /* HAVE_OSS_INCLUDE_IN_MACHINE */
# endif /* HAVE_OSS_INCLUDE_IN_ROOT */
#endif /* HAVE_OSS_INCLUDE_IN_SYS */

#include "common.h"
#include "gstosssink.h"

#include <gst/gst-i18n-plugin.h>

GST_DEBUG_CATEGORY_EXTERN (oss_debug);
#define GST_CAT_DEFAULT oss_debug

static void gst_oss_sink_dispose (GObject * object);
static void gst_oss_sink_finalise (GObject * object);

static void gst_oss_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_oss_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static GstCaps *gst_oss_sink_getcaps (GstBaseSink * bsink, GstCaps * filter);

static gboolean gst_oss_sink_open (GstAudioSink * asink);
static gboolean gst_oss_sink_close (GstAudioSink * asink);
static gboolean gst_oss_sink_prepare (GstAudioSink * asink,
    GstAudioRingBufferSpec * spec);
static gboolean gst_oss_sink_unprepare (GstAudioSink * asink);
static gint gst_oss_sink_write (GstAudioSink * asink, gpointer data,
    guint length);
static guint gst_oss_sink_delay (GstAudioSink * asink);
static void gst_oss_sink_reset (GstAudioSink * asink);

/* OssSink signals and args */
enum
{
  LAST_SIGNAL
};

#define DEFAULT_DEVICE  "/dev/dsp"
enum
{
  PROP_0,
  PROP_DEVICE,
};

#define FORMATS "{" GST_AUDIO_NE(S16)","GST_AUDIO_NE(U16)", S8, U8 }"

static GstStaticPadTemplate osssink_sink_factory =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " FORMATS ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) 1; "
        "audio/x-raw, "
        "format = (string) " FORMATS ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) 2, " "channel-mask = (bitmask) 0x3")
    );

/* static guint gst_oss_sink_signals[LAST_SIGNAL] = { 0 }; */

#define gst_oss_sink_parent_class parent_class
G_DEFINE_TYPE (GstOssSink, gst_oss_sink, GST_TYPE_AUDIO_SINK);

static void
gst_oss_sink_dispose (GObject * object)
{
  GstOssSink *osssink = GST_OSSSINK (object);

  if (osssink->probed_caps) {
    gst_caps_unref (osssink->probed_caps);
    osssink->probed_caps = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_oss_sink_class_init (GstOssSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstAudioSinkClass *gstaudiosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstaudiosink_class = (GstAudioSinkClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = gst_oss_sink_dispose;
  gobject_class->finalize = gst_oss_sink_finalise;
  gobject_class->get_property = gst_oss_sink_get_property;
  gobject_class->set_property = gst_oss_sink_set_property;

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device",
          "OSS device (usually /dev/dspN)", DEFAULT_DEVICE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_oss_sink_getcaps);

  gstaudiosink_class->open = GST_DEBUG_FUNCPTR (gst_oss_sink_open);
  gstaudiosink_class->close = GST_DEBUG_FUNCPTR (gst_oss_sink_close);
  gstaudiosink_class->prepare = GST_DEBUG_FUNCPTR (gst_oss_sink_prepare);
  gstaudiosink_class->unprepare = GST_DEBUG_FUNCPTR (gst_oss_sink_unprepare);
  gstaudiosink_class->write = GST_DEBUG_FUNCPTR (gst_oss_sink_write);
  gstaudiosink_class->delay = GST_DEBUG_FUNCPTR (gst_oss_sink_delay);
  gstaudiosink_class->reset = GST_DEBUG_FUNCPTR (gst_oss_sink_reset);

  gst_element_class_set_static_metadata (gstelement_class, "Audio Sink (OSS)",
      "Sink/Audio",
      "Output to a sound card via OSS",
      "Erik Walthinsen <omega@cse.ogi.edu>, "
      "Wim Taymans <wim.taymans@chello.be>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &osssink_sink_factory);
}

static void
gst_oss_sink_init (GstOssSink * osssink)
{
  const gchar *device;

  GST_DEBUG_OBJECT (osssink, "initializing osssink");

  device = g_getenv ("AUDIODEV");
  if (device == NULL)
    device = DEFAULT_DEVICE;
  osssink->device = g_strdup (device);
  osssink->fd = -1;
}

static void
gst_oss_sink_finalise (GObject * object)
{
  GstOssSink *osssink = GST_OSSSINK (object);

  g_free (osssink->device);

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (object));
}

static void
gst_oss_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOssSink *sink;

  sink = GST_OSSSINK (object);

  switch (prop_id) {
    case PROP_DEVICE:
      g_free (sink->device);
      sink->device = g_value_dup_string (value);
      if (sink->probed_caps) {
        gst_caps_unref (sink->probed_caps);
        sink->probed_caps = NULL;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_oss_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstOssSink *sink;

  sink = GST_OSSSINK (object);

  switch (prop_id) {
    case PROP_DEVICE:
      g_value_set_string (value, sink->device);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstCaps *
gst_oss_sink_getcaps (GstBaseSink * bsink, GstCaps * filter)
{
  GstOssSink *osssink;
  GstCaps *caps;

  osssink = GST_OSSSINK (bsink);

  if (osssink->fd == -1) {
    caps = gst_pad_get_pad_template_caps (GST_BASE_SINK_PAD (bsink));
  } else if (osssink->probed_caps) {
    caps = gst_caps_ref (osssink->probed_caps);
  } else {
    caps = gst_oss_helper_probe_caps (osssink->fd);
    if (caps && !gst_caps_is_empty (caps)) {
      osssink->probed_caps = gst_caps_ref (caps);
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

static gint
ilog2 (gint x)
{
  /* well... hacker's delight explains... */
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  x = (x + (x >> 4)) & 0x0f0f0f0f;
  x = x + (x >> 8);
  x = x + (x >> 16);
  return (x & 0x0000003f) - 1;
}

static gint
gst_oss_sink_get_format (GstAudioRingBufferFormatType fmt, GstAudioFormat rfmt)
{
  gint result;

  switch (fmt) {
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MU_LAW:
      result = AFMT_MU_LAW;
      break;
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_A_LAW:
      result = AFMT_A_LAW;
      break;
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_IMA_ADPCM:
      result = AFMT_IMA_ADPCM;
      break;
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MPEG:
      result = AFMT_MPEG;
      break;
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_RAW:
    {
      switch (rfmt) {
        case GST_AUDIO_FORMAT_U8:
          result = AFMT_U8;
          break;
        case GST_AUDIO_FORMAT_S16LE:
          result = AFMT_S16_LE;
          break;
        case GST_AUDIO_FORMAT_S16BE:
          result = AFMT_S16_BE;
          break;
        case GST_AUDIO_FORMAT_S8:
          result = AFMT_S8;
          break;
        case GST_AUDIO_FORMAT_U16LE:
          result = AFMT_U16_LE;
          break;
        case GST_AUDIO_FORMAT_U16BE:
          result = AFMT_U16_BE;
          break;
        default:
          result = 0;
          break;
      }
      break;
    }
    default:
      result = 0;
      break;
  }
  return result;
}

static gboolean
gst_oss_sink_open (GstAudioSink * asink)
{
  GstOssSink *oss;
  int mode;

  oss = GST_OSSSINK (asink);

  mode = O_WRONLY;
  mode |= O_NONBLOCK;

  oss->fd = open (oss->device, mode, 0);
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

  return TRUE;

  /* ERRORS */
busy:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, BUSY,
        (_("Could not open audio device for playback. "
                "Device is being used by another application.")), (NULL));
    return FALSE;
  }
no_permission:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_WRITE,
        (_("Could not open audio device for playback. "
                "You don't have permission to open the device.")),
        GST_ERROR_SYSTEM);
    return FALSE;
  }
open_failed:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_WRITE,
        (_("Could not open audio device for playback.")), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

static gboolean
gst_oss_sink_close (GstAudioSink * asink)
{
  close (GST_OSSSINK (asink)->fd);
  GST_OSSSINK (asink)->fd = -1;
  return TRUE;
}

static gboolean
gst_oss_sink_prepare (GstAudioSink * asink, GstAudioRingBufferSpec * spec)
{
  GstOssSink *oss;
  struct audio_buf_info info;
  int mode;
  int tmp;
  guint width, rate, channels;

  oss = GST_OSSSINK (asink);

  /* we opened non-blocking so that we can detect if the device is available
   * without hanging forever. We now want to remove the non-blocking flag. */
  mode = fcntl (oss->fd, F_GETFL);
  mode &= ~O_NONBLOCK;
  if (fcntl (oss->fd, F_SETFL, mode) == -1) {
    /* some drivers do no support unsetting the non-blocking flag, try to
     * close/open the device then. This is racy but we error out properly. */
    gst_oss_sink_close (asink);
    if ((oss->fd = open (oss->device, O_WRONLY, 0)) == -1)
      goto non_block;
  }

  tmp = gst_oss_sink_get_format (spec->type,
      GST_AUDIO_INFO_FORMAT (&spec->info));
  if (tmp == 0)
    goto wrong_format;

  width = GST_AUDIO_INFO_WIDTH (&spec->info);
  rate = GST_AUDIO_INFO_RATE (&spec->info);
  channels = GST_AUDIO_INFO_CHANNELS (&spec->info);

  if (width != 16 && width != 8)
    goto dodgy_width;

  SET_PARAM (oss, SNDCTL_DSP_SETFMT, tmp, "SETFMT");
  if (channels == 2)
    SET_PARAM (oss, SNDCTL_DSP_STEREO, 1, "STEREO");
  SET_PARAM (oss, SNDCTL_DSP_CHANNELS, channels, "CHANNELS");
  SET_PARAM (oss, SNDCTL_DSP_SPEED, rate, "SPEED");

  tmp = ilog2 (spec->segsize);
  tmp = ((spec->segtotal & 0x7fff) << 16) | tmp;
  GST_DEBUG_OBJECT (oss, "set segsize: %d, segtotal: %d, value: %08x",
      spec->segsize, spec->segtotal, tmp);

  SET_PARAM (oss, SNDCTL_DSP_SETFRAGMENT, tmp, "SETFRAGMENT");
  GET_PARAM (oss, SNDCTL_DSP_GETOSPACE, &info, "GETOSPACE");

  spec->segsize = info.fragsize;
  spec->segtotal = info.fragstotal;

  oss->bytes_per_sample = GST_AUDIO_INFO_BPF (&spec->info);

  GST_DEBUG_OBJECT (oss, "got segsize: %d, segtotal: %d, value: %08x",
      spec->segsize, spec->segtotal, tmp);

  return TRUE;

  /* ERRORS */
non_block:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, SETTINGS, (NULL),
        ("Unable to set device %s in non blocking mode: %s",
            oss->device, g_strerror (errno)));
    return FALSE;
  }
wrong_format:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, SETTINGS, (NULL),
        ("Unable to get format (%d, %d)", spec->type,
            GST_AUDIO_INFO_FORMAT (&spec->info)));
    return FALSE;
  }
dodgy_width:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, SETTINGS, (NULL),
        ("unexpected width %d", width));
    return FALSE;
  }
}

static gboolean
gst_oss_sink_unprepare (GstAudioSink * asink)
{
  /* could do a SNDCTL_DSP_RESET, but the OSS manual recommends a close/open */

  if (!gst_oss_sink_close (asink))
    goto couldnt_close;

  if (!gst_oss_sink_open (asink))
    goto couldnt_reopen;

  return TRUE;

  /* ERRORS */
couldnt_close:
  {
    GST_DEBUG_OBJECT (asink, "Could not close the audio device");
    return FALSE;
  }
couldnt_reopen:
  {
    GST_DEBUG_OBJECT (asink, "Could not reopen the audio device");
    return FALSE;
  }
}

static gint
gst_oss_sink_write (GstAudioSink * asink, gpointer data, guint length)
{
  return write (GST_OSSSINK (asink)->fd, data, length);
}

static guint
gst_oss_sink_delay (GstAudioSink * asink)
{
  GstOssSink *oss;
  gint delay = 0;
  gint ret;

  oss = GST_OSSSINK (asink);

#ifdef SNDCTL_DSP_GETODELAY
  ret = ioctl (oss->fd, SNDCTL_DSP_GETODELAY, &delay);
#else
  ret = -1;
#endif
  if (ret < 0) {
    audio_buf_info info;

    ret = ioctl (oss->fd, SNDCTL_DSP_GETOSPACE, &info);

    delay = (ret < 0 ? 0 : (info.fragstotal * info.fragsize) - info.bytes);
  }
  return delay / oss->bytes_per_sample;
}

static void
gst_oss_sink_reset (GstAudioSink * asink)
{
  /* There's nothing we can do here really: OSS can't handle access to the
   * same device/fd from multiple threads and might deadlock or blow up in
   * other ways if we try an ioctl SNDCTL_DSP_RESET or similar */
}
