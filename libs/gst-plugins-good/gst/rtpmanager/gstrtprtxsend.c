/* RTP Retransmission sender element for GStreamer
 *
 * gstrtprtxsend.c:
 *
 * Copyright (C) 2013 Collabora Ltd.
 *   @author Julien Isorce <julien.isorce@collabora.co.uk>
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
 * SECTION:element-rtprtxsend
 * @title: rtprtxsend
 *
 * See #GstRtpRtxReceive for examples
 *
 * The purpose of the sender RTX object is to keep a history of RTP packets up
 * to a configurable limit (max-size-time or max-size-packets). It will listen
 * for upstream custom retransmission events (GstRTPRetransmissionRequest) that
 * comes from downstream (#GstRtpSession). When receiving a request it will
 * look up the requested seqnum in its list of stored packets. If the packet
 * is available, it will create a RTX packet according to RFC 4588 and send
 * this as an auxiliary stream. RTX is SSRC-multiplexed
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <string.h>
#include <stdlib.h>

#include "gstrtprtxsend.h"

GST_DEBUG_CATEGORY_STATIC (gst_rtp_rtx_send_debug);
#define GST_CAT_DEFAULT gst_rtp_rtx_send_debug

#define DEFAULT_RTX_PAYLOAD_TYPE 0
#define DEFAULT_MAX_SIZE_TIME    0
#define DEFAULT_MAX_SIZE_PACKETS 100

enum
{
  PROP_0,
  PROP_SSRC_MAP,
  PROP_PAYLOAD_TYPE_MAP,
  PROP_MAX_SIZE_TIME,
  PROP_MAX_SIZE_PACKETS,
  PROP_NUM_RTX_REQUESTS,
  PROP_NUM_RTX_PACKETS
};

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, " "clock-rate = (int) [1, MAX]")
    );

static gboolean gst_rtp_rtx_send_queue_check_full (GstDataQueue * queue,
    guint visible, guint bytes, guint64 time, gpointer checkdata);

static gboolean gst_rtp_rtx_send_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_rtp_rtx_send_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_rtp_rtx_send_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);
static GstFlowReturn gst_rtp_rtx_send_chain_list (GstPad * pad,
    GstObject * parent, GstBufferList * list);

static void gst_rtp_rtx_send_src_loop (GstRtpRtxSend * rtx);
static gboolean gst_rtp_rtx_send_activate_mode (GstPad * pad,
    GstObject * parent, GstPadMode mode, gboolean active);

static GstStateChangeReturn gst_rtp_rtx_send_change_state (GstElement *
    element, GstStateChange transition);

static void gst_rtp_rtx_send_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_rtx_send_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_rtp_rtx_send_finalize (GObject * object);

G_DEFINE_TYPE (GstRtpRtxSend, gst_rtp_rtx_send, GST_TYPE_ELEMENT);

typedef struct
{
  guint16 seqnum;
  guint32 timestamp;
  GstBuffer *buffer;
} BufferQueueItem;

static void
buffer_queue_item_free (BufferQueueItem * item)
{
  gst_buffer_unref (item->buffer);
  g_slice_free (BufferQueueItem, item);
}

typedef struct
{
  guint32 rtx_ssrc;
  guint16 seqnum_base, next_seqnum;
  gint clock_rate;

  /* history of rtp packets */
  GSequence *queue;
} SSRCRtxData;

static SSRCRtxData *
ssrc_rtx_data_new (guint32 rtx_ssrc)
{
  SSRCRtxData *data = g_slice_new0 (SSRCRtxData);

  data->rtx_ssrc = rtx_ssrc;
  data->next_seqnum = data->seqnum_base = g_random_int_range (0, G_MAXUINT16);
  data->queue = g_sequence_new ((GDestroyNotify) buffer_queue_item_free);

  return data;
}

static void
ssrc_rtx_data_free (SSRCRtxData * data)
{
  g_sequence_free (data->queue);
  g_slice_free (SSRCRtxData, data);
}

