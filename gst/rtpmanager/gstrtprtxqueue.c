/* RTP Retransmission queue element for GStreamer
 *
 * gstrtprtxqueue.c:
 *
 * Copyright (C) 2013 Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:element-rtprtxqueue
 * @title: rtprtxqueue
 *
 * rtprtxqueue maintains a queue of transmitted RTP packets, up to a
 * configurable limit (see #GstRTPRtxQueue:max-size-time,
 * #GstRTPRtxQueue:max-size-packets), and retransmits them upon request
 * from the downstream rtpsession (GstRTPRetransmissionRequest event).
 *
 * This element is similar to rtprtxsend, but it has differences:
 * - Retransmission from rtprtxqueue is not RFC 4588 compliant. The
 * retransmitted packets have the same ssrc and payload type as the original
 * stream.
 * - As a side-effect of the above, rtprtxqueue does not require the use of
 * rtprtxreceive on the receiving end. rtpjitterbuffer alone is able to
 * reconstruct the stream.
 * - Retransmission from rtprtxqueue happens as soon as the next regular flow
 * packet is chained, while rtprtxsend retransmits as soon as the retransmission
 * event is received, using a helper thread.
 * - rtprtxqueue can be used with rtpbin without the need of hooking to its
 * #GstRtpBin::request-aux-sender signal, which means it can be used with
 * rtpbin using gst-launch.
 *
 * See also #GstRtpRtxSend, #GstRtpRtxReceive
 *
 * # Example pipelines
 *
 * |[
 * gst-launch-1.0 rtpbin name=b rtp-profile=avpf \
 *    audiotestsrc is-live=true ! opusenc ! rtpopuspay pt=96 ! rtprtxqueue ! b.send_rtp_sink_0 \
 *    b.send_rtp_src_0 ! identity drop-probability=0.01 ! udpsink host="127.0.0.1" port=5000 \
 *    udpsrc port=5001 ! b.recv_rtcp_sink_0 \
 *    b.send_rtcp_src_0 ! udpsink host="127.0.0.1" port=5002 sync=false async=false
 * ]|
 * Sender pipeline
 *
 * |[
 * gst-launch-1.0 rtpbin name=b rtp-profile=avpf do-retransmission=true \
 *    udpsrc port=5000 caps="application/x-rtp,media=(string)audio,clock-rate=(int)48000,encoding-name=(string)OPUS,payload=(int)96" ! \
 *        b.recv_rtp_sink_0 \
 *    b. ! rtpopusdepay ! opusdec ! audioconvert ! audioresample ! autoaudiosink \
 *    udpsrc port=5002 ! b.recv_rtcp_sink_0 \
 *    b.send_rtcp_src_0 ! udpsink host="127.0.0.1" port=5001 sync=false async=false
 * ]|
 * Receiver pipeline
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <string.h>

#include "gstrtprtxqueue.h"

GST_DEBUG_CATEGORY_STATIC (gst_rtp_rtx_queue_debug);
#define GST_CAT_DEFAULT gst_rtp_rtx_queue_debug

#define DEFAULT_MAX_SIZE_TIME    0
#define DEFAULT_MAX_SIZE_PACKETS 100

enum
{
  PROP_0,
  PROP_MAX_SIZE_TIME,
  PROP_MAX_SIZE_PACKETS,
  PROP_REQUESTS,
  PROP_FULFILLED_REQUESTS,
};

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static gboolean gst_rtp_rtx_queue_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_rtp_rtx_queue_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_rtp_rtx_queue_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);
static GstFlowReturn gst_rtp_rtx_queue_chain_list (GstPad * pad,
    GstObject * parent, GstBufferList * list);

static GstStateChangeReturn gst_rtp_rtx_queue_change_state (GstElement *
    element, GstStateChange transition);

static void gst_rtp_rtx_queue_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_rtx_queue_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_rtp_rtx_queue_finalize (GObject * object);

G_DEFINE_TYPE (GstRTPRtxQueue, gst_rtp_rtx_queue, GST_TYPE_ELEMENT);

static void
gst_rtp_rtx_queue_class_init (GstRTPRtxQueueClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->get_property = gst_rtp_rtx_queue_get_property;
  gobject_class->set_property = gst_rtp_rtx_queue_set_property;
  gobject_class->finalize = gst_rtp_rtx_queue_finalize;

  g_object_class_install_property (gobject_class, PROP_MAX_SIZE_TIME,
      g_param_spec_uint ("max-size-time", "Max Size Times",
          "Amount of ms to queue (0 = unlimited)", 0, G_MAXUINT,
          DEFAULT_MAX_SIZE_TIME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_SIZE_PACKETS,
      g_param_spec_uint ("max-size-packets", "Max Size Packets",
          "Amount of packets to queue (0 = unlimited)", 0, G_MAXUINT,
          DEFAULT_MAX_SIZE_PACKETS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_REQUESTS,
      g_param_spec_uint ("requests", "Requests",
          "Total number of retransmission requests", 0, G_MAXUINT,
          0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FULFILLED_REQUESTS,
      g_param_spec_uint ("fulfilled-requests", "Fulfilled Requests",
          "Number of fulfilled retransmission requests", 0, G_MAXUINT,
          0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_factory);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Retransmission Queue", "Codec",
      "Keep RTP packets in a queue for retransmission",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_queue_change_state);
}

static void
gst_rtp_rtx_queue_reset (GstRTPRtxQueue * rtx, gboolean full)
{
  g_mutex_lock (&rtx->lock);
  g_queue_foreach (rtx->queue, (GFunc) gst_buffer_unref, NULL);
  g_queue_clear (rtx->queue);
  g_list_foreach (rtx->pending, (GFunc) gst_buffer_unref, NULL);
  g_list_free (rtx->pending);
  rtx->pending = NULL;
  rtx->n_requests = 0;
  rtx->n_fulfilled_requests = 0;
  g_mutex_unlock (&rtx->lock);
}

static void
gst_rtp_rtx_queue_finalize (GObject * object)
{
  GstRTPRtxQueue *rtx = GST_RTP_RTX_QUEUE (object);

  gst_rtp_rtx_queue_reset (rtx, TRUE);
  g_queue_free (rtx->queue);
  g_mutex_clear (&rtx->lock);

  G_OBJECT_CLASS (gst_rtp_rtx_queue_parent_class)->finalize (object);
}

static void
gst_rtp_rtx_queue_init (GstRTPRtxQueue * rtx)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (rtx);

  rtx->srcpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "src"), "src");
  GST_PAD_SET_PROXY_CAPS (rtx->srcpad);
  GST_PAD_SET_PROXY_ALLOCATION (rtx->srcpad);
  gst_pad_set_event_function (rtx->srcpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_queue_src_event));
  gst_element_add_pad (GST_ELEMENT (rtx), rtx->srcpad);

  rtx->sinkpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "sink"), "sink");
  GST_PAD_SET_PROXY_CAPS (rtx->sinkpad);
  GST_PAD_SET_PROXY_ALLOCATION (rtx->sinkpad);
  gst_pad_set_event_function (rtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_queue_sink_event));
  gst_pad_set_chain_function (rtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_queue_chain));
  gst_pad_set_chain_list_function (rtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_queue_chain_list));
  gst_element_add_pad (GST_ELEMENT (rtx), rtx->sinkpad);

  rtx->queue = g_queue_new ();
  g_mutex_init (&rtx->lock);

  rtx->max_size_time = DEFAULT_MAX_SIZE_TIME;
  rtx->max_size_packets = DEFAULT_MAX_SIZE_PACKETS;
}

typedef struct
{
  GstRTPRtxQueue *rtx;
  guint seqnum;
  gboolean found;
} RTXData;

static void
push_seqnum (GstBuffer * buffer, RTXData * data)
{
  GstRTPRtxQueue *rtx = data->rtx;
  GstRTPBuffer rtpbuffer = GST_RTP_BUFFER_INIT;
  guint16 seqnum;

  if (data->found)
    return;

  if (!GST_IS_BUFFER (buffer) ||
      !gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtpbuffer))
    return;

  seqnum = gst_rtp_buffer_get_seq (&rtpbuffer);
  gst_rtp_buffer_unmap (&rtpbuffer);

  if (seqnum == data->seqnum) {
    data->found = TRUE;
    GST_DEBUG_OBJECT (rtx, "found %d", seqnum);
    rtx->pending = g_list_prepend (rtx->pending, gst_buffer_ref (buffer));
  }
}

static gboolean
gst_rtp_rtx_queue_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstRTPRtxQueue *rtx = GST_RTP_RTX_QUEUE (parent);
  gboolean res;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
    {
      const GstStructure *s;

      s = gst_event_get_structure (event);
      if (gst_structure_has_name (s, "GstRTPRetransmissionRequest")) {
        guint seqnum;
        RTXData data;

        if (!gst_structure_get_uint (s, "seqnum", &seqnum))
          seqnum = -1;

        GST_DEBUG_OBJECT (rtx, "request %d", seqnum);

        g_mutex_lock (&rtx->lock);
        data.rtx = rtx;
        data.seqnum = seqnum;
        data.found = FALSE;
        rtx->n_requests += 1;
        g_queue_foreach (rtx->queue, (GFunc) push_seqnum, &data);
        g_mutex_unlock (&rtx->lock);

        gst_event_unref (event);
        res = TRUE;
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
gst_rtp_rtx_queue_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRTPRtxQueue *rtx = GST_RTP_RTX_QUEUE (parent);
  gboolean res;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    {
      g_mutex_lock (&rtx->lock);
      gst_event_copy_segment (event, &rtx->head_segment);
      g_queue_push_head (rtx->queue, gst_event_ref (event));
      g_mutex_unlock (&rtx->lock);
      /* fall through */
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }
  return res;
}

