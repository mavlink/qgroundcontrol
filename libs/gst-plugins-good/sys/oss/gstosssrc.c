/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *               2000,2005 Wim Taymans <wim@fluendo.com>
 *
 * gstosssrc.c: 
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
 * SECTION:element-osssrc
 * @title: osssrc
 *
 * This element lets you record sound using the Open Sound System (OSS).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v osssrc ! audioconvert ! vorbisenc ! oggmux ! filesink location=mymusic.ogg
 * ]| will record sound from your sound card using OSS and encode it to an
 * Ogg/Vorbis file (this will only work if your mixer settings are right
 * and the right inputs enabled etc.)
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
#include "gstosssrc.h"

#include <gst/gst-i18n-plugin.h>

GST_DEBUG_CATEGORY_EXTERN (oss_debug);
#define GST_CAT_DEFAULT oss_debug

#define DEFAULT_DEVICE          "/dev/dsp"
#define DEFAULT_DEVICE_NAME     ""

enum
{
  PROP_0,
  PROP_DEVICE,
  PROP_DEVICE_NAME,
};

#define gst_oss_src_parent_class parent_class
G_DEFINE_TYPE (GstOssSrc, gst_oss_src, GST_TYPE_AUDIO_SRC);

static void gst_oss_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_oss_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_oss_src_dispose (GObject * object);
static void gst_oss_src_finalize (GstOssSrc * osssrc);

static GstCaps *gst_oss_src_getcaps (GstBaseSrc * bsrc, GstCaps * filter);

static gboolean gst_oss_src_open (GstAudioSrc * asrc);
static gboolean gst_oss_src_close (GstAudioSrc * asrc);
static gboolean gst_oss_src_prepare (GstAudioSrc * asrc,
    GstAudioRingBufferSpec * spec);
static gboolean gst_oss_src_unprepare (GstAudioSrc * asrc);
static guint gst_oss_src_read (GstAudioSrc * asrc, gpointer data, guint length,
    GstClockTime * timestamp);
static guint gst_oss_src_delay (GstAudioSrc * asrc);
static void gst_oss_src_reset (GstAudioSrc * asrc);

#define FORMATS "{" GST_AUDIO_NE(S16)","GST_AUDIO_NE(U16)", S8, U8 }"

static GstStaticPadTemplate osssrc_src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
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