static void
gst_rtp_rtx_send_class_init (GstRtpRtxSendClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->get_property = gst_rtp_rtx_send_get_property;
  gobject_class->set_property = gst_rtp_rtx_send_set_property;
  gobject_class->finalize = gst_rtp_rtx_send_finalize;

  g_object_class_install_property (gobject_class, PROP_SSRC_MAP,
      g_param_spec_boxed ("ssrc-map", "SSRC Map",
          "Map of SSRCs to their retransmission SSRCs for SSRC-multiplexed mode"
          " (default = random)", GST_TYPE_STRUCTURE,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PAYLOAD_TYPE_MAP,
      g_param_spec_boxed ("payload-type-map", "Payload Type Map",
          "Map of original payload types to their retransmission payload types",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_SIZE_TIME,
      g_param_spec_uint ("max-size-time", "Max Size Time",
          "Amount of ms to queue (0 = unlimited)", 0, G_MAXUINT,
          DEFAULT_MAX_SIZE_TIME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_SIZE_PACKETS,
      g_param_spec_uint ("max-size-packets", "Max Size Packets",
          "Amount of packets to queue (0 = unlimited)", 0, G_MAXINT16,
          DEFAULT_MAX_SIZE_PACKETS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NUM_RTX_REQUESTS,
      g_param_spec_uint ("num-rtx-requests", "Num RTX Requests",
          "Number of retransmission events received", 0, G_MAXUINT,
          0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NUM_RTX_PACKETS,
      g_param_spec_uint ("num-rtx-packets", "Num RTX Packets",
          " Number of retransmission packets sent", 0, G_MAXUINT,
          0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_factory);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Retransmission Sender", "Codec",
      "Retransmit RTP packets when needed, according to RFC4588",
      "Julien Isorce <julien.isorce@collabora.co.uk>");

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_send_change_state);
}

static void
gst_rtp_rtx_send_reset (GstRtpRtxSend * rtx)
{
  GST_OBJECT_LOCK (rtx);
  gst_data_queue_flush (rtx->queue);
  g_hash_table_remove_all (rtx->ssrc_data);
  g_hash_table_remove_all (rtx->rtx_ssrcs);
  rtx->num_rtx_requests = 0;
  rtx->num_rtx_packets = 0;
  GST_OBJECT_UNLOCK (rtx);
}

static void
gst_rtp_rtx_send_finalize (GObject * object)
{
  GstRtpRtxSend *rtx = GST_RTP_RTX_SEND (object);

  g_hash_table_unref (rtx->ssrc_data);
  g_hash_table_unref (rtx->rtx_ssrcs);
  if (rtx->external_ssrc_map)
    gst_structure_free (rtx->external_ssrc_map);
  g_hash_table_unref (rtx->rtx_pt_map);
  if (rtx->rtx_pt_map_structure)
    gst_structure_free (rtx->rtx_pt_map_structure);
  g_object_unref (rtx->queue);

  G_OBJECT_CLASS (gst_rtp_rtx_send_parent_class)->finalize (object);
}

static void
gst_rtp_rtx_send_init (GstRtpRtxSend * rtx)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (rtx);

  rtx->srcpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "src"), "src");
  GST_PAD_SET_PROXY_CAPS (rtx->srcpad);
  GST_PAD_SET_PROXY_ALLOCATION (rtx->srcpad);
  gst_pad_set_event_function (rtx->srcpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_send_src_event));
  gst_pad_set_activatemode_function (rtx->srcpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_send_activate_mode));
  gst_element_add_pad (GST_ELEMENT (rtx), rtx->srcpad);

  rtx->sinkpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "sink"), "sink");
  GST_PAD_SET_PROXY_CAPS (rtx->sinkpad);
  GST_PAD_SET_PROXY_ALLOCATION (rtx->sinkpad);
  gst_pad_set_event_function (rtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_send_sink_event));
  gst_pad_set_chain_function (rtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_send_chain));
  gst_pad_set_chain_list_function (rtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_send_chain_list));
  gst_element_add_pad (GST_ELEMENT (rtx), rtx->sinkpad);

  rtx->queue = gst_data_queue_new (gst_rtp_rtx_send_queue_check_full, NULL,
      NULL, rtx);
  rtx->ssrc_data = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) ssrc_rtx_data_free);
  rtx->rtx_ssrcs = g_hash_table_new (g_direct_hash, g_direct_equal);
  rtx->rtx_pt_map = g_hash_table_new (g_direct_hash, g_direct_equal);

  rtx->max_size_time = DEFAULT_MAX_SIZE_TIME;
  rtx->max_size_packets = DEFAULT_MAX_SIZE_PACKETS;
}

