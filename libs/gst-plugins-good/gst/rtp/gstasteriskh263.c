/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
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

#include <gst/rtp/gstrtpbuffer.h>
#include "gstasteriskh263.h"

#define GST_ASTERISKH263_HEADER_LEN 6

typedef struct _GstAsteriskH263Header
{
  guint32 timestamp;            /* Timestamp */
  guint16 length;               /* Length */
} GstAsteriskH263Header;

#define GST_ASTERISKH263_HEADER_TIMESTAMP(data) (((GstAsteriskH263Header *)(data))->timestamp)
#define GST_ASTERISKH263_HEADER_LENGTH(data) (((GstAsteriskH263Header *)(data))->length)

static GstStaticPadTemplate gst_asteriskh263_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-asteriskh263")
    );

static GstStaticPadTemplate gst_asteriskh263_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) [ 96, 127 ], "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H263-1998\"")
    );

static void gst_asteriskh263_finalize (GObject * object);

static GstFlowReturn gst_asteriskh263_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

static GstStateChangeReturn gst_asteriskh263_change_state (GstElement *
    element, GstStateChange transition);

#define gst_asteriskh263_parent_class parent_class
G_DEFINE_TYPE (GstAsteriskh263, gst_asteriskh263, GST_TYPE_ELEMENT);

static void
gst_asteriskh263_class_init (GstAsteriskh263Class * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->finalize = gst_asteriskh263_finalize;

  gstelement_class->change_state = gst_asteriskh263_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_asteriskh263_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_asteriskh263_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Asterisk H263 depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts H263 video from RTP and encodes in Asterisk H263 format",
      "Neil Stratford <neils@vipadia.com>");
}

static void
gst_asteriskh263_init (GstAsteriskh263 * asteriskh263)
{
  asteriskh263->srcpad =
      gst_pad_new_from_static_template (&gst_asteriskh263_src_template, "src");
  gst_element_add_pad (GST_ELEMENT (asteriskh263), asteriskh263->srcpad);

  asteriskh263->sinkpad =
      gst_pad_new_from_static_template (&gst_asteriskh263_sink_template,
      "sink");
  gst_pad_set_chain_function (asteriskh263->sinkpad, gst_asteriskh263_chain);
  gst_element_add_pad (GST_ELEMENT (asteriskh263), asteriskh263->sinkpad);

  asteriskh263->adapter = gst_adapter_new ();
}

static void
gst_asteriskh263_finalize (GObject * object)
{
  GstAsteriskh263 *asteriskh263;

  asteriskh263 = GST_ASTERISK_H263 (object);

  g_object_unref (asteriskh263->adapter);
  asteriskh263->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstFlowReturn
gst_asteriskh263_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstAsteriskh263 *asteriskh263;
  GstBuffer *outbuf;
  GstFlowReturn ret;

  asteriskh263 = GST_ASTERISK_H263 (parent);

  {
    gint payload_len;
    guint8 *payload;
    gboolean M;
    guint32 timestamp;
    guint32 samples;
    guint16 asterisk_len;
    GstRTPBuffer rtp = { NULL };
    GstMapInfo map;

    if (!gst_rtp_buffer_map (buf, GST_MAP_READ, &rtp))
      goto bad_packet;

    payload_len = gst_rtp_buffer_get_payload_len (&rtp);
    payload = gst_rtp_buffer_get_payload (&rtp);

    M = gst_rtp_buffer_get_marker (&rtp);
    timestamp = gst_rtp_buffer_get_timestamp (&rtp);

    gst_rtp_buffer_unmap (&rtp);

    outbuf = gst_buffer_new_and_alloc (payload_len +
        GST_ASTERISKH263_HEADER_LEN);

    /* build the asterisk header */
    asterisk_len = payload_len;
    if (M)
      asterisk_len |= 0x8000;
    if (!asteriskh263->lastts)
      asteriskh263->lastts = timestamp;
    samples = timestamp - asteriskh263->lastts;
    asteriskh263->lastts = timestamp;

    gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
    GST_ASTERISKH263_HEADER_TIMESTAMP (map.data) = g_htonl (samples);
    GST_ASTERISKH263_HEADER_LENGTH (map.data) = g_htons (asterisk_len);

    /* copy the data into place */
    memcpy (map.data + GST_ASTERISKH263_HEADER_LEN, payload, payload_len);

    gst_buffer_unmap (outbuf, &map);

    GST_BUFFER_PTS (outbuf) = timestamp;
    if (!gst_pad_has_current_caps (asteriskh263->srcpad)) {
      GstCaps *caps;

      caps = gst_pad_get_pad_template_caps (asteriskh263->srcpad);
      gst_pad_set_caps (asteriskh263->srcpad, caps);
      gst_caps_unref (caps);
    }

    ret = gst_pad_push (asteriskh263->srcpad, outbuf);

    gst_buffer_unref (buf);
  }

  return ret;

bad_packet:
  {
    GST_DEBUG ("Packet does not validate");
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
}

static GstStateChangeReturn
gst_asteriskh263_change_state (GstElement * element, GstStateChange transition)
{
  GstAsteriskh263 *asteriskh263;
  GstStateChangeReturn ret;

  asteriskh263 = GST_ASTERISK_H263 (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_adapter_clear (asteriskh263->adapter);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  /*
     switch (transition) {
     case GST_STATE_CHANGE_READY_TO_NULL:
     break;
     default:
     break;
     }
   */
  return ret;
}

gboolean
gst_asteriskh263_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "asteriskh263",
      GST_RANK_NONE, GST_TYPE_ASTERISK_H263);
}
