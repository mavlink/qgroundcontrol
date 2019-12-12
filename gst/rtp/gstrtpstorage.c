/* GStreamer plugin for forward error correction
 * Copyright (C) 2017 Pexip
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Mikhail Fludkov <misha@pexip.com>
 */

/**
 * SECTION:element-rtpstorage
 * @short_description: RTP storage for forward error correction (FEC) in rtpbin
 * @title: rtpstorage
 *
 * Helper element for storing packets to aid later packet recovery from packet
 * loss using RED/FEC (Forward Error Correction).
 *
 * The purpose of this element is to store a moving window of packets which
 * downstream elements such as #GstRtpUlpFecDec can request in order to perform
 * recovery of lost packets upon receiving custom GstRtpPacketLost events,
 * usually from #GstRtpJitterBuffer.
 *
 * As such, when building a pipeline manually, it should have the form:
 *
 * ```
 * rtpstorage ! rtpjitterbuffer ! rtpulpfecdec
 * ```
 *
 * where rtpulpfecdec get passed a reference to the object pointed to by
 * the #GstRtpStorage:internal-storage property.
 *
 * The #GstRtpStorage:size-time property should be configured with a value
 * equal to the #GstRtpJitterBuffer latency, plus some tolerance, in the order
 * of milliseconds, for example in the example found at
 * <https://github.com/sdroege/gstreamer-rs/blob/master/examples/src/bin/rtpfecclient.rs>,
 * `size-time` is configured as 200 + 50 milliseconds (latency + tolerance).
 *
 * When using #GstRtpBin, a storage element is created automatically, and
 * can be configured upon receiving the #GstRtpBin::new-storage signal.
 *
 * See also: #GstRtpBin, #GstRtpUlpFecDec
 * Since: 1.14
 */

#include "gstrtpstorage.h"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );

enum
{
  PROP_0,
  PROP_SIZE_TIME,
  PROP_INTERNAL_STORAGE,
  N_PROPERTIES
};

static GParamSpec *klass_properties[N_PROPERTIES] = { NULL, };

#define DEFAULT_SIZE_TIME (0)

GST_DEBUG_CATEGORY (gst_rtp_storage_debug);
#define GST_CAT_DEFAULT (gst_rtp_storage_debug)

G_DEFINE_TYPE (GstRtpStorage, gst_rtp_storage, GST_TYPE_ELEMENT);

static GstFlowReturn
gst_rtp_storage_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstRtpStorage *self = GST_RTP_STORAGE (parent);;

  if (rtp_storage_append_buffer (self->storage, buf))
    return gst_pad_push (self->srcpad, buf);
  return GST_FLOW_OK;
}

static void
gst_rtp_storage_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpStorage *self = GST_RTP_STORAGE (object);

  switch (prop_id) {
    case PROP_SIZE_TIME:
      GST_DEBUG_OBJECT (self, "RTP storage size set to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (g_value_get_uint64 (value)));
      rtp_storage_set_size (self->storage, g_value_get_uint64 (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_storage_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpStorage *self = GST_RTP_STORAGE (object);
  switch (prop_id) {
    case PROP_SIZE_TIME:
      g_value_set_uint64 (value, rtp_storage_get_size (self->storage));
      break;
    case PROP_INTERNAL_STORAGE:
    {
      g_value_set_object (value, self->storage);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_rtp_storage_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  GstRtpStorage *self = GST_RTP_STORAGE (parent);

  if (GST_QUERY_TYPE (query) == GST_QUERY_CUSTOM) {
    GstStructure *s = gst_query_writable_structure (query);

    if (gst_structure_has_name (s, "GstRtpStorage")) {
      gst_structure_set (s, "storage", G_TYPE_OBJECT, self->storage, NULL);
      return TRUE;
    }
  }

  return gst_pad_query_default (pad, parent, query);
}

static void
gst_rtp_storage_init (GstRtpStorage * self)
{
  self->srcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  self->sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  GST_PAD_SET_PROXY_CAPS (self->sinkpad);
  GST_PAD_SET_PROXY_ALLOCATION (self->sinkpad);
  gst_pad_set_chain_function (self->sinkpad, gst_rtp_storage_chain);

  gst_pad_set_query_function (self->srcpad, gst_rtp_storage_src_query);

  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  self->storage = rtp_storage_new ();
}

static void
gst_rtp_storage_dispose (GObject * obj)
{
  GstRtpStorage *self = GST_RTP_STORAGE (obj);
  g_object_unref (self->storage);
  G_OBJECT_CLASS (gst_rtp_storage_parent_class)->dispose (obj);
}

static void
gst_rtp_storage_class_init (GstRtpStorageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_rtp_storage_debug,
      "rtpstorage", 0, "RTP Storage");
  GST_DEBUG_REGISTER_FUNCPTR (gst_rtp_storage_chain);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sinktemplate));

  gst_element_class_set_static_metadata (element_class,
      "RTP storage",
      "Analyzer/RTP",
      "Helper element for various purposes "
      "(ex. recovering from packet loss using RED/FEC). "
      "Saves given number of RTP packets. "
      "Should be instantiated before jitterbuffer",
      "Mikhail Fludkov <misha@pexip.com>");

  gobject_class->set_property = gst_rtp_storage_set_property;
  gobject_class->get_property = gst_rtp_storage_get_property;
  gobject_class->dispose = gst_rtp_storage_dispose;

  klass_properties[PROP_SIZE_TIME] =
      g_param_spec_uint64 ("size-time", "Storage size (in ns)",
      "The amount of data to keep in the storage (in ns, 0-disable)", 0,
      G_MAXUINT64, DEFAULT_SIZE_TIME,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  klass_properties[PROP_INTERNAL_STORAGE] =
      g_param_spec_object ("internal-storage", "Internal storage",
      "Internal RtpStorage object", G_TYPE_OBJECT,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES,
      klass_properties);
}