static void
gst_rtp_rtx_send_set_flushing (GstRtpRtxSend * rtx, gboolean flush)
{
  GST_OBJECT_LOCK (rtx);
  gst_data_queue_set_flushing (rtx->queue, flush);
  gst_data_queue_flush (rtx->queue);
  GST_OBJECT_UNLOCK (rtx);
}

static gboolean
gst_rtp_rtx_send_queue_check_full (GstDataQueue * queue,
    guint visible, guint bytes, guint64 time, gpointer checkdata)
{
  return FALSE;
}

static void
gst_rtp_rtx_data_queue_item_free (gpointer item)
{
  GstDataQueueItem *data = item;
  if (data->object)
    gst_mini_object_unref (data->object);
  g_slice_free (GstDataQueueItem, data);
}

static gboolean
gst_rtp_rtx_send_push_out (GstRtpRtxSend * rtx, gpointer object)
{
  GstDataQueueItem *data;
  gboolean success;

  data = g_slice_new0 (GstDataQueueItem);
  data->object = GST_MINI_OBJECT (object);
  data->size = 1;
  data->duration = 1;
  data->visible = TRUE;
  data->destroy = gst_rtp_rtx_data_queue_item_free;

  success = gst_data_queue_push (rtx->queue, data);

  if (!success)
    data->destroy (data);

  return success;
}

static guint32
gst_rtp_rtx_send_choose_ssrc (GstRtpRtxSend * rtx, guint32 choice,
    gboolean consider_choice)
{
  guint32 ssrc = consider_choice ? choice : g_random_int ();

  /* make sure to be different than any other */
  while (g_hash_table_contains (rtx->ssrc_data, GUINT_TO_POINTER (ssrc)) ||
      g_hash_table_contains (rtx->rtx_ssrcs, GUINT_TO_POINTER (ssrc))) {
    ssrc = g_random_int ();
  }

  return ssrc;
}

static SSRCRtxData *
gst_rtp_rtx_send_get_ssrc_data (GstRtpRtxSend * rtx, guint32 ssrc)
{
  SSRCRtxData *data;
  guint32 rtx_ssrc = 0;
  gboolean consider = FALSE;

  if (G_UNLIKELY (!g_hash_table_contains (rtx->ssrc_data,
              GUINT_TO_POINTER (ssrc)))) {
    if (rtx->external_ssrc_map) {
      gchar *ssrc_str;
      ssrc_str = g_strdup_printf ("%" G_GUINT32_FORMAT, ssrc);
      consider = gst_structure_get_uint (rtx->external_ssrc_map, ssrc_str,
          &rtx_ssrc);
      g_free (ssrc_str);
    }
    rtx_ssrc = gst_rtp_rtx_send_choose_ssrc (rtx, rtx_ssrc, consider);
    data = ssrc_rtx_data_new (rtx_ssrc);
    g_hash_table_insert (rtx->ssrc_data, GUINT_TO_POINTER (ssrc), data);
    g_hash_table_insert (rtx->rtx_ssrcs, GUINT_TO_POINTER (rtx_ssrc),
        GUINT_TO_POINTER (ssrc));
  } else {
    data = g_hash_table_lookup (rtx->ssrc_data, GUINT_TO_POINTER (ssrc));
  }
  return data;
}

/* Copy fixed header and extension. Add OSN before to copy payload
 * Copy memory to avoid to manually copy each rtp buffer field.
 */
