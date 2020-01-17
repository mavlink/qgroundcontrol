/* GStreamer
 * Copyright (C) <2009> Edward Hervey <bilboed@bilboed.com>
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
#include <gst/audio/audio.h>
#include "gstrtpqdmdepay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY (rtpqdm2depay_debug);
#define GST_CAT_DEFAULT rtpqdm2depay_debug

static GstStaticPadTemplate gst_rtp_qdm2_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-qdm2")
    );

static GstStaticPadTemplate gst_rtp_qdm2_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", " "encoding-name = (string)\"X-QDM\"")
    );

#define gst_rtp_qdm2_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpQDM2Depay, gst_rtp_qdm2_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static const guint8 headheader[20] = {
  0x0, 0x0, 0x0, 0xc, 0x66, 0x72, 0x6d, 0x61,
  0x51, 0x44, 0x4d, 0x32, 0x0, 0x0, 0x0, 0x24,
  0x51, 0x44, 0x43, 0x41
};

static void gst_rtp_qdm2_depay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_qdm2_depay_change_state (GstElement *
    element, GstStateChange transition);

static GstBuffer *gst_rtp_qdm2_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
gboolean gst_rtp_qdm2_depay_setcaps (GstRTPBaseDepayload * filter,
    GstCaps * caps);

static void
gst_rtp_qdm2_depay_class_init (GstRtpQDM2DepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_qdm2_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_qdm2_depay_setcaps;

  gobject_class->finalize = gst_rtp_qdm2_depay_finalize;

  gstelement_class->change_state = gst_rtp_qdm2_depay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_qdm2_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_qdm2_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP QDM2 depayloader",
      "Codec/Depayloader/Network/RTP",
      "Extracts QDM2 audio from RTP packets (no RFC)",
      "Edward Hervey <bilboed@bilboed.com>");
}

static void
gst_rtp_qdm2_depay_init (GstRtpQDM2Depay * rtpqdm2depay)
{
  rtpqdm2depay->adapter = gst_adapter_new ();
}

static void
gst_rtp_qdm2_depay_finalize (GObject * object)
{
  GstRtpQDM2Depay *rtpqdm2depay;

  rtpqdm2depay = GST_RTP_QDM2_DEPAY (object);

  g_object_unref (rtpqdm2depay->adapter);
  rtpqdm2depay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* only on the sink */
gboolean
gst_rtp_qdm2_depay_setcaps (GstRTPBaseDepayload * filter, GstCaps * caps)
{
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  gint clock_rate;

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 44100;         /* default */
  filter->clock_rate = clock_rate;

  /* will set caps later */

  return TRUE;
}

static void
flush_data (GstRtpQDM2Depay * depay)
{
  guint i;
  guint avail;

  if ((avail = gst_adapter_available (depay->adapter)))
    gst_adapter_flush (depay->adapter, avail);

  GST_DEBUG ("Flushing %d packets", depay->nbpackets);

  for (i = 0; depay->packets[i]; i++) {
    QDM2Packet *pack = depay->packets[i];
    guint32 crc = 0;
    int i = 0;
    GstBuffer *buf;
    guint8 *data;

    /* CRC is the sum of everything (including first bytes) */

    data = pack->data;

    if (G_UNLIKELY (data == NULL))
      continue;

    /* If the packet size is bigger than 0xff, we need 2 bytes to store the size */
    if (depay->packetsize > 0xff) {
      /* Expanded size 0x02 | 0x80 */
      data[0] = 0x82;
      GST_WRITE_UINT16_BE (data + 1, depay->packetsize - 3);
    } else {
      data[0] = 0x2;
      data[1] = depay->packetsize - 2;
    }

    /* Calculate CRC */
    for (; i < depay->packetsize; i++)
      crc += data[i];

    GST_DEBUG ("CRC is 0x%x", crc);

    /* Write CRC */
    if (depay->packetsize > 0xff)
      GST_WRITE_UINT16_BE (data + 3, crc);
    else
      GST_WRITE_UINT16_BE (data + 2, crc);

    GST_MEMDUMP ("Extracted packet", data, depay->packetsize);

    buf = gst_buffer_new ();
    gst_buffer_append_memory (buf,
        gst_memory_new_wrapped (0, data, depay->packetsize, 0,
            depay->packetsize, data, g_free));

    gst_adapter_push (depay->adapter, buf);

    pack->data = NULL;
  }
}

