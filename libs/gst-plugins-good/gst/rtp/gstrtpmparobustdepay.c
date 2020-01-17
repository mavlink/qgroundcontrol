/* GStreamer
 * Copyright (C) <2010> Mark Nauwelaerts <mark.nauwelaerts@collabora.co.uk>
 * Copyright (C) <2010> Nokia Corporation
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

#include <gst/rtp/gstrtpbuffer.h>

#include <stdio.h>
#include <string.h>
#include "gstrtpmparobustdepay.h"

GST_DEBUG_CATEGORY_STATIC (rtpmparobustdepay_debug);
#define GST_CAT_DEFAULT (rtpmparobustdepay_debug)

static GstStaticPadTemplate gst_rtp_mpa_robust_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, " "mpegversion = (int) 1")
    );

static GstStaticPadTemplate gst_rtp_mpa_robust_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 90000, "
        "encoding-name = (string) \"MPA-ROBUST\" " "; "
        /* draft versions appear still in use out there */
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) [1, MAX], "
        "encoding-name = (string) { \"X-MP3-DRAFT-00\", \"X-MP3-DRAFT-01\", "
        " \"X-MP3-DRAFT-02\", \"X-MP3-DRAFT-03\", \"X-MP3-DRAFT-04\", "
        " \"X-MP3-DRAFT-05\", \"X-MP3-DRAFT-06\" }")
    );

typedef struct _GstADUFrame
{
  guint32 header;
  gint size;
  gint side_info;
  gint data_size;
  gint layer;
  gint backpointer;

  GstBuffer *buffer;
} GstADUFrame;

#define gst_rtp_mpa_robust_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpMPARobustDepay, gst_rtp_mpa_robust_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static GstStateChangeReturn gst_rtp_mpa_robust_change_state (GstElement *
    element, GstStateChange transition);

static gboolean gst_rtp_mpa_robust_depay_setcaps (GstRTPBaseDepayload *
    depayload, GstCaps * caps);
static GstBuffer *gst_rtp_mpa_robust_depay_process (GstRTPBaseDepayload *
    depayload, GstRTPBuffer * rtp);

static void
gst_rtp_mpa_robust_depay_finalize (GObject * object)
{
  GstRtpMPARobustDepay *rtpmpadepay;

  rtpmpadepay = (GstRtpMPARobustDepay *) object;

  g_object_unref (rtpmpadepay->adapter);
  g_queue_free (rtpmpadepay->adu_frames);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_mpa_robust_depay_class_init (GstRtpMPARobustDepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpmparobustdepay_debug, "rtpmparobustdepay", 0,
      "Robust MPEG Audio RTP Depayloader");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_mpa_robust_depay_finalize;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_mpa_robust_change_state);

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mpa_robust_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mpa_robust_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP MPEG audio depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts MPEG audio from RTP packets (RFC 5219)",
      "Mark Nauwelaerts <mark.nauwelaerts@collabora.co.uk>");

  gstrtpbasedepayload_class->set_caps = gst_rtp_mpa_robust_depay_setcaps;
  gstrtpbasedepayload_class->process_rtp_packet =
      gst_rtp_mpa_robust_depay_process;
}

static void
gst_rtp_mpa_robust_depay_init (GstRtpMPARobustDepay * rtpmpadepay)
{
  rtpmpadepay->adapter = gst_adapter_new ();
  rtpmpadepay->adu_frames = g_queue_new ();
}

static gboolean
gst_rtp_mpa_robust_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps)
{
  GstRtpMPARobustDepay *rtpmpadepay;
  GstStructure *structure;
  GstCaps *outcaps;
  gint clock_rate, draft;
  gboolean res;
  const gchar *encoding;

  rtpmpadepay = GST_RTP_MPA_ROBUST_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;
  depayload->clock_rate = clock_rate;

  rtpmpadepay->has_descriptor = TRUE;
  if ((encoding = gst_structure_get_string (structure, "encoding-name"))) {
    if (sscanf (encoding, "X-MP3-DRAFT-%d", &draft) && (draft == 0))
      rtpmpadepay->has_descriptor = FALSE;
  }

  outcaps =
      gst_caps_new_simple ("audio/mpeg", "mpegversion", G_TYPE_INT, 1, NULL);
  res = gst_pad_set_caps (depayload->srcpad, outcaps);
  gst_caps_unref (outcaps);

  return res;
}

