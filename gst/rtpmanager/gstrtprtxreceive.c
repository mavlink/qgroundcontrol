/* RTP Retransmission receiver element for GStreamer
 *
 * gstrtprtxreceive.c:
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
 * SECTION:element-rtprtxreceive
 * @title: rtprtxreceive
 * @see_also: rtprtxsend, rtpsession, rtpjitterbuffer
 *
 * rtprtxreceive listens to the retransmission events from the
 * downstream rtpjitterbuffer and remembers the SSRC (ssrc1) of the stream and
 * the sequence number that was requested. When it receives a packet with
 * a sequence number equal to one of the ones stored and with a different SSRC,
 * it identifies the new SSRC (ssrc2) as the retransmission stream of ssrc1.
 * From this point on, it replaces ssrc2 with ssrc1 in all packets of the
 * ssrc2 stream and flags them as retransmissions, so that rtpjitterbuffer
 * can reconstruct the original stream.
 *
 * This algorithm is implemented as specified in RFC 4588.
 *
 * This element is meant to be used with rtprtxsend on the sender side.
 * See #GstRtpRtxSend
 *
 * Below you can see some examples that illustrate how rtprtxreceive and
 * rtprtxsend fit among the other rtp elements and how they work internally.
 * Normally, hoewever, you should avoid using such pipelines and use
 * rtpbin instead, with its #GstRtpBin::request-aux-sender and
 * #GstRtpBin::request-aux-receiver signals. See #GstRtpBin.
 *
 * ## Example pipelines
 *
 * |[
 * gst-launch-1.0 rtpsession name=rtpsession rtp-profile=avpf \
 *     audiotestsrc is-live=true ! opusenc ! rtpopuspay pt=96 ! \
 *         rtprtxsend payload-type-map="application/x-rtp-pt-map,96=(uint)97" ! \
 *         rtpsession.send_rtp_sink \
 *     rtpsession.send_rtp_src ! identity drop-probability=0.01 ! \
 *         udpsink host="127.0.0.1" port=5000 \
 *     udpsrc port=5001 ! rtpsession.recv_rtcp_sink \
 *     rtpsession.send_rtcp_src ! udpsink host="127.0.0.1" port=5002 \
 *         sync=false async=false
 * ]| Send audio stream through port 5000 (5001 and 5002 are just the rtcp
 * link with the receiver)
 *
 * |[
 * gst-launch-1.0 rtpsession name=rtpsession rtp-profile=avpf \
 *     udpsrc port=5000 caps="application/x-rtp,media=(string)audio,clock-rate=(int)48000,encoding-name=(string)OPUS,payload=(int)96" ! \
 *         rtpsession.recv_rtp_sink \
 *     rtpsession.recv_rtp_src ! \
 *         rtprtxreceive payload-type-map="application/x-rtp-pt-map,96=(uint)97" ! \
 *         rtpssrcdemux ! rtpjitterbuffer do-retransmission=true ! \
 *         rtpopusdepay ! opusdec ! audioconvert ! audioresample ! autoaudiosink \
 *     rtpsession.send_rtcp_src ! \
 *         udpsink host="127.0.0.1" port=5001 sync=false async=false \
 *     udpsrc port=5002 ! rtpsession.recv_rtcp_sink
 * ]|
 * Receive audio stream from port 5000 (5001 and 5002 are just the rtcp
 * link with the sender)
 *
 * In this example we can see a simple streaming of an OPUS stream with some
 * of the packets being artificially dropped by the identity element.
 * Thanks to retransmission, you should still hear a clear sound when setting
 * drop-probability to something greater than 0.
 *
 * Internally, the rtpjitterbuffer will generate a custom upstream event,
 * GstRTPRetransmissionRequest, when it detects that one packet is missing.
 * Then this request is translated to a FB NACK in the rtcp link by rtpsession.
 * Finally the rtpsession of the sender side will re-convert it in a
 * GstRTPRetransmissionRequest that will be handled by rtprtxsend. rtprtxsend
 * will then re-send the missing packet with a new srrc and a different payload
 * type (here, 97), but with the same original sequence number. On the receiver
 * side, rtprtxreceive will associate this new stream with the original and
 * forward the retransmission packets to rtpjitterbuffer with the original
 * ssrc and payload type.
 *
 * |[
 * gst-launch-1.0 rtpsession name=rtpsession rtp-profile=avpf \
 *     audiotestsrc is-live=true ! opusenc ! rtpopuspay pt=97 seqnum-offset=1 ! \
 *         rtprtxsend payload-type-map="application/x-rtp-pt-map,97=(uint)99" ! \
 *         funnel name=f ! rtpsession.send_rtp_sink \
 *     audiotestsrc freq=660.0 is-live=true ! opusenc ! \
 *         rtpopuspay pt=97 seqnum-offset=100 ! \
 *         rtprtxsend payload-type-map="application/x-rtp-pt-map,97=(uint)99" ! \
 *         f. \
 *     rtpsession.send_rtp_src ! identity drop-probability=0.01 ! \
 *         udpsink host="127.0.0.1" port=5000 \
 *     udpsrc port=5001 ! rtpsession.recv_rtcp_sink \
 *     rtpsession.send_rtcp_src ! udpsink host="127.0.0.1" port=5002 \
 *         sync=false async=false
 * ]|
 * Send two audio streams to port 5000.
 * |[
 * gst-launch-1.0 rtpsession name=rtpsession rtp-profile=avpf \
 *     udpsrc port=5000 caps="application/x-rtp,media=(string)audio,clock-rate=(int)48000,encoding-name=(string)OPUS,payload=(int)97" ! \
 *         rtpsession.recv_rtp_sink \
 *     rtpsession.recv_rtp_src ! \
 *         rtprtxreceive payload-type-map="application/x-rtp-pt-map,97=(uint)99" ! \
 *         rtpssrcdemux name=demux \
 *     demux. ! queue ! rtpjitterbuffer do-retransmission=true ! rtpopusdepay ! \
 *         opusdec ! audioconvert ! autoaudiosink \
 *     demux. ! queue ! rtpjitterbuffer do-retransmission=true ! rtpopusdepay ! \
 *         opusdec ! audioconvert ! autoaudiosink \
 *     udpsrc port=5002 ! rtpsession.recv_rtcp_sink \
 *     rtpsession.send_rtcp_src ! udpsink host="127.0.0.1" port=5001 \
 *         sync=false async=false
 * ]|
 * Receive two audio streams from port 5000.
 *
 * In this example we are streaming two streams of the same type through the
 * same port. They, however, are using a different SSRC (ssrc is randomly
 * generated on each payloader - rtpopuspay in this example), so they can be
 * identified and demultiplexed by rtpssrcdemux on the receiver side. This is
 * an example of SSRC-multiplexing.
 *
 * It is important here to use a different starting sequence number
 * (seqnum-offset), since this is the only means of identification that
 * rtprtxreceive uses the very first time to identify retransmission streams.
 * It is an error, according to RFC4588 to have two retransmission requests for
 * packets belonging to two different streams but with the same sequence number.
 * Note that the default seqnum-offset value (-1, which means random) would
 * work just fine, but it is overridden here for illustration purposes.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <string.h>
#include <stdlib.h>

#include "gstrtprtxreceive.h"

#define ASSOC_TIMEOUT (GST_SECOND)

GST_DEBUG_CATEGORY_STATIC (gst_rtp_rtx_receive_debug);
#define GST_CAT_DEFAULT gst_rtp_rtx_receive_debug

enum
{
  PROP_0,
  PROP_PAYLOAD_TYPE_MAP,
  PROP_NUM_RTX_REQUESTS,
  PROP_NUM_RTX_PACKETS,
  PROP_NUM_RTX_ASSOC_PACKETS
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

static gboolean gst_rtp_rtx_receive_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_rtp_rtx_receive_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);

static GstStateChangeReturn gst_rtp_rtx_receive_change_state (GstElement *
    element, GstStateChange transition);

static void gst_rtp_rtx_receive_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_rtx_receive_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_rtp_rtx_receive_finalize (GObject * object);

G_DEFINE_TYPE (GstRtpRtxReceive, gst_rtp_rtx_receive, GST_TYPE_ELEMENT);

static void
gst_rtp_rtx_receive_class_init (GstRtpRtxReceiveClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->get_property = gst_rtp_rtx_receive_get_property;
  gobject_class->set_property = gst_rtp_rtx_receive_set_property;
  gobject_class->finalize = gst_rtp_rtx_receive_finalize;

  g_object_class_install_property (gobject_class, PROP_PAYLOAD_TYPE_MAP,
      g_param_spec_boxed ("payload-type-map", "Payload Type Map",
          "Map of original payload types to their retransmission payload types",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NUM_RTX_REQUESTS,
      g_param_spec_uint ("num-rtx-requests", "Num RTX Requests",
          "Number of retransmission events received", 0, G_MAXUINT,
          0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NUM_RTX_PACKETS,
      g_param_spec_uint ("num-rtx-packets", "Num RTX Packets",
          " Number of retransmission packets received", 0, G_MAXUINT,
          0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NUM_RTX_ASSOC_PACKETS,
      g_param_spec_uint ("num-rtx-assoc-packets",
          "Num RTX Associated Packets", "Number of retransmission packets "
          "correctly associated with retransmission requests", 0, G_MAXUINT,
          0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_factory);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Retransmission receiver", "Codec",
      "Receive retransmitted RTP packets according to RFC4588",
      "Julien Isorce <julien.isorce@collabora.co.uk>");

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_receive_change_state);
}

static void
gst_rtp_rtx_receive_reset (GstRtpRtxReceive * rtx)
{
  GST_OBJECT_LOCK (rtx);
  g_hash_table_remove_all (rtx->ssrc2_ssrc1_map);
  g_hash_table_remove_all (rtx->seqnum_ssrc1_map);
  rtx->num_rtx_requests = 0;
  rtx->num_rtx_packets = 0;
  rtx->num_rtx_assoc_packets = 0;
  GST_OBJECT_UNLOCK (rtx);
}

static void
gst_rtp_rtx_receive_finalize (GObject * object)
{
  GstRtpRtxReceive *rtx = GST_RTP_RTX_RECEIVE (object);

  g_hash_table_unref (rtx->ssrc2_ssrc1_map);
  g_hash_table_unref (rtx->seqnum_ssrc1_map);
  g_hash_table_unref (rtx->rtx_pt_map);
  if (rtx->rtx_pt_map_structure)
    gst_structure_free (rtx->rtx_pt_map_structure);

  G_OBJECT_CLASS (gst_rtp_rtx_receive_parent_class)->finalize (object);
}

typedef struct
{
  guint32 ssrc;
  GstClockTime time;
} SsrcAssoc;

static SsrcAssoc *
ssrc_assoc_new (guint32 ssrc, GstClockTime time)
{
  SsrcAssoc *assoc = g_slice_new (SsrcAssoc);

  assoc->ssrc = ssrc;
  assoc->time = time;

  return assoc;
}

static void
ssrc_assoc_free (SsrcAssoc * assoc)
{
  g_slice_free (SsrcAssoc, assoc);
}

static void
gst_rtp_rtx_receive_init (GstRtpRtxReceive * rtx)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (rtx);

  rtx->srcpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "src"), "src");
  GST_PAD_SET_PROXY_CAPS (rtx->srcpad);
  GST_PAD_SET_PROXY_ALLOCATION (rtx->srcpad);
  gst_pad_set_event_function (rtx->srcpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_receive_src_event));
  gst_element_add_pad (GST_ELEMENT (rtx), rtx->srcpad);

  rtx->sinkpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "sink"), "sink");
  GST_PAD_SET_PROXY_CAPS (rtx->sinkpad);
  GST_PAD_SET_PROXY_ALLOCATION (rtx->sinkpad);
  gst_pad_set_chain_function (rtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rtp_rtx_receive_chain));
  gst_element_add_pad (GST_ELEMENT (rtx), rtx->sinkpad);

  rtx->ssrc2_ssrc1_map = g_hash_table_new (g_direct_hash, g_direct_equal);
  rtx->seqnum_ssrc1_map = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) ssrc_assoc_free);

  rtx->rtx_pt_map = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static gboolean
gst_rtp_rtx_receive_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRtpRtxReceive *rtx = GST_RTP_RTX_RECEIVE (parent);
  gboolean res;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
    {
      const GstStructure *s = gst_event_get_structure (event);

      /* This event usually comes from the downstream gstrtpjitterbuffer */
      if (gst_structure_has_name (s, "GstRTPRetransmissionRequest")) {
        guint seqnum = 0;
        guint ssrc = 0;
        gpointer ssrc2 = 0;

        /* retrieve seqnum of the packet that need to be retransmitted */
        if (!gst_structure_get_uint (s, "seqnum", &seqnum))
          seqnum = -1;

        /* retrieve ssrc of the packet that need to be retransmitted
         * it's useful when reconstructing the original packet from the rtx packet */
        if (!gst_structure_get_uint (s, "ssrc", &ssrc))
          ssrc = -1;

        GST_DEBUG_OBJECT (rtx, "got rtx request for seqnum: %u, ssrc: %X",
            seqnum, ssrc);

        GST_OBJECT_LOCK (rtx);

        /* increase number of seen requests for our statistics */
        ++rtx->num_rtx_requests;

        /* First, we lookup in our map to see if we have already associate this
         * master stream ssrc with its retransmitted stream.
         * Every ssrc are unique so we can use the same hash table
         * for both retrieving the ssrc1 from ssrc2 and also ssrc2 from ssrc1
         */
        if (g_hash_table_lookup_extended (rtx->ssrc2_ssrc1_map,
                GUINT_TO_POINTER (ssrc), NULL, &ssrc2)
            && GPOINTER_TO_UINT (ssrc2) != GPOINTER_TO_UINT (ssrc)) {
          GST_TRACE_OBJECT (rtx, "Retransmitted stream %X already associated "
              "to its master, %X", GPOINTER_TO_UINT (ssrc2), ssrc);
        } else {
          SsrcAssoc *assoc;

          /* not already associated but also we have to check that we have not
           * already considered this request.
           */
          if (g_hash_table_lookup_extended (rtx->seqnum_ssrc1_map,
                  GUINT_TO_POINTER (seqnum), NULL, (gpointer *) & assoc)) {
            if (assoc->ssrc == ssrc) {
              /* same seqnum, same ssrc */

              /* do nothing because we have already considered this request
               * The jitter may be too impatient of the rtx packet has been
               * lost too.
               * It does not mean we reject the event, we still want to forward
               * the request to the gstrtpsession to be translator into a FB NACK
               */
              GST_LOG_OBJECT (rtx, "Duplicate request: seqnum: %u, ssrc: %X",
                  seqnum, ssrc);
            } else {
              /* same seqnum, different ssrc */

              /* If the association attempt is larger than ASSOC_TIMEOUT,
               * then we give up on it, and try this one.
               */
              if (!GST_CLOCK_TIME_IS_VALID (rtx->last_time) ||
                  !GST_CLOCK_TIME_IS_VALID (assoc->time) ||
                  assoc->time + ASSOC_TIMEOUT < rtx->last_time) {
                /* From RFC 4588:
                 * the receiver MUST NOT have two outstanding requests for the
                 * same packet sequence number in two different original streams
                 * before the association is resolved. Otherwise it's impossible
                 * to associate a rtx stream and its master stream
                 */

                /* remove seqnum in order to reuse the spot */
                g_hash_table_remove (rtx->seqnum_ssrc1_map,
                    GUINT_TO_POINTER (seqnum));
                goto retransmit;
              } else {
                GST_INFO_OBJECT (rtx, "rejecting request for seqnum %u"
                    " of master stream %X; there is already a pending request "
                    "for the same seqnum on ssrc %X that has not expired",
                    seqnum, ssrc, assoc->ssrc);

                /* do not forward the event as we are rejecting this request */
                GST_OBJECT_UNLOCK (rtx);
                gst_event_unref (event);
                return TRUE;
              }
            }
          } else {
          retransmit:
            /* the request has not been already considered
             * insert it for the first time */
            g_hash_table_insert (rtx->seqnum_ssrc1_map,
                GUINT_TO_POINTER (seqnum),
                ssrc_assoc_new (ssrc, rtx->last_time));
          }
        }

        GST_DEBUG_OBJECT (rtx, "packet number %u of master stream %X"
            " needs to be retransmitted", seqnum, ssrc);

        GST_OBJECT_UNLOCK (rtx);
      }

      /* Transfer event upstream so that the request can actually by translated
       * through gstrtpsession through the network */
      res = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }
  return res;
}

