/* GStreamer
 * Copyright (C) <2006> Wim Taymans <wim.taymans@gmail.com>
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

#include <gst/tag/tag.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include <string.h>
#include "gstrtpvorbisdepay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpvorbisdepay_debug);
#define GST_CAT_DEFAULT (rtpvorbisdepay_debug)

/* references:
 * http://www.rfc-editor.org/rfc/rfc5215.txt
 */

static GstStaticPadTemplate gst_rtp_vorbis_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) [1, MAX ], " "encoding-name = (string) \"VORBIS\""
        /* All required parameters 
         *
         * "encoding-params = (string) <num channels>"
         * "configuration = (string) ANY" 
         */
    )
    );

static GstStaticPadTemplate gst_rtp_vorbis_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-vorbis")
    );

#define gst_rtp_vorbis_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpVorbisDepay, gst_rtp_vorbis_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static gboolean gst_rtp_vorbis_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_vorbis_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

static void gst_rtp_vorbis_depay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_vorbis_depay_change_state (GstElement *
    element, GstStateChange transition);

static void
gst_rtp_vorbis_depay_class_init (GstRtpVorbisDepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_vorbis_depay_finalize;

  gstelement_class->change_state = gst_rtp_vorbis_depay_change_state;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_vorbis_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_vorbis_depay_setcaps;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_vorbis_depay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_vorbis_depay_src_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Vorbis depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts Vorbis Audio from RTP packets (RFC 5215)",
      "Wim Taymans <wim.taymans@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (rtpvorbisdepay_debug, "rtpvorbisdepay", 0,
      "Vorbis RTP Depayloader");
}

static void
gst_rtp_vorbis_depay_init (GstRtpVorbisDepay * rtpvorbisdepay)
{
  rtpvorbisdepay->adapter = gst_adapter_new ();
}

static void
free_config (GstRtpVorbisConfig * conf)
{
  g_list_free_full (conf->headers, (GDestroyNotify) gst_buffer_unref);
  g_free (conf);
}

static void
free_indents (GstRtpVorbisDepay * rtpvorbisdepay)
{
  g_list_free_full (rtpvorbisdepay->configs, (GDestroyNotify) free_config);
  rtpvorbisdepay->configs = NULL;
}

