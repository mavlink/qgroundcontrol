/*
 * RTP Demux element
 *
 * Copyright (C) 2005 Nokia Corporation.
 * @author Kai Vehmanen <kai.vehmanen@nokia.com>
 *
 * Loosely based on GStreamer gstdecodebin
 * Copyright (C) <2004> Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:element-rtpptdemux
 * @title: rtpptdemux
 *
 * rtpptdemux acts as a demuxer for RTP packets based on the payload type of
 * the packets. Its main purpose is to allow an application to easily receive
 * and decode an RTP stream with multiple payload types.
 *
 * For each payload type that is detected, a new pad will be created and the
 * #GstRtpPtDemux::new-payload-type signal will be emitted. When the payload for
 * the RTP stream changes, the #GstRtpPtDemux::payload-type-change signal will be
 * emitted.
 *
 * The element will try to set complete and unique application/x-rtp caps
 * on the output pads based on the result of the #GstRtpPtDemux::request-pt-map
 * signal.
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 udpsrc caps="application/x-rtp" ! rtpptdemux ! fakesink
 * ]| Takes an RTP stream and send the RTP packets with the first detected
 * payload type to fakesink, discarding the other payload types.
 *
 */

/*
 * Contributors:
 * Andre Moreira Magalhaes <andre.magalhaes@indt.org.br>
 */
/*
 * Status:
 *  - works with the test_rtpdemux.c tool
 *
 * Check:
 *  - is emitting a signal enough, or should we
 *    use GstEvent to notify downstream elements
 *    of the new packet... no?
 *
 * Notes:
 *  - emits event both for new PTs, and whenever
 *    a PT is changed
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gst/gst.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpptdemux.h"

/* generic templates */
static GstStaticPadTemplate rtp_pt_demux_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate rtp_pt_demux_src_template =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtp, " "payload = (int) [ 0, 255 ]")
    );

GST_DEBUG_CATEGORY_STATIC (gst_rtp_pt_demux_debug);
#define GST_CAT_DEFAULT gst_rtp_pt_demux_debug

/*
 * Item for storing GstPad<->pt pairs.
 */
struct _GstRtpPtDemuxPad
{
  GstPad *pad;                  /*< pointer to the actual pad */
  gint pt;                      /*< RTP payload-type attached to pad */
  gboolean newcaps;
};

/* signals */
enum
{
  SIGNAL_REQUEST_PT_MAP,
  SIGNAL_NEW_PAYLOAD_TYPE,
  SIGNAL_PAYLOAD_TYPE_CHANGE,
  SIGNAL_CLEAR_PT_MAP,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IGNORED_PTS,
};

#define gst_rtp_pt_demux_parent_class parent_class
G_DEFINE_TYPE (GstRtpPtDemux, gst_rtp_pt_demux, GST_TYPE_ELEMENT);

static void gst_rtp_pt_demux_finalize (GObject * object);

static void gst_rtp_pt_demux_release (GstRtpPtDemux * ptdemux);
static gboolean gst_rtp_pt_demux_setup (GstRtpPtDemux * ptdemux);

static gboolean gst_rtp_pt_demux_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_rtp_pt_demux_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buf);
static GstStateChangeReturn gst_rtp_pt_demux_change_state (GstElement * element,
    GstStateChange transition);
static void gst_rtp_pt_demux_clear_pt_map (GstRtpPtDemux * rtpdemux);

static GstPad *find_pad_for_pt (GstRtpPtDemux * rtpdemux, guint8 pt);

static gboolean gst_rtp_pt_demux_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);


static guint gst_rtp_pt_demux_signals[LAST_SIGNAL] = { 0 };