/* Copy fixed header and extension. Replace current ssrc by ssrc1,
 * remove OSN and replace current seq num by OSN.
 * Copy memory to avoid to manually copy each rtp buffer field.
 */
static GstBuffer *
_gst_rtp_buffer_new_from_rtx (GstRTPBuffer * rtp, guint32 ssrc1,
    guint16 orign_seqnum, guint8 origin_payload_type)
{
  GstMemory *mem = NULL;
  GstRTPBuffer new_rtp = GST_RTP_BUFFER_INIT;
  GstBuffer *new_buffer = gst_buffer_new ();
  GstMapInfo map;
  guint payload_len = 0;

  /* copy fixed header */
  mem = gst_memory_copy (rtp->map[0].memory,
      (guint8 *) rtp->data[0] - rtp->map[0].data, rtp->size[0]);
  gst_buffer_append_memory (new_buffer, mem);

  /* copy extension if any */
  if (rtp->size[1]) {
    mem = gst_memory_copy (rtp->map[1].memory,
        (guint8 *) rtp->data[1] - rtp->map[1].data, rtp->size[1]);
    gst_buffer_append_memory (new_buffer, mem);
  }

  /* copy payload and remove OSN */
  payload_len = rtp->size[2] - 2;
  mem = gst_allocator_alloc (NULL, payload_len, NULL);

  gst_memory_map (mem, &map, GST_MAP_WRITE);
  if (rtp->size[2])
    memcpy (map.data, (guint8 *) rtp->data[2] + 2, payload_len);
  gst_memory_unmap (mem, &map);
  gst_buffer_append_memory (new_buffer, mem);

  /* the sender always constructs rtx packets without padding,
   * But the receiver can still receive rtx packets with padding.
   * So just copy it.
   */
  if (rtp->size[3]) {
    guint pad_len = rtp->size[3];

    mem = gst_allocator_alloc (NULL, pad_len, NULL);

    gst_memory_map (mem, &map, GST_MAP_WRITE);
    map.data[pad_len - 1] = pad_len;
    gst_memory_unmap (mem, &map);

    gst_buffer_append_memory (new_buffer, mem);
  }

  /* set ssrc and seq num */
  gst_rtp_buffer_map (new_buffer, GST_MAP_WRITE, &new_rtp);
  gst_rtp_buffer_set_ssrc (&new_rtp, ssrc1);
  gst_rtp_buffer_set_seq (&new_rtp, orign_seqnum);
  gst_rtp_buffer_set_payload_type (&new_rtp, origin_payload_type);
  gst_rtp_buffer_unmap (&new_rtp);

  gst_buffer_copy_into (new_buffer, rtp->buffer,
      GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, -1);
  GST_BUFFER_FLAG_SET (new_buffer, GST_RTP_BUFFER_FLAG_RETRANSMISSION);

  return new_buffer;
}

