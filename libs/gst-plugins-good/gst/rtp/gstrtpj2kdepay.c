/* GStreamer
 * Copyright (C) <2009> Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:element-rtpj2kdepay
 * @title: rtpj2kdepay
 *
 * Depayload an RTP-payloaded JPEG 2000 image into RTP packets according to RFC 5371
 * and RFC 5372.
 * For detailed information see: https://datatracker.ietf.org/doc/rfc5371/
 * and https://datatracker.ietf.org/doc/rfc5372/
 */


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>

#include <string.h>
#include "gstrtpj2kcommon.h"
#include "gstrtpj2kdepay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpj2kdepay_debug);
#define GST_CAT_DEFAULT (rtpj2kdepay_debug)

static GstStaticPadTemplate gst_rtp_j2k_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/x-jpc, "
        "colorspace = (string) { sRGB, sYUV, GRAY }")
    );

static GstStaticPadTemplate gst_rtp_j2k_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", " "clock-rate = (int) 90000, "
        GST_RTP_J2K_SAMPLING_LIST ","
        "encoding-name = (string) \"JPEG2000\";"
        "application/x-rtp, "
        "media = (string) \"video\", " "clock-rate = (int) 90000, "
        "colorspace = (string) { sRGB, sYUV, GRAY }, "
        "encoding-name = (string) \"JPEG2000\";")
    );

enum
{
  PROP_0,
  PROP_LAST
};

#define gst_rtp_j2k_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpJ2KDepay, gst_rtp_j2k_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static void gst_rtp_j2k_depay_finalize (GObject * object);

static void gst_rtp_j2k_depay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_j2k_depay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn
gst_rtp_j2k_depay_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_rtp_j2k_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_j2k_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

static void
gst_rtp_j2k_depay_class_init (GstRtpJ2KDepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_j2k_depay_finalize;

  gobject_class->set_property = gst_rtp_j2k_depay_set_property;
  gobject_class->get_property = gst_rtp_j2k_depay_get_property;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_j2k_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_j2k_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP JPEG 2000 depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts JPEG 2000 video from RTP packets (RFC 5371)",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstelement_class->change_state = gst_rtp_j2k_depay_change_state;

  gstrtpbasedepayload_class->set_caps = gst_rtp_j2k_depay_setcaps;
  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_j2k_depay_process;

  GST_DEBUG_CATEGORY_INIT (rtpj2kdepay_debug, "rtpj2kdepay", 0,
      "J2K Video RTP Depayloader");
}

static void
gst_rtp_j2k_depay_init (GstRtpJ2KDepay * rtpj2kdepay)
{
  rtpj2kdepay->pu_adapter = gst_adapter_new ();
  rtpj2kdepay->t_adapter = gst_adapter_new ();
  rtpj2kdepay->f_adapter = gst_adapter_new ();
}

static void
store_mheader (GstRtpJ2KDepay * rtpj2kdepay, guint idx, GstBuffer * buf)
{
  GstBuffer *old;

  GST_DEBUG_OBJECT (rtpj2kdepay, "storing main header %p at index %u", buf,
      idx);
  if ((old = rtpj2kdepay->MH[idx]))
    gst_buffer_unref (old);
  rtpj2kdepay->MH[idx] = buf;
}

static void
clear_mheaders (GstRtpJ2KDepay * rtpj2kdepay)
{
  guint i;

  for (i = 0; i < 8; i++)
    store_mheader (rtpj2kdepay, i, NULL);
}

static void
gst_rtp_j2k_depay_reset (GstRtpJ2KDepay * rtpj2kdepay)
{
  clear_mheaders (rtpj2kdepay);
  gst_adapter_clear (rtpj2kdepay->pu_adapter);
  gst_adapter_clear (rtpj2kdepay->t_adapter);
  gst_adapter_clear (rtpj2kdepay->f_adapter);
  rtpj2kdepay->next_frag = 0;
}