/* thanks again go to mp3parse ... */

static const guint mp3types_bitrates[2][3][16] = {
  {
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,},
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,},
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320,}
      },
  {
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256,},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,}
      },
};

static const guint mp3types_freqs[3][3] = { {44100, 48000, 32000},
{22050, 24000, 16000},
{11025, 12000, 8000}
};

static inline guint
mp3_type_frame_length_from_header (GstElement * mp3parse, guint32 header,
    guint * put_version, guint * put_layer, guint * put_channels,
    guint * put_bitrate, guint * put_samplerate, guint * put_mode,
    guint * put_crc)
{
  guint length;
  gulong mode, samplerate, bitrate, layer, channels, padding, crc;
  gulong version;
  gint lsf, mpg25;

  if (header & (1 << 20)) {
    lsf = (header & (1 << 19)) ? 0 : 1;
    mpg25 = 0;
  } else {
    lsf = 1;
    mpg25 = 1;
  }

  version = 1 + lsf + mpg25;

  layer = 4 - ((header >> 17) & 0x3);

  crc = (header >> 16) & 0x1;

  bitrate = (header >> 12) & 0xF;
  bitrate = mp3types_bitrates[lsf][layer - 1][bitrate] * 1000;
  /* The caller has ensured we have a valid header, so bitrate can't be
     zero here. */
  if (bitrate == 0) {
    GST_DEBUG_OBJECT (mp3parse, "invalid bitrate");
    return 0;
  }

  samplerate = (header >> 10) & 0x3;
  samplerate = mp3types_freqs[lsf + mpg25][samplerate];

  padding = (header >> 9) & 0x1;

  mode = (header >> 6) & 0x3;
  channels = (mode == 3) ? 1 : 2;

  switch (layer) {
    case 1:
      length = 4 * ((bitrate * 12) / samplerate + padding);
      break;
    case 2:
      length = (bitrate * 144) / samplerate + padding;
      break;
    default:
    case 3:
      length = (bitrate * 144) / (samplerate << lsf) + padding;
      break;
  }

  GST_LOG_OBJECT (mp3parse, "Calculated mp3 frame length of %u bytes", length);
  GST_LOG_OBJECT (mp3parse, "samplerate = %lu, bitrate = %lu, version = %lu, "
      "layer = %lu, channels = %lu, mode = %lu", samplerate, bitrate, version,
      layer, channels, mode);

  if (put_version)
    *put_version = version;
  if (put_layer)
    *put_layer = layer;
  if (put_channels)
    *put_channels = channels;
  if (put_bitrate)
    *put_bitrate = bitrate;
  if (put_samplerate)
    *put_samplerate = samplerate;
  if (put_mode)
    *put_mode = mode;
  if (put_crc)
    *put_crc = crc;

  GST_LOG_OBJECT (mp3parse, "size = %u", length);
  return length;
}

/* generate empty/silent/dummy frame that mimics @frame,
 * except for rate, where maximum possible is selected */