static GstBuffer *
gst_rtp_rtx_buffer_new (GstRtpRtxSend * rtx, GstBuffer * buffer)
{
  GstMemory *mem = NULL;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstRTPBuffer new_rtp = GST_RTP_BUFFER_INIT;
  GstBuffer *new_buffer = gst_buffer_new ();
  GstMapInfo map;
  guint payload_len = 0;
  SSRCRtxData *data;
  guint32 ssrc;
  guint16 seqnum;
  guint8 fmtp;

  gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp);

  /* get needed data from GstRtpRtxSend */
  ssrc = gst_rtp_buffer_get_ssrc (&rtp);
  data = gst_rtp_rtx_send_get_ssrc_data (rtx, ssrc);
  ssrc = data->rtx_ssrc;
  seqnum = data->next_seqnum++;
  fmtp = GPOINTER_TO_UINT (g_hash_table_lookup (rtx->rtx_pt_map,
          GUINT_TO_POINTER (gst_rtp_buffer_get_payload_type (&rtp))));

  GST_DEBUG_OBJECT (rtx, "creating rtx buffer, orig seqnum: %u, "
      "rtx seqnum: %u, rtx ssrc: %X", gst_rtp_buffer_get_seq (&rtp),
      seqnum, ssrc);

  /* gst_rtp_buffer_map does not map the payload so do it now */
  gst_rtp_buffer_get_payload (&rtp);

  /* copy fixed header */
  mem = gst_memory_copy (rtp.map[0].memory, 0, rtp.size[0]);
  gst_buffer_append_memory (new_buffer, mem);

  /* copy extension if any */
  if (rtp.size[1]) {
    mem = gst_allocator_alloc (NULL, rtp.size[1], NULL);
    gst_memory_map (mem, &map, GST_MAP_WRITE);
    memcpy (map.data, rtp.data[1], rtp.size[1]);
    gst_memory_unmap (mem, &map);
    gst_buffer_append_memory (new_buffer, mem);
  }

  /* copy payload and add OSN just before */
  payload_len = 2 + rtp.size[2];
  mem = gst_allocator_alloc (NULL, payload_len, NULL);

  gst_memory_map (mem, &map, GST_MAP_WRITE);
  GST_WRITE_UINT16_BE (map.data, gst_rtp_buffer_get_seq (&rtp));
  if (rtp.size[2])
    memcpy (map.data + 2, rtp.data[2], rtp.size[2]);
  gst_memory_unmap (mem, &map);
  gst_buffer_append_memory (new_buffer, mem);

  /* everything needed is copied */
  gst_rtp_buffer_unmap (&rtp);

  /* set ssrc, seqnum and fmtp */
  gst_rtp_buffer_map (new_buffer, GST_MAP_WRITE, &new_rtp);
  gst_rtp_buffer_set_ssrc (&new_rtp, ssrc);
  gst_rtp_buffer_set_seq (&new_rtp, seqnum);
  gst_rtp_buffer_set_payload_type (&new_rtp, fmtp);
  /* RFC 4588: let other elements do the padding, as normal */
  gst_rtp_buffer_set_padding (&new_rtp, FALSE);
  gst_rtp_buffer_unmap (&new_rtp);

  /* Copy over timestamps */
  gst_buffer_copy_into (new_buffer, buffer, GST_BUFFER_COPY_TIMESTAMPS, 0, -1);

  return new_buffer;
}

static gint
buffer_queue_items_cmp (BufferQueueItem * a, BufferQueueItem * b,
    gpointer user_data)
{
  /* gst_rtp_buffer_compare_seqnum returns the opposite of what we want,
   * it returns negative when seqnum1 > seqnum2 and we want negative
   * when b > a, i.e. a is smaller, so it comes first in the sequence */
  return gst_rtp_buffer_compare_seqnum (b->seqnum, a->seqnum);
}