static void
gst_rtp_pt_demux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpPtDemux *rtpptdemux = GST_RTP_PT_DEMUX (object);

  switch (prop_id) {
    case PROP_IGNORED_PTS:
      g_value_copy (value, &rtpptdemux->ignored_pts);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_pt_demux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpPtDemux *rtpptdemux = GST_RTP_PT_DEMUX (object);

  switch (prop_id) {
    case PROP_IGNORED_PTS:
      g_value_copy (&rtpptdemux->ignored_pts, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_pt_demux_class_init (GstRtpPtDemuxClass * klass)
{
  GObjectClass *gobject_klass;
  GstElementClass *gstelement_klass;

  gobject_klass = (GObjectClass *) klass;
  gstelement_klass = (GstElementClass *) klass;

  /**
   * GstRtpPtDemux::request-pt-map:
   * @demux: the object which received the signal
   * @pt: the payload type
   *
   * Request the payload type as #GstCaps for @pt.
   */
  gst_rtp_pt_demux_signals[SIGNAL_REQUEST_PT_MAP] =
      g_signal_new ("request-pt-map", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpPtDemuxClass, request_pt_map),
      NULL, NULL, NULL, GST_TYPE_CAPS, 1, G_TYPE_UINT);

  /**
   * GstRtpPtDemux::new-payload-type:
   * @demux: the object which received the signal
   * @pt: the payload type
   * @pad: the pad with the new payload
   *
   * Emitted when a new payload type pad has been created in @demux.
   */
  gst_rtp_pt_demux_signals[SIGNAL_NEW_PAYLOAD_TYPE] =
      g_signal_new ("new-payload-type", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpPtDemuxClass, new_payload_type),
      NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, GST_TYPE_PAD);

  /**
   * GstRtpPtDemux::payload-type-change:
   * @demux: the object which received the signal
   * @pt: the new payload type
   *
   * Emitted when the payload type changed.
   */
  gst_rtp_pt_demux_signals[SIGNAL_PAYLOAD_TYPE_CHANGE] =
      g_signal_new ("payload-type-change", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpPtDemuxClass,
          payload_type_change), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);

  /**
   * GstRtpPtDemux::clear-pt-map:
   * @demux: the object which received the signal
   *
   * The application can call this signal to instruct the element to discard the
   * currently cached payload type map.
   */
  gst_rtp_pt_demux_signals[SIGNAL_CLEAR_PT_MAP] =
      g_signal_new ("clear-pt-map", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_ACTION | G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstRtpPtDemuxClass,
          clear_pt_map), NULL, NULL, NULL, G_TYPE_NONE, 0, G_TYPE_NONE);

  gobject_klass->set_property = gst_rtp_pt_demux_set_property;
  gobject_klass->get_property = gst_rtp_pt_demux_get_property;

  /**
   * GstRtpPtDemux:ignored-payload-types:
   *
   * If specified, packets with an ignored payload type will be dropped,
   * instead of causing a new pad to be exposed for these to be pushed on.
   *
   * This is for example useful to drop FEC protection packets, as they
   * need to go through the #GstRtpJitterBuffer, but cease to be useful
   * past that point, #GstRtpBin will make use of this property for that
   * purpose.
   *
   * Since: 1.14
   */
  g_object_class_install_property (gobject_klass, PROP_IGNORED_PTS,
      gst_param_spec_array ("ignored-payload-types",
          "Ignored payload types",
          "Packets with these payload types will be dropped",
          g_param_spec_int ("payload-types", "payload-types", "Payload types",
              0, G_MAXINT, 0,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gobject_klass->finalize = gst_rtp_pt_demux_finalize;

  gstelement_klass->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_pt_demux_change_state);

  klass->clear_pt_map = GST_DEBUG_FUNCPTR (gst_rtp_pt_demux_clear_pt_map);

  gst_element_class_add_static_pad_template (gstelement_klass,
      &rtp_pt_demux_sink_template);
  gst_element_class_add_static_pad_template (gstelement_klass,
      &rtp_pt_demux_src_template);

  gst_element_class_set_static_metadata (gstelement_klass, "RTP Demux",
      "Demux/Network/RTP",
      "Parses codec streams transmitted in the same RTP session",
      "Kai Vehmanen <kai.vehmanen@nokia.com>");

  GST_DEBUG_CATEGORY_INIT (gst_rtp_pt_demux_debug,
      "rtpptdemux", 0, "RTP codec demuxer");

  GST_DEBUG_REGISTER_FUNCPTR (gst_rtp_pt_demux_chain);
}

static void
gst_rtp_pt_demux_init (GstRtpPtDemux * ptdemux)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (ptdemux);

  ptdemux->sink =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "sink"), "sink");
  g_assert (ptdemux->sink != NULL);

  gst_pad_set_chain_function (ptdemux->sink, gst_rtp_pt_demux_chain);
  gst_pad_set_event_function (ptdemux->sink, gst_rtp_pt_demux_sink_event);

  gst_element_add_pad (GST_ELEMENT (ptdemux), ptdemux->sink);

  g_value_init (&ptdemux->ignored_pts, GST_TYPE_ARRAY);
}