static void
do_push (GstBuffer * buffer, GstRTPRtxQueue * rtx)
{
  rtx->n_fulfilled_requests += 1;
  gst_pad_push (rtx->srcpad, buffer);
}

static guint32
get_ts_diff (GstRTPRtxQueue * rtx)
{
  GstClockTime high_ts, low_ts;
  GstClockTimeDiff result;
  GstBuffer *high_buf, *low_buf;

  high_buf = g_queue_peek_head (rtx->queue);

  while (GST_IS_EVENT ((low_buf = g_queue_peek_tail (rtx->queue)))) {
    GstEvent *event = g_queue_pop_tail (rtx->queue);
    gst_event_copy_segment (event, &rtx->tail_segment);
    gst_event_unref (event);
  }

  if (!high_buf || !low_buf || high_buf == low_buf)
    return 0;

  high_ts = GST_BUFFER_TIMESTAMP (high_buf);
  low_ts = GST_BUFFER_TIMESTAMP (low_buf);

  high_ts = gst_segment_to_running_time (&rtx->head_segment, GST_FORMAT_TIME,
      high_ts);
  low_ts = gst_segment_to_running_time (&rtx->tail_segment, GST_FORMAT_TIME,
      low_ts);

  result = high_ts - low_ts;

  /* return value in ms instead of ns */
  return (guint32) gst_util_uint64_scale_int (result, 1, GST_MSECOND);
}

