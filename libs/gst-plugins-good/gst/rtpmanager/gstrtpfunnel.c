/* RTP funnel element for GStreamer
 *
 * gstrtpfunnel.c:
 *
 * Copyright (C) <2017> Pexip.
 *   Contact: Havard Graff <havard@pexip.com>
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
#include "config.h"
#endif

#include "gstrtpfunnel.h"

GST_DEBUG_CATEGORY_STATIC (gst_rtp_funnel_debug);
#define GST_CAT_DEFAULT gst_rtp_funnel_debug

struct _GstRtpFunnelPadClass
{
  GstPadClass class;
};

struct _GstRtpFunnelPad
{
  GstPad pad;
  guint32 ssrc;
};

enum
{
  PROP_0,
  PROP_COMMON_TS_OFFSET,
};

#define DEFAULT_COMMON_TS_OFFSET -1

G_DEFINE_TYPE (GstRtpFunnelPad, gst_rtp_funnel_pad, GST_TYPE_PAD);

static void
gst_rtp_funnel_pad_class_init (GstRtpFunnelPadClass * klass)
{
  (void) klass;
}

static void
gst_rtp_funnel_pad_init (GstRtpFunnelPad * pad)
{
  (void) pad;
}

struct _GstRtpFunnelClass
{
  GstElementClass class;
};

struct _GstRtpFunnel
{
  GstElement element;

  GstPad *srcpad;
  GstCaps *srccaps;
  gboolean send_sticky_events;
  GHashTable *ssrc_to_pad;
  /* The last pad data was chained on */
  GstPad *current_pad;

  /* properties */
  gint common_ts_offset;
};

#define RTP_CAPS "application/x-rtp"

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (RTP_CAPS));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (RTP_CAPS));

#define gst_rtp_funnel_parent_class parent_class
G_DEFINE_TYPE (GstRtpFunnel, gst_rtp_funnel, GST_TYPE_ELEMENT);


static void
gst_rtp_funnel_send_sticky (GstRtpFunnel * funnel, GstPad * pad)
{
  GstEvent *stream_start;
  GstEvent *caps;

  if (!funnel->send_sticky_events)
    goto done;

  stream_start = gst_pad_get_sticky_event (pad, GST_EVENT_STREAM_START, 0);
  if (stream_start && !gst_pad_push_event (funnel->srcpad, stream_start)) {
    GST_ERROR_OBJECT (funnel, "Could not push stream start");
    goto done;
  }

  caps = gst_event_new_caps (funnel->srccaps);
  if (caps && !gst_pad_push_event (funnel->srcpad, caps)) {
    GST_ERROR_OBJECT (funnel, "Could not push caps");
    goto done;
  }

  funnel->send_sticky_events = FALSE;

done:
  return;
}

static void
gst_rtp_funnel_forward_segment (GstRtpFunnel * funnel, GstPad * pad)
{
  GstEvent *segment;

  if (pad == funnel->current_pad) {
    goto done;
  }

  segment = gst_pad_get_sticky_event (pad, GST_EVENT_SEGMENT, 0);
  if (segment && !gst_pad_push_event (funnel->srcpad, segment)) {
    GST_ERROR_OBJECT (funnel, "Could not push segment");
    goto done;
  }

  funnel->current_pad = pad;

done:
  return;
}

static GstFlowReturn
gst_rtp_funnel_sink_chain_object (GstPad * pad, GstRtpFunnel * funnel,
    gboolean is_list, GstMiniObject * obj)
{
  GstFlowReturn res;

  GST_DEBUG_OBJECT (pad, "received %" GST_PTR_FORMAT, obj);

  GST_PAD_STREAM_LOCK (funnel->srcpad);

  gst_rtp_funnel_send_sticky (funnel, pad);
  gst_rtp_funnel_forward_segment (funnel, pad);

  if (is_list)
    res = gst_pad_push_list (funnel->srcpad, GST_BUFFER_LIST_CAST (obj));
  else
    res = gst_pad_push (funnel->srcpad, GST_BUFFER_CAST (obj));

  GST_PAD_STREAM_UNLOCK (funnel->srcpad);

  return res;
}