static void
gst_rtp_pt_demux_finalize (GObject * object)
{
  gst_rtp_pt_demux_release (GST_RTP_PT_DEMUX (object));

  g_value_unset (&GST_RTP_PT_DEMUX (object)->ignored_pts);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstCaps *
gst_rtp_pt_demux_get_caps (GstRtpPtDemux * rtpdemux, guint pt)
{
  GstCaps *caps;
  GValue ret = { 0 };
  GValue args[2] = { {0}, {0} };
  GstCaps *sink_caps;

  /* figure out the caps */
  g_value_init (&args[0], GST_TYPE_ELEMENT);
  g_value_set_object (&args[0], rtpdemux);
  g_value_init (&args[1], G_TYPE_UINT);
  g_value_set_uint (&args[1], pt);

  g_value_init (&ret, GST_TYPE_CAPS);
  g_value_set_boxed (&ret, NULL);

  g_signal_emitv (args, gst_rtp_pt_demux_signals[SIGNAL_REQUEST_PT_MAP], 0,
      &ret);

  g_value_unset (&args[0]);
  g_value_unset (&args[1]);
  caps = g_value_dup_boxed (&ret);
  g_value_unset (&ret);

  sink_caps = gst_pad_get_current_caps (rtpdemux->sink);

  if (sink_caps) {
    if (caps == NULL) {
      caps = gst_caps_ref (sink_caps);
    } else {
      GstStructure *s1;
      GstStructure *s2;
      guint ssrc;

      caps = gst_caps_make_writable (caps);
      s1 = gst_caps_get_structure (sink_caps, 0);
      s2 = gst_caps_get_structure (caps, 0);

      gst_structure_get_uint (s1, "ssrc", &ssrc);
      gst_structure_set (s2, "ssrc", G_TYPE_UINT, ssrc, NULL);
    }

    gst_caps_unref (sink_caps);
  }

  GST_DEBUG ("pt %d, got caps %" GST_PTR_FORMAT, pt, caps);

  return caps;
}

static void
gst_rtp_pt_demux_clear_pt_map (GstRtpPtDemux * rtpdemux)
{
  GSList *walk;

  GST_OBJECT_LOCK (rtpdemux);
  GST_DEBUG ("clearing pt map");
  for (walk = rtpdemux->srcpads; walk; walk = g_slist_next (walk)) {
    GstRtpPtDemuxPad *pad = walk->data;

    pad->newcaps = TRUE;
  }
  GST_OBJECT_UNLOCK (rtpdemux);
}

static gboolean
need_caps_for_pt (GstRtpPtDemux * rtpdemux, guint8 pt)
{
  GSList *walk;
  gboolean ret = FALSE;

  GST_OBJECT_LOCK (rtpdemux);
  for (walk = rtpdemux->srcpads; walk; walk = g_slist_next (walk)) {
    GstRtpPtDemuxPad *pad = walk->data;

    if (pad->pt == pt) {
      ret = pad->newcaps;
    }
  }
  GST_OBJECT_UNLOCK (rtpdemux);

  return ret;
}


static void
clear_newcaps_for_pt (GstRtpPtDemux * rtpdemux, guint8 pt)
{
  GSList *walk;

  GST_OBJECT_LOCK (rtpdemux);
  for (walk = rtpdemux->srcpads; walk; walk = g_slist_next (walk)) {
    GstRtpPtDemuxPad *pad = walk->data;

    if (pad->pt == pt) {
      pad->newcaps = FALSE;
      break;
    }
  }
  GST_OBJECT_UNLOCK (rtpdemux);
}

static gboolean
forward_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GstPad *srcpad = GST_PAD_CAST (user_data);

  /* Stream start and caps have already been pushed */
  if (GST_EVENT_TYPE (*event) >= GST_EVENT_SEGMENT)
    gst_pad_push_event (srcpad, gst_event_ref (*event));

  return TRUE;
}

static gboolean
gst_rtp_pt_demux_pt_is_ignored (GstRtpPtDemux * ptdemux, guint8 pt)
{
  gboolean ret = FALSE;
  guint i;

  for (i = 0; i < gst_value_array_get_size (&ptdemux->ignored_pts); i++) {
    const GValue *tmp = gst_value_array_get_value (&ptdemux->ignored_pts, i);

    if (g_value_get_int (tmp) == pt) {
      ret = TRUE;
      break;
    }
  }

  return ret;
}