static gboolean
gst_rtp_rtx_send_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstRtpRtxSend *rtx = GST_RTP_RTX_SEND (parent);
  gboolean res;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
    {
      const GstStructure *s = gst_event_get_structure (event);

      /* This event usually comes from the downstream gstrtpsession */
      if (gst_structure_has_name (s, "GstRTPRetransmissionRequest")) {
        guint seqnum = 0;
        guint ssrc = 0;
        GstBuffer *rtx_buf = NULL;

        /* retrieve seqnum of the packet that need to be retransmitted */
        if (!gst_structure_get_uint (s, "seqnum", &seqnum))
          seqnum = -1;

        /* retrieve ssrc of the packet that need to be retransmitted */
        if (!gst_structure_get_uint (s, "ssrc", &ssrc))
          ssrc = -1;

        GST_DEBUG_OBJECT (rtx, "got rtx request for seqnum: %u, ssrc: %X",
            seqnum, ssrc);

        GST_OBJECT_LOCK (rtx);
        /* check if request is for us */
        if (g_hash_table_contains (rtx->ssrc_data, GUINT_TO_POINTER (ssrc))) {
          SSRCRtxData *data;
          GSequenceIter *iter;
          BufferQueueItem search_item;

          /* update statistics */
          ++rtx->num_rtx_requests;

          data = gst_rtp_rtx_send_get_ssrc_data (rtx, ssrc);

          search_item.seqnum = seqnum;
          iter = g_sequence_lookup (data->queue, &search_item,
              (GCompareDataFunc) buffer_queue_items_cmp, NULL);
          if (iter) {
            BufferQueueItem *item = g_sequence_get (iter);
            GST_LOG_OBJECT (rtx, "found %u", item->seqnum);
            rtx_buf = gst_rtp_rtx_buffer_new (rtx, item->buffer);
          }
#ifndef GST_DISABLE_DEBUG
          else {
            BufferQueueItem *item = NULL;

            iter = g_sequence_get_begin_iter (data->queue);
            if (!g_sequence_iter_is_end (iter))
              item = g_sequence_get (iter);

            if (item && seqnum < item->seqnum) {
              GST_DEBUG_OBJECT (rtx, "requested seqnum %u has already been "
                  "removed from the rtx queue; the first available is %u",
                  seqnum, item->seqnum);
            } else {
              GST_WARNING_OBJECT (rtx, "requested seqnum %u has not been "
                  "transmitted yet in the original stream; either the remote end "
                  "is not configured correctly, or the source is too slow",
                  seqnum);
            }
          }
#endif
        }
        GST_OBJECT_UNLOCK (rtx);

        if (rtx_buf)
          gst_rtp_rtx_send_push_out (rtx, rtx_buf);

        gst_event_unref (event);
        res = TRUE;

        /* This event usually comes from the downstream gstrtpsession */
      } else if (gst_structure_has_name (s, "GstRTPCollision")) {
        guint ssrc = 0;

        if (!gst_structure_get_uint (s, "ssrc", &ssrc))
          ssrc = -1;

        GST_DEBUG_OBJECT (rtx, "got ssrc collision, ssrc: %X", ssrc);

        GST_OBJECT_LOCK (rtx);

        /* choose another ssrc for our retransmitted stream */
        if (g_hash_table_contains (rtx->rtx_ssrcs, GUINT_TO_POINTER (ssrc))) {
          guint master_ssrc;
          SSRCRtxData *data;

          master_ssrc = GPOINTER_TO_UINT (g_hash_table_lookup (rtx->rtx_ssrcs,
                  GUINT_TO_POINTER (ssrc)));
          data = gst_rtp_rtx_send_get_ssrc_data (rtx, master_ssrc);

          /* change rtx_ssrc and update the reverse map */
          data->rtx_ssrc = gst_rtp_rtx_send_choose_ssrc (rtx, 0, FALSE);
          g_hash_table_remove (rtx->rtx_ssrcs, GUINT_TO_POINTER (ssrc));
          g_hash_table_insert (rtx->rtx_ssrcs,
              GUINT_TO_POINTER (data->rtx_ssrc),
              GUINT_TO_POINTER (master_ssrc));

          GST_OBJECT_UNLOCK (rtx);

          /* no need to forward to payloader because we make sure to have
           * a different ssrc
           */
          gst_event_unref (event);
          res = TRUE;
        } else {
          /* if master ssrc has collided, remove it from our data, as it
           * is not going to be used any longer */
          if (g_hash_table_contains (rtx->ssrc_data, GUINT_TO_POINTER (ssrc))) {
            SSRCRtxData *data;
            data = gst_rtp_rtx_send_get_ssrc_data (rtx, ssrc);
            g_hash_table_remove (rtx->rtx_ssrcs,
                GUINT_TO_POINTER (data->rtx_ssrc));
            g_hash_table_remove (rtx->ssrc_data, GUINT_TO_POINTER (ssrc));
          }

          GST_OBJECT_UNLOCK (rtx);

          /* forward event to payloader in case collided ssrc is
           * master stream */
          res = gst_pad_event_default (pad, parent, event);
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
gst_rtp_rtx_send_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstRtpRtxSend *rtx = GST_RTP_RTX_SEND (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
      gst_pad_push_event (rtx->srcpad, event);
      gst_rtp_rtx_send_set_flushing (rtx, TRUE);
      gst_pad_pause_task (rtx->srcpad);
      return TRUE;
    case GST_EVENT_FLUSH_STOP:
      gst_pad_push_event (rtx->srcpad, event);
      gst_rtp_rtx_send_set_flushing (rtx, FALSE);
      gst_pad_start_task (rtx->srcpad,
          (GstTaskFunction) gst_rtp_rtx_send_src_loop, rtx, NULL);
      return TRUE;
    case GST_EVENT_EOS:
      GST_INFO_OBJECT (rtx, "Got EOS - enqueueing it");
      gst_rtp_rtx_send_push_out (rtx, event);
      return TRUE;
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      GstStructure *s;
      guint ssrc;
      gint payload;
      gpointer rtx_payload;
      SSRCRtxData *data;

      gst_event_parse_caps (event, &caps);

      s = gst_caps_get_structure (caps, 0);
      if (!gst_structure_get_uint (s, "ssrc", &ssrc))
        ssrc = -1;
      if (!gst_structure_get_int (s, "payload", &payload))
        payload = -1;

      if (payload == -1)
        GST_WARNING_OBJECT (rtx, "No payload in caps");

      GST_OBJECT_LOCK (rtx);
      data = gst_rtp_rtx_send_get_ssrc_data (rtx, ssrc);
      if (!g_hash_table_lookup_extended (rtx->rtx_pt_map,
              GUINT_TO_POINTER (payload), NULL, &rtx_payload))
        rtx_payload = GINT_TO_POINTER (-1);

      if (GPOINTER_TO_INT (rtx_payload) == -1 && payload != -1)
        GST_WARNING_OBJECT (rtx, "Payload %d not in rtx-pt-map", payload);

      GST_DEBUG_OBJECT (rtx,
          "got caps for payload: %d->%d, ssrc: %u->%u : %" GST_PTR_FORMAT,
          payload, GPOINTER_TO_INT (rtx_payload), ssrc, data->rtx_ssrc, caps);

      gst_structure_get_int (s, "clock-rate", &data->clock_rate);

      /* The session might need to know the RTX ssrc */
      caps = gst_caps_copy (caps);
      gst_caps_set_simple (caps, "rtx-ssrc", G_TYPE_UINT, data->rtx_ssrc,
          "rtx-seqnum-offset", G_TYPE_UINT, data->seqnum_base, NULL);

      if (GPOINTER_TO_INT (rtx_payload) != -1)
        gst_caps_set_simple (caps, "rtx-payload", G_TYPE_INT,
            GPOINTER_TO_INT (rtx_payload), NULL);

      GST_DEBUG_OBJECT (rtx, "got clock-rate from caps: %d for ssrc: %u",
          data->clock_rate, ssrc);
      GST_OBJECT_UNLOCK (rtx);

      gst_event_unref (event);
      event = gst_event_new_caps (caps);
      gst_caps_unref (caps);
      break;
    }
    default:
      break;
  }
  return gst_pad_event_default (pad, parent, event);
}

/* like rtp_jitter_buffer_get_ts_diff() */
static guint32
gst_rtp_rtx_send_get_ts_diff (SSRCRtxData * data)
{
  guint64 high_ts, low_ts;
  BufferQueueItem *high_buf, *low_buf;
  guint32 result;

  high_buf =
      g_sequence_get (g_sequence_iter_prev (g_sequence_get_end_iter
          (data->queue)));
  low_buf = g_sequence_get (g_sequence_get_begin_iter (data->queue));

  if (!high_buf || !low_buf || high_buf == low_buf)
    return 0;

  high_ts = high_buf->timestamp;
  low_ts = low_buf->timestamp;

  /* it needs to work if ts wraps */
  if (high_ts >= low_ts) {
    result = (guint32) (high_ts - low_ts);
  } else {
    result = (guint32) (high_ts + G_MAXUINT32 + 1 - low_ts);
  }

  /* return value in ms instead of clock ticks */
  return (guint32) gst_util_uint64_scale_int (result, 1000, data->clock_rate);
}

/* Must be called with lock */
static void
process_buffer (GstRtpRtxSend * rtx, GstBuffer * buffer)
{
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  BufferQueueItem *item;
  SSRCRtxData *data;
  guint16 seqnum;
  guint8 payload_type;
  guint32 ssrc, rtptime;

  /* read the information we want from the buffer */
  gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp);
  seqnum = gst_rtp_buffer_get_seq (&rtp);
  payload_type = gst_rtp_buffer_get_payload_type (&rtp);
  ssrc = gst_rtp_buffer_get_ssrc (&rtp);
  rtptime = gst_rtp_buffer_get_timestamp (&rtp);
  gst_rtp_buffer_unmap (&rtp);

  GST_TRACE_OBJECT (rtx, "Processing buffer seqnum: %u, ssrc: %X", seqnum,
      ssrc);

  /* do not store the buffer if it's payload type is unknown */
  if (g_hash_table_contains (rtx->rtx_pt_map, GUINT_TO_POINTER (payload_type))) {
    data = gst_rtp_rtx_send_get_ssrc_data (rtx, ssrc);

    /* add current rtp buffer to queue history */
    item = g_slice_new0 (BufferQueueItem);
    item->seqnum = seqnum;
    item->timestamp = rtptime;
    item->buffer = gst_buffer_ref (buffer);
    g_sequence_append (data->queue, item);

    /* remove oldest packets from history if they are too many */
    if (rtx->max_size_packets) {
      while (g_sequence_get_length (data->queue) > rtx->max_size_packets)
        g_sequence_remove (g_sequence_get_begin_iter (data->queue));
    }
    if (rtx->max_size_time) {
      while (gst_rtp_rtx_send_get_ts_diff (data) > rtx->max_size_time)
        g_sequence_remove (g_sequence_get_begin_iter (data->queue));
    }
  }
}