static void
gst_rtp_vorbis_depay_finalize (GObject * object)
{
  GstRtpVorbisDepay *rtpvorbisdepay = GST_RTP_VORBIS_DEPAY (object);

  g_object_unref (rtpvorbisdepay->adapter);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_vorbis_depay_has_ident (GstRtpVorbisDepay * rtpvorbisdepay,
    guint32 ident)
{
  GList *walk;

  for (walk = rtpvorbisdepay->configs; walk; walk = g_list_next (walk)) {
    GstRtpVorbisConfig *conf = (GstRtpVorbisConfig *) walk->data;

    if (conf->ident == ident)
      return TRUE;
  }

  return FALSE;
}

/* takes ownership of confbuf */
static gboolean
gst_rtp_vorbis_depay_parse_configuration (GstRtpVorbisDepay * rtpvorbisdepay,
    GstBuffer * confbuf)
{
  GstBuffer *buf;
  guint32 num_headers;
  GstMapInfo map;
  guint8 *data;
  gsize size;
  guint offset;
  gint i, j;

  gst_buffer_map (confbuf, &map, GST_MAP_READ);
  data = map.data;
  size = map.size;

  GST_DEBUG_OBJECT (rtpvorbisdepay, "config size %" G_GSIZE_FORMAT, size);

  /* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                     Number of packed headers                  |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          Packed header                        |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          Packed header                        |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          ....                                 |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
  if (size < 4)
    goto too_small;

  num_headers = GST_READ_UINT32_BE (data);
  size -= 4;
  data += 4;
  offset = 4;

  GST_DEBUG_OBJECT (rtpvorbisdepay, "have %u headers", num_headers);

  /*  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                   Ident                       | length       ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              | n. of headers |    length1    |    length2   ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              |             Identification Header            ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              |         Comment Header                       ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..                        Comment Header                        |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          Setup Header                        ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..                         Setup Header                         |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
  for (i = 0; i < num_headers; i++) {
    guint32 ident;
    guint16 length;
    guint8 n_headers, b;
    GstRtpVorbisConfig *conf;
    guint *h_sizes;
    guint extra = 1;

    if (size < 6)
      goto too_small;

    ident = (data[0] << 16) | (data[1] << 8) | data[2];
    length = (data[3] << 8) | data[4];
    n_headers = data[5];
    size -= 6;
    data += 6;
    offset += 6;

    GST_DEBUG_OBJECT (rtpvorbisdepay,
        "header %d, ident 0x%08x, length %u, left %" G_GSIZE_FORMAT, i, ident,
        length, size);

    /* FIXME check if we already got this ident */

    /* length might also include count of following size fields */
    if (size < length && size + 1 != length)
      goto too_small;

    if (gst_rtp_vorbis_depay_has_ident (rtpvorbisdepay, ident)) {
      size -= length;
      data += length;
      offset += length;
      continue;
    }

    /* read header sizes we read 2 sizes, the third size (for which we allocate
     * space) must be derived from the total packed header length. */
    h_sizes = g_newa (guint, n_headers + 1);
    for (j = 0; j < n_headers; j++) {
      guint h_size;

      h_size = 0;
      do {
        if (size < 1)
          goto too_small;
        b = *data++;
        offset++;
        extra++;
        size--;
        h_size = (h_size << 7) | (b & 0x7f);
      } while (b & 0x80);
      GST_DEBUG_OBJECT (rtpvorbisdepay, "headers %d: size: %u", j, h_size);

      if (length < h_size)
        goto too_small;

      h_sizes[j] = h_size;
      length -= h_size;
    }
    /* last header length is the remaining space */
    GST_DEBUG_OBJECT (rtpvorbisdepay, "last header size: %u", length);
    h_sizes[j] = length;

    GST_DEBUG_OBJECT (rtpvorbisdepay, "preparing headers");
    conf = g_new0 (GstRtpVorbisConfig, 1);
    conf->ident = ident;

    for (j = 0; j <= n_headers; j++) {
      guint h_size;

      h_size = h_sizes[j];
      if (size < h_size) {
        if (j != n_headers || size + extra != h_size) {
          free_config (conf);
          goto too_small;
        } else {
          /* otherwise means that overall length field contained total length,
           * including extra fields */
          h_size -= extra;
        }
      }

      GST_DEBUG_OBJECT (rtpvorbisdepay, "reading header %d, size %u", j,
          h_size);

      buf = gst_buffer_copy_region (confbuf, GST_BUFFER_COPY_ALL, offset,
          h_size);
      conf->headers = g_list_append (conf->headers, buf);
      offset += h_size;
      size -= h_size;
    }
    rtpvorbisdepay->configs = g_list_append (rtpvorbisdepay->configs, conf);
  }

  gst_buffer_unmap (confbuf, &map);
  gst_buffer_unref (confbuf);

  return TRUE;

  /* ERRORS */
too_small:
  {
    GST_DEBUG_OBJECT (rtpvorbisdepay, "configuration too small");
    gst_buffer_unmap (confbuf, &map);
    gst_buffer_unref (confbuf);
    return FALSE;
  }
}

static gboolean
gst_rtp_vorbis_depay_parse_inband_configuration (GstRtpVorbisDepay *
    rtpvorbisdepay, guint ident, guint8 * configuration, guint size,
    guint length)
{
  GstBuffer *confbuf;
  GstMapInfo map;

  if (G_UNLIKELY (size < 4))
    return FALSE;

  /* transform inline to out-of-band and parse that one */
  confbuf = gst_buffer_new_and_alloc (size + 9);
  gst_buffer_map (confbuf, &map, GST_MAP_WRITE);
  /* 1 header */
  GST_WRITE_UINT32_BE (map.data, 1);
  /* write Ident */
  GST_WRITE_UINT24_BE (map.data + 4, ident);
  /* write sort-of-length */
  GST_WRITE_UINT16_BE (map.data + 7, length);
  /* copy remainder */
  memcpy (map.data + 9, configuration, size);
  gst_buffer_unmap (confbuf, &map);

  return gst_rtp_vorbis_depay_parse_configuration (rtpvorbisdepay, confbuf);
}

static gboolean
gst_rtp_vorbis_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpVorbisDepay *rtpvorbisdepay;
  GstCaps *srccaps;
  const gchar *configuration;
  gint clock_rate;
  gboolean res;

  rtpvorbisdepay = GST_RTP_VORBIS_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  /* get clockrate */
  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    goto no_rate;

  /* read and parse configuration string */
  configuration = gst_structure_get_string (structure, "configuration");
  if (configuration) {
    GstBuffer *confbuf;
    guint8 *data;
    gsize size;

    /* deserialize base64 to buffer */
    data = g_base64_decode (configuration, &size);

    confbuf = gst_buffer_new ();
    gst_buffer_append_memory (confbuf,
        gst_memory_new_wrapped (0, data, size, 0, size, data, g_free));
    if (!gst_rtp_vorbis_depay_parse_configuration (rtpvorbisdepay, confbuf))
      goto invalid_configuration;
  } else {
    GST_WARNING_OBJECT (rtpvorbisdepay, "no configuration specified");
  }

  /* caps seem good, configure element */
  depayload->clock_rate = clock_rate;

  /* set caps on pad and on header */
  srccaps = gst_caps_new_empty_simple ("audio/x-vorbis");
  res = gst_pad_set_caps (depayload->srcpad, srccaps);
  gst_caps_unref (srccaps);

  return res;

  /* ERRORS */
