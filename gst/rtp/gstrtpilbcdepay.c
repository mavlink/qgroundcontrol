/* GStreamer
 * Copyright (C) <2006> Philippe Khalaf <burger@speedy.org>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>
#include "gstrtpilbcdepay.h"
#include "gstrtputils.h"

/* RtpiLBCDepay signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

#define DEFAULT_MODE GST_ILBC_MODE_30

enum
{
  PROP_0,
  PROP_MODE
};

/* FIXME, mode should be string because it is a parameter in SDP fmtp */
static GstStaticPadTemplate gst_rtp_ilbc_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 8000, " "encoding-name = (string) \"ILBC\"")
    /* "mode = (string) { \"20\", \"30\" }" */
    );

static GstStaticPadTemplate gst_rtp_ilbc_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-iLBC, " "mode = (int) { 20, 30 }")
    );

static void gst_ilbc_depay_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_ilbc_depay_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static GstBuffer *gst_rtp_ilbc_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static gboolean gst_rtp_ilbc_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);

#define gst_rtp_ilbc_depay_parent_class parent_class
G_DEFINE_TYPE (GstRTPiLBCDepay, gst_rtp_ilbc_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

#define GST_TYPE_ILBC_MODE (gst_ilbc_mode_get_type())
static GType
gst_ilbc_mode_get_type (void)
{
  static GType ilbc_mode_type = 0;
  static const GEnumValue ilbc_modes[] = {
    {GST_ILBC_MODE_20, "20ms frames", "20ms"},
    {GST_ILBC_MODE_30, "30ms frames", "30ms"},
    {0, NULL, NULL},
  };

  if (!ilbc_mode_type) {
    ilbc_mode_type = g_enum_register_static ("iLBCMode", ilbc_modes);
  }
  return ilbc_mode_type;
}

static void
gst_rtp_ilbc_depay_class_init (GstRTPiLBCDepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->set_property = gst_ilbc_depay_set_property;
  gobject_class->get_property = gst_ilbc_depay_get_property;

  /* FIXME, mode is in the caps */
  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode", "iLBC frame mode",
          GST_TYPE_ILBC_MODE, DEFAULT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_ilbc_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_ilbc_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP iLBC depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts iLBC audio from RTP packets (RFC 3952)",
      "Philippe Kalaf <philippe.kalaf@collabora.co.uk>");

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_ilbc_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_ilbc_depay_setcaps;
}

static void
gst_rtp_ilbc_depay_init (GstRTPiLBCDepay * rtpilbcdepay)
{
  /* Set default mode */
  rtpilbcdepay->mode = DEFAULT_MODE;
}

static gboolean
gst_rtp_ilbc_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstRTPiLBCDepay *rtpilbcdepay = GST_RTP_ILBC_DEPAY (depayload);
  GstCaps *srccaps;
  GstStructure *structure;
  const gchar *mode_str = NULL;
  gint mode, clock_rate;
  gboolean ret;

  structure = gst_caps_get_structure (caps, 0);

  mode = rtpilbcdepay->mode;

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 8000;
  depayload->clock_rate = clock_rate;

  /* parse mode, if we can */
  mode_str = gst_structure_get_string (structure, "mode");
  if (mode_str) {
    mode = strtol (mode_str, NULL, 10);
    if (mode != 20 && mode != 30)
      mode = rtpilbcdepay->mode;
  }

  rtpilbcdepay->mode = mode;

  srccaps = gst_caps_new_simple ("audio/x-iLBC",
      "mode", G_TYPE_INT, rtpilbcdepay->mode, NULL);
  ret = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), srccaps);

  GST_DEBUG ("set caps on source: %" GST_PTR_FORMAT " (ret=%d)", srccaps, ret);
  gst_caps_unref (srccaps);

  return ret;
}

static GstBuffer *
gst_rtp_ilbc_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstBuffer *outbuf;
  gboolean marker;

  marker = gst_rtp_buffer_get_marker (rtp);

  GST_DEBUG ("process : got %" G_GSIZE_FORMAT " bytes, mark %d ts %u seqn %d",
      gst_buffer_get_size (rtp->buffer), marker,
      gst_rtp_buffer_get_timestamp (rtp), gst_rtp_buffer_get_seq (rtp));

  outbuf = gst_rtp_buffer_get_payload_buffer (rtp);

  if (marker && outbuf) {
    /* mark start of talkspurt with RESYNC */
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
  }

  if (outbuf) {
    gst_rtp_drop_non_audio_meta (depayload, outbuf);
  }

  return outbuf;
}

static void
gst_ilbc_depay_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstRTPiLBCDepay *rtpilbcdepay = GST_RTP_ILBC_DEPAY (object);

  switch (prop_id) {
    case PROP_MODE:
      rtpilbcdepay->mode = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_ilbc_depay_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstRTPiLBCDepay *rtpilbcdepay = GST_RTP_ILBC_DEPAY (object);

  switch (prop_id) {
    case PROP_MODE:
      g_value_set_enum (value, rtpilbcdepay->mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_rtp_ilbc_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpilbcdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_ILBC_DEPAY);
}