static GstADUFrame *
gst_rtp_mpa_robust_depay_generate_dummy_frame (GstRtpMPARobustDepay *
    rtpmpadepay, GstADUFrame * frame)
{
  GstADUFrame *dummy;
  GstMapInfo map;

  dummy = g_slice_dup (GstADUFrame, frame);

  /* go for maximum bitrate */
  dummy->header = (frame->header & ~(0xf << 12)) | (0xe << 12);
  dummy->size =
      mp3_type_frame_length_from_header (GST_ELEMENT_CAST (rtpmpadepay),
      dummy->header, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  dummy->data_size = dummy->size - 4 - dummy->side_info;
  dummy->backpointer = 0;

  dummy->buffer = gst_buffer_new_and_alloc (dummy->side_info + 4);

  gst_buffer_map (dummy->buffer, &map, GST_MAP_WRITE);
  memset (map.data, 0, map.size);
  GST_WRITE_UINT32_BE (map.data, dummy->header);
  gst_buffer_unmap (dummy->buffer, &map);

  GST_BUFFER_PTS (dummy->buffer) = GST_BUFFER_PTS (frame->buffer);

  return dummy;
}

/* validates and parses @buf, and queues for further transformation if valid,
 * otherwise discards @buf
 * Takes ownership of @buf. */
static gboolean
gst_rtp_mpa_robust_depay_queue_frame (GstRtpMPARobustDepay * rtpmpadepay,
    GstBuffer * buf)
{
  GstADUFrame *frame = NULL;
  guint version, layer, channels, size;
  guint crc;
  GstMapInfo map;

  g_return_val_if_fail (buf != NULL, FALSE);

  gst_buffer_map (buf, &map, GST_MAP_READ);

  if (map.size < 6)
    goto corrupt_frame;

  frame = g_slice_new0 (GstADUFrame);
  frame->header = GST_READ_UINT32_BE (map.data);

  size = mp3_type_frame_length_from_header (GST_ELEMENT_CAST (rtpmpadepay),
      frame->header, &version, &layer, &channels, NULL, NULL, NULL, &crc);
  if (!size)
    goto corrupt_frame;

  frame->size = size;
  frame->layer = layer;
  if (version == 1 && channels == 2)
    frame->side_info = 32;
  else if ((version == 1 && channels == 1) || (version >= 2 && channels == 2))
    frame->side_info = 17;
  else if (version >= 2 && channels == 1)
    frame->side_info = 9;
  else {
    g_assert_not_reached ();
    goto corrupt_frame;
  }

  /* backpointer */
  if (layer == 3) {
    frame->backpointer = GST_READ_UINT16_BE (map.data + 4);
    frame->backpointer >>= 7;
    GST_LOG_OBJECT (rtpmpadepay, "backpointer: %d", frame->backpointer);
  }

  if (!crc)
    frame->side_info += 2;

  GST_LOG_OBJECT (rtpmpadepay, "side info: %d", frame->side_info);
  frame->data_size = frame->size - 4 - frame->side_info;

  /* some size validation checks */
  if (4 + frame->side_info > map.size)
    goto corrupt_frame;

  /* ADU data would then extend past MP3 frame,
   * even using past byte reservoir */
  if (-frame->backpointer + (gint) (map.size) > frame->size)
    goto corrupt_frame;

  gst_buffer_unmap (buf, &map);

  /* ok, take buffer and queue */
  frame->buffer = buf;
  g_queue_push_tail (rtpmpadepay->adu_frames, frame);

  return TRUE;

  /* ERRORS */
corrupt_frame:
  {
    GST_DEBUG_OBJECT (rtpmpadepay, "frame is corrupt");
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    if (frame)
      g_slice_free (GstADUFrame, frame);
    return FALSE;
  }
}

static inline void
gst_rtp_mpa_robust_depay_free_frame (GstADUFrame * frame)
{
  if (frame->buffer)
    gst_buffer_unref (frame->buffer);
  g_slice_free (GstADUFrame, frame);
}

static inline void
gst_rtp_mpa_robust_depay_dequeue_frame (GstRtpMPARobustDepay * rtpmpadepay)
{
  GstADUFrame *head;

  GST_LOG_OBJECT (rtpmpadepay, "dequeueing ADU frame");

  if (rtpmpadepay->adu_frames->head == rtpmpadepay->cur_adu_frame)
    rtpmpadepay->cur_adu_frame = NULL;

  head = g_queue_pop_head (rtpmpadepay->adu_frames);
  g_assert (head->buffer);
  gst_rtp_mpa_robust_depay_free_frame (head);

  return;
}

/* returns TRUE if at least one new ADU frame was enqueued for MP3 conversion.
 * Takes ownership of @buf. */
static gboolean
gst_rtp_mpa_robust_depay_deinterleave (GstRtpMPARobustDepay * rtpmpadepay,
    GstBuffer * buf)
{
  gboolean ret = FALSE;
  GstMapInfo map;
  guint val, iindex, icc;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  val = GST_READ_UINT16_BE (map.data) >> 5;
  gst_buffer_unmap (buf, &map);

  iindex = val >> 3;
  icc = val & 0x7;

  GST_LOG_OBJECT (rtpmpadepay, "sync: 0x%x, index: %u, cycle count: %u",
      val, iindex, icc);

  /* basic case; no interleaving ever seen */
  if (val == 0x7ff && rtpmpadepay->last_icc < 0) {
    ret = gst_rtp_mpa_robust_depay_queue_frame (rtpmpadepay, buf);
  } else {
    if (G_UNLIKELY (rtpmpadepay->last_icc < 0)) {
      rtpmpadepay->last_icc = icc;
      rtpmpadepay->last_ii = iindex;
    }
    if (icc != rtpmpadepay->last_icc || iindex == rtpmpadepay->last_ii) {
      gint i;

      for (i = 0; i < 256; ++i) {
        if (rtpmpadepay->deinter[i] != NULL) {
          ret |= gst_rtp_mpa_robust_depay_queue_frame (rtpmpadepay,
              rtpmpadepay->deinter[i]);
          rtpmpadepay->deinter[i] = NULL;
        }
      }
    }
    /* rewrite buffer sync header */
    gst_buffer_map (buf, &map, GST_MAP_READWRITE);
    val = GST_READ_UINT16_BE (map.data);
    val = (0x7ff << 5) | val;
    GST_WRITE_UINT16_BE (map.data, val);
    gst_buffer_unmap (buf, &map);
    /* store and keep track of last indices */
    rtpmpadepay->last_icc = icc;
    rtpmpadepay->last_ii = iindex;
    rtpmpadepay->deinter[iindex] = buf;
  }

  return ret;
}

/* Head ADU frame corresponds to mp3_frame (i.e. in header in side-info) that
 * is currently being written
 * cur_adu_frame refers to ADU frame whose data should be bytewritten next
 * (possibly starting from offset rather than start 0) (and is typicall tail
 * at time of last push round).
 * If at start, position where it should start writing depends on (data) sizes
 * of previous mp3 frames (corresponding to foregoing ADU frames) kept in size,
 * and its backpointer */
static GstFlowReturn
gst_rtp_mpa_robust_depay_push_mp3_frames (GstRtpMPARobustDepay * rtpmpadepay)
{
  GstBuffer *buf;
  GstADUFrame *frame, *head;
  gint av;
  GstFlowReturn ret = GST_FLOW_OK;

  while (1) {
    GstMapInfo map;

    if (G_UNLIKELY (!rtpmpadepay->cur_adu_frame)) {
      rtpmpadepay->cur_adu_frame = rtpmpadepay->adu_frames->head;
      rtpmpadepay->offset = 0;
      rtpmpadepay->size = 0;
    }

    if (G_UNLIKELY (!rtpmpadepay->cur_adu_frame))
      break;

    frame = (GstADUFrame *) rtpmpadepay->cur_adu_frame->data;
    head = (GstADUFrame *) rtpmpadepay->adu_frames->head->data;

    /* special case: non-layer III are sent straight through */
    if (G_UNLIKELY (frame->layer != 3)) {
      GST_DEBUG_OBJECT (rtpmpadepay, "layer %d frame, sending as-is",
          frame->layer);
      gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtpmpadepay),
          frame->buffer);
      frame->buffer = NULL;
      /* and remove it from any further consideration */
      g_slice_free (GstADUFrame, frame);
      g_queue_delete_link (rtpmpadepay->adu_frames, rtpmpadepay->cur_adu_frame);
      rtpmpadepay->cur_adu_frame = NULL;
      continue;
    }

    if (rtpmpadepay->offset == gst_buffer_get_size (frame->buffer)) {
      if (g_list_next (rtpmpadepay->cur_adu_frame)) {
        rtpmpadepay->size += frame->data_size;
        rtpmpadepay->cur_adu_frame = g_list_next (rtpmpadepay->cur_adu_frame);
        frame = (GstADUFrame *) rtpmpadepay->cur_adu_frame->data;
        rtpmpadepay->offset = 0;
        GST_LOG_OBJECT (rtpmpadepay,
            "moving to next ADU frame, size %d, side_info %d, backpointer %d",
            frame->size, frame->side_info, frame->backpointer);
        /* layer I and II packets have no bitreservoir and must be sent as-is;
         * so flush any pending frame */
        if (G_UNLIKELY (frame->layer != 3 && rtpmpadepay->mp3_frame))
          goto flush;
      } else {
        break;
      }
    }

    if (G_UNLIKELY (!rtpmpadepay->mp3_frame)) {
      GST_LOG_OBJECT (rtpmpadepay,
          "setting up new MP3 frame of size %d, side_info %d",
          head->size, head->side_info);
      rtpmpadepay->mp3_frame = gst_byte_writer_new_with_size (head->size, TRUE);
      /* 0-fill possible gaps */
      gst_byte_writer_fill_unchecked (rtpmpadepay->mp3_frame, 0, head->size);
      gst_byte_writer_set_pos (rtpmpadepay->mp3_frame, 0);
      /* bytewriter corresponds to head frame,
       * i.e. the header and the side info must match */
      g_assert (4 + head->side_info <= head->size);
      gst_buffer_map (head->buffer, &map, GST_MAP_READ);
      gst_byte_writer_put_data_unchecked (rtpmpadepay->mp3_frame,
          map.data, 4 + head->side_info);
      gst_buffer_unmap (head->buffer, &map);
    }

    buf = frame->buffer;
    av = gst_byte_writer_get_remaining (rtpmpadepay->mp3_frame);
    GST_LOG_OBJECT (rtpmpadepay, "current mp3 frame remaining: %d", av);
    GST_LOG_OBJECT (rtpmpadepay, "accumulated ADU frame data_size: %d",
        rtpmpadepay->size);

    if (rtpmpadepay->offset) {
      gst_buffer_map (buf, &map, GST_MAP_READ);
      /* no need to position, simply append */
      g_assert (map.size > rtpmpadepay->offset);
      av = MIN (av, map.size - rtpmpadepay->offset);
      GST_LOG_OBJECT (rtpmpadepay,
          "appending %d bytes from ADU frame at offset %d", av,
          rtpmpadepay->offset);
      gst_byte_writer_put_data_unchecked (rtpmpadepay->mp3_frame,
          map.data + rtpmpadepay->offset, av);
      rtpmpadepay->offset += av;
      gst_buffer_unmap (buf, &map);
    } else {
      gint pos, tpos;

      /* position writing according to ADU frame backpointer */
      pos = gst_byte_writer_get_pos (rtpmpadepay->mp3_frame);
      tpos = rtpmpadepay->size - frame->backpointer + 4 + head->side_info;
      GST_LOG_OBJECT (rtpmpadepay, "current MP3 frame at position %d, "
          "starting new ADU frame data at offset %d", pos, tpos);
      if (tpos < pos) {
        GstADUFrame *dummy;

        /* try to insert as few frames as possible,
         * so go for a reasonably large dummy frame size */
        GST_LOG_OBJECT (rtpmpadepay,
            "overlapping previous data; inserting dummy frame");
        dummy =
            gst_rtp_mpa_robust_depay_generate_dummy_frame (rtpmpadepay, frame);
        g_queue_insert_before (rtpmpadepay->adu_frames,
            rtpmpadepay->cur_adu_frame, dummy);
        /* offset is known to be zero, so we can shift current one */
        rtpmpadepay->cur_adu_frame = rtpmpadepay->cur_adu_frame->prev;
        if (!rtpmpadepay->size) {
          g_assert (rtpmpadepay->cur_adu_frame ==
              rtpmpadepay->adu_frames->head);
          GST_LOG_OBJECT (rtpmpadepay, "... which is new head frame");
          gst_byte_writer_free (rtpmpadepay->mp3_frame);
          rtpmpadepay->mp3_frame = NULL;
        }
        /* ... and continue adding that empty one immediately,
         * and then see if that provided enough extra space */
        continue;
      } else if (tpos >= pos + av) {
        /* ADU frame no longer needs current MP3 frame; move to its end */
        GST_LOG_OBJECT (rtpmpadepay, "passed current MP3 frame");
        gst_byte_writer_set_pos (rtpmpadepay->mp3_frame, pos + av);
      } else {
        /* position and append */
        gst_buffer_map (buf, &map, GST_MAP_READ);
        GST_LOG_OBJECT (rtpmpadepay, "adding to current MP3 frame");
        gst_byte_writer_set_pos (rtpmpadepay->mp3_frame, tpos);
        av -= (tpos - pos);
        g_assert (map.size >= 4 + frame->side_info);
        av = MIN (av, map.size - 4 - frame->side_info);
        gst_byte_writer_put_data_unchecked (rtpmpadepay->mp3_frame,
            map.data + 4 + frame->side_info, av);
        rtpmpadepay->offset += av + 4 + frame->side_info;
        gst_buffer_unmap (buf, &map);
      }
    }

    /* if mp3 frame filled, send on its way */
    if (gst_byte_writer_get_remaining (rtpmpadepay->mp3_frame) == 0) {
    flush:
      buf = gst_byte_writer_free_and_get_buffer (rtpmpadepay->mp3_frame);
      rtpmpadepay->mp3_frame = NULL;
      GST_BUFFER_PTS (buf) = GST_BUFFER_PTS (head->buffer);
      /* no longer need head ADU frame header and side info */
      /* NOTE maybe head == current, then size and offset go off a bit,
       * but current gets reset to NULL, and then also offset and size */
      rtpmpadepay->size -= head->data_size;
      gst_rtp_mpa_robust_depay_dequeue_frame (rtpmpadepay);
      /* send */
      ret = gst_rtp_base_depayload_push (GST_RTP_BASE_DEPAYLOAD (rtpmpadepay),
          buf);
    }
  }

  return ret;
}