invalid_configuration:
  {
    GST_ERROR_OBJECT (rtpvorbisdepay, "invalid configuration specified");
    return FALSE;
  }
no_rate:
  {
    GST_ERROR_OBJECT (rtpvorbisdepay, "no clock-rate specified");
    return FALSE;
  }
}

static gboolean
gst_rtp_vorbis_depay_switch_codebook (GstRtpVorbisDepay * rtpvorbisdepay,
    guint32 ident)
{
  GList *walk;
  gboolean res = FALSE;

  GST_DEBUG_OBJECT (rtpvorbisdepay, "Looking up code book ident 0x%08x", ident);
  for (walk = rtpvorbisdepay->configs; walk; walk = g_list_next (walk)) {
    GstRtpVorbisConfig *conf = (GstRtpVorbisConfig *) walk->data;

    if (conf->ident == ident) {
      GList *headers;

      /* FIXME, remove pads, create new pad.. */

      /* push out all the headers */
      for (headers = conf->headers; headers; headers = g_list_next (headers)) {
        GstBuffer *header = GST_BUFFER_CAST (headers->data);

        gst_buffer_ref (header);
        gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtpvorbisdepay),
            header);
      }
      /* remember the current config */
      rtpvorbisdepay->config = conf;
      res = TRUE;
    }
  }
  if (!res) {
    /* we don't know about the headers, figure out an alternative method for
     * getting the codebooks. FIXME, fail for now. */
  }
  return res;
}