static GstFlowReturn
gst_rtp_rtx_send_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstRtpRtxSend *rtx = GST_RTP_RTX_SEND (parent);
  GstFlowReturn ret;

  GST_OBJECT_LOCK (rtx);
  process_buffer (rtx, buffer);
  GST_OBJECT_UNLOCK (rtx);
  ret = gst_pad_push (rtx->srcpad, buffer);

  return ret;
}

static gboolean
process_buffer_from_list (GstBuffer ** buffer, guint idx, gpointer user_data)
{
  process_buffer (user_data, *buffer);
  return TRUE;
}

static GstFlowReturn
gst_rtp_rtx_send_chain_list (GstPad * pad, GstObject * parent,
    GstBufferList * list)
{
  GstRtpRtxSend *rtx = GST_RTP_RTX_SEND (parent);
  GstFlowReturn ret;

  GST_OBJECT_LOCK (rtx);
  gst_buffer_list_foreach (list, process_buffer_from_list, rtx);
  GST_OBJECT_UNLOCK (rtx);

  ret = gst_pad_push_list (rtx->srcpad, list);

  return ret;
}

static void
gst_rtp_rtx_send_src_loop (GstRtpRtxSend * rtx)
{
  GstDataQueueItem *data;

  if (gst_data_queue_pop (rtx->queue, &data)) {
    GST_LOG_OBJECT (rtx, "pushing rtx buffer %p", data->object);

    if (G_LIKELY (GST_IS_BUFFER (data->object))) {
      GST_OBJECT_LOCK (rtx);
      /* Update statistics just before pushing. */
      rtx->num_rtx_packets++;
      GST_OBJECT_UNLOCK (rtx);

      gst_pad_push (rtx->srcpad, GST_BUFFER (data->object));
    } else if (GST_IS_EVENT (data->object)) {
      gst_pad_push_event (rtx->srcpad, GST_EVENT (data->object));

      /* after EOS, we should not send any more buffers,
       * even if there are more requests coming in */
      if (GST_EVENT_TYPE (data->object) == GST_EVENT_EOS) {
        gst_rtp_rtx_send_set_flushing (rtx, TRUE);
      }
    } else {
      g_assert_not_reached ();
    }

    data->object = NULL;        /* we no longer own that object */
    data->destroy (data);
  } else {
    GST_LOG_OBJECT (rtx, "flushing");
    gst_pad_pause_task (rtx->srcpad);
  }
}

