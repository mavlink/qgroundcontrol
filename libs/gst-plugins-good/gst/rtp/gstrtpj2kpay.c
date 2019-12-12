/* GStreamer
 * Copyright (C) 2009 Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:element-rtpj2kpay
 * @title: rtpj2kpay
 *
 * Payload encode JPEG 2000 images into RTP packets according to RFC 5371
 * and RFC 5372.
 * For detailed information see: https://datatracker.ietf.org/doc/rfc5371/
 * and https://datatracker.ietf.org/doc/rfc5372/
 *
 * The payloader takes a JPEG 2000 image, scans it for "packetization
 * units" and constructs the RTP packet header followed by the JPEG 2000
 * codestream. A "packetization unit" is defined as either a JPEG 2000 main header,
 * a JPEG 2000 tile-part header, or a JPEG 2000 packet.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include "gstrtpj2kcommon.h"
#include "gstrtpj2kpay.h"
#include "gstrtputils.h"

static GstStaticPadTemplate gst_rtp_j2k_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/x-jpc, " GST_RTP_J2K_SAMPLING_LIST)
    );


static GstStaticPadTemplate gst_rtp_j2k_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "  media = (string) \"video\", "
        "  payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "  clock-rate = (int) 90000, "
        GST_RTP_J2K_SAMPLING_LIST "," "  encoding-name = (string) \"JPEG2000\"")
    );

GST_DEBUG_CATEGORY_STATIC (rtpj2kpay_debug);
#define GST_CAT_DEFAULT (rtpj2kpay_debug)


enum
{
  PROP_0,
  PROP_LAST
};

typedef struct
{
  guint tp:2;
  guint MHF:2;
  guint mh_id:3;
  guint T:1;
  guint priority:8;
  guint tile:16;
  guint offset:24;
} RtpJ2KHeader;

static void gst_rtp_j2k_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_j2k_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rtp_j2k_pay_setcaps (GstRTPBasePayload * basepayload,
    GstCaps * caps);

static GstFlowReturn gst_rtp_j2k_pay_handle_buffer (GstRTPBasePayload * pad,
    GstBuffer * buffer);

#define gst_rtp_j2k_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpJ2KPay, gst_rtp_j2k_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_j2k_pay_class_init (GstRtpJ2KPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->set_property = gst_rtp_j2k_pay_set_property;
  gobject_class->get_property = gst_rtp_j2k_pay_get_property;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_j2k_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_j2k_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP JPEG 2000 payloader", "Codec/Payloader/Network/RTP",
      "Payload-encodes JPEG 2000 pictures into RTP packets (RFC 5371)",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_j2k_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_j2k_pay_handle_buffer;

  GST_DEBUG_CATEGORY_INIT (rtpj2kpay_debug, "rtpj2kpay", 0,
      "JPEG 2000 RTP Payloader");
}

static void
gst_rtp_j2k_pay_init (GstRtpJ2KPay * pay)
{
}

static gboolean
gst_rtp_j2k_pay_setcaps (GstRTPBasePayload * basepayload, GstCaps * caps)
{
  GstStructure *caps_structure = gst_caps_get_structure (caps, 0);
  gboolean res;
  gint width = 0, height = 0;
  const gchar *sampling = NULL;

  gboolean has_width = gst_structure_get_int (caps_structure, "width", &width);
  gboolean has_height =
      gst_structure_get_int (caps_structure, "height", &height);


  /* sampling is a required field */
  sampling = gst_structure_get_string (caps_structure, "sampling");

  gst_rtp_base_payload_set_options (basepayload, "video", TRUE, "JPEG2000",
      90000);

  if (has_width && has_height)
    res = gst_rtp_base_payload_set_outcaps (basepayload,
        "sampling", G_TYPE_STRING, sampling, "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height, NULL);
  else
    res =
        gst_rtp_base_payload_set_outcaps (basepayload, "sampling",
        G_TYPE_STRING, sampling, NULL);
  return res;
}