static void
gst_rtp_j2k_depay_finalize (GObject * object)
{
  GstRtpJ2KDepay *rtpj2kdepay;

  rtpj2kdepay = GST_RTP_J2K_DEPAY (object);

  clear_mheaders (rtpj2kdepay);

  g_object_unref (rtpj2kdepay->pu_adapter);
  g_object_unref (rtpj2kdepay->t_adapter);
  g_object_unref (rtpj2kdepay->f_adapter);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_j2k_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure = NULL;
  gint clock_rate;
  GstCaps *outcaps = NULL;
  gboolean res = FALSE;
  const gchar *colorspace = NULL;
  const gchar *sampling = NULL;

  structure = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;
  depayload->clock_rate = clock_rate;

  sampling = gst_structure_get_string (structure, "sampling");
  if (sampling) {
    if (!strcmp (sampling, GST_RTP_J2K_RGB) ||
        !strcmp (sampling, GST_RTP_J2K_RGBA) ||
        !strcmp (sampling, GST_RTP_J2K_BGR) ||
        !strcmp (sampling, GST_RTP_J2K_BGRA))
      colorspace = "sRGB";
    else if (!strcmp (sampling, GST_RTP_J2K_GRAYSCALE))
      colorspace = "GRAY";
    else
      colorspace = "sYUV";
  } else {
    GST_ELEMENT_WARNING (depayload, STREAM, DEMUX, NULL,
        ("Non-compliant stream: sampling field missing. Frames my appear incorrect"));
    colorspace = gst_structure_get_string (structure, "colorspace");
    if (!strcmp (colorspace, "GRAY")) {
      sampling = GST_RTP_J2K_GRAYSCALE;
    }
  }

  outcaps = gst_caps_new_simple ("image/x-jpc",
      "framerate", GST_TYPE_FRACTION, 0, 1,
      "fields", G_TYPE_INT, 1, "colorspace", G_TYPE_STRING, colorspace, NULL);

  if (sampling)
    gst_caps_set_simple (outcaps, "sampling", G_TYPE_STRING, sampling, NULL);

  res = gst_pad_set_caps (depayload->srcpad, outcaps);

  gst_caps_unref (outcaps);

  return res;
}

static void
gst_rtp_j2k_depay_clear_pu (GstRtpJ2KDepay * rtpj2kdepay)
{
  gst_adapter_clear (rtpj2kdepay->pu_adapter);
  rtpj2kdepay->have_sync = FALSE;
}