static GstFlowReturn
gst_rtp_funnel_sink_chain_list (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (parent);

  return gst_rtp_funnel_sink_chain_object (pad, funnel, TRUE,
      GST_MINI_OBJECT_CAST (list));
}

static GstFlowReturn
gst_rtp_funnel_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (parent);

  return gst_rtp_funnel_sink_chain_object (pad, funnel, FALSE,
      GST_MINI_OBJECT_CAST (buffer));
}

static gboolean
gst_rtp_funnel_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (parent);
  gboolean forward = TRUE;
  gboolean ret = TRUE;

  GST_DEBUG_OBJECT (pad, "received event %" GST_PTR_FORMAT, event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_STREAM_START:
    case GST_EVENT_SEGMENT:
      forward = FALSE;
      break;
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      GstStructure *s;
      guint ssrc;
      gst_event_parse_caps (event, &caps);

      if (!gst_caps_can_intersect (funnel->srccaps, caps)) {
        GST_ERROR_OBJECT (funnel, "Can't intersect with caps %" GST_PTR_FORMAT,
            caps);
        g_assert_not_reached ();
      }

      s = gst_caps_get_structure (caps, 0);
      if (gst_structure_get_uint (s, "ssrc", &ssrc)) {
        GstRtpFunnelPad *fpad = GST_RTP_FUNNEL_PAD_CAST (pad);
        fpad->ssrc = ssrc;
        GST_DEBUG_OBJECT (pad, "Got ssrc: %u", ssrc);
        GST_OBJECT_LOCK (funnel);
        g_hash_table_insert (funnel->ssrc_to_pad, GUINT_TO_POINTER (ssrc), pad);
        GST_OBJECT_UNLOCK (funnel);
      }

      forward = FALSE;
      break;
    }
    default:
      break;
  }

  if (forward) {
    ret = gst_pad_event_default (pad, parent, event);
  } else {
    gst_event_unref (event);
  }

  return ret;
}

static gboolean
gst_rtp_funnel_sink_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (parent);
  gboolean res = FALSE;
  (void) funnel;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter_caps;
      GstCaps *new_caps;

      gst_query_parse_caps (query, &filter_caps);

      if (filter_caps) {
        new_caps = gst_caps_intersect_full (funnel->srccaps, filter_caps,
            GST_CAPS_INTERSECT_FIRST);
      } else {
        new_caps = gst_caps_copy (funnel->srccaps);
      }

      if (funnel->common_ts_offset >= 0)
        gst_caps_set_simple (new_caps, "timestamp-offset", G_TYPE_UINT,
            (guint) funnel->common_ts_offset, NULL);

      gst_query_set_caps_result (query, new_caps);
      GST_DEBUG_OBJECT (pad, "Answering caps-query with caps: %"
          GST_PTR_FORMAT, new_caps);
      gst_caps_unref (new_caps);
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

static gboolean
gst_rtp_funnel_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (parent);
  gboolean handled = FALSE;
  gboolean ret = TRUE;

  GST_DEBUG_OBJECT (pad, "received event %" GST_PTR_FORMAT, event);

  if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_UPSTREAM) {
    const GstStructure *s = gst_event_get_structure (event);
    GstPad *fpad;
    guint ssrc;
    if (s && gst_structure_get_uint (s, "ssrc", &ssrc)) {
      handled = TRUE;

      GST_OBJECT_LOCK (funnel);
      fpad = g_hash_table_lookup (funnel->ssrc_to_pad, GUINT_TO_POINTER (ssrc));
      if (fpad)
        gst_object_ref (fpad);
      GST_OBJECT_UNLOCK (funnel);

      if (fpad) {
        GST_INFO_OBJECT (pad, "Sending %" GST_PTR_FORMAT " to %" GST_PTR_FORMAT,
            event, fpad);
        ret = gst_pad_push_event (fpad, event);
        gst_object_unref (fpad);
      } else {
        gst_event_unref (event);
      }
    }
  }

  if (!handled) {
    gst_pad_event_default (pad, parent, event);
  }

  return ret;
}

