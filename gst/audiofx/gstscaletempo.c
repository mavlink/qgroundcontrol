/*
 * GStreamer
 * Copyright (C) 2008 Rov Juvano <rovjuvano@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-scaletempo
 * @title: scaletempo
 *
 * Scale tempo while maintaining pitch
 * (WSOLA-like technique with cross correlation)
 * Inspired by SoundTouch library by Olli Parviainen
 *
 * Use Sceletempo to apply playback rates without the chipmunk effect.
 *
 * ## Example pipelines
 *
 * |[
 * filesrc location=media.ext ! decodebin name=d \
 *     d. ! queue ! audioconvert ! audioresample ! scaletempo ! audioconvert ! audioresample ! autoaudiosink \
 *     d. ! queue ! videoconvert ! autovideosink
 * ]|
 * OR
 * |[
 * playbin uri=... audio_sink="scaletempo ! audioconvert ! audioresample ! autoaudiosink"
 * ]|
 * When an application sends a seek event with rate != 1.0, Scaletempo applies
 * the rate change by scaling the tempo without scaling the pitch.
 *
 * Scaletempo works by producing audio in constant sized chunks
 * (#GstScaletempo:stride) but consuming chunks proportional to the playback
 * rate.
 *
 * Scaletempo then smooths the output by blending the end of one stride with
 * the next (#GstScaletempo:overlap).
 *
 * Scaletempo smooths the overlap further by searching within the input buffer
 * for the best overlap position.  Scaletempo uses a statistical cross
 * correlation (roughly a dot-product).  Scaletempo consumes most of its CPU
 * cycles here. One can use the #GstScaletempo:search propery to tune how far
 * the algorithm looks.
 *
 */

/*
 * Note: frame = audio key unit (i.e. one sample for each channel)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <string.h>             /* for memset */

#include "gstscaletempo.h"

GST_DEBUG_CATEGORY_STATIC (gst_scaletempo_debug);
#define GST_CAT_DEFAULT gst_scaletempo_debug

/* Filter signals and args */
enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_RATE,
  PROP_STRIDE,
  PROP_OVERLAP,
  PROP_SEARCH,
};

#define SUPPORTED_CAPS \
GST_STATIC_CAPS ( \
    GST_AUDIO_CAPS_MAKE (GST_AUDIO_NE (F32)) ", layout=(string)interleaved; " \
    GST_AUDIO_CAPS_MAKE (GST_AUDIO_NE (F64)) ", layout=(string)interleaved; " \
    GST_AUDIO_CAPS_MAKE (GST_AUDIO_NE (S16)) ", layout=(string)interleaved" \
)

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    SUPPORTED_CAPS);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    SUPPORTED_CAPS);

#define DEBUG_INIT(bla) GST_DEBUG_CATEGORY_INIT (gst_scaletempo_debug, "scaletempo", 0, "scaletempo element");

#define gst_scaletempo_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstScaletempo, gst_scaletempo,
    GST_TYPE_BASE_TRANSFORM, DEBUG_INIT (0));