static GstFlowReturn
gst_rtp_j2k_depay_flush_pu (GstRTPBaseDepayload * depayload)
{
  GstRtpJ2KDepay *rtpj2kdepay;
  GstBuffer *mheader;
  guint avail, MHF, mh_id;

  rtpj2kdepay = GST_RTP_J2K_DEPAY (depayload);

  /* take all available buffers */
  avail = gst_adapter_available (rtpj2kdepay->pu_adapter);
  if (avail == 0)
    goto done;

  MHF = rtpj2kdepay->pu_MHF;
  mh_id = rtpj2kdepay->last_mh_id;

  GST_DEBUG_OBJECT (rtpj2kdepay, "flushing PU of size %u", avail);

  if (MHF == 0) {
    GList *packets, *walk;

    packets = gst_adapter_take_list (rtpj2kdepay->pu_adapter, avail);
    /* append packets */
    for (walk = packets; walk; walk = g_list_next (walk)) {
      GstBuffer *buf = GST_BUFFER_CAST (walk->data);
      GST_DEBUG_OBJECT (rtpj2kdepay,
          "append pu packet of size %" G_GSIZE_FORMAT,
          gst_buffer_get_size (buf));
      gst_adapter_push (rtpj2kdepay->t_adapter, buf);
    }
    g_list_free (packets);
  } else {
    /* we have a header */
    GST_DEBUG_OBJECT (rtpj2kdepay, "keeping header %u", mh_id);
    /* we managed to see the start and end of the header, take all from
     * adapter and keep in header  */
    mheader = gst_adapter_take_buffer (rtpj2kdepay->pu_adapter, avail);

    store_mheader (rtpj2kdepay, mh_id, mheader);
  }

done:
  rtpj2kdepay->have_sync = FALSE;

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_rtp_j2k_depay_flush_tile (GstRTPBaseDepayload * depayload)
{
  GstRtpJ2KDepay *rtpj2kdepay;
  guint avail, mh_id;
  GList *packets, *walk;
  guint8 end[2];
  GstFlowReturn ret = GST_FLOW_OK;
  GstMapInfo map;
  GstBuffer *buf;

  rtpj2kdepay = GST_RTP_J2K_DEPAY (depayload);

  /* flush pending PU */
  gst_rtp_j2k_depay_flush_pu (depayload);

  /* take all available buffers */
  avail = gst_adapter_available (rtpj2kdepay->t_adapter);
  if (avail == 0)
    goto done;

  mh_id = rtpj2kdepay->last_mh_id;

  GST_DEBUG_OBJECT (rtpj2kdepay, "flushing tile of size %u", avail);

  if (gst_adapter_available (rtpj2kdepay->f_adapter) == 0) {
    GstBuffer *mheader;

    /* we need a header now */
    if ((mheader = rtpj2kdepay->MH[mh_id]) == NULL)
      goto waiting_header;

    /* push header in the adapter */
    GST_DEBUG_OBJECT (rtpj2kdepay, "pushing header %u", mh_id);
    gst_adapter_push (rtpj2kdepay->f_adapter, gst_buffer_ref (mheader));
  }

  /* check for last bytes */
  gst_adapter_copy (rtpj2kdepay->t_adapter, end, avail - 2, 2);

  /* now append the tile packets to the frame */
  packets = gst_adapter_take_list (rtpj2kdepay->t_adapter, avail);
  for (walk = packets; walk; walk = g_list_next (walk)) {
    buf = GST_BUFFER_CAST (walk->data);

    if (walk == packets) {
      /* first buffer should contain the SOT */
      gst_buffer_map (buf, &map, GST_MAP_READ);

      if (map.size < 12)
        goto invalid_tile;

      if (map.data[0] == GST_J2K_MARKER && map.data[1] == GST_J2K_MARKER_SOT) {
        guint Psot, nPsot;

        if (end[0] == GST_J2K_MARKER && end[1] == GST_J2K_MARKER_EOC)
          nPsot = avail - 2;
        else
          nPsot = avail;

        Psot = GST_READ_UINT32_BE (&map.data[6]);
        if (Psot != nPsot && Psot != 0) {
          /* Psot must match the size of the tile */
          GST_DEBUG_OBJECT (rtpj2kdepay, "set Psot from %u to %u", Psot, nPsot);
          gst_buffer_unmap (buf, &map);

          buf = gst_buffer_make_writable (buf);

          gst_buffer_map (buf, &map, GST_MAP_WRITE);
          GST_WRITE_UINT32_BE (&map.data[6], nPsot);
        }
      }
      gst_buffer_unmap (buf, &map);
    }

    GST_DEBUG_OBJECT (rtpj2kdepay, "append pu packet of size %" G_GSIZE_FORMAT,
        gst_buffer_get_size (buf));
    gst_adapter_push (rtpj2kdepay->f_adapter, buf);
  }
  g_list_free (packets);

done:
  rtpj2kdepay->last_tile = -1;

  return ret;

  /* errors */
waiting_header:
  {
    GST_DEBUG_OBJECT (rtpj2kdepay, "waiting for header %u", mh_id);
    gst_adapter_clear (rtpj2kdepay->t_adapter);
    rtpj2kdepay->last_tile = -1;
    return ret;
  }
invalid_tile:
  {
    GST_ELEMENT_WARNING (rtpj2kdepay, STREAM, DECODE, ("Invalid tile"), (NULL));
    gst_buffer_unmap (buf, &map);
    gst_adapter_clear (rtpj2kdepay->t_adapter);
    rtpj2kdepay->last_tile = -1;
    return ret;
  }
}

static GstFlowReturn
gst_rtp_j2k_depay_flush_frame (GstRTPBaseDepayload * depayload)
{
  GstRtpJ2KDepay *rtpj2kdepay;
  guint8 end[2];
  guint avail;

  GstFlowReturn ret = GST_FLOW_OK;

  rtpj2kdepay = GST_RTP_J2K_DEPAY (depayload);

  /* flush pending tile */
  gst_rtp_j2k_depay_flush_tile (depayload);

  /* last buffer take all data out of the adapter */
  avail = gst_adapter_available (rtpj2kdepay->f_adapter);
  if (avail == 0)
    goto done;

  if (avail > 2) {
    GstBuffer *outbuf;

    /* take the last bytes of the JPEG 2000 data to see if there is an EOC
     * marker */
    gst_adapter_copy (rtpj2kdepay->f_adapter, end, avail - 2, 2);

    if (end[0] != GST_J2K_MARKER && end[1] != GST_J2K_MARKER_EOC) {
      end[0] = GST_J2K_MARKER;
      end[1] = GST_J2K_MARKER_EOC;

      GST_DEBUG_OBJECT (rtpj2kdepay, "no EOC marker, adding one");

      /* no EOI marker, add one */
      outbuf = gst_buffer_new_and_alloc (2);
      gst_buffer_fill (outbuf, 0, end, 2);

      gst_adapter_push (rtpj2kdepay->f_adapter, outbuf);
      avail += 2;
    }

    GST_DEBUG_OBJECT (rtpj2kdepay, "pushing buffer of %u bytes", avail);
    outbuf = gst_adapter_take_buffer (rtpj2kdepay->f_adapter, avail);
    gst_rtp_drop_non_video_meta (depayload, outbuf);
    ret = gst_rtp_base_depayload_push (depayload, outbuf);
  } else {
    GST_WARNING_OBJECT (rtpj2kdepay, "empty packet");
    gst_adapter_clear (rtpj2kdepay->f_adapter);
  }

  /* we accept any mh_id now */
  rtpj2kdepay->last_mh_id = -1;

  /* reset state */
  rtpj2kdepay->next_frag = 0;
  rtpj2kdepay->have_sync = FALSE;

done:
  /* we can't keep headers with mh_id of 0 */
  store_mheader (rtpj2kdepay, 0, NULL);

  return ret;
}

static GstBuffer *
gst_rtp_j2k_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpJ2KDepay *rtpj2kdepay;
  guint8 *payload;
  guint MHF, mh_id, frag_offset, tile, payload_len, j2klen;
  gint gap;
  guint32 rtptime;

  rtpj2kdepay = GST_RTP_J2K_DEPAY (depayload);

  payload = gst_rtp_buffer_get_payload (rtp);
  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  /* we need at least a header */
  if (payload_len < GST_RTP_J2K_HEADER_SIZE)
    goto empty_packet;

  rtptime = gst_rtp_buffer_get_timestamp (rtp);

  /* new timestamp marks new frame */
  if (rtpj2kdepay->last_rtptime != rtptime) {
    rtpj2kdepay->last_rtptime = rtptime;
    /* flush pending frame */
    gst_rtp_j2k_depay_flush_frame (depayload);
  }

  /*
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |tp |MHF|mh_id|T|     priority  |           tile number         |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |reserved       |             fragment offset                   |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
  MHF = (payload[0] & 0x30) >> 4;
  mh_id = (payload[0] & 0xe) >> 1;

  if (rtpj2kdepay->last_mh_id == -1)
    rtpj2kdepay->last_mh_id = mh_id;
  else if (rtpj2kdepay->last_mh_id != mh_id)
    goto wrong_mh_id;

  tile = (payload[2] << 8) | payload[3];
  frag_offset = (payload[5] << 16) | (payload[6] << 8) | payload[7];
  j2klen = payload_len - GST_RTP_J2K_HEADER_SIZE;

  GST_DEBUG_OBJECT (rtpj2kdepay, "MHF %u, tile %u, frag %u, expected %u", MHF,
      tile, frag_offset, rtpj2kdepay->next_frag);

  /* calculate the gap between expected frag */
  gap = frag_offset - rtpj2kdepay->next_frag;
  /* calculate next frag */
  rtpj2kdepay->next_frag = frag_offset + j2klen;

  if (gap != 0) {
    GST_DEBUG_OBJECT (rtpj2kdepay, "discont of %d, clear PU", gap);
    /* discont, clear pu adapter and resync */
    gst_rtp_j2k_depay_clear_pu (rtpj2kdepay);
  }

  /* check for sync code */
  if (j2klen > 2 && payload[GST_RTP_J2K_HEADER_SIZE] == GST_J2K_MARKER) {
    guint marker = payload[GST_RTP_J2K_HEADER_SIZE + 1];

    /* packets must start with SOC, SOT or SOP */
    switch (marker) {
      case GST_J2K_MARKER_SOC:
        GST_DEBUG_OBJECT (rtpj2kdepay, "found SOC packet");
        /* flush the previous frame, should have happened when the timestamp
         * changed above. */
        gst_rtp_j2k_depay_flush_frame (depayload);
        rtpj2kdepay->have_sync = TRUE;
        break;
      case GST_J2K_MARKER_SOT:
        /* flush the previous tile */
        gst_rtp_j2k_depay_flush_tile (depayload);
        GST_DEBUG_OBJECT (rtpj2kdepay, "found SOT packet");
        rtpj2kdepay->have_sync = TRUE;
        /* we sync on the tile now */
        rtpj2kdepay->last_tile = tile;
        break;
      case GST_J2K_MARKER_SOP:
        GST_DEBUG_OBJECT (rtpj2kdepay, "found SOP packet");
        /* flush the previous PU */
        gst_rtp_j2k_depay_flush_pu (depayload);
        if (rtpj2kdepay->last_tile != tile) {
          /* wrong tile, we lose sync and we need a new SOT or SOC to regain
           * sync. First flush out the previous tile if we have one. */
          if (rtpj2kdepay->last_tile != -1)
            gst_rtp_j2k_depay_flush_tile (depayload);
          /* now we have no more valid tile and no sync */
          rtpj2kdepay->last_tile = -1;
          rtpj2kdepay->have_sync = FALSE;
        } else {
          rtpj2kdepay->have_sync = TRUE;
        }
        break;
      default:
        GST_DEBUG_OBJECT (rtpj2kdepay, "no sync packet 0x%02d", marker);
        break;
    }
  }

  if (rtpj2kdepay->have_sync) {
    GstBuffer *pu_frag;

    if (gst_adapter_available (rtpj2kdepay->pu_adapter) == 0) {
      /* first part of pu, record state */
      GST_DEBUG_OBJECT (rtpj2kdepay, "first PU");
      rtpj2kdepay->pu_MHF = MHF;
    }
    /* and push in pu adapter */
    GST_DEBUG_OBJECT (rtpj2kdepay, "push pu of size %u in adapter", j2klen);
    pu_frag = gst_rtp_buffer_get_payload_subbuffer (rtp, 8, -1);
    gst_adapter_push (rtpj2kdepay->pu_adapter, pu_frag);

    if (MHF & 2) {
      /* last part of main header received, we can flush it */
      GST_DEBUG_OBJECT (rtpj2kdepay, "header end, flush pu");
      gst_rtp_j2k_depay_flush_pu (depayload);
    }
  } else {
    GST_DEBUG_OBJECT (rtpj2kdepay, "discard packet, no sync");
  }

  /* marker bit finishes the frame */
  if (gst_rtp_buffer_get_marker (rtp)) {
    GST_DEBUG_OBJECT (rtpj2kdepay, "marker set, last buffer");
    /* then flush frame */
    gst_rtp_j2k_depay_flush_frame (depayload);
  }

  return NULL;

  /* ERRORS */
empty_packet:
  {
    GST_ELEMENT_WARNING (rtpj2kdepay, STREAM, DECODE,
        ("Empty Payload."), (NULL));
    return NULL;
  }
wrong_mh_id:
  {
    GST_ELEMENT_WARNING (rtpj2kdepay, STREAM, DECODE,
        ("Invalid mh_id %u, expected %u", mh_id, rtpj2kdepay->last_mh_id),
        (NULL));
    gst_rtp_j2k_depay_clear_pu (rtpj2kdepay);
    return NULL;
  }
}

static void
gst_rtp_j2k_depay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_j2k_depay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_rtp_j2k_depay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpJ2KDepay *rtpj2kdepay;
  GstStateChangeReturn ret;

  rtpj2kdepay = GST_RTP_J2K_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_j2k_depay_reset (rtpj2kdepay);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_j2k_depay_reset (rtpj2kdepay);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_j2k_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpj2kdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_J2K_DEPAY);
}