static GstPad *
gst_rtp_funnel_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name, const GstCaps * caps)
{
  GstPad *sinkpad;
  (void) caps;

  GST_DEBUG_OBJECT (element, "requesting pad");

  sinkpad = GST_PAD_CAST (g_object_new (GST_TYPE_RTP_FUNNEL_PAD,
          "name", name, "direction", templ->direction, "template", templ,
          NULL));

  gst_pad_set_chain_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_funnel_sink_chain));
  gst_pad_set_chain_list_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_funnel_sink_chain_list));
  gst_pad_set_event_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_funnel_sink_event));
  gst_pad_set_query_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_funnel_sink_query));

  GST_OBJECT_FLAG_SET (sinkpad, GST_PAD_FLAG_PROXY_CAPS);
  GST_OBJECT_FLAG_SET (sinkpad, GST_PAD_FLAG_PROXY_ALLOCATION);

  gst_pad_set_active (sinkpad, TRUE);

  gst_element_add_pad (element, sinkpad);

  GST_DEBUG_OBJECT (element, "requested pad %s:%s",
      GST_DEBUG_PAD_NAME (sinkpad));

  return sinkpad;
}

static void
gst_rtp_funnel_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (object);

  switch (prop_id) {
    case PROP_COMMON_TS_OFFSET:
      funnel->common_ts_offset = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_funnel_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (object);

  switch (prop_id) {
    case PROP_COMMON_TS_OFFSET:
      g_value_set_int (value, funnel->common_ts_offset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_rtp_funnel_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (element);
  GstStateChangeReturn ret;

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      funnel->send_sticky_events = TRUE;
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
_remove_pad_func (gpointer key, gpointer value, gpointer user_data)
{
  (void) key;
  if (GST_PAD_CAST (value) == GST_PAD_CAST (user_data))
    return TRUE;
  return FALSE;
}

static void
gst_rtp_funnel_release_pad (GstElement * element, GstPad * pad)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (element);

  GST_DEBUG_OBJECT (funnel, "releasing pad %s:%s", GST_DEBUG_PAD_NAME (pad));

  g_hash_table_foreach_remove (funnel->ssrc_to_pad, _remove_pad_func, pad);

  gst_pad_set_active (pad, FALSE);
  gst_element_remove_pad (GST_ELEMENT_CAST (funnel), pad);
}

static void
gst_rtp_funnel_finalize (GObject * object)
{
  GstRtpFunnel *funnel = GST_RTP_FUNNEL_CAST (object);

  gst_caps_unref (funnel->srccaps);
  g_hash_table_destroy (funnel->ssrc_to_pad);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_funnel_class_init (GstRtpFunnelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_rtp_funnel_finalize);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_rtp_funnel_get_property);
  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_rtp_funnel_set_property);
  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_rtp_funnel_request_new_pad);
  gstelement_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_rtp_funnel_release_pad);
  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_funnel_change_state);

  gst_element_class_set_static_metadata (gstelement_class, "RTP funnel",
      "RTP Funneling",
      "Funnel RTP buffers together for multiplexing",
      "Havard Graff <havard@gstip.com>");

  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &src_template);

  g_object_class_install_property (gobject_class, PROP_COMMON_TS_OFFSET,
      g_param_spec_int ("common-ts-offset", "Common Timestamp Offset",
          "Use the same RTP timestamp offset for all sinkpads (-1 = disable)",
          -1, G_MAXINT32, DEFAULT_COMMON_TS_OFFSET,
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (gst_rtp_funnel_debug,
      "gstrtpfunnel", 0, "funnel element");
}

static void
gst_rtp_funnel_init (GstRtpFunnel * funnel)
{
  funnel->srcpad = gst_pad_new_from_static_template (&src_template, "src");
  gst_pad_use_fixed_caps (funnel->srcpad);
  gst_pad_set_event_function (funnel->srcpad,
      GST_DEBUG_FUNCPTR (gst_rtp_funnel_src_event));
  gst_element_add_pad (GST_ELEMENT (funnel), funnel->srcpad);

  funnel->send_sticky_events = TRUE;
  funnel->srccaps = gst_caps_new_empty_simple (RTP_CAPS);
  funnel->ssrc_to_pad = g_hash_table_new (NULL, NULL);
  funnel->current_pad = NULL;
}