static GstFlowReturn
gst_rtp_rtx_receive_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstRtpRtxReceive *rtx = GST_RTP_RTX_RECEIVE (parent);
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret = GST_FLOW_OK;
  GstBuffer *new_buffer = NULL;
  guint32 ssrc = 0;
  gpointer ssrc1 = 0;
  guint32 ssrc2 = 0;
  guint16 seqnum = 0;
  guint16 orign_seqnum = 0;
  guint8 payload_type = 0;
  gpointer payload = NULL;
  guint8 origin_payload_type = 0;
  gboolean is_rtx;
  gboolean drop = FALSE;

  /* map current rtp packet to parse its header */
  if (!gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp))
    goto invalid_buffer;

  ssrc = gst_rtp_buffer_get_ssrc (&rtp);
  seqnum = gst_rtp_buffer_get_seq (&rtp);
  payload_type = gst_rtp_buffer_get_payload_type (&rtp);

  /* check if we have a retransmission packet (this information comes from SDP) */
  GST_OBJECT_LOCK (rtx);

  is_rtx =
      g_hash_table_lookup_extended (rtx->rtx_pt_map,
      GUINT_TO_POINTER (payload_type), NULL, NULL);

  if (is_rtx) {
    payload = gst_rtp_buffer_get_payload (&rtp);

    if (!payload || gst_rtp_buffer_get_payload_len (&rtp) < 2) {
      GST_OBJECT_UNLOCK (rtx);
      gst_rtp_buffer_unmap (&rtp);
      goto invalid_buffer;
    }
  }

  rtx->last_time = GST_BUFFER_PTS (buffer);

  if (g_hash_table_size (rtx->seqnum_ssrc1_map) > 0) {
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, rtx->seqnum_ssrc1_map);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
      SsrcAssoc *assoc = value;

      /* remove association request if it is too old */
      if (GST_CLOCK_TIME_IS_VALID (rtx->last_time) &&
          GST_CLOCK_TIME_IS_VALID (assoc->time) &&
          assoc->time + ASSOC_TIMEOUT < rtx->last_time) {
        g_hash_table_iter_remove (&iter);
      }
    }
  }

  /* if the current packet is from a retransmission stream */
  if (is_rtx) {
    /* increase our statistic */
    ++rtx->num_rtx_packets;

    /* read OSN in the rtx payload */
    orign_seqnum = GST_READ_UINT16_BE (gst_rtp_buffer_get_payload (&rtp));
    origin_payload_type =
        GPOINTER_TO_UINT (g_hash_table_lookup (rtx->rtx_pt_map,
            GUINT_TO_POINTER (payload_type)));

    GST_DEBUG_OBJECT (rtx, "Got rtx packet: rtx seqnum %u, rtx ssrc %X, "
        "rtx pt %u, orig seqnum %u, orig pt %u", seqnum, ssrc, payload_type,
        orign_seqnum, origin_payload_type);

    /* first we check if we already have associated this retransmission stream
     * to a master stream */
    if (g_hash_table_lookup_extended (rtx->ssrc2_ssrc1_map,
            GUINT_TO_POINTER (ssrc), NULL, &ssrc1)) {
      GST_TRACE_OBJECT (rtx,
          "packet is from retransmission stream %X already associated to "
          "master stream %X", ssrc, GPOINTER_TO_UINT (ssrc1));
      ssrc2 = ssrc;
    } else {
      SsrcAssoc *assoc;

      /* the current retransmitted packet has its rtx stream not already
       * associated to a master stream, so retrieve it from our request
       * history */
      if (g_hash_table_lookup_extended (rtx->seqnum_ssrc1_map,
              GUINT_TO_POINTER (orign_seqnum), NULL, (gpointer *) & assoc)) {
        GST_LOG_OBJECT (rtx,
            "associating retransmitted stream %X to master stream %X thanks "
            "to rtx packet %u (orig seqnum %u)", ssrc, assoc->ssrc, seqnum,
            orign_seqnum);
        ssrc1 = GUINT_TO_POINTER (assoc->ssrc);
        ssrc2 = ssrc;

        /* just put a guard */
        if (GPOINTER_TO_UINT (ssrc1) == ssrc2)
          GST_WARNING_OBJECT (rtx, "RTX receiver ssrc2_ssrc1_map bad state, "
              "master and rtx SSRCs are the same (%X)\n", ssrc);

        /* free the spot so that this seqnum can be used to do another
         * association */
        g_hash_table_remove (rtx->seqnum_ssrc1_map,
            GUINT_TO_POINTER (orign_seqnum));

        /* actually do the association between rtx stream and master stream */
        g_hash_table_insert (rtx->ssrc2_ssrc1_map, GUINT_TO_POINTER (ssrc2),
            ssrc1);

        /* also do the association between master stream and rtx stream
         * every ssrc are unique so we can use the same hash table
         * for both retrieving the ssrc1 from ssrc2 and also ssrc2 from ssrc1
         */
        g_hash_table_insert (rtx->ssrc2_ssrc1_map, ssrc1,
            GUINT_TO_POINTER (ssrc2));

      } else {
        /* we are not able to associate this rtx packet with a master stream */
        GST_INFO_OBJECT (rtx,
            "dropping rtx packet %u because its orig seqnum (%u) is not in our"
            " pending retransmission requests", seqnum, orign_seqnum);
        drop = TRUE;
      }
    }
  }

  /* if not dropped the packet was successfully associated */
  if (is_rtx && !drop)
    ++rtx->num_rtx_assoc_packets;

  GST_OBJECT_UNLOCK (rtx);

  /* just drop the packet if the association could not have been made */
  if (drop) {
    gst_rtp_buffer_unmap (&rtp);
    gst_buffer_unref (buffer);
    return GST_FLOW_OK;
  }

  /* create the retransmission packet */
  if (is_rtx)
    new_buffer =
        _gst_rtp_buffer_new_from_rtx (&rtp, GPOINTER_TO_UINT (ssrc1),
        orign_seqnum, origin_payload_type);

  gst_rtp_buffer_unmap (&rtp);

  /* push the packet */
  if (is_rtx) {
    gst_buffer_unref (buffer);
    GST_LOG_OBJECT (rtx, "pushing packet seqnum:%u from restransmission "
        "stream ssrc: %X (master ssrc %X)", orign_seqnum, ssrc2,
        GPOINTER_TO_UINT (ssrc1));
    ret = gst_pad_push (rtx->srcpad, new_buffer);
  } else {
    GST_TRACE_OBJECT (rtx, "pushing packet seqnum:%u from master stream "
        "ssrc: %X", seqnum, ssrc);
    ret = gst_pad_push (rtx->srcpad, buffer);
  }

  return ret;