static void
gst_oss_src_dispose (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_oss_src_class_init (GstOssSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstAudioSrcClass *gstaudiosrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstaudiosrc_class = (GstAudioSrcClass *) klass;

  gobject_class->dispose = gst_oss_src_dispose;
  gobject_class->finalize = (GObjectFinalizeFunc) gst_oss_src_finalize;
  gobject_class->get_property = gst_oss_src_get_property;
  gobject_class->set_property = gst_oss_src_set_property;

  gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_oss_src_getcaps);

  gstaudiosrc_class->open = GST_DEBUG_FUNCPTR (gst_oss_src_open);
  gstaudiosrc_class->prepare = GST_DEBUG_FUNCPTR (gst_oss_src_prepare);
  gstaudiosrc_class->unprepare = GST_DEBUG_FUNCPTR (gst_oss_src_unprepare);
  gstaudiosrc_class->close = GST_DEBUG_FUNCPTR (gst_oss_src_close);
  gstaudiosrc_class->read = GST_DEBUG_FUNCPTR (gst_oss_src_read);
  gstaudiosrc_class->delay = GST_DEBUG_FUNCPTR (gst_oss_src_delay);
  gstaudiosrc_class->reset = GST_DEBUG_FUNCPTR (gst_oss_src_reset);

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device",
          "OSS device (usually /dev/dspN)", DEFAULT_DEVICE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Human-readable name of the sound device", DEFAULT_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));


  gst_element_class_set_static_metadata (gstelement_class, "Audio Source (OSS)",
      "Source/Audio",
      "Capture from a sound card via OSS",
      "Erik Walthinsen <omega@cse.ogi.edu>, " "Wim Taymans <wim@fluendo.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &osssrc_src_factory);
}

static void
gst_oss_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOssSrc *src;

  src = GST_OSS_SRC (object);

  switch (prop_id) {
    case PROP_DEVICE:
      g_free (src->device);
      src->device = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_oss_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstOssSrc *src;

  src = GST_OSS_SRC (object);

  switch (prop_id) {
    case PROP_DEVICE:
      g_value_set_string (value, src->device);
      break;
    case PROP_DEVICE_NAME:
      g_value_set_string (value, src->device_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_oss_src_init (GstOssSrc * osssrc)
{
  const gchar *device;

  GST_DEBUG ("initializing osssrc");

  device = g_getenv ("AUDIODEV");
  if (device == NULL)
    device = DEFAULT_DEVICE;

  osssrc->fd = -1;
  osssrc->device = g_strdup (device);
  osssrc->device_name = g_strdup (DEFAULT_DEVICE_NAME);
  osssrc->probed_caps = NULL;
}

static void
gst_oss_src_finalize (GstOssSrc * osssrc)
{
  g_free (osssrc->device);
  g_free (osssrc->device_name);

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (osssrc));
}

static GstCaps *
gst_oss_src_getcaps (GstBaseSrc * bsrc, GstCaps * filter)
{
  GstOssSrc *osssrc;
  GstCaps *caps;

  osssrc = GST_OSS_SRC (bsrc);

  if (osssrc->fd == -1) {
    GST_DEBUG_OBJECT (osssrc, "device not open, using template caps");
    return NULL;                /* base class will get template caps for us */
  }

  if (osssrc->probed_caps) {
    GST_LOG_OBJECT (osssrc, "Returning cached caps");
    return gst_caps_ref (osssrc->probed_caps);
  }

  caps = gst_oss_helper_probe_caps (osssrc->fd);

  if (caps) {
    osssrc->probed_caps = gst_caps_ref (caps);
  }

  GST_INFO_OBJECT (osssrc, "returning caps %" GST_PTR_FORMAT, caps);

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
gst_oss_src_get_format (GstAudioRingBufferFormatType fmt, GstAudioFormat rfmt)
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
gst_oss_src_open (GstAudioSrc * asrc)
{
  GstOssSrc *oss;
  int mode;

  oss = GST_OSS_SRC (asrc);

  mode = O_RDONLY;
  mode |= O_NONBLOCK;

  oss->fd = open (oss->device, mode, 0);
  if (oss->fd == -1) {
    switch (errno) {
      case EACCES:
        goto no_permission;
      default:
        goto open_failed;
    }
  }

  g_free (oss->device_name);
  oss->device_name = gst_oss_helper_get_card_name ("/dev/mixer");

  return TRUE;

no_permission:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_READ,
        (_("Could not open audio device for recording. "
                "You don't have permission to open the device.")),
        GST_ERROR_SYSTEM);
    return FALSE;
  }
open_failed:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_READ,
        (_("Could not open audio device for recording.")),
        ("Unable to open device %s for recording: %s",
            oss->device, g_strerror (errno)));
    return FALSE;
  }
}

static gboolean
gst_oss_src_close (GstAudioSrc * asrc)
{
  GstOssSrc *oss;

  oss = GST_OSS_SRC (asrc);

  close (oss->fd);

  gst_caps_replace (&oss->probed_caps, NULL);

  return TRUE;
}