static GstFlowReturn
gst_rtp_pt_demux_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstRtpPtDemux *rtpdemux;
  guint8 pt;
  GstPad *srcpad;
  GstCaps *caps;
  GstRTPBuffer rtp = { NULL };

  rtpdemux = GST_RTP_PT_DEMUX (parent);

  if (!gst_rtp_buffer_map (buf, GST_MAP_READ, &rtp))
    goto invalid_buffer;

  pt = gst_rtp_buffer_get_payload_type (&rtp);
  gst_rtp_buffer_unmap (&rtp);

  if (gst_rtp_pt_demux_pt_is_ignored (rtpdemux, pt))
    goto ignored;

  GST_DEBUG_OBJECT (rtpdemux, "received buffer for pt %d", pt);

  srcpad = find_pad_for_pt (rtpdemux, pt);
  if (srcpad == NULL) {
    /* new PT, create a src pad */
    GstRtpPtDemuxPad *rtpdemuxpad;
    GstElementClass *klass;
    GstPadTemplate *templ;
    gchar *padname;

    caps = gst_rtp_pt_demux_get_caps (rtpdemux, pt);
    if (!caps)
      goto no_caps;

    if (gst_rtp_pt_demux_pt_is_ignored (rtpdemux, pt))
      goto ignored;

    klass = GST_ELEMENT_GET_CLASS (rtpdemux);
    templ = gst_element_class_get_pad_template (klass, "src_%u");
    padname = g_strdup_printf ("src_%u", pt);
    srcpad = gst_pad_new_from_template (templ, padname);
    gst_pad_use_fixed_caps (srcpad);
    g_free (padname);
    gst_pad_set_event_function (srcpad, gst_rtp_pt_demux_src_event);

    GST_DEBUG ("Adding pt=%d to the list.", pt);
    rtpdemuxpad = g_slice_new0 (GstRtpPtDemuxPad);
    rtpdemuxpad->pt = pt;
    rtpdemuxpad->newcaps = FALSE;
    rtpdemuxpad->pad = srcpad;
    gst_object_ref (srcpad);
    GST_OBJECT_LOCK (rtpdemux);
    rtpdemux->srcpads = g_slist_append (rtpdemux->srcpads, rtpdemuxpad);
    GST_OBJECT_UNLOCK (rtpdemux);

    gst_pad_set_active (srcpad, TRUE);


    /* First push the stream-start event, it must always come first */
    gst_pad_push_event (srcpad,
        gst_pad_get_sticky_event (rtpdemux->sink, GST_EVENT_STREAM_START, 0));

    /* Then caps event is sent */
    caps = gst_caps_make_writable (caps);
    gst_caps_set_simple (caps, "payload", G_TYPE_INT, pt, NULL);
    gst_pad_set_caps (srcpad, caps);
    gst_caps_unref (caps);

    /* First sticky events on sink pad are forwarded to the new src pad */
    gst_pad_sticky_events_foreach (rtpdemux->sink, forward_sticky_events,
        srcpad);

    gst_element_add_pad (GST_ELEMENT_CAST (rtpdemux), srcpad);

    GST_DEBUG ("emitting new-payload-type for pt %d", pt);
    g_signal_emit (G_OBJECT (rtpdemux),
        gst_rtp_pt_demux_signals[SIGNAL_NEW_PAYLOAD_TYPE], 0, pt, srcpad);
  }

  if (pt != rtpdemux->last_pt) {
    gint emit_pt = pt;

    /* our own signal with an extra flag that this is the only pad */
    rtpdemux->last_pt = pt;
    GST_DEBUG ("emitting payload-type-changed for pt %d", emit_pt);
    g_signal_emit (G_OBJECT (rtpdemux),
        gst_rtp_pt_demux_signals[SIGNAL_PAYLOAD_TYPE_CHANGE], 0, emit_pt);
  }

  while (need_caps_for_pt (rtpdemux, pt)) {
    GST_DEBUG ("need new caps for %d", pt);
    caps = gst_rtp_pt_demux_get_caps (rtpdemux, pt);
    if (!caps)
      goto no_caps;

    clear_newcaps_for_pt (rtpdemux, pt);

    caps = gst_caps_make_writable (caps);
    gst_caps_set_simple (caps, "payload", G_TYPE_INT, pt, NULL);
    gst_pad_set_caps (srcpad, caps);
    gst_caps_unref (caps);
  }

  /* push to srcpad */
  ret = gst_pad_push (srcpad, buf);

  gst_object_unref (srcpad);

  return ret;

ignored:
  {
    GST_DEBUG_OBJECT (rtpdemux, "Dropped buffer for pt %d", pt);
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }

  /* ERRORS */