static guint
gst_rtp_j2k_pay_header_size (const guint8 * data, guint offset)
{
  return data[offset] << 8 | data[offset + 1];
}


static GstRtpJ2KMarker
gst_rtp_j2k_pay_scan_marker (const guint8 * data, guint size, guint * offset)
{
  while ((data[(*offset)++] != GST_J2K_MARKER) && ((*offset) < size));

  if (G_UNLIKELY ((*offset) >= size)) {
    return GST_J2K_MARKER_EOC;
  } else {
    guint8 marker = data[(*offset)++];
    return (GstRtpJ2KMarker) marker;
  }
}

typedef struct
{
  RtpJ2KHeader header;
  gboolean multi_tile;
  gboolean bitstream;
  guint next_sot;
  gboolean force_packet;
} RtpJ2KState;


/* Note: The standard recommends that headers be put in their own RTP packets, so we follow
 * this recommendation in the code. Also, this method groups together all J2K packets
 * for a tile part and treats this group as a packetization unit. According to the RFC,
 * only an individual J2K packet is considered a packetization unit.
 */

static guint
find_pu_end (GstRtpJ2KPay * pay, const guint8 * data, guint size,
    guint offset, RtpJ2KState * state)
{
  gboolean cut_sop = FALSE;
  GstRtpJ2KMarker marker;

  /* parse the j2k header for 'start of codestream' */
  GST_LOG_OBJECT (pay, "checking from offset %u", offset);
  while (offset < size) {
    marker = gst_rtp_j2k_pay_scan_marker (data, size, &offset);

    if (state->bitstream) {
      /* parsing bitstream, only look for SOP */
      switch (marker) {
        case GST_J2K_MARKER_SOP:
          GST_LOG_OBJECT (pay, "found SOP at %u", offset);
          if (cut_sop)
            return offset - 2;
          cut_sop = TRUE;
          break;
        case GST_J2K_MARKER_EPH:
          /* just skip over EPH */
          GST_LOG_OBJECT (pay, "found EPH at %u", offset);
          break;
        default:
          if (offset >= state->next_sot) {
            GST_LOG_OBJECT (pay, "reached next SOT at %u", offset);
            state->bitstream = FALSE;
            state->force_packet = TRUE;
            if (marker == GST_J2K_MARKER_EOC && state->next_sot + 2 <= size)
              /* include EOC but never go past the max size */
              return state->next_sot + 2;
            else
              return state->next_sot;
          }
          break;
      }
    } else {
      switch (marker) {
        case GST_J2K_MARKER_SOC:
          GST_LOG_OBJECT (pay, "found SOC at %u", offset);
          /* start off by assuming that we will fit the entire header
             into the RTP payload */
          state->header.MHF = 3;
          break;
        case GST_J2K_MARKER_SOT:
        {
          guint len, Psot, tile;

          GST_LOG_OBJECT (pay, "found SOT at %u", offset);
          /* SOT for first tile part in code stream:
             force close of current RTP packet, so that it
             only contains main header  */
          if (state->header.MHF) {
            state->force_packet = TRUE;
            return offset - 2;
          }

          /* parse SOT but do some sanity checks first */
          len = gst_rtp_j2k_pay_header_size (data, offset);
          GST_LOG_OBJECT (pay, "SOT length %u", len);
          if (len < 8)
            return size;
          if (offset + len >= size)
            return size;

          /* Isot */
          tile = GST_READ_UINT16_BE (&data[offset + 2]);

          if (!state->multi_tile) {
            /* we have detected multiple tiles in this rtp packet : tile bit is now invalid */
            if (state->header.T == 0 && state->header.tile != tile) {
              state->header.T = 1;
              state->multi_tile = TRUE;
            } else {
              state->header.T = 0;
            }
          }
          state->header.tile = tile;

          /* Note: Tile parts from multiple tiles in single RTP packet 
             will make T invalid.
             This cannot happen in our case since we always
             send tile headers in their own RTP packets, so we cannot mix
             tile parts in a single RTP packet  */

          /* Psot: offset of next tile. If it's 0, next tile goes all the way
             to the end of the data */
          Psot = GST_READ_UINT32_BE (&data[offset + 4]);
          if (Psot == 0)
            state->next_sot = size;
          else
            state->next_sot = offset - 2 + Psot;

          offset += len;
          GST_LOG_OBJECT (pay, "Isot %u, Psot %u, next %u", state->header.tile,
              Psot, state->next_sot);
          break;
        }
        case GST_J2K_MARKER_SOD:
          GST_LOG_OBJECT (pay, "found SOD at %u", offset);
          /* go to bitstream parsing */
          state->bitstream = TRUE;
          /* cut at the next SOP or else include all data */
          cut_sop = TRUE;
          /* force a new packet when we see SOP, this can be optional but the
           * spec recommends packing headers separately */
          state->force_packet = TRUE;
          break;
        case GST_J2K_MARKER_EOC:
          GST_LOG_OBJECT (pay, "found EOC at %u", offset);
          return offset;
        default:
        {
          guint len = gst_rtp_j2k_pay_header_size (data, offset);
          GST_LOG_OBJECT (pay, "skip 0x%02x len %u", marker, len);
          offset += len;
          break;
        }
      }
    }
  }
  GST_DEBUG_OBJECT (pay, "reached end of data");
  return size;
}