#define CREATE_BEST_OVERLAP_OFFSET_FLOAT_FUNC(type) \
static guint \
best_overlap_offset_##type (GstScaletempo * st) \
{ \
  g##type *pw, *po, *ppc, *search_start; \
  g##type best_corr = G_MININT; \
  guint best_off = 0; \
  gint i, off; \
  \
  pw = st->table_window; \
  po = st->buf_overlap; \
  po += st->samples_per_frame; \
  ppc = st->buf_pre_corr; \
  for (i = st->samples_per_frame; i < st->samples_overlap; i++) { \
    *ppc++ = *pw++ * *po++; \
  } \
  \
  search_start = (g##type *) st->buf_queue + st->samples_per_frame; \
  for (off = 0; off < st->frames_search; off++) { \
    g##type corr = 0; \
    g##type *ps = search_start; \
    ppc = st->buf_pre_corr; \
    for (i = st->samples_per_frame; i < st->samples_overlap; i++) { \
      corr += *ppc++ * *ps++; \
    } \
    if (corr > best_corr) { \
      best_corr = corr; \
      best_off = off; \
    } \
    search_start += st->samples_per_frame; \
  } \
  \
  return best_off * st->bytes_per_frame; \
}

CREATE_BEST_OVERLAP_OFFSET_FLOAT_FUNC (float);
CREATE_BEST_OVERLAP_OFFSET_FLOAT_FUNC (double);

/* buffer padding for loop optimization: sizeof(gint32) * (loop_size - 1) */
#define UNROLL_PADDING (4*3)
static guint
best_overlap_offset_s16 (GstScaletempo * st)
{
  gint32 *pw, *ppc;
  gint16 *po, *search_start;
  gint64 best_corr = G_MININT64;
  guint best_off = 0;
  guint off;
  glong i;

  pw = st->table_window;
  po = st->buf_overlap;
  po += st->samples_per_frame;
  ppc = st->buf_pre_corr;
  for (i = st->samples_per_frame; i < st->samples_overlap; i++) {
    *ppc++ = (*pw++ * *po++) >> 15;
  }

  search_start = (gint16 *) st->buf_queue + st->samples_per_frame;
  for (off = 0; off < st->frames_search; off++) {
    gint64 corr = 0;
    gint16 *ps = search_start;
    ppc = st->buf_pre_corr;
    ppc += st->samples_overlap - st->samples_per_frame;
    ps += st->samples_overlap - st->samples_per_frame;
    i = -((glong) st->samples_overlap - (glong) st->samples_per_frame);
    do {
      corr += ppc[i + 0] * ps[i + 0];
      corr += ppc[i + 1] * ps[i + 1];
      corr += ppc[i + 2] * ps[i + 2];
      corr += ppc[i + 3] * ps[i + 3];
      i += 4;
    } while (i < 0);
    if (corr > best_corr) {
      best_corr = corr;
      best_off = off;
    }
    search_start += st->samples_per_frame;
  }

  return best_off * st->bytes_per_frame;
}

#define CREATE_OUTPUT_OVERLAP_FLOAT_FUNC(type) \
static void \
output_overlap_##type (GstScaletempo * st, gpointer buf_out, guint bytes_off) \
{ \
  g##type *pout = buf_out; \
  g##type *pb = st->table_blend; \
  g##type *po = st->buf_overlap; \
  g##type *pin = (g##type *) (st->buf_queue + bytes_off); \
  gint i; \
  for (i = 0; i < st->samples_overlap; i++) { \
    *pout++ = *po - *pb++ * (*po - *pin++); \
    po++; \
  } \
}

CREATE_OUTPUT_OVERLAP_FLOAT_FUNC (float);
CREATE_OUTPUT_OVERLAP_FLOAT_FUNC (double);

static void
output_overlap_s16 (GstScaletempo * st, gpointer buf_out, guint bytes_off)
{
  gint16 *pout = buf_out;
  gint32 *pb = st->table_blend;
  gint16 *po = st->buf_overlap;
  gint16 *pin = (gint16 *) (st->buf_queue + bytes_off);
  gint i;
  for (i = 0; i < st->samples_overlap; i++) {
    *pout++ = *po - ((*pb++ * (*po - *pin++)) >> 16);
    po++;
  }
}

static guint
fill_queue (GstScaletempo * st, GstBuffer * buf_in, guint offset)
{
  guint bytes_in = gst_buffer_get_size (buf_in) - offset;
  guint offset_unchanged = offset;
  GstMapInfo map;

  gst_buffer_map (buf_in, &map, GST_MAP_READ);
  if (st->bytes_to_slide > 0) {
    if (st->bytes_to_slide < st->bytes_queued) {
      guint bytes_in_move = st->bytes_queued - st->bytes_to_slide;
      memmove (st->buf_queue, st->buf_queue + st->bytes_to_slide,
          bytes_in_move);
      st->bytes_to_slide = 0;
      st->bytes_queued = bytes_in_move;
    } else {
      guint bytes_in_skip;
      st->bytes_to_slide -= st->bytes_queued;
      bytes_in_skip = MIN (st->bytes_to_slide, bytes_in);
      st->bytes_queued = 0;
      st->bytes_to_slide -= bytes_in_skip;
      offset += bytes_in_skip;
      bytes_in -= bytes_in_skip;
    }
  }

  if (bytes_in > 0) {
    guint bytes_in_copy =
        MIN (st->bytes_queue_max - st->bytes_queued, bytes_in);
    memcpy (st->buf_queue + st->bytes_queued, map.data + offset, bytes_in_copy);
    st->bytes_queued += bytes_in_copy;
    offset += bytes_in_copy;
  }
  gst_buffer_unmap (buf_in, &map);

  return offset - offset_unchanged;
}

static void
reinit_buffers (GstScaletempo * st)
{
  gint i, j;
  guint frames_overlap;
  guint new_size;
  GstClockTime latency;

  guint frames_stride = st->ms_stride * st->sample_rate / 1000.0;
  st->bytes_stride = frames_stride * st->bytes_per_frame;

  /* overlap */
  frames_overlap = frames_stride * st->percent_overlap;
  if (frames_overlap < 1) {     /* if no overlap */
    st->bytes_overlap = 0;
    st->bytes_standing = st->bytes_stride;
    st->samples_standing = st->bytes_standing / st->bytes_per_sample;
    st->output_overlap = NULL;
  } else {
    guint prev_overlap = st->bytes_overlap;
    st->bytes_overlap = frames_overlap * st->bytes_per_frame;
    st->samples_overlap = frames_overlap * st->samples_per_frame;
    st->bytes_standing = st->bytes_stride - st->bytes_overlap;
    st->samples_standing = st->bytes_standing / st->bytes_per_sample;
    st->buf_overlap = g_realloc (st->buf_overlap, st->bytes_overlap);
    /* S16 uses gint32 blend table, floats/doubles use their respective type */
    st->table_blend =
        g_realloc (st->table_blend,
        st->samples_overlap * (st->format ==
            GST_AUDIO_FORMAT_S16 ? 4 : st->bytes_per_sample));
    if (st->bytes_overlap > prev_overlap) {
      memset ((guint8 *) st->buf_overlap + prev_overlap, 0,
          st->bytes_overlap - prev_overlap);
    }
    if (st->format == GST_AUDIO_FORMAT_S16) {
      gint32 *pb = st->table_blend;
      gint64 blend = 0;
      for (i = 0; i < frames_overlap; i++) {
        gint32 v = blend / frames_overlap;
        for (j = 0; j < st->samples_per_frame; j++) {
          *pb++ = v;
        }
        blend += 65535;         /* 2^16 */
      }
      st->output_overlap = output_overlap_s16;
    } else if (st->format == GST_AUDIO_FORMAT_F32) {
      gfloat *pb = st->table_blend;
      gfloat t = (gfloat) frames_overlap;
      for (i = 0; i < frames_overlap; i++) {
        gfloat v = i / t;
        for (j = 0; j < st->samples_per_frame; j++) {
          *pb++ = v;
        }
      }
      st->output_overlap = output_overlap_float;
    } else {
      gdouble *pb = st->table_blend;
      gdouble t = (gdouble) frames_overlap;
      for (i = 0; i < frames_overlap; i++) {
        gdouble v = i / t;
        for (j = 0; j < st->samples_per_frame; j++) {
          *pb++ = v;
        }
      }
      st->output_overlap = output_overlap_double;
    }
  }

  /* best overlap */
  st->frames_search =
      (frames_overlap <= 1) ? 0 : st->ms_search * st->sample_rate / 1000.0;
  if (st->frames_search < 1) {  /* if no search */
    st->best_overlap_offset = NULL;
  } else {
    /* S16 uses gint32 buffer, floats/doubles use their respective type */
    guint bytes_pre_corr =
        (st->samples_overlap - st->samples_per_frame) * (st->format ==
        GST_AUDIO_FORMAT_S16 ? 4 : st->bytes_per_sample);
    st->buf_pre_corr =
        g_realloc (st->buf_pre_corr, bytes_pre_corr + UNROLL_PADDING);
    st->table_window = g_realloc (st->table_window, bytes_pre_corr);
    if (st->format == GST_AUDIO_FORMAT_S16) {
      gint64 t = frames_overlap;
      gint32 n = 8589934588LL / (t * t);        /* 4 * (2^31 - 1) / t^2 */
      gint32 *pw;

      memset ((guint8 *) st->buf_pre_corr + bytes_pre_corr, 0, UNROLL_PADDING);
      pw = st->table_window;
      for (i = 1; i < frames_overlap; i++) {
        gint32 v = (i * (t - i) * n) >> 15;
        for (j = 0; j < st->samples_per_frame; j++) {
          *pw++ = v;
        }
      }
      st->best_overlap_offset = best_overlap_offset_s16;
    } else if (st->format == GST_AUDIO_FORMAT_F32) {
      gfloat *pw = st->table_window;
      for (i = 1; i < frames_overlap; i++) {
        gfloat v = i * (frames_overlap - i);
        for (j = 0; j < st->samples_per_frame; j++) {
          *pw++ = v;
        }
      }
      st->best_overlap_offset = best_overlap_offset_float;
    } else {
      gdouble *pw = st->table_window;
      for (i = 1; i < frames_overlap; i++) {
        gdouble v = i * (frames_overlap - i);
        for (j = 0; j < st->samples_per_frame; j++) {
          *pw++ = v;
        }
      }
      st->best_overlap_offset = best_overlap_offset_double;
    }
  }

  new_size =
      (st->frames_search + frames_stride +
      frames_overlap) * st->bytes_per_frame;
  if (st->bytes_queued > new_size) {
    if (st->bytes_to_slide > st->bytes_queued) {
      st->bytes_to_slide -= st->bytes_queued;
      st->bytes_queued = 0;
    } else {
      guint new_queued = MIN (st->bytes_queued - st->bytes_to_slide, new_size);
      memmove (st->buf_queue,
          st->buf_queue + st->bytes_queued - new_queued, new_queued);
      st->bytes_to_slide = 0;
      st->bytes_queued = new_queued;
    }
  }

  st->bytes_queue_max = new_size;
  st->buf_queue = g_realloc (st->buf_queue, st->bytes_queue_max);

  latency =
      gst_util_uint64_scale (st->bytes_queue_max, GST_SECOND,
      st->bytes_per_frame * st->sample_rate);
  if (st->latency != latency) {
    st->latency = latency;
    gst_element_post_message (GST_ELEMENT (st),
        gst_message_new_latency (GST_OBJECT (st)));
  }

  st->bytes_stride_scaled = st->bytes_stride * st->scale;
  st->frames_stride_scaled = st->bytes_stride_scaled / st->bytes_per_frame;

  GST_DEBUG
      ("%.3f scale, %.3f stride_in, %i stride_out, %i standing, %i overlap, %i search, %i queue, %s mode",
      st->scale, st->frames_stride_scaled,
      (gint) (st->bytes_stride / st->bytes_per_frame),
      (gint) (st->bytes_standing / st->bytes_per_frame),
      (gint) (st->bytes_overlap / st->bytes_per_frame), st->frames_search,
      (gint) (st->bytes_queue_max / st->bytes_per_frame),
      gst_audio_format_to_string (st->format));

  st->reinit_buffers = FALSE;
}

static GstBuffer *
reverse_buffer (GstScaletempo * st, GstBuffer * inbuf)
{
  GstBuffer *outbuf;
  GstMapInfo imap, omap;

  gst_buffer_map (inbuf, &imap, GST_MAP_READ);
  outbuf = gst_buffer_new_and_alloc (imap.size);
  gst_buffer_map (outbuf, &omap, GST_MAP_WRITE);

  if (st->format == GST_AUDIO_FORMAT_F64) {
    const gint64 *ip = (const gint64 *) imap.data;
    gint64 *op = (gint64 *) (omap.data + omap.size - 8 * st->samples_per_frame);
    guint i, n = imap.size / (8 * st->samples_per_frame);
    guint j, c = st->samples_per_frame;

    for (i = 0; i < n; i++) {
      for (j = 0; j < c; j++)
        op[j] = ip[j];
      op -= c;
      ip += c;
    }
  } else {
    const gint32 *ip = (const gint32 *) imap.data;
    gint32 *op = (gint32 *) (omap.data + omap.size - 4 * st->samples_per_frame);
    guint i, n = imap.size / (4 * st->samples_per_frame);
    guint j, c = st->samples_per_frame;

    for (i = 0; i < n; i++) {
      for (j = 0; j < c; j++)
        op[j] = ip[j];
      op -= c;
      ip += c;
    }
  }

  gst_buffer_unmap (inbuf, &imap);
  gst_buffer_unmap (outbuf, &omap);

  return outbuf;
}

/* GstBaseTransform vmethod implementations */
static GstFlowReturn
gst_scaletempo_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstScaletempo *st = GST_SCALETEMPO (trans);
  gint8 *pout;
  guint offset_in, bytes_out;
  GstMapInfo omap;
  GstClockTime timestamp;
  GstBuffer *tmpbuf = NULL;

  if (st->reverse)
    tmpbuf = reverse_buffer (st, inbuf);

  gst_buffer_map (outbuf, &omap, GST_MAP_WRITE);
  pout = (gint8 *) omap.data;
  bytes_out = omap.size;

  offset_in = fill_queue (st, tmpbuf ? tmpbuf : inbuf, 0);
  bytes_out = 0;
  while (st->bytes_queued >= st->bytes_queue_max) {
    guint bytes_off = 0;
    gdouble frames_to_slide;
    guint frames_to_stride_whole;

    /* output stride */
    if (st->output_overlap) {
      if (st->best_overlap_offset) {
        bytes_off = st->best_overlap_offset (st);
      }
      st->output_overlap (st, pout, bytes_off);
    }
    memcpy (pout + st->bytes_overlap,
        st->buf_queue + bytes_off + st->bytes_overlap, st->bytes_standing);
    pout += st->bytes_stride;
    bytes_out += st->bytes_stride;

    /* input stride */
    memcpy (st->buf_overlap,
        st->buf_queue + bytes_off + st->bytes_stride, st->bytes_overlap);
    frames_to_slide = st->frames_stride_scaled + st->frames_stride_error;
    frames_to_stride_whole = (gint) frames_to_slide;
    st->bytes_to_slide = frames_to_stride_whole * st->bytes_per_frame;
    st->frames_stride_error = frames_to_slide - frames_to_stride_whole;

    offset_in += fill_queue (st, tmpbuf ? tmpbuf : inbuf, offset_in);
  }
  gst_buffer_unmap (outbuf, &omap);

  if (st->reverse) {
    timestamp = st->in_segment.stop - GST_BUFFER_TIMESTAMP (inbuf);
    if (timestamp < st->latency)
      timestamp = 0;
    else
      timestamp -= st->latency;
  } else {
    timestamp = GST_BUFFER_TIMESTAMP (inbuf) - st->in_segment.start;
    if (timestamp < st->latency)
      timestamp = 0;
    else
      timestamp -= st->latency;
  }
  GST_BUFFER_TIMESTAMP (outbuf) = timestamp / st->scale + st->in_segment.start;
  GST_BUFFER_DURATION (outbuf) =
      gst_util_uint64_scale (bytes_out, GST_SECOND,
      st->bytes_per_frame * st->sample_rate);
  gst_buffer_set_size (outbuf, bytes_out);

  if (tmpbuf)
    gst_buffer_unref (tmpbuf);

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_scaletempo_submit_input_buffer (GstBaseTransform * trans,
    gboolean is_discont, GstBuffer * input)
{
  GstScaletempo *scaletempo = GST_SCALETEMPO (trans);

  if (scaletempo->in_segment.format == GST_FORMAT_TIME) {
    input =
        gst_audio_buffer_clip (input, &scaletempo->in_segment,
        scaletempo->sample_rate, scaletempo->bytes_per_frame);
    if (!input)
      return GST_FLOW_OK;
  }

  return GST_BASE_TRANSFORM_CLASS (parent_class)->submit_input_buffer (trans,
      is_discont, input);
}

static gboolean
gst_scaletempo_transform_size (GstBaseTransform * trans,
    GstPadDirection direction,
    GstCaps * caps, gsize size, GstCaps * othercaps, gsize * othersize)
{
  if (direction == GST_PAD_SINK) {
    GstScaletempo *scaletempo = GST_SCALETEMPO (trans);
    gint bytes_to_out;

    if (scaletempo->reinit_buffers)
      reinit_buffers (scaletempo);

    bytes_to_out = size + scaletempo->bytes_queued - scaletempo->bytes_to_slide;
    if (bytes_to_out < (gint) scaletempo->bytes_queue_max) {
      *othersize = 0;
    } else {
      /* while (total_buffered - stride_length * n >= queue_max) n++ */
      *othersize = scaletempo->bytes_stride * ((guint) (
              (bytes_to_out - scaletempo->bytes_queue_max +
                  /* rounding protection */ scaletempo->bytes_per_frame)
              / scaletempo->bytes_stride_scaled) + 1);
    }

    return TRUE;
  }
  return FALSE;
}

static gboolean
gst_scaletempo_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstScaletempo *scaletempo = GST_SCALETEMPO (trans);

  if (GST_EVENT_TYPE (event) == GST_EVENT_SEGMENT) {
    GstSegment segment;

    gst_event_copy_segment (event, &segment);

    if (segment.format != GST_FORMAT_TIME
        || scaletempo->scale != ABS (segment.rate)
        || ! !scaletempo->reverse != ! !(segment.rate < 0.0)) {
      if (segment.format != GST_FORMAT_TIME || ABS (segment.rate - 1.0) < 1e-10) {
        scaletempo->scale = 1.0;
        gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (scaletempo),
            TRUE);
      } else {
        gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (scaletempo),
            FALSE);
        scaletempo->scale = ABS (segment.rate);
        scaletempo->reverse = segment.rate < 0.0;
        scaletempo->bytes_stride_scaled =
            scaletempo->bytes_stride * scaletempo->scale;
        scaletempo->frames_stride_scaled =
            scaletempo->bytes_stride_scaled / scaletempo->bytes_per_frame;
        GST_DEBUG ("%.3f scale, %.3f stride_in, %i stride_out",
            scaletempo->scale, scaletempo->frames_stride_scaled,
            (gint) (scaletempo->bytes_stride / scaletempo->bytes_per_frame));

        scaletempo->bytes_to_slide = 0;
      }
    }

    scaletempo->in_segment = segment;
    scaletempo->out_segment = segment;

    if (scaletempo->scale != 1.0 || scaletempo->reverse) {
      guint32 seqnum;

      segment.applied_rate = segment.rate;
      segment.rate = 1.0;

      if (segment.stop != -1) {
        segment.stop =
            (segment.stop - segment.start) / ABS (segment.applied_rate) +
            segment.start;
      }

      scaletempo->out_segment = segment;

      seqnum = gst_event_get_seqnum (event);
      gst_event_unref (event);

      event = gst_event_new_segment (&segment);
      gst_event_set_seqnum (event, seqnum);

      return gst_pad_push_event (GST_BASE_TRANSFORM_SRC_PAD (trans), event);
    }
  } else if (GST_EVENT_TYPE (event) == GST_EVENT_FLUSH_STOP) {
    gst_segment_init (&scaletempo->in_segment, GST_FORMAT_UNDEFINED);
    gst_segment_init (&scaletempo->out_segment, GST_FORMAT_UNDEFINED);
  } else if (GST_EVENT_TYPE (event) == GST_EVENT_GAP) {
    if (scaletempo->scale != 1.0) {
      GstClockTime gap_ts, gap_duration;
      gst_event_parse_gap (event, &gap_ts, &gap_duration);
      if (scaletempo->reverse) {
        gap_ts = scaletempo->in_segment.stop - gap_ts;
      } else {
        gap_ts = gap_ts - scaletempo->in_segment.start;
      }
      gap_ts = gap_ts / scaletempo->scale + scaletempo->in_segment.start;
      if (GST_CLOCK_TIME_IS_VALID (gap_duration)) {
        gap_duration = gap_duration / ABS (scaletempo->scale);
      }
      gst_event_unref (event);
      event = gst_event_new_gap (gap_ts, gap_duration);
    }
  }

  return GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);
}