/* process ADU frame @buf through:
 * - deinterleaving
 * - converting to MP3 frames
 * Takes ownership of @buf.
 */
static GstFlowReturn
gst_rtp_mpa_robust_depay_submit_adu (GstRtpMPARobustDepay * rtpmpadepay,
    GstBuffer * buf)
{
  if (gst_rtp_mpa_robust_depay_deinterleave (rtpmpadepay, buf))
    return gst_rtp_mpa_robust_depay_push_mp3_frames (rtpmpadepay);

  return GST_FLOW_OK;
}

static GstBuffer *
gst_rtp_mpa_robust_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp)
{
  GstRtpMPARobustDepay *rtpmpadepay;
  gint payload_len, offset;
  guint8 *payload;
  gboolean cont, dtype;
  guint av, size;
  GstClockTime timestamp;
  GstBuffer *buf;

  rtpmpadepay = GST_RTP_MPA_ROBUST_DEPAY (depayload);

  timestamp = GST_BUFFER_PTS (rtp->buffer);

  payload_len = gst_rtp_buffer_get_payload_len (rtp);
  if (payload_len <= 1)
    goto short_read;

  payload = gst_rtp_buffer_get_payload (rtp);
  offset = 0;
  GST_LOG_OBJECT (rtpmpadepay, "payload_len: %d", payload_len);

  /* strip off descriptor
   *
   *  0                   1
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |C|T|            ADU size         |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *
   * C: if 1, data is continuation
   * T: if 1, size is 14 bits, otherwise 6 bits
   * ADU size: size of following packet (not including descriptor)
   */
  while (payload_len) {
    if (G_LIKELY (rtpmpadepay->has_descriptor)) {
      cont = ! !(payload[offset] & 0x80);
      dtype = ! !(payload[offset] & 0x40);
      if (dtype) {
        size = (payload[offset] & 0x3f) << 8 | payload[offset + 1];
        payload_len--;
        offset++;
      } else if (payload_len >= 2) {
        size = (payload[offset] & 0x3f);
        payload_len -= 2;
        offset += 2;
      } else {
        goto short_read;
      }
    } else {
      cont = FALSE;
      dtype = -1;
      size = payload_len;
    }

    GST_LOG_OBJECT (rtpmpadepay, "offset %d has cont: %d, dtype: %d, size: %d",
        offset, cont, dtype, size);

    buf = gst_rtp_buffer_get_payload_subbuffer (rtp, offset,
        MIN (size, payload_len));

    if (cont) {
      av = gst_adapter_available (rtpmpadepay->adapter);
      if (G_UNLIKELY (!av)) {
        GST_DEBUG_OBJECT (rtpmpadepay,
            "discarding continuation fragment without prior fragment");
        gst_buffer_unref (buf);
      } else {
        av += gst_buffer_get_size (buf);
        gst_adapter_push (rtpmpadepay->adapter, buf);
        if (av == size) {
          timestamp = gst_adapter_prev_pts (rtpmpadepay->adapter, NULL);
          buf = gst_adapter_take_buffer (rtpmpadepay->adapter, size);
          GST_BUFFER_PTS (buf) = timestamp;
          gst_rtp_mpa_robust_depay_submit_adu (rtpmpadepay, buf);
        } else if (av > size) {
          GST_DEBUG_OBJECT (rtpmpadepay,
              "assembled ADU size %d larger than expected %d; discarding",
              av, size);
          gst_adapter_clear (rtpmpadepay->adapter);
        }
      }
      size = payload_len;
    } else {
      /* not continuation, first fragment or whole ADU */
      if (payload_len == size) {
        /* whole ADU */
        GST_BUFFER_PTS (buf) = timestamp;
        gst_rtp_mpa_robust_depay_submit_adu (rtpmpadepay, buf);
      } else if (payload_len < size) {
        /* first fragment */
        gst_adapter_push (rtpmpadepay->adapter, buf);
        size = payload_len;
      }
    }

    offset += size;
    payload_len -= size;

    /* timestamp applies to first payload, no idea for subsequent ones */
    timestamp = GST_CLOCK_TIME_NONE;
  }

  return NULL;

  /* ERRORS */
short_read:
  {
    GST_ELEMENT_WARNING (rtpmpadepay, STREAM, DECODE,
        (NULL), ("Packet contains invalid data"));
    return NULL;
  }
}

static GstStateChangeReturn
gst_rtp_mpa_robust_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstRtpMPARobustDepay *rtpmpadepay;

  rtpmpadepay = GST_RTP_MPA_ROBUST_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      rtpmpadepay->last_ii = -1;
      rtpmpadepay->last_icc = -1;
      rtpmpadepay->size = 0;
      rtpmpadepay->offset = 0;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret != GST_STATE_CHANGE_SUCCESS)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
      gint i;

      gst_adapter_clear (rtpmpadepay->adapter);
      for (i = 0; i < G_N_ELEMENTS (rtpmpadepay->deinter); i++) {
        gst_buffer_replace (&rtpmpadepay->deinter[i], NULL);
      }
      rtpmpadepay->cur_adu_frame = NULL;
      g_queue_foreach (rtpmpadepay->adu_frames,
          (GFunc) gst_rtp_mpa_robust_depay_free_frame, NULL);
      g_queue_clear (rtpmpadepay->adu_frames);
      if (rtpmpadepay->mp3_frame)
        gst_byte_writer_free (rtpmpadepay->mp3_frame);
      break;
    }
    default:
      break;
  }

  return ret;
}

gboolean
gst_rtp_mpa_robust_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmparobustdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MPA_ROBUST_DEPAY);
}