static GstFlowReturn
gst_rtp_j2k_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpJ2KPay *pay;
  GstClockTime timestamp;
  GstFlowReturn ret = GST_FLOW_ERROR;
  RtpJ2KState state;
  GstBufferList *list = NULL;
  GstMapInfo map;
  guint mtu, max_size;
  guint offset;
  guint end, pos;

  pay = GST_RTP_J2K_PAY (basepayload);
  mtu = GST_RTP_BASE_PAYLOAD_MTU (pay);

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  timestamp = GST_BUFFER_PTS (buffer);
  offset = pos = end = 0;

  GST_LOG_OBJECT (pay,
      "got buffer size %" G_GSIZE_FORMAT ", timestamp %" GST_TIME_FORMAT,
      map.size, GST_TIME_ARGS (timestamp));

  /* do some header defaults first */
  state.header.tp = 0;          /* only progressive scan */
  state.header.MHF = 0;         /* no header */
  state.header.mh_id = 0;       /* always 0 for now */
  state.header.T = 1;           /* invalid tile, because we always begin with the main header */
  state.header.priority = 255;  /* always 255 for now */
  state.header.tile = 0xffff;   /* no tile number */
  state.header.offset = 0;      /* offset of 0 */
  state.multi_tile = FALSE;
  state.bitstream = FALSE;
  state.next_sot = 0;
  state.force_packet = FALSE;

  /* get max packet length */
  max_size =
      gst_rtp_buffer_calc_payload_len (mtu - GST_RTP_J2K_HEADER_SIZE, 0, 0);

  list = gst_buffer_list_new_sized ((mtu / max_size) + 1);

  do {
    GstBuffer *outbuf;
    guint8 *header;
    guint payload_size;
    guint pu_size;
    GstRTPBuffer rtp = { NULL };

    /* try to pack as much as we can */
    do {
      /* see how much we have scanned already */
      pu_size = end - offset;
      GST_DEBUG_OBJECT (pay, "scanned pu size %u", pu_size);

      /* we need to make a new packet */
      if (state.force_packet) {
        GST_DEBUG_OBJECT (pay, "need to force a new packet");
        state.force_packet = FALSE;
        pos = end;
        break;
      }

      /* else see if we have enough */
      if (pu_size > max_size) {
        if (pos != offset)
          /* the packet became too large, use previous scanpos */
          pu_size = pos - offset;
        else
          /* the already scanned data was already too big, make sure we start
           * scanning from the last searched position */
          pos = end;

        GST_DEBUG_OBJECT (pay, "max size exceeded pu_size %u", pu_size);
        break;
      }

      pos = end;

      /* exit when finished */
      if (pos == map.size)
        break;

      /* scan next packetization unit and fill in the header */
      end = find_pu_end (pay, map.data, map.size, pos, &state);
    } while (TRUE);

    while (pu_size > 0) {
      guint packet_size, data_size;
      GstBuffer *paybuf;

      /* calculate the packet size */
      packet_size =
          gst_rtp_buffer_calc_packet_len (pu_size + GST_RTP_J2K_HEADER_SIZE, 0,
          0);

      if (packet_size > mtu) {
        GST_DEBUG_OBJECT (pay, "needed packet size %u clamped to MTU %u",
            packet_size, mtu);
        packet_size = mtu;
      } else {
        GST_DEBUG_OBJECT (pay, "needed packet size %u fits in MTU %u",
            packet_size, mtu);
      }

      /* get total payload size and data size */
      payload_size = gst_rtp_buffer_calc_payload_len (packet_size, 0, 0);
      data_size = payload_size - GST_RTP_J2K_HEADER_SIZE;

      /* make buffer for header */
      outbuf = gst_rtp_buffer_new_allocate (GST_RTP_J2K_HEADER_SIZE, 0, 0);

      GST_BUFFER_PTS (outbuf) = timestamp;

      gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

      /* get pointer to header */
      header = gst_rtp_buffer_get_payload (&rtp);

      pu_size -= data_size;

      /* reached the end of a packetization unit */
      if (pu_size == 0 && end >= map.size) {
        gst_rtp_buffer_set_marker (&rtp, TRUE);
      }
      /* If we were processing a header, see if all fits in one RTP packet
         or if we have to fragment it */
      if (state.header.MHF) {
        switch (state.header.MHF) {
          case 3:
            if (pu_size > 0)
              state.header.MHF = 1;
            break;
          case 1:
            if (pu_size == 0)
              state.header.MHF = 2;
            break;
          default:
            break;
        }
      }

      /*
       * RtpJ2KHeader:
       * @tp: type (0 progressive, 1 odd field, 2 even field)
       * @MHF: Main Header Flag
       * @mh_id: Main Header Identification
       * @T: Tile field invalidation flag
       * @priority: priority
       * @tile number: the tile number of the payload
       * @reserved: set to 0
       * @fragment offset: the byte offset of the current payload
       *
       *  0                   1                   2                   3
       *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |tp |MHF|mh_id|T|     priority  |           tile number         |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |reserved       |             fragment offset                   |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       */
      header[0] = (state.header.tp << 6) | (state.header.MHF << 4) |
          (state.header.mh_id << 1) | state.header.T;
      header[1] = state.header.priority;
      header[2] = state.header.tile >> 8;
      header[3] = state.header.tile & 0xff;
      header[4] = 0;
      header[5] = state.header.offset >> 16;
      header[6] = (state.header.offset >> 8) & 0xff;
      header[7] = state.header.offset & 0xff;

      gst_rtp_buffer_unmap (&rtp);

      /* make subbuffer of j2k data */
      paybuf = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_ALL,
          offset, data_size);
      gst_rtp_copy_video_meta (basepayload, outbuf, paybuf);
      outbuf = gst_buffer_append (outbuf, paybuf);

      gst_buffer_list_add (list, outbuf);

      /* reset multi_tile */
      state.multi_tile = FALSE;


      /* set MHF to zero if there is no more main header to process */
      if (state.header.MHF & 2)
        state.header.MHF = 0;

      /* tile is valid, if there is no more header to process */
      if (!state.header.MHF)
        state.header.T = 0;


      offset += data_size;
      state.header.offset = offset;
    }
    offset = pos;
  } while (offset < map.size);

  gst_buffer_unmap (buffer, &map);
  gst_buffer_unref (buffer);

  /* push the whole buffer list at once */
  ret = gst_rtp_base_payload_push_list (basepayload, list);

  return ret;
}

static void
gst_rtp_j2k_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_j2k_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_rtp_j2k_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpj2kpay", GST_RANK_SECONDARY,
      GST_TYPE_RTP_J2K_PAY);
}