static gboolean
gst_rtp_rtx_send_activate_mode (GstPad * pad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  GstRtpRtxSend *rtx = GST_RTP_RTX_SEND (parent);
  gboolean ret = FALSE;

  switch (mode) {
    case GST_PAD_MODE_PUSH:
      if (active) {
        gst_rtp_rtx_send_set_flushing (rtx, FALSE);
        ret = gst_pad_start_task (rtx->srcpad,
            (GstTaskFunction) gst_rtp_rtx_send_src_loop, rtx, NULL);
      } else {
        gst_rtp_rtx_send_set_flushing (rtx, TRUE);
        ret = gst_pad_stop_task (rtx->srcpad);
      }
      GST_INFO_OBJECT (rtx, "activate_mode: active %d, ret %d", active, ret);
      break;
    default:
      break;
  }
  return ret;
}

static void
gst_rtp_rtx_send_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstRtpRtxSend *rtx = GST_RTP_RTX_SEND (object);

  switch (prop_id) {
    case PROP_PAYLOAD_TYPE_MAP:
      GST_OBJECT_LOCK (rtx);
      g_value_set_boxed (value, rtx->rtx_pt_map_structure);
      GST_OBJECT_UNLOCK (rtx);
      break;
    case PROP_MAX_SIZE_TIME:
      GST_OBJECT_LOCK (rtx);
      g_value_set_uint (value, rtx->max_size_time);
      GST_OBJECT_UNLOCK (rtx);
      break;
    case PROP_MAX_SIZE_PACKETS:
      GST_OBJECT_LOCK (rtx);
      g_value_set_uint (value, rtx->max_size_packets);
      GST_OBJECT_UNLOCK (rtx);
      break;
    case PROP_NUM_RTX_REQUESTS:
      GST_OBJECT_LOCK (rtx);
      g_value_set_uint (value, rtx->num_rtx_requests);
      GST_OBJECT_UNLOCK (rtx);
      break;
    case PROP_NUM_RTX_PACKETS:
      GST_OBJECT_LOCK (rtx);
      g_value_set_uint (value, rtx->num_rtx_packets);
      GST_OBJECT_UNLOCK (rtx);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
structure_to_hash_table (GQuark field_id, const GValue * value, gpointer hash)
{
  const gchar *field_str;
  guint field_uint;
  guint value_uint;

  field_str = g_quark_to_string (field_id);
  field_uint = atoi (field_str);
  value_uint = g_value_get_uint (value);
  g_hash_table_insert ((GHashTable *) hash, GUINT_TO_POINTER (field_uint),
      GUINT_TO_POINTER (value_uint));

  return TRUE;
}

static void
gst_rtp_rtx_send_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstRtpRtxSend *rtx = GST_RTP_RTX_SEND (object);

  switch (prop_id) {
    case PROP_SSRC_MAP:
      GST_OBJECT_LOCK (rtx);
      if (rtx->external_ssrc_map)
        gst_structure_free (rtx->external_ssrc_map);
      rtx->external_ssrc_map = g_value_dup_boxed (value);
      GST_OBJECT_UNLOCK (rtx);
      break;
    case PROP_PAYLOAD_TYPE_MAP:
      GST_OBJECT_LOCK (rtx);
      if (rtx->rtx_pt_map_structure)
        gst_structure_free (rtx->rtx_pt_map_structure);
      rtx->rtx_pt_map_structure = g_value_dup_boxed (value);
      g_hash_table_remove_all (rtx->rtx_pt_map);
      gst_structure_foreach (rtx->rtx_pt_map_structure, structure_to_hash_table,
          rtx->rtx_pt_map);
      GST_OBJECT_UNLOCK (rtx);
      break;
    case PROP_MAX_SIZE_TIME:
      GST_OBJECT_LOCK (rtx);
      rtx->max_size_time = g_value_get_uint (value);
      GST_OBJECT_UNLOCK (rtx);
      break;
    case PROP_MAX_SIZE_PACKETS:
      GST_OBJECT_LOCK (rtx);
      rtx->max_size_packets = g_value_get_uint (value);
      GST_OBJECT_UNLOCK (rtx);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_rtp_rtx_send_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstRtpRtxSend *rtx;

  rtx = GST_RTP_RTX_SEND (element);

  switch (transition) {
    default:
      break;
  }

  ret =
      GST_ELEMENT_CLASS (gst_rtp_rtx_send_parent_class)->change_state (element,
      transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_rtx_send_reset (rtx);
      break;
    default:
      break;
  }

  return ret;
}

gboolean
gst_rtp_rtx_send_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_rtp_rtx_send_debug, "rtprtxsend", 0,
      "rtp retransmission sender");

  return gst_element_register (plugin, "rtprtxsend", GST_RANK_NONE,
      GST_TYPE_RTP_RTX_SEND);
}