invalid_buffer:
  {
    GST_ELEMENT_WARNING (rtx, STREAM, DECODE, (NULL),
        ("Received invalid RTP payload, dropping"));
    gst_buffer_unref (buffer);
    return GST_FLOW_OK;
  }
}

static void
gst_rtp_rtx_receive_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstRtpRtxReceive *rtx = GST_RTP_RTX_RECEIVE (object);

  switch (prop_id) {
    case PROP_PAYLOAD_TYPE_MAP:
      GST_OBJECT_LOCK (rtx);
      g_value_set_boxed (value, rtx->rtx_pt_map_structure);
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
    case PROP_NUM_RTX_ASSOC_PACKETS:
      GST_OBJECT_LOCK (rtx);
      g_value_set_uint (value, rtx->num_rtx_assoc_packets);
      GST_OBJECT_UNLOCK (rtx);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
structure_to_hash_table_inv (GQuark field_id, const GValue * value,
    gpointer hash)
{
  const gchar *field_str;
  guint field_uint;
  guint value_uint;

  field_str = g_quark_to_string (field_id);
  field_uint = atoi (field_str);
  value_uint = g_value_get_uint (value);
  g_hash_table_insert ((GHashTable *) hash, GUINT_TO_POINTER (value_uint),
      GUINT_TO_POINTER (field_uint));

  return TRUE;
}

static void
gst_rtp_rtx_receive_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstRtpRtxReceive *rtx = GST_RTP_RTX_RECEIVE (object);

  switch (prop_id) {
    case PROP_PAYLOAD_TYPE_MAP:
      GST_OBJECT_LOCK (rtx);
      if (rtx->rtx_pt_map_structure)
        gst_structure_free (rtx->rtx_pt_map_structure);
      rtx->rtx_pt_map_structure = g_value_dup_boxed (value);
      g_hash_table_remove_all (rtx->rtx_pt_map);
      gst_structure_foreach (rtx->rtx_pt_map_structure,
          structure_to_hash_table_inv, rtx->rtx_pt_map);
      GST_OBJECT_UNLOCK (rtx);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_rtp_rtx_receive_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstRtpRtxReceive *rtx;

  rtx = GST_RTP_RTX_RECEIVE (element);

  switch (transition) {
    default:
      break;
  }

  ret =
      GST_ELEMENT_CLASS (gst_rtp_rtx_receive_parent_class)->change_state
      (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_rtx_receive_reset (rtx);
      break;
    default:
      break;
  }

  return ret;
}

gboolean
gst_rtp_rtx_receive_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_rtp_rtx_receive_debug, "rtprtxreceive", 0,
      "rtp retransmission receiver");

  return gst_element_register (plugin, "rtprtxreceive", GST_RANK_NONE,
      GST_TYPE_RTP_RTX_RECEIVE);
}