static GstBuffer *
gst_rtp_vorbis_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp)
{
  GstRtpVorbisDepay *rtpvorbisdepay;
  GstBuffer *outbuf;
  GstFlowReturn ret;
  gint payload_len;
  GstBuffer *payload_buffer = NULL;
  guint8 *payload;
  GstMapInfo map;
  guint32 header, ident;
  guint8 F, VDT, packets;
  guint length;

  rtpvorbisdepay = GST_RTP_VORBIS_DEPAY (depayload);

  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  GST_DEBUG_OBJECT (depayload, "got RTP packet of size %d", payload_len);

  /* we need at least 4 bytes for the packet header */
  if (G_UNLIKELY (payload_len < 4))
    goto packet_short;

  payload = gst_rtp_buffer_get_payload (rtp);
  header = GST_READ_UINT32_BE (payload);
  /*
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                     Ident                     | F |VDT|# pkts.|
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *
   * F: Fragment type (0=none, 1=start, 2=cont, 3=end)
   * VDT: Vorbis data type (0=vorbis, 1=config, 2=comment, 3=reserved)
   * pkts: number of packets.
   */
  VDT = (header & 0x30) >> 4;
  if (G_UNLIKELY (VDT == 3))
    goto ignore_reserved;

  GST_DEBUG_OBJECT (depayload, "header: 0x%08x", header);
  ident = (header >> 8) & 0xffffff;
  F = (header & 0xc0) >> 6;
  packets = (header & 0xf);

  if (VDT == 0) {
    gboolean do_switch = FALSE;

    /* we have a raw payload, find the codebook for the ident */
    if (!rtpvorbisdepay->config) {
      /* we don't have an active codebook, find the codebook and
       * activate it */
      GST_DEBUG_OBJECT (rtpvorbisdepay, "No active codebook, switching");
      do_switch = TRUE;
    } else if (rtpvorbisdepay->config->ident != ident) {
      /* codebook changed */
      GST_DEBUG_OBJECT (rtpvorbisdepay, "codebook changed, switching");
      do_switch = TRUE;
    }
    if (do_switch) {
      if (!gst_rtp_vorbis_depay_switch_codebook (rtpvorbisdepay, ident))
        goto switch_failed;
    }
  }

  GST_DEBUG_OBJECT (depayload, "ident: %u, F: %d, VDT: %d, packets: %d", ident,
      F, VDT, packets);

  /* fragmented packets, assemble */
  if (F != 0) {
    GstBuffer *vdata;

    if (F == 1) {
      /* if we start a packet, clear adapter and start assembling. */
      gst_adapter_clear (rtpvorbisdepay->adapter);
      GST_DEBUG_OBJECT (depayload, "start assemble");
      rtpvorbisdepay->assembling = TRUE;
    }

    if (!rtpvorbisdepay->assembling)
      goto no_output;

    /* skip header and length. */
    vdata = gst_rtp_buffer_get_payload_subbuffer (rtp, 6, -1);

    GST_DEBUG_OBJECT (depayload, "assemble vorbis packet");
    gst_adapter_push (rtpvorbisdepay->adapter, vdata);

    /* packet is not complete, we are done */
    if (F != 3)
      goto no_output;

    /* construct assembled buffer */
    length = gst_adapter_available (rtpvorbisdepay->adapter);
    payload_buffer = gst_adapter_take_buffer (rtpvorbisdepay->adapter, length);
  } else {
    payload_buffer = gst_rtp_buffer_get_payload_subbuffer (rtp, 4, -1);
    length = 0;
  }

  GST_DEBUG_OBJECT (depayload, "assemble done");

  gst_buffer_map (payload_buffer, &map, GST_MAP_READ);
  payload = map.data;
  payload_len = map.size;

  /* we not assembling anymore now */
  rtpvorbisdepay->assembling = FALSE;
  gst_adapter_clear (rtpvorbisdepay->adapter);

  /* payload now points to a length with that many vorbis data bytes.
   * Iterate over the packets and send them out.
   *
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |             length            |          vorbis data         ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..                        vorbis data                           |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |            length             |   next vorbis packet data    ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..                        vorbis data                           |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*
   */
  while (payload_len > 2) {
    /* If length is not 0, we have a reassembled packet for which we
     * calculated the length already and don't have to skip over the
     * length field anymore
     */
    if (length == 0) {
      length = GST_READ_UINT16_BE (payload);
      payload += 2;
      payload_len -= 2;
    }

    GST_DEBUG_OBJECT (depayload, "read length %u, avail: %d", length,
        payload_len);

    /* skip packet if something odd happens */
    if (G_UNLIKELY (length > payload_len))
      goto length_short;

    /* handle in-band configuration */
    if (G_UNLIKELY (VDT == 1)) {
      GST_DEBUG_OBJECT (rtpvorbisdepay, "in-band configuration");
      if (!gst_rtp_vorbis_depay_parse_inband_configuration (rtpvorbisdepay,
              ident, payload, payload_len, length))
        goto invalid_configuration;
      goto no_output;
    }

    /* create buffer for packet */
    outbuf =
        gst_buffer_copy_region (payload_buffer, GST_BUFFER_COPY_ALL,
        payload - map.data, length);

    payload += length;
    payload_len -= length;
    /* make sure to read next length */
    length = 0;

    ret = gst_rtp_base_depayload_push (depayload, outbuf);
    if (ret != GST_FLOW_OK)
      break;
  }

  gst_buffer_unmap (payload_buffer, &map);
  gst_buffer_unref (payload_buffer);

  return NULL;

no_output:
  {
    if (payload_buffer) {
      gst_buffer_unmap (payload_buffer, &map);
      gst_buffer_unref (payload_buffer);
    }
    return NULL;
  }
  /* ERRORS */
switch_failed:
  {
    GST_ELEMENT_WARNING (rtpvorbisdepay, STREAM, DECODE,
        (NULL), ("Could not switch codebooks"));
    return NULL;
  }
packet_short:
  {
    GST_ELEMENT_WARNING (rtpvorbisdepay, STREAM, DECODE,
        (NULL), ("Packet was too short (%d < 4)", payload_len));
    return NULL;
  }
ignore_reserved:
  {
    GST_WARNING_OBJECT (rtpvorbisdepay, "reserved VDT ignored");
    return NULL;
  }
length_short:
  {
    GST_ELEMENT_WARNING (rtpvorbisdepay, STREAM, DECODE,
        (NULL), ("Packet contains invalid data"));
    if (payload_buffer) {
      gst_buffer_unmap (payload_buffer, &map);
      gst_buffer_unref (payload_buffer);
    }
    return NULL;
  }
invalid_configuration:
  {
    /* fatal, as we otherwise risk carrying on without output */
    GST_ELEMENT_ERROR (rtpvorbisdepay, STREAM, DECODE,
        (NULL), ("Packet contains invalid configuration"));
    if (payload_buffer) {
      gst_buffer_unmap (payload_buffer, &map);
      gst_buffer_unref (payload_buffer);
    }
    return NULL;
  }
}

static GstStateChangeReturn
gst_rtp_vorbis_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpVorbisDepay *rtpvorbisdepay;
  GstStateChangeReturn ret;

  rtpvorbisdepay = GST_RTP_VORBIS_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      free_indents (rtpvorbisdepay);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_vorbis_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpvorbisdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_VORBIS_DEPAY);
}