static gboolean
gst_scaletempo_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps)
{
  GstScaletempo *scaletempo = GST_SCALETEMPO (trans);

  gint width, bps, nch, rate;
  GstAudioInfo info;
  GstAudioFormat format;

  if (!gst_audio_info_from_caps (&info, incaps))
    return FALSE;

  nch = GST_AUDIO_INFO_CHANNELS (&info);
  rate = GST_AUDIO_INFO_RATE (&info);
  width = GST_AUDIO_INFO_WIDTH (&info);
  format = GST_AUDIO_INFO_FORMAT (&info);

  bps = width / 8;

  GST_DEBUG ("caps: %" GST_PTR_FORMAT ", %d bps", incaps, bps);

  if (rate != scaletempo->sample_rate
      || nch != scaletempo->samples_per_frame
      || bps != scaletempo->bytes_per_sample || format != scaletempo->format) {
    scaletempo->sample_rate = rate;
    scaletempo->samples_per_frame = nch;
    scaletempo->bytes_per_sample = bps;
    scaletempo->bytes_per_frame = nch * bps;
    scaletempo->format = format;
    scaletempo->reinit_buffers = TRUE;
  }

  return TRUE;
}

static gboolean
gst_scaletempo_start (GstBaseTransform * trans)
{
  GstScaletempo *scaletempo = GST_SCALETEMPO (trans);

  gst_segment_init (&scaletempo->in_segment, GST_FORMAT_UNDEFINED);
  gst_segment_init (&scaletempo->out_segment, GST_FORMAT_UNDEFINED);
  scaletempo->reinit_buffers = TRUE;

  return TRUE;
}