/* Must be called with rtx->lock */
static void
shrink_queue (GstRTPRtxQueue * rtx)
{
  if (rtx->max_size_packets) {
    while (g_queue_get_length (rtx->queue) > rtx->max_size_packets)
      gst_buffer_unref (g_queue_pop_tail (rtx->queue));
  }
  if (rtx->max_size_time) {
    while (get_ts_diff (rtx) > rtx->max_size_time)
      gst_buffer_unref (g_queue_pop_tail (rtx->queue));
  }
}

static GstFlowReturn
gst_rtp_rtx_queue_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstRTPRtxQueue *rtx;
  GstFlowReturn ret;
  GList *pending;

  rtx = GST_RTP_RTX_QUEUE (parent);

  g_mutex_lock (&rtx->lock);
  g_queue_push_head (rtx->queue, gst_buffer_ref (buffer));
  shrink_queue (rtx);

  pending = rtx->pending;
  rtx->pending = NULL;
  g_mutex_unlock (&rtx->lock);

  pending = g_list_reverse (pending);
  g_list_foreach (pending, (GFunc) do_push, rtx);
  g_list_free (pending);

  ret = gst_pad_push (rtx->srcpad, buffer);

  return ret;
}

static gboolean
push_to_queue (GstBuffer ** buffer, guint idx, gpointer user_data)
{
  GQueue *queue = user_data;

  g_queue_push_head (queue, gst_buffer_ref (*buffer));

  return TRUE;
}

static GstFlowReturn
gst_rtp_rtx_queue_chain_list (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstRTPRtxQueue *rtx;
  GstFlowReturn ret;
  GList *pending;

  rtx = GST_RTP_RTX_QUEUE (parent);

  g_mutex_lock (&rtx->lock);
  gst_buffer_list_foreach (list, push_to_queue, rtx->queue);
  shrink_queue (rtx);

  pending = rtx->pending;
  rtx->pending = NULL;
  g_mutex_unlock (&rtx->lock);

  pending = g_list_reverse (pending);
  g_list_foreach (pending, (GFunc) do_push, rtx);
  g_list_free (pending);

  ret = gst_pad_push_list (rtx->srcpad, list);

  return ret;
}

static void
gst_rtp_rtx_queue_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstRTPRtxQueue *rtx = GST_RTP_RTX_QUEUE (object);

  switch (prop_id) {
    case PROP_MAX_SIZE_TIME:
      g_value_set_uint (value, rtx->max_size_time);
      break;
    case PROP_MAX_SIZE_PACKETS:
      g_value_set_uint (value, rtx->max_size_packets);
      break;
    case PROP_REQUESTS:
      g_value_set_uint (value, rtx->n_requests);
      break;
    case PROP_FULFILLED_REQUESTS:
      g_value_set_uint (value, rtx->n_fulfilled_requests);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_rtx_queue_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstRTPRtxQueue *rtx = GST_RTP_RTX_QUEUE (object);

  switch (prop_id) {
    case PROP_MAX_SIZE_TIME:
      rtx->max_size_time = g_value_get_uint (value);
      break;
    case PROP_MAX_SIZE_PACKETS:
      rtx->max_size_packets = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_rtp_rtx_queue_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstRTPRtxQueue *rtx;

  rtx = GST_RTP_RTX_QUEUE (element);

  switch (transition) {
    default:
      break;
  }

  ret =
      GST_ELEMENT_CLASS (gst_rtp_rtx_queue_parent_class)->change_state (element,
      transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_rtx_queue_reset (rtx, TRUE);
      break;
    default:
      break;
  }

  return ret;
}

gboolean
gst_rtp_rtx_queue_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_rtp_rtx_queue_debug, "rtprtxqueue", 0,
      "rtp retransmission queue");

  return gst_element_register (plugin, "rtprtxqueue", GST_RANK_NONE,
      GST_TYPE_RTP_RTX_QUEUE);
}