invalid_buffer:
  {
    /* this should not be fatal */
    GST_ELEMENT_WARNING (rtpdemux, STREAM, DEMUX, (NULL),
        ("Dropping invalid RTP payload"));
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
no_caps:
  {
    GST_ELEMENT_ERROR (rtpdemux, STREAM, DECODE, (NULL),
        ("Could not get caps for payload"));
    gst_buffer_unref (buf);
    if (srcpad)
      gst_object_unref (srcpad);
    return GST_FLOW_ERROR;
  }
}

static GstPad *
find_pad_for_pt (GstRtpPtDemux * rtpdemux, guint8 pt)
{
  GstPad *respad = NULL;
  GSList *walk;

  GST_OBJECT_LOCK (rtpdemux);
  for (walk = rtpdemux->srcpads; walk; walk = g_slist_next (walk)) {
    GstRtpPtDemuxPad *pad = walk->data;

    if (pad->pt == pt) {
      respad = gst_object_ref (pad->pad);
      break;
    }
  }
  GST_OBJECT_UNLOCK (rtpdemux);

  return respad;
}

static gboolean
gst_rtp_pt_demux_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstRtpPtDemux *rtpdemux;
  gboolean res = FALSE;

  rtpdemux = GST_RTP_PT_DEMUX (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      gst_rtp_pt_demux_clear_pt_map (rtpdemux);
      /* don't forward the event, we cleared the ptmap and on the next buffer we
       * will add the pt to the caps and push a new caps event */
      gst_event_unref (event);
      res = TRUE;
      break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM:
    {
      const GstStructure *s;

      s = gst_event_get_structure (event);

      if (gst_structure_has_name (s, "GstRTPPacketLost")) {
        GstPad *srcpad = find_pad_for_pt (rtpdemux, rtpdemux->last_pt);

        if (srcpad) {
          res = gst_pad_push_event (srcpad, event);
          gst_object_unref (srcpad);
        } else {
          gst_event_unref (event);
        }

      } else {
        res = gst_pad_event_default (pad, parent, event);
      }
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}


static gboolean
gst_rtp_pt_demux_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstRtpPtDemux *demux;
  const GstStructure *s;

  demux = GST_RTP_PT_DEMUX (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
    case GST_EVENT_CUSTOM_BOTH:
    case GST_EVENT_CUSTOM_BOTH_OOB:
      s = gst_event_get_structure (event);
      if (s && !gst_structure_has_field (s, "payload")) {
        GSList *walk;

        GST_OBJECT_LOCK (demux);
        for (walk = demux->srcpads; walk; walk = g_slist_next (walk)) {
          GstRtpPtDemuxPad *dpad = (GstRtpPtDemuxPad *) walk->data;

          if (dpad->pad == pad) {
            GstStructure *ws;

            event =
                GST_EVENT_CAST (gst_mini_object_make_writable
                (GST_MINI_OBJECT_CAST (event)));
            ws = gst_event_writable_structure (event);
            gst_structure_set (ws, "payload", G_TYPE_UINT, dpad->pt, NULL);
            break;
          }
        }
        GST_OBJECT_UNLOCK (demux);
      }
      break;
    default:
      break;
  }

  return gst_pad_event_default (pad, parent, event);
}

/*
 * Reserves resources for the object.
 */
static gboolean
gst_rtp_pt_demux_setup (GstRtpPtDemux * ptdemux)
{
  ptdemux->srcpads = NULL;
  ptdemux->last_pt = 0xFFFF;

  return TRUE;
}

/*
 * Free resources for the object.
 */
static void
gst_rtp_pt_demux_release (GstRtpPtDemux * ptdemux)
{
  GSList *tmppads;
  GSList *walk;

  GST_OBJECT_LOCK (ptdemux);
  tmppads = ptdemux->srcpads;
  ptdemux->srcpads = NULL;
  GST_OBJECT_UNLOCK (ptdemux);

  for (walk = tmppads; walk; walk = g_slist_next (walk)) {
    GstRtpPtDemuxPad *pad = walk->data;

    gst_pad_set_active (pad->pad, FALSE);
    gst_element_remove_pad (GST_ELEMENT_CAST (ptdemux), pad->pad);
    g_slice_free (GstRtpPtDemuxPad, pad);
  }
  g_slist_free (tmppads);
}

static GstStateChangeReturn
gst_rtp_pt_demux_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstRtpPtDemux *ptdemux;

  ptdemux = GST_RTP_PT_DEMUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (gst_rtp_pt_demux_setup (ptdemux) != TRUE)
        ret = GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_rtp_pt_demux_release (ptdemux);
      break;
    default:
      break;
  }

  return ret;
}