static gboolean
gst_scaletempo_stop (GstBaseTransform * trans)
{
  GstScaletempo *scaletempo = GST_SCALETEMPO (trans);

  g_free (scaletempo->buf_queue);
  scaletempo->buf_queue = NULL;
  g_free (scaletempo->buf_overlap);
  scaletempo->buf_overlap = NULL;
  g_free (scaletempo->table_blend);
  scaletempo->table_blend = NULL;
  g_free (scaletempo->buf_pre_corr);
  scaletempo->buf_pre_corr = NULL;
  g_free (scaletempo->table_window);
  scaletempo->table_window = NULL;
  scaletempo->reinit_buffers = TRUE;

  return TRUE;
}

static gboolean
gst_scaletempo_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  GstScaletempo *scaletempo = GST_SCALETEMPO (trans);

  if (direction == GST_PAD_SRC) {
    switch (GST_QUERY_TYPE (query)) {
      case GST_QUERY_SEGMENT:
      {
        GstFormat format;
        gint64 start, stop;

        format = scaletempo->out_segment.format;

        start =
            gst_segment_to_stream_time (&scaletempo->out_segment, format,
            scaletempo->out_segment.start);
        if ((stop = scaletempo->out_segment.stop) == -1)
          stop = scaletempo->out_segment.duration;
        else
          stop =
              gst_segment_to_stream_time (&scaletempo->out_segment, format,
              stop);

        gst_query_set_segment (query, scaletempo->out_segment.rate, format,
            start, stop);
        return TRUE;
      }
      case GST_QUERY_LATENCY:{
        GstPad *peer;

        if ((peer = gst_pad_get_peer (GST_BASE_TRANSFORM_SINK_PAD (trans)))) {
          if ((gst_pad_query (peer, query))) {
            GstClockTime min, max;
            gboolean live;

            gst_query_parse_latency (query, &live, &min, &max);

            GST_DEBUG_OBJECT (scaletempo, "Peer latency: min %"
                GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
                GST_TIME_ARGS (min), GST_TIME_ARGS (max));

            /* add our own latency */
            GST_DEBUG_OBJECT (scaletempo, "Our latency: %" GST_TIME_FORMAT,
                GST_TIME_ARGS (scaletempo->latency));
            min += scaletempo->latency;
            if (max != GST_CLOCK_TIME_NONE)
              max += scaletempo->latency;

            GST_DEBUG_OBJECT (scaletempo, "Calculated total latency : min %"
                GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
                GST_TIME_ARGS (min), GST_TIME_ARGS (max));
            gst_query_set_latency (query, live, min, max);
          }
          gst_object_unref (peer);
        }

        return TRUE;
      }
      default:{
        return GST_BASE_TRANSFORM_CLASS (parent_class)->query (trans, direction,
            query);
      }
    }
  } else {
    return GST_BASE_TRANSFORM_CLASS (parent_class)->query (trans, direction,
        query);
  }
}