static gboolean
gst_oss_src_prepare (GstAudioSrc * asrc, GstAudioRingBufferSpec * spec)
{
  GstOssSrc *oss;
  struct audio_buf_info info;
  int mode;
  int fmt, tmp;
  guint width, rate, channels;

  oss = GST_OSS_SRC (asrc);

  mode = fcntl (oss->fd, F_GETFL);
  mode &= ~O_NONBLOCK;
  if (fcntl (oss->fd, F_SETFL, mode) == -1)
    goto non_block;

  fmt = gst_oss_src_get_format (spec->type,
      GST_AUDIO_INFO_FORMAT (&spec->info));
  if (fmt == 0)
    goto wrong_format;

  width = GST_AUDIO_INFO_WIDTH (&spec->info);
  rate = GST_AUDIO_INFO_RATE (&spec->info);
  channels = GST_AUDIO_INFO_CHANNELS (&spec->info);

  if (width != 16 && width != 8)
    goto dodgy_width;

  tmp = ilog2 (spec->segsize);
  tmp = ((spec->segtotal & 0x7fff) << 16) | tmp;
  GST_DEBUG_OBJECT (oss, "set segsize: %d, segtotal: %d, value: %08x",
      spec->segsize, spec->segtotal, tmp);

  SET_PARAM (oss, SNDCTL_DSP_SETFRAGMENT, tmp, "SETFRAGMENT");

  SET_PARAM (oss, SNDCTL_DSP_RESET, 0, "RESET");

  SET_PARAM (oss, SNDCTL_DSP_SETFMT, fmt, "SETFMT");
  if (channels == 2)
    SET_PARAM (oss, SNDCTL_DSP_STEREO, 1, "STEREO");
  SET_PARAM (oss, SNDCTL_DSP_CHANNELS, channels, "CHANNELS");
  SET_PARAM (oss, SNDCTL_DSP_SPEED, rate, "SPEED");

  GET_PARAM (oss, SNDCTL_DSP_GETISPACE, &info, "GETISPACE");

  spec->segsize = info.fragsize;
  spec->segtotal = info.fragstotal;

  oss->bytes_per_sample = GST_AUDIO_INFO_BPF (&spec->info);

  GST_DEBUG_OBJECT (oss, "got segsize: %d, segtotal: %d, value: %08x",
      spec->segsize, spec->segtotal, tmp);

  return TRUE;

non_block:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_READ,
        ("Unable to set device %s in non blocking mode: %s",
            oss->device, g_strerror (errno)), (NULL));
    return FALSE;
  }
wrong_format:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_READ,
        ("Unable to get format (%d, %d)", spec->type,
            GST_AUDIO_INFO_FORMAT (&spec->info)), (NULL));
    return FALSE;
  }
dodgy_width:
  {
    GST_ELEMENT_ERROR (oss, RESOURCE, OPEN_READ,
        ("Unexpected width %d", width), (NULL));
    return FALSE;
  }
}

static gboolean
gst_oss_src_unprepare (GstAudioSrc * asrc)
{
  /* could do a SNDCTL_DSP_RESET, but the OSS manual recommends a close/open */

  if (!gst_oss_src_close (asrc))
    goto couldnt_close;

  if (!gst_oss_src_open (asrc))
    goto couldnt_reopen;

  return TRUE;

couldnt_close:
  {
    GST_DEBUG_OBJECT (asrc, "Could not close the audio device");
    return FALSE;
  }
couldnt_reopen:
  {
    GST_DEBUG_OBJECT (asrc, "Could not reopen the audio device");
    return FALSE;
  }
}

static guint
gst_oss_src_read (GstAudioSrc * asrc, gpointer data, guint length,
    GstClockTime * timestamp)
{
  return read (GST_OSS_SRC (asrc)->fd, data, length);
}

static guint
gst_oss_src_delay (GstAudioSrc * asrc)
{
  GstOssSrc *oss;
  gint delay = 0;
  gint ret;

  oss = GST_OSS_SRC (asrc);

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
gst_oss_src_reset (GstAudioSrc * asrc)
{
  /* There's nothing we can do here really: OSS can't handle access to the
   * same device/fd from multiple threads and might deadlock or blow up in
   * other ways if we try an ioctl SNDCTL_DSP_RESET or similar */
}