static void
add_packet (GstRtpQDM2Depay * depay, guint32 pid, guint32 len, guint8 * data)
{
  QDM2Packet *packet;

  if (G_UNLIKELY (!depay->configured))
    return;

  GST_DEBUG ("pid:%d, len:%d, data:%p", pid, len, data);

  if (G_UNLIKELY (depay->packets[pid] == NULL)) {
    depay->packets[pid] = g_malloc0 (sizeof (QDM2Packet));
    depay->nbpackets = MAX (depay->nbpackets, pid + 1);
  }
  packet = depay->packets[pid];

  GST_DEBUG ("packet:%p", packet);
  GST_DEBUG ("packet->data:%p", packet->data);

  if (G_UNLIKELY (packet->data == NULL)) {
    packet->data = g_malloc0 (depay->packetsize);
    /* We leave space for the header/crc */
    if (depay->packetsize > 0xff)
      packet->offs = 5;
    else
      packet->offs = 4;
  }

  /* Finally copy the data over */
  memcpy (packet->data + packet->offs, data, len);
  packet->offs += len;
}

static GstBuffer *
gst_rtp_qdm2_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpQDM2Depay *rtpqdm2depay;
  GstBuffer *outbuf = NULL;
  guint16 seq;

  rtpqdm2depay = GST_RTP_QDM2_DEPAY (depayload);

  {
    gint payload_len;
    guint8 *payload;
    guint avail;
    guint pos = 0;

    payload_len = gst_rtp_buffer_get_payload_len (rtp);
    if (payload_len < 3)
      goto bad_packet;

    payload = gst_rtp_buffer_get_payload (rtp);
    seq = gst_rtp_buffer_get_seq (rtp);
    if (G_UNLIKELY (seq != rtpqdm2depay->nextseq)) {
      GST_DEBUG ("GAP in sequence number, Resetting data !");
      /* Flush previous data */
      flush_data (rtpqdm2depay);
      /* And store new timestamp */
      rtpqdm2depay->ptimestamp = rtpqdm2depay->timestamp;
      rtpqdm2depay->timestamp = GST_BUFFER_PTS (rtp->buffer);
      /* And that previous data will be pushed at the bottom */
    }
    rtpqdm2depay->nextseq = seq + 1;

    GST_DEBUG ("Payload size %d 0x%x sequence:%d", payload_len, payload_len,
        seq);

    GST_MEMDUMP ("Incoming payload", payload, payload_len);

    while (pos < payload_len) {
      switch (payload[pos]) {
        case 0x80:{
          GST_DEBUG ("Unrecognized 0x80 marker, skipping 12 bytes");
          pos += 12;
        }
          break;
        case 0xff:
          /* HEADERS */
          GST_DEBUG ("Headers");
          /* Store the incoming timestamp */
          rtpqdm2depay->ptimestamp = rtpqdm2depay->timestamp;
          rtpqdm2depay->timestamp = GST_BUFFER_PTS (rtp->buffer);
          /* flush the internal data if needed */
          flush_data (rtpqdm2depay);
          if (G_UNLIKELY (!rtpqdm2depay->configured)) {
            guint8 *ourdata;
            GstBuffer *codecdata;
            GstMapInfo cmap;
            GstCaps *caps;

            /* First bytes are unknown */
            GST_MEMDUMP ("Header", payload + pos, 32);
            ourdata = payload + pos + 10;
            pos += 10;
            rtpqdm2depay->channs = GST_READ_UINT32_BE (payload + pos + 4);
            rtpqdm2depay->samplerate = GST_READ_UINT32_BE (payload + pos + 8);
            rtpqdm2depay->bitrate = GST_READ_UINT32_BE (payload + pos + 12);
            rtpqdm2depay->blocksize = GST_READ_UINT32_BE (payload + pos + 16);
            rtpqdm2depay->framesize = GST_READ_UINT32_BE (payload + pos + 20);
            rtpqdm2depay->packetsize = GST_READ_UINT32_BE (payload + pos + 24);
            /* 16 bit empty block (0x02 0x00) */
            pos += 30;
            GST_DEBUG
                ("channs:%d, samplerate:%d, bitrate:%d, blocksize:%d, framesize:%d, packetsize:%d",
                rtpqdm2depay->channs, rtpqdm2depay->samplerate,
                rtpqdm2depay->bitrate, rtpqdm2depay->blocksize,
                rtpqdm2depay->framesize, rtpqdm2depay->packetsize);

            /* Caps */
            codecdata = gst_buffer_new_and_alloc (48);
            gst_buffer_map (codecdata, &cmap, GST_MAP_WRITE);
            memcpy (cmap.data, headheader, 20);
            memcpy (cmap.data + 20, ourdata, 28);
            gst_buffer_unmap (codecdata, &cmap);

            caps = gst_caps_new_simple ("audio/x-qdm2",
                "samplesize", G_TYPE_INT, 16,
                "rate", G_TYPE_INT, rtpqdm2depay->samplerate,
                "channels", G_TYPE_INT, rtpqdm2depay->channs,
                "codec_data", GST_TYPE_BUFFER, codecdata, NULL);
            gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), caps);
            gst_caps_unref (caps);
            rtpqdm2depay->configured = TRUE;
          } else {
            GST_DEBUG ("Already configured, skipping headers");
            pos += 40;
          }
          break;
        default:{
          /* Shuffled packet contents */
          guint packetid = payload[pos++];
          guint packettype = payload[pos++];
          guint packlen = payload[pos++];
          guint hsize = 2;

          GST_DEBUG ("Packet id:%d, type:0x%x, len:%d",
              packetid, packettype, packlen);

          /* Packets bigger than 0xff bytes have a type with the high bit set */
          if (G_UNLIKELY (packettype & 0x80)) {
            packettype &= 0x7f;
            packlen <<= 8;
            packlen |= payload[pos++];
            hsize = 3;
            GST_DEBUG ("Packet id:%d, type:0x%x, len:%d",
                packetid, packettype, packlen);
          }

          if (packettype > 0x7f) {
            GST_ERROR ("HOUSTON WE HAVE A PROBLEM !!!!");
          }
          add_packet (rtpqdm2depay, packetid, packlen + hsize,
              payload + pos - hsize);
          pos += packlen;
        }
      }
    }

    GST_DEBUG ("final pos %d", pos);

    avail = gst_adapter_available (rtpqdm2depay->adapter);
    if (G_UNLIKELY (avail)) {
      GST_DEBUG ("Pushing out %d bytes of collected data", avail);
      outbuf = gst_adapter_take_buffer (rtpqdm2depay->adapter, avail);
      GST_BUFFER_PTS (outbuf) = rtpqdm2depay->ptimestamp;
      GST_DEBUG ("Outgoing buffer timestamp %" GST_TIME_FORMAT,
          GST_TIME_ARGS (rtpqdm2depay->ptimestamp));
    }
  }

  return outbuf;

  /* ERRORS */
bad_packet:
  {
    GST_ELEMENT_WARNING (rtpqdm2depay, STREAM, DECODE,
        (NULL), ("Packet was too short"));
    return NULL;
  }
}

static GstStateChangeReturn
gst_rtp_qdm2_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpQDM2Depay *rtpqdm2depay;
  GstStateChangeReturn ret;

  rtpqdm2depay = GST_RTP_QDM2_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_adapter_clear (rtpqdm2depay->adapter);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_qdm2_depay_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (rtpqdm2depay_debug, "rtpqdm2depay", 0,
      "RTP QDM2 depayloader");

  return gst_element_register (plugin, "rtpqdm2depay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_QDM2_DEPAY);
}