/* GObject vmethod implementations */
static void
gst_scaletempo_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstScaletempo *scaletempo = GST_SCALETEMPO (object);

  switch (prop_id) {
    case PROP_RATE:
      g_value_set_double (value, scaletempo->scale);
      break;
    case PROP_STRIDE:
      g_value_set_uint (value, scaletempo->ms_stride);
      break;
    case PROP_OVERLAP:
      g_value_set_double (value, scaletempo->percent_overlap);
      break;
    case PROP_SEARCH:
      g_value_set_uint (value, scaletempo->ms_search);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_scaletempo_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstScaletempo *scaletempo = GST_SCALETEMPO (object);

  switch (prop_id) {
    case PROP_STRIDE:{
      guint new_value = g_value_get_uint (value);
      if (scaletempo->ms_stride != new_value) {
        scaletempo->ms_stride = new_value;
        scaletempo->reinit_buffers = TRUE;
      }
      break;
    }
    case PROP_OVERLAP:{
      gdouble new_value = g_value_get_double (value);
      if (scaletempo->percent_overlap != new_value) {
        scaletempo->percent_overlap = new_value;
        scaletempo->reinit_buffers = TRUE;
      }
      break;
    }
    case PROP_SEARCH:{
      guint new_value = g_value_get_uint (value);
      if (scaletempo->ms_search != new_value) {
        scaletempo->ms_search = new_value;
        scaletempo->reinit_buffers = TRUE;
      }
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_scaletempo_class_init (GstScaletempoClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstBaseTransformClass *basetransform_class = GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_scaletempo_get_property);
  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_scaletempo_set_property);

  g_object_class_install_property (gobject_class, PROP_RATE,
      g_param_spec_double ("rate", "Playback Rate", "Current playback rate",
          G_MININT, G_MAXINT, 1.0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STRIDE,
      g_param_spec_uint ("stride", "Stride Length",
          "Length in milliseconds to output each stride", 1, 5000, 30,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OVERLAP,
      g_param_spec_double ("overlap", "Overlap Length",
          "Percentage of stride to overlap", 0, 1, .2,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SEARCH,
      g_param_spec_uint ("search", "Search Length",
          "Length in milliseconds to search for best overlap position", 0, 500,
          14, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &src_template);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);
  gst_element_class_set_static_metadata (gstelement_class, "Scaletempo",
      "Filter/Effect/Rate/Audio",
      "Sync audio tempo with playback rate",
      "Rov Juvano <rovjuvano@users.sourceforge.net>");

  basetransform_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_scaletempo_sink_event);
  basetransform_class->set_caps = GST_DEBUG_FUNCPTR (gst_scaletempo_set_caps);
  basetransform_class->transform_size =
      GST_DEBUG_FUNCPTR (gst_scaletempo_transform_size);
  basetransform_class->transform = GST_DEBUG_FUNCPTR (gst_scaletempo_transform);
  basetransform_class->query = GST_DEBUG_FUNCPTR (gst_scaletempo_query);
  basetransform_class->start = GST_DEBUG_FUNCPTR (gst_scaletempo_start);
  basetransform_class->stop = GST_DEBUG_FUNCPTR (gst_scaletempo_stop);
  basetransform_class->submit_input_buffer =
      GST_DEBUG_FUNCPTR (gst_scaletempo_submit_input_buffer);
}

static void
gst_scaletempo_init (GstScaletempo * scaletempo)
{
  /* defaults */
  scaletempo->ms_stride = 30;
  scaletempo->percent_overlap = .2;
  scaletempo->ms_search = 14;

  /* uninitialized */
  scaletempo->scale = 0;
  scaletempo->sample_rate = 0;
  scaletempo->frames_stride_error = 0;
  scaletempo->bytes_stride = 0;
  scaletempo->bytes_queued = 0;
  scaletempo->bytes_to_slide = 0;
  gst_segment_init (&scaletempo->in_segment, GST_FORMAT_UNDEFINED);
  gst_segment_init (&scaletempo->out_segment, GST_FORMAT_UNDEFINED);
}
