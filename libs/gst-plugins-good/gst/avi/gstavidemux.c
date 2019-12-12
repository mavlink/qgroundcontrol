/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@temple-baptist.com>
 * Copyright (C) <2006> Nokia Corporation (contact <stefan.kost@nokia.com>)
 * Copyright (C) <2009-2010> STEricsson <benjamin.gaignard@stericsson.com>
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
/* Element-Checklist-Version: 5 */

/**
 * SECTION:element-avidemux
 * @title: avidemux
 *
 * Demuxes an .avi file into raw or compressed audio and/or video streams.
 *
 * This element supports both push and pull-based scheduling, depending on the
 * capabilities of the upstream elements.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=test.avi ! avidemux name=demux  demux.audio_00 ! decodebin ! audioconvert ! audioresample ! autoaudiosink   demux.video_00 ! queue ! decodebin ! videoconvert ! videoscale ! autovideosink
 * ]| Play (parse and decode) an .avi file and try to output it to
 * an automatically detected soundcard and videosink. If the AVI file contains
 * compressed audio or video data, this will only work if you have the
 * right decoder elements/plugins installed.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>

#include "gst/riff/riff-media.h"
#include "gstavidemux.h"
#include "avi-ids.h"
#include <gst/gst-i18n-plugin.h>
#include <gst/base/gstadapter.h>
#include <gst/tag/tag.h>

#define DIV_ROUND_UP(s,v) (((s) + ((v)-1)) / (v))

#define GST_AVI_KEYFRAME (1 << 0)
#define ENTRY_IS_KEYFRAME(e) ((e)->flags == GST_AVI_KEYFRAME)
#define ENTRY_SET_KEYFRAME(e) ((e)->flags = GST_AVI_KEYFRAME)
#define ENTRY_UNSET_KEYFRAME(e) ((e)->flags = 0)


GST_DEBUG_CATEGORY_STATIC (avidemux_debug);
#define GST_CAT_DEFAULT avidemux_debug

static GstStaticPadTemplate sink_templ = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-msvideo")
    );

#ifndef GST_DISABLE_GST_DEBUG
static const char *const snap_types[2][2] = {
  {"any", "after"},
  {"before", "nearest"},
};
#endif

static void gst_avi_demux_finalize (GObject * object);

static void gst_avi_demux_reset (GstAviDemux * avi);

#if 0
static const GstEventMask *gst_avi_demux_get_event_mask (GstPad * pad);
#endif
static gboolean gst_avi_demux_handle_src_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_avi_demux_handle_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_avi_demux_push_event (GstAviDemux * avi, GstEvent * event);

#if 0
static const GstFormat *gst_avi_demux_get_src_formats (GstPad * pad);
#endif
static gboolean gst_avi_demux_handle_src_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static gboolean gst_avi_demux_src_convert (GstPad * pad, GstFormat src_format,
    gint64 src_value, GstFormat * dest_format, gint64 * dest_value);

static gboolean gst_avi_demux_do_seek (GstAviDemux * avi, GstSegment * segment,
    GstSeekFlags flags);
static gboolean gst_avi_demux_handle_seek (GstAviDemux * avi, GstPad * pad,
    GstEvent * event);
static gboolean gst_avi_demux_handle_seek_push (GstAviDemux * avi, GstPad * pad,
    GstEvent * event);
static void gst_avi_demux_loop (GstPad * pad);
static gboolean gst_avi_demux_sink_activate (GstPad * sinkpad,
    GstObject * parent);
static gboolean gst_avi_demux_sink_activate_mode (GstPad * sinkpad,
    GstObject * parent, GstPadMode mode, gboolean active);
static GstFlowReturn gst_avi_demux_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buf);
#if 0
static void gst_avi_demux_set_index (GstElement * element, GstIndex * index);
static GstIndex *gst_avi_demux_get_index (GstElement * element);
#endif
static GstStateChangeReturn gst_avi_demux_change_state (GstElement * element,
    GstStateChange transition);
static void gst_avi_demux_calculate_durations_from_index (GstAviDemux * avi);
static void gst_avi_demux_get_buffer_info (GstAviDemux * avi,
    GstAviStream * stream, guint entry_n, GstClockTime * timestamp,
    GstClockTime * ts_end, guint64 * offset, guint64 * offset_end);

static void gst_avi_demux_parse_idit (GstAviDemux * avi, GstBuffer * buf);
static void gst_avi_demux_parse_strd (GstAviDemux * avi, GstBuffer * buf);

static void parse_tag_value (GstAviDemux * avi, GstTagList * taglist,
    const gchar * type, guint8 * ptr, guint tsize);

/* GObject methods */

#define gst_avi_demux_parent_class parent_class
G_DEFINE_TYPE (GstAviDemux, gst_avi_demux, GST_TYPE_ELEMENT);

static void
gst_avi_demux_class_init (GstAviDemuxClass * klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstPadTemplate *videosrctempl, *audiosrctempl, *subsrctempl, *subpicsrctempl;
  GstCaps *audcaps, *vidcaps, *subcaps, *subpiccaps;

  GST_DEBUG_CATEGORY_INIT (avidemux_debug, "avidemux",
      0, "Demuxer for AVI streams");

  gobject_class->finalize = gst_avi_demux_finalize;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_avi_demux_change_state);
#if 0
  gstelement_class->set_index = GST_DEBUG_FUNCPTR (gst_avi_demux_set_index);
  gstelement_class->get_index = GST_DEBUG_FUNCPTR (gst_avi_demux_get_index);
#endif

  audcaps = gst_riff_create_audio_template_caps ();
  gst_caps_append (audcaps, gst_caps_new_empty_simple ("audio/x-avi-unknown"));
  audiosrctempl = gst_pad_template_new ("audio_%u",
      GST_PAD_SRC, GST_PAD_SOMETIMES, audcaps);

  vidcaps = gst_riff_create_video_template_caps ();
  gst_caps_append (vidcaps, gst_riff_create_iavs_template_caps ());
  gst_caps_append (vidcaps, gst_caps_new_empty_simple ("video/x-avi-unknown"));
  videosrctempl = gst_pad_template_new ("video_%u",
      GST_PAD_SRC, GST_PAD_SOMETIMES, vidcaps);

  subcaps = gst_caps_new_empty_simple ("application/x-subtitle-avi");
  subsrctempl = gst_pad_template_new ("subtitle_%u",
      GST_PAD_SRC, GST_PAD_SOMETIMES, subcaps);
  subpiccaps = gst_caps_new_empty_simple ("subpicture/x-xsub");
  subpicsrctempl = gst_pad_template_new ("subpicture_%u",
      GST_PAD_SRC, GST_PAD_SOMETIMES, subpiccaps);
  gst_element_class_add_pad_template (gstelement_class, audiosrctempl);
  gst_element_class_add_pad_template (gstelement_class, videosrctempl);
  gst_element_class_add_pad_template (gstelement_class, subsrctempl);
  gst_element_class_add_pad_template (gstelement_class, subpicsrctempl);
  gst_element_class_add_static_pad_template (gstelement_class, &sink_templ);

  gst_caps_unref (audcaps);
  gst_caps_unref (vidcaps);
  gst_caps_unref (subcaps);
  gst_caps_unref (subpiccaps);

  gst_element_class_set_static_metadata (gstelement_class, "Avi demuxer",
      "Codec/Demuxer",
      "Demultiplex an avi file into audio and video",
      "Erik Walthinsen <omega@cse.ogi.edu>, "
      "Wim Taymans <wim.taymans@chello.be>, "
      "Thijs Vermeir <thijsvermeir@gmail.com>");
}

static void
gst_avi_demux_init (GstAviDemux * avi)
{
  avi->sinkpad = gst_pad_new_from_static_template (&sink_templ, "sink");
  gst_pad_set_activate_function (avi->sinkpad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_sink_activate));
  gst_pad_set_activatemode_function (avi->sinkpad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_sink_activate_mode));
  gst_pad_set_chain_function (avi->sinkpad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_chain));
  gst_pad_set_event_function (avi->sinkpad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_handle_sink_event));
  gst_element_add_pad (GST_ELEMENT_CAST (avi), avi->sinkpad);

  avi->adapter = gst_adapter_new ();
  avi->flowcombiner = gst_flow_combiner_new ();

  gst_avi_demux_reset (avi);

  GST_OBJECT_FLAG_SET (avi, GST_ELEMENT_FLAG_INDEXABLE);
}

static void
gst_avi_demux_finalize (GObject * object)
{
  GstAviDemux *avi = GST_AVI_DEMUX (object);

  GST_DEBUG ("AVI: finalize");

  g_object_unref (avi->adapter);
  gst_flow_combiner_free (avi->flowcombiner);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_avi_demux_reset_stream (GstAviDemux * avi, GstAviStream * stream)
{
  g_free (stream->strh);
  g_free (stream->strf.data);
  g_free (stream->name);
  g_free (stream->index);
  g_free (stream->indexes);
  if (stream->initdata)
    gst_buffer_unref (stream->initdata);
  if (stream->extradata)
    gst_buffer_unref (stream->extradata);
  if (stream->rgb8_palette)
    gst_buffer_unref (stream->rgb8_palette);
  if (stream->pad) {
    if (stream->exposed) {
      gst_pad_set_active (stream->pad, FALSE);
      gst_element_remove_pad (GST_ELEMENT_CAST (avi), stream->pad);
      gst_flow_combiner_remove_pad (avi->flowcombiner, stream->pad);
    } else
      gst_object_unref (stream->pad);
  }
  if (stream->taglist) {
    gst_tag_list_unref (stream->taglist);
    stream->taglist = NULL;
  }
  memset (stream, 0, sizeof (GstAviStream));
}

static void
gst_avi_demux_reset (GstAviDemux * avi)
{
  gint i;

  GST_DEBUG ("AVI: reset");

  for (i = 0; i < avi->num_streams; i++)
    gst_avi_demux_reset_stream (avi, &avi->stream[i]);

  avi->header_state = GST_AVI_DEMUX_HEADER_TAG_LIST;
  avi->num_streams = 0;
  avi->num_v_streams = 0;
  avi->num_a_streams = 0;
  avi->num_t_streams = 0;
  avi->num_sp_streams = 0;
  avi->main_stream = -1;

  avi->have_group_id = FALSE;
  avi->group_id = G_MAXUINT;

  avi->state = GST_AVI_DEMUX_START;
  avi->offset = 0;
  avi->building_index = FALSE;

  avi->index_offset = 0;
  g_free (avi->avih);
  avi->avih = NULL;

#if 0
  if (avi->element_index)
    gst_object_unref (avi->element_index);
  avi->element_index = NULL;
#endif

  if (avi->seg_event) {
    gst_event_unref (avi->seg_event);
    avi->seg_event = NULL;
  }
  if (avi->seek_event) {
    gst_event_unref (avi->seek_event);
    avi->seek_event = NULL;
  }

  if (avi->globaltags)
    gst_tag_list_unref (avi->globaltags);
  avi->globaltags = NULL;

  avi->got_tags = TRUE;         /* we always want to push global tags */
  avi->have_eos = FALSE;
  avi->seekable = TRUE;

  gst_adapter_clear (avi->adapter);

  gst_segment_init (&avi->segment, GST_FORMAT_TIME);
  avi->segment_seqnum = 0;
}


/* GstElement methods */

#if 0
static const GstFormat *
gst_avi_demux_get_src_formats (GstPad * pad)
{
  GstAviStream *stream = gst_pad_get_element_private (pad);

  static const GstFormat src_a_formats[] = {
    GST_FORMAT_TIME,
    GST_FORMAT_BYTES,
    GST_FORMAT_DEFAULT,
    0
  };
  static const GstFormat src_v_formats[] = {
    GST_FORMAT_TIME,
    GST_FORMAT_DEFAULT,
    0
  };

  return (stream->strh->type == GST_RIFF_FCC_auds ?
      src_a_formats : src_v_formats);
}
#endif

/* assumes stream->strf.auds->av_bps != 0 */
static inline GstClockTime
avi_stream_convert_bytes_to_time_unchecked (GstAviStream * stream,
    guint64 bytes)
{
  return gst_util_uint64_scale_int (bytes, GST_SECOND,
      stream->strf.auds->av_bps);
}

static inline guint64
avi_stream_convert_time_to_bytes_unchecked (GstAviStream * stream,
    GstClockTime time)
{
  return gst_util_uint64_scale_int (time, stream->strf.auds->av_bps,
      GST_SECOND);
}

/* assumes stream->strh->rate != 0 */
static inline GstClockTime
avi_stream_convert_frames_to_time_unchecked (GstAviStream * stream,
    guint64 frames)
{
  return gst_util_uint64_scale (frames, stream->strh->scale * GST_SECOND,
      stream->strh->rate);
}

static inline guint64
avi_stream_convert_time_to_frames_unchecked (GstAviStream * stream,
    GstClockTime time)
{
  return gst_util_uint64_scale (time, stream->strh->rate,
      stream->strh->scale * GST_SECOND);
}

static gboolean
gst_avi_demux_src_convert (GstPad * pad,
    GstFormat src_format,
    gint64 src_value, GstFormat * dest_format, gint64 * dest_value)
{
  GstAviStream *stream = gst_pad_get_element_private (pad);
  gboolean res = TRUE;

  GST_LOG_OBJECT (pad,
      "Received  src_format:%s, src_value:%" G_GUINT64_FORMAT
      ", dest_format:%s", gst_format_get_name (src_format), src_value,
      gst_format_get_name (*dest_format));

  if (G_UNLIKELY (src_format == *dest_format)) {
    *dest_value = src_value;
    goto done;
  }
  if (G_UNLIKELY (!stream->strh || !stream->strf.data)) {
    res = FALSE;
    goto done;
  }
  if (G_UNLIKELY (stream->strh->type == GST_RIFF_FCC_vids &&
          (src_format == GST_FORMAT_BYTES
              || *dest_format == GST_FORMAT_BYTES))) {
    res = FALSE;
    goto done;
  }

  switch (src_format) {
    case GST_FORMAT_TIME:
      switch (*dest_format) {
        case GST_FORMAT_BYTES:
          *dest_value = gst_util_uint64_scale_int (src_value,
              stream->strf.auds->av_bps, GST_SECOND);
          break;
        case GST_FORMAT_DEFAULT:
          *dest_value =
              gst_util_uint64_scale_round (src_value, stream->strh->rate,
              stream->strh->scale * GST_SECOND);
          break;
        default:
          res = FALSE;
          break;
      }
      break;
    case GST_FORMAT_BYTES:
      switch (*dest_format) {
        case GST_FORMAT_TIME:
          if (stream->strf.auds->av_bps != 0) {
            *dest_value = avi_stream_convert_bytes_to_time_unchecked (stream,
                src_value);
          } else
            res = FALSE;
          break;
        default:
          res = FALSE;
          break;
      }
      break;
    case GST_FORMAT_DEFAULT:
      switch (*dest_format) {
        case GST_FORMAT_TIME:
          *dest_value =
              avi_stream_convert_frames_to_time_unchecked (stream, src_value);
          break;
        default:
          res = FALSE;
          break;
      }
      break;
    default:
      res = FALSE;
  }

done:
  GST_LOG_OBJECT (pad,
      "Returning res:%d dest_format:%s dest_value:%" G_GUINT64_FORMAT, res,
      gst_format_get_name (*dest_format), *dest_value);
  return res;
}

static gboolean
gst_avi_demux_handle_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean res = TRUE;
  GstAviDemux *avi = GST_AVI_DEMUX (parent);

  GstAviStream *stream = gst_pad_get_element_private (pad);

  if (!stream->strh || !stream->strf.data)
    return gst_pad_query_default (pad, parent, query);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:{
      gint64 pos = 0;

      GST_DEBUG ("pos query for stream %u: frames %u, bytes %u",
          stream->num, stream->current_entry, stream->current_total);

      /* FIXME, this looks clumsy */
      if (stream->strh->type == GST_RIFF_FCC_auds) {
        if (stream->is_vbr) {
          /* VBR */
          pos = avi_stream_convert_frames_to_time_unchecked (stream,
              stream->current_entry);
          GST_DEBUG_OBJECT (avi, "VBR convert frame %u, time %"
              GST_TIME_FORMAT, stream->current_entry, GST_TIME_ARGS (pos));
        } else if (stream->strf.auds->av_bps != 0) {
          /* CBR */
          pos = avi_stream_convert_bytes_to_time_unchecked (stream,
              stream->current_total);
          GST_DEBUG_OBJECT (avi,
              "CBR convert bytes %u, time %" GST_TIME_FORMAT,
              stream->current_total, GST_TIME_ARGS (pos));
        } else if (stream->idx_n != 0 && stream->total_bytes != 0) {
          /* calculate timestamps based on percentage of length */
          guint64 xlen = avi->avih->us_frame *
              avi->avih->tot_frames * GST_USECOND;

          pos = gst_util_uint64_scale (xlen, stream->current_total,
              stream->total_bytes);
          GST_DEBUG_OBJECT (avi,
              "CBR perc convert bytes %u, time %" GST_TIME_FORMAT,
              stream->current_total, GST_TIME_ARGS (pos));
        } else {
          /* we don't know */
          res = FALSE;
        }
      } else {
        if (stream->strh->rate != 0) {
          pos = gst_util_uint64_scale ((guint64) stream->current_entry *
              stream->strh->scale, GST_SECOND, (guint64) stream->strh->rate);
        } else {
          pos = stream->current_entry * avi->avih->us_frame * GST_USECOND;
        }
      }
      if (res) {
        GST_DEBUG ("pos query : %" GST_TIME_FORMAT, GST_TIME_ARGS (pos));
        gst_query_set_position (query, GST_FORMAT_TIME, pos);
      } else
        GST_WARNING ("pos query failed");
      break;
    }
    case GST_QUERY_DURATION:
    {
      GstFormat fmt;
      GstClockTime duration;

      /* only act on audio or video streams */
      if (stream->strh->type != GST_RIFF_FCC_auds &&
          stream->strh->type != GST_RIFF_FCC_vids &&
          stream->strh->type != GST_RIFF_FCC_iavs) {
        res = FALSE;
        break;
      }

      /* take stream duration, fall back to avih duration */
      if ((duration = stream->duration) == -1)
        if ((duration = stream->hdr_duration) == -1)
          duration = avi->duration;

      gst_query_parse_duration (query, &fmt, NULL);

      switch (fmt) {
        case GST_FORMAT_TIME:
          gst_query_set_duration (query, fmt, duration);
          break;
        case GST_FORMAT_DEFAULT:
        {
          gint64 dur;
          GST_DEBUG_OBJECT (query, "total frames is %" G_GUINT32_FORMAT,
              stream->idx_n);

          if (stream->idx_n > 0)
            gst_query_set_duration (query, fmt, stream->idx_n);
          else if (gst_pad_query_convert (pad, GST_FORMAT_TIME,
                  duration, fmt, &dur))
            gst_query_set_duration (query, fmt, dur);
          break;
        }
        default:
          res = FALSE;
          break;
      }
      break;
    }
    case GST_QUERY_SEEKING:{
      GstFormat fmt;

      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);
      if (fmt == GST_FORMAT_TIME) {
        gboolean seekable = TRUE;

        if (avi->streaming) {
          seekable = avi->seekable;
        }

        gst_query_set_seeking (query, GST_FORMAT_TIME, seekable,
            0, stream->duration);
        res = TRUE;
      }
      break;
    }
    case GST_QUERY_CONVERT:{
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if ((res = gst_avi_demux_src_convert (pad, src_fmt, src_val, &dest_fmt,
                  &dest_val)))
        gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      else
        res = gst_pad_query_default (pad, parent, query);
      break;
    }
    case GST_QUERY_SEGMENT:
    {
      GstFormat format;
      gint64 start, stop;

      format = avi->segment.format;

      start =
          gst_segment_to_stream_time (&avi->segment, format,
          avi->segment.start);
      if ((stop = avi->segment.stop) == -1)
        stop = avi->segment.duration;
      else
        stop = gst_segment_to_stream_time (&avi->segment, format, stop);

      gst_query_set_segment (query, avi->segment.rate, format, start, stop);
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

#if 0
static const GstEventMask *
gst_avi_demux_get_event_mask (GstPad * pad)
{
  static const GstEventMask masks[] = {
    {GST_EVENT_SEEK, GST_SEEK_METHOD_SET | GST_SEEK_FLAG_KEY_UNIT},
    {0,}
  };

  return masks;
}
#endif

#if 0
static guint64
gst_avi_demux_seek_streams (GstAviDemux * avi, guint64 offset, gboolean before)
{
  GstAviStream *stream;
  GstIndexEntry *entry;
  gint i;
  gint64 val, min = offset;

  for (i = 0; i < avi->num_streams; i++) {
    stream = &avi->stream[i];

    entry = gst_index_get_assoc_entry (avi->element_index, stream->index_id,
        before ? GST_INDEX_LOOKUP_BEFORE : GST_INDEX_LOOKUP_AFTER,
        GST_ASSOCIATION_FLAG_NONE, GST_FORMAT_BYTES, offset);

    if (before) {
      if (entry) {
        gst_index_entry_assoc_map (entry, GST_FORMAT_BYTES, &val);
        GST_DEBUG_OBJECT (avi, "stream %d, previous entry at %"
            G_GUINT64_FORMAT, i, val);
        if (val < min)
          min = val;
      }
      continue;
    }

    if (!entry) {
      GST_DEBUG_OBJECT (avi, "no position for stream %d, assuming at start", i);
      stream->current_entry = 0;
      stream->current_total = 0;
      continue;
    }

    gst_index_entry_assoc_map (entry, GST_FORMAT_BYTES, &val);
    GST_DEBUG_OBJECT (avi, "stream %d, next entry at %" G_GUINT64_FORMAT,
        i, val);

    gst_index_entry_assoc_map (entry, GST_FORMAT_TIME, &val);
    stream->current_total = val;
    gst_index_entry_assoc_map (entry, GST_FORMAT_DEFAULT, &val);
    stream->current_entry = val;
  }

  return min;
}
#endif

static gint
gst_avi_demux_index_entry_offset_search (GstAviIndexEntry * entry,
    guint64 * offset)
{
  if (entry->offset < *offset)
    return -1;
  else if (entry->offset > *offset)
    return 1;
  return 0;
}

static guint64
gst_avi_demux_seek_streams_index (GstAviDemux * avi, guint64 offset,
    gboolean before)
{
  GstAviStream *stream;
  GstAviIndexEntry *entry;
  gint i;
  gint64 val, min = offset;
  guint index = 0;

  for (i = 0; i < avi->num_streams; i++) {
    stream = &avi->stream[i];

    /* compensate for chunk header */
    offset += 8;
    entry =
        gst_util_array_binary_search (stream->index, stream->idx_n,
        sizeof (GstAviIndexEntry),
        (GCompareDataFunc) gst_avi_demux_index_entry_offset_search,
        before ? GST_SEARCH_MODE_BEFORE : GST_SEARCH_MODE_AFTER, &offset, NULL);
    offset -= 8;

    if (entry)
      index = entry - stream->index;

    if (before) {
      if (entry) {
        val = stream->index[index].offset;
        GST_DEBUG_OBJECT (avi,
            "stream %d, previous entry at %" G_GUINT64_FORMAT, i, val);
        if (val < min)
          min = val;
      }
      continue;
    }

    if (!entry) {
      GST_DEBUG_OBJECT (avi, "no position for stream %d, assuming at start", i);
      stream->current_entry = 0;
      stream->current_total = 0;
      continue;
    }

    val = stream->index[index].offset - 8;
    GST_DEBUG_OBJECT (avi, "stream %d, next entry at %" G_GUINT64_FORMAT, i,
        val);

    stream->current_total = stream->index[index].total;
    stream->current_entry = index;
  }

  return min;
}

#define GST_AVI_SEEK_PUSH_DISPLACE     (4 * GST_SECOND)

static gboolean
gst_avi_demux_handle_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res = TRUE;
  GstAviDemux *avi = GST_AVI_DEMUX (parent);

  GST_DEBUG_OBJECT (avi,
      "have event type %s: %p on sink pad", GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    {
      gint64 boffset, offset = 0;
      GstSegment segment;
      GstEvent *segment_event;

      /* some debug output */
      gst_event_copy_segment (event, &segment);
      GST_DEBUG_OBJECT (avi, "received newsegment %" GST_SEGMENT_FORMAT,
          &segment);

      /* chain will send initial newsegment after pads have been added */
      if (avi->state != GST_AVI_DEMUX_MOVI) {
        GST_DEBUG_OBJECT (avi, "still starting, eating event");
        goto exit;
      }

      /* we only expect a BYTE segment, e.g. following a seek */
      if (segment.format != GST_FORMAT_BYTES) {
        GST_DEBUG_OBJECT (avi, "unsupported segment format, ignoring");
        goto exit;
      }

      if (avi->have_index) {
        GstAviIndexEntry *entry;
        guint i = 0, index = 0, k = 0;
        GstAviStream *stream;

        /* compensate chunk header, stored index offset points after header */
        boffset = segment.start + 8;
        /* find which stream we're on */
        do {
          stream = &avi->stream[i];

          /* find the index for start bytes offset */
          entry = gst_util_array_binary_search (stream->index,
              stream->idx_n, sizeof (GstAviIndexEntry),
              (GCompareDataFunc) gst_avi_demux_index_entry_offset_search,
              GST_SEARCH_MODE_AFTER, &boffset, NULL);

          if (entry == NULL)
            continue;
          index = entry - stream->index;

          /* we are on the stream with a chunk start offset closest to start */
          if (!offset || stream->index[index].offset < offset) {
            offset = stream->index[index].offset;
            k = i;
          }
          /* exact match needs no further searching */
          if (stream->index[index].offset == boffset)
            break;
        } while (++i < avi->num_streams);
        boffset -= 8;
        offset -= 8;
        stream = &avi->stream[k];

        /* so we have no idea what is to come, or where we are */
        if (!offset) {
          GST_WARNING_OBJECT (avi, "insufficient index data, forcing EOS");
          goto eos;
        }

        /* get the ts corresponding to start offset bytes for the stream */
        gst_avi_demux_get_buffer_info (avi, stream, index,
            (GstClockTime *) & segment.time, NULL, NULL, NULL);
#if 0
      } else if (avi->element_index) {
        GstIndexEntry *entry;

        /* Let's check if we have an index entry for this position */
        entry = gst_index_get_assoc_entry (avi->element_index, avi->index_id,
            GST_INDEX_LOOKUP_AFTER, GST_ASSOCIATION_FLAG_NONE,
            GST_FORMAT_BYTES, segment.start);

        /* we can not go where we have not yet been before ... */
        if (!entry) {
          GST_WARNING_OBJECT (avi, "insufficient index data, forcing EOS");
          goto eos;
        }

        gst_index_entry_assoc_map (entry, GST_FORMAT_TIME,
            (gint64 *) & segment.time);
        gst_index_entry_assoc_map (entry, GST_FORMAT_BYTES, &offset);
#endif
      } else {
        GST_WARNING_OBJECT (avi, "no index data, forcing EOS");
        goto eos;
      }

      segment.format = GST_FORMAT_TIME;
      segment.start = segment.time;
      segment.stop = GST_CLOCK_TIME_NONE;
      segment.position = segment.start;

      /* rescue duration */
      segment.duration = avi->segment.duration;

      /* set up segment and send downstream */
      gst_segment_copy_into (&segment, &avi->segment);

      GST_DEBUG_OBJECT (avi, "Pushing newseg %" GST_SEGMENT_FORMAT, &segment);
      avi->segment_seqnum = gst_event_get_seqnum (event);
      segment_event = gst_event_new_segment (&segment);
      gst_event_set_seqnum (segment_event, gst_event_get_seqnum (event));
      gst_avi_demux_push_event (avi, segment_event);

      GST_DEBUG_OBJECT (avi, "next chunk expected at %" G_GINT64_FORMAT,
          boffset);

      /* adjust state for streaming thread accordingly */
      if (avi->have_index)
        gst_avi_demux_seek_streams_index (avi, offset, FALSE);
#if 0
      else
        gst_avi_demux_seek_streams (avi, offset, FALSE);
#endif

      /* set up streaming thread */
      g_assert (offset >= boffset);
      avi->offset = boffset;
      avi->todrop = offset - boffset;

    exit:
      gst_event_unref (event);
      res = TRUE;
      break;
    eos:
      /* set up for EOS */
      avi->have_eos = TRUE;
      goto exit;
    }
    case GST_EVENT_EOS:
    {
      if (avi->state != GST_AVI_DEMUX_MOVI) {
        gst_event_unref (event);
        GST_ELEMENT_ERROR (avi, STREAM, DEMUX,
            (NULL), ("got eos and didn't receive a complete header object"));
      } else if (!gst_avi_demux_push_event (avi, event)) {
        GST_ELEMENT_ERROR (avi, STREAM, DEMUX,
            (NULL), ("got eos but no streams (yet)"));
      }
      break;
    }
    case GST_EVENT_FLUSH_STOP:
    {
      gint i;

      gst_adapter_clear (avi->adapter);
      avi->have_eos = FALSE;
      for (i = 0; i < avi->num_streams; i++) {
        avi->stream[i].discont = TRUE;
      }
      /* fall through to default case so that the event gets passed downstream */
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static gboolean
gst_avi_demux_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res = TRUE;
  GstAviDemux *avi = GST_AVI_DEMUX (parent);

  GST_DEBUG_OBJECT (avi,
      "have event type %s: %p on src pad", GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      if (!avi->streaming) {
        res = gst_avi_demux_handle_seek (avi, pad, event);
      } else {
        res = gst_avi_demux_handle_seek_push (avi, pad, event);
      }
      gst_event_unref (event);
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

/* streaming helper (push) */

/*
 * gst_avi_demux_peek_chunk_info:
 * @avi: Avi object
 * @tag: holder for tag
 * @size: holder for tag size
 *
 * Peek next chunk info (tag and size)
 *
 * Returns: TRUE when one chunk info has been got
 */
static gboolean
gst_avi_demux_peek_chunk_info (GstAviDemux * avi, guint32 * tag, guint32 * size)
{
  const guint8 *data = NULL;

  if (gst_adapter_available (avi->adapter) < 8)
    return FALSE;

  data = gst_adapter_map (avi->adapter, 8);
  *tag = GST_READ_UINT32_LE (data);
  *size = GST_READ_UINT32_LE (data + 4);
  gst_adapter_unmap (avi->adapter);

  return TRUE;
}

/*
 * gst_avi_demux_peek_chunk:
 * @avi: Avi object
 * @tag: holder for tag
 * @size: holder for tag size
 *
 * Peek enough data for one full chunk
 *
 * Returns: %TRUE when one chunk has been got
 */
static gboolean
gst_avi_demux_peek_chunk (GstAviDemux * avi, guint32 * tag, guint32 * size)
{
  guint32 peek_size = 0;
  gint available;

  if (!gst_avi_demux_peek_chunk_info (avi, tag, size))
    goto peek_failed;

  /* size 0 -> empty data buffer would surprise most callers,
   * large size -> do not bother trying to squeeze that into adapter,
   * so we throw poor man's exception, which can be caught if caller really
   * wants to handle 0 size chunk */
  if (!(*size) || (*size) >= (1 << 30))
    goto strange_size;

  peek_size = (*size + 1) & ~1;
  available = gst_adapter_available (avi->adapter);

  GST_DEBUG_OBJECT (avi,
      "Need to peek chunk of %d bytes to read chunk %" GST_FOURCC_FORMAT
      ", %d bytes available", *size, GST_FOURCC_ARGS (*tag), available);

  if (available < (8 + peek_size))
    goto need_more;

  return TRUE;

  /* ERRORS */
peek_failed:
  {
    GST_INFO_OBJECT (avi, "Failed to peek");
    return FALSE;
  }
strange_size:
  {
    GST_INFO_OBJECT (avi,
        "Invalid/unexpected chunk size %d for tag %" GST_FOURCC_FORMAT, *size,
        GST_FOURCC_ARGS (*tag));
    /* chain should give up */
    avi->abort_buffering = TRUE;
    return FALSE;
  }
need_more:
  {
    GST_INFO_OBJECT (avi, "need more %d < %" G_GUINT32_FORMAT,
        available, 8 + peek_size);
    return FALSE;
  }
}

/* AVI init */

/*
 * gst_avi_demux_parse_file_header:
 * @element: caller element (used for errors/debug).
 * @buf: input data to be used for parsing.
 *
 * "Open" a RIFF/AVI file. The buffer should be at least 12
 * bytes long. Takes ownership of @buf.
 *
 * Returns: TRUE if the file is a RIFF/AVI file, FALSE otherwise.
 *          Throws an error, caller should error out (fatal).
 */
static gboolean
gst_avi_demux_parse_file_header (GstElement * element, GstBuffer * buf)
{
  guint32 doctype;
  GstClockTime stamp;

  stamp = gst_util_get_timestamp ();

  /* riff_parse posts an error */
  if (!gst_riff_parse_file_header (element, buf, &doctype))
    return FALSE;

  if (doctype != GST_RIFF_RIFF_AVI)
    goto not_avi;

  stamp = gst_util_get_timestamp () - stamp;
  GST_DEBUG_OBJECT (element, "header parsing took %" GST_TIME_FORMAT,
      GST_TIME_ARGS (stamp));

  return TRUE;

  /* ERRORS */
not_avi:
  {
    GST_ELEMENT_ERROR (element, STREAM, WRONG_TYPE, (NULL),
        ("File is not an AVI file: 0x%" G_GINT32_MODIFIER "x", doctype));
    return FALSE;
  }
}

/*
 * Read AVI file tag when streaming
 */
static GstFlowReturn
gst_avi_demux_stream_init_push (GstAviDemux * avi)
{
  if (gst_adapter_available (avi->adapter) >= 12) {
    GstBuffer *tmp;

    tmp = gst_adapter_take_buffer (avi->adapter, 12);

    GST_DEBUG ("Parsing avi header");
    if (!gst_avi_demux_parse_file_header (GST_ELEMENT_CAST (avi), tmp)) {
      return GST_FLOW_ERROR;
    }
    GST_DEBUG ("header ok");
    avi->offset += 12;

    avi->state = GST_AVI_DEMUX_HEADER;
  }
  return GST_FLOW_OK;
}

/*
 * Read AVI file tag
 */
static GstFlowReturn
gst_avi_demux_stream_init_pull (GstAviDemux * avi)
{
  GstFlowReturn res;
  GstBuffer *buf = NULL;

  res = gst_pad_pull_range (avi->sinkpad, avi->offset, 12, &buf);
  if (res != GST_FLOW_OK)
    return res;
  else if (!gst_avi_demux_parse_file_header (GST_ELEMENT_CAST (avi), buf))
    goto wrong_header;

  avi->offset += 12;

  return GST_FLOW_OK;

  /* ERRORS */
wrong_header:
  {
    GST_DEBUG_OBJECT (avi, "error parsing file header");
    return GST_FLOW_ERROR;
  }
}

/* AVI header handling */
/*
 * gst_avi_demux_parse_avih:
 * @avi: caller element (used for errors/debug).
 * @buf: input data to be used for parsing.
 * @avih: pointer to structure (filled in by function) containing
 *        stream information (such as flags, number of streams, etc.).
 *
 * Read 'avih' header. Discards buffer after use.
 *
 * Returns: TRUE on success, FALSE otherwise. Throws an error if
 *          the header is invalid. The caller should error out
 *          (fatal).
 */
static gboolean
gst_avi_demux_parse_avih (GstAviDemux * avi,
    GstBuffer * buf, gst_riff_avih ** _avih)
{
  gst_riff_avih *avih;
  gsize size;

  if (buf == NULL)
    goto no_buffer;

  size = gst_buffer_get_size (buf);
  if (size < sizeof (gst_riff_avih))
    goto avih_too_small;

  avih = g_malloc (size);
  gst_buffer_extract (buf, 0, avih, size);

#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  avih->us_frame = GUINT32_FROM_LE (avih->us_frame);
  avih->max_bps = GUINT32_FROM_LE (avih->max_bps);
  avih->pad_gran = GUINT32_FROM_LE (avih->pad_gran);
  avih->flags = GUINT32_FROM_LE (avih->flags);
  avih->tot_frames = GUINT32_FROM_LE (avih->tot_frames);
  avih->init_frames = GUINT32_FROM_LE (avih->init_frames);
  avih->streams = GUINT32_FROM_LE (avih->streams);
  avih->bufsize = GUINT32_FROM_LE (avih->bufsize);
  avih->width = GUINT32_FROM_LE (avih->width);
  avih->height = GUINT32_FROM_LE (avih->height);
  avih->scale = GUINT32_FROM_LE (avih->scale);
  avih->rate = GUINT32_FROM_LE (avih->rate);
  avih->start = GUINT32_FROM_LE (avih->start);
  avih->length = GUINT32_FROM_LE (avih->length);
#endif

  /* debug stuff */
  GST_INFO_OBJECT (avi, "avih tag found:");
  GST_INFO_OBJECT (avi, " us_frame    %u", avih->us_frame);
  GST_INFO_OBJECT (avi, " max_bps     %u", avih->max_bps);
  GST_INFO_OBJECT (avi, " pad_gran    %u", avih->pad_gran);
  GST_INFO_OBJECT (avi, " flags       0x%08x", avih->flags);
  GST_INFO_OBJECT (avi, " tot_frames  %u", avih->tot_frames);
  GST_INFO_OBJECT (avi, " init_frames %u", avih->init_frames);
  GST_INFO_OBJECT (avi, " streams     %u", avih->streams);
  GST_INFO_OBJECT (avi, " bufsize     %u", avih->bufsize);
  GST_INFO_OBJECT (avi, " width       %u", avih->width);
  GST_INFO_OBJECT (avi, " height      %u", avih->height);
  GST_INFO_OBJECT (avi, " scale       %u", avih->scale);
  GST_INFO_OBJECT (avi, " rate        %u", avih->rate);
  GST_INFO_OBJECT (avi, " start       %u", avih->start);
  GST_INFO_OBJECT (avi, " length      %u", avih->length);

  *_avih = avih;
  gst_buffer_unref (buf);

  if (avih->us_frame != 0 && avih->tot_frames != 0)
    avi->duration =
        (guint64) avih->us_frame * (guint64) avih->tot_frames * 1000;
  else
    avi->duration = GST_CLOCK_TIME_NONE;

  GST_INFO_OBJECT (avi, " header duration  %" GST_TIME_FORMAT,
      GST_TIME_ARGS (avi->duration));

  return TRUE;

  /* ERRORS */
no_buffer:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL), ("No buffer"));
    return FALSE;
  }
avih_too_small:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("Too small avih (%" G_GSIZE_FORMAT " available, %d needed)",
            size, (int) sizeof (gst_riff_avih)));
    gst_buffer_unref (buf);
    return FALSE;
  }
}

/*
 * gst_avi_demux_parse_superindex:
 * @avi: caller element (used for debugging/errors).
 * @buf: input data to use for parsing.
 * @locations: locations in the file (byte-offsets) that contain
 *             the actual indexes (see get_avi_demux_parse_subindex()).
 *             The array ends with GST_BUFFER_OFFSET_NONE.
 *
 * Reads superindex (openDML-2 spec stuff) from the provided data.
 *
 * Returns: TRUE on success, FALSE otherwise. Indexes should be skipped
 *          on error, but they are not fatal.
 */
static gboolean
gst_avi_demux_parse_superindex (GstAviDemux * avi,
    GstBuffer * buf, guint64 ** _indexes)
{
  GstMapInfo map;
  guint8 *data;
  guint16 bpe = 16;
  guint32 num, i;
  guint64 *indexes;
  gsize size;

  *_indexes = NULL;

  if (buf) {
    gst_buffer_map (buf, &map, GST_MAP_READ);
    data = map.data;
    size = map.size;
  } else {
    data = NULL;
    size = 0;
  }

  if (size < 24)
    goto too_small;

  /* check type of index. The opendml2 specs state that
   * there should be 4 dwords per array entry. Type can be
   * either frame or field (and we don't care). */
  if (GST_READ_UINT16_LE (data) != 4 ||
      (data[2] & 0xfe) != 0x0 || data[3] != 0x0) {
    GST_WARNING_OBJECT (avi,
        "Superindex for stream has unexpected "
        "size_entry %d (bytes) or flags 0x%02x/0x%02x",
        GST_READ_UINT16_LE (data), data[2], data[3]);
    bpe = GST_READ_UINT16_LE (data) * 4;
  }
  num = GST_READ_UINT32_LE (&data[4]);

  GST_DEBUG_OBJECT (avi, "got %d indexes", num);

  /* this can't work out well ... */
  if (num > G_MAXUINT32 >> 1 || bpe < 8) {
    goto invalid_params;
  }

  indexes = g_new (guint64, num + 1);
  for (i = 0; i < num; i++) {
    if (size < 24 + bpe * (i + 1))
      break;
    indexes[i] = GST_READ_UINT64_LE (&data[24 + bpe * i]);
    GST_DEBUG_OBJECT (avi, "index %d at %" G_GUINT64_FORMAT, i, indexes[i]);
  }
  indexes[i] = GST_BUFFER_OFFSET_NONE;
  *_indexes = indexes;

  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  return TRUE;

  /* ERRORS */
too_small:
  {
    GST_ERROR_OBJECT (avi,
        "Not enough data to parse superindex (%" G_GSIZE_FORMAT
        " available, 24 needed)", size);
    if (buf) {
      gst_buffer_unmap (buf, &map);
      gst_buffer_unref (buf);
    }
    return FALSE;
  }
invalid_params:
  {
    GST_ERROR_OBJECT (avi, "invalid index parameters (num = %d, bpe = %d)",
        num, bpe);
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return FALSE;
  }
}

/* add an entry to the index of a stream. @num should be an estimate of the
 * total amount of index entries for all streams and is used to dynamically
 * allocate memory for the index entries. */
static inline gboolean
gst_avi_demux_add_index (GstAviDemux * avi, GstAviStream * stream,
    guint num, GstAviIndexEntry * entry)
{
  /* ensure index memory */
  if (G_UNLIKELY (stream->idx_n >= stream->idx_max)) {
    guint idx_max = stream->idx_max;
    GstAviIndexEntry *new_idx;

    /* we need to make some more room */
    if (idx_max == 0) {
      /* initial size guess, assume each stream has an equal amount of entries,
       * overshoot with at least 8K */
      idx_max = (num / avi->num_streams) + (8192 / sizeof (GstAviIndexEntry));
    } else {
      idx_max += 8192 / sizeof (GstAviIndexEntry);
      GST_DEBUG_OBJECT (avi, "expanded index from %u to %u",
          stream->idx_max, idx_max);
    }
    new_idx = g_try_renew (GstAviIndexEntry, stream->index, idx_max);
    /* out of memory, if this fails stream->index is untouched. */
    if (G_UNLIKELY (!new_idx))
      return FALSE;
    /* use new index */
    stream->index = new_idx;
    stream->idx_max = idx_max;
  }

  /* update entry total and stream stats. The entry total can be converted to
   * the timestamp of the entry easily. */
  if (stream->strh->type == GST_RIFF_FCC_auds) {
    gint blockalign;

    if (stream->is_vbr) {
      entry->total = stream->total_blocks;
    } else {
      entry->total = stream->total_bytes;
    }
    blockalign = stream->strf.auds->blockalign;
    if (blockalign > 0)
      stream->total_blocks += DIV_ROUND_UP (entry->size, blockalign);
    else
      stream->total_blocks++;
  } else {
    if (stream->is_vbr) {
      entry->total = stream->idx_n;
    } else {
      entry->total = stream->total_bytes;
    }
  }
  stream->total_bytes += entry->size;
  if (ENTRY_IS_KEYFRAME (entry))
    stream->n_keyframes++;

  /* and add */
  GST_LOG_OBJECT (avi,
      "Adding stream %u, index entry %d, kf %d, size %u "
      ", offset %" G_GUINT64_FORMAT ", total %" G_GUINT64_FORMAT, stream->num,
      stream->idx_n, ENTRY_IS_KEYFRAME (entry), entry->size, entry->offset,
      entry->total);
  stream->index[stream->idx_n++] = *entry;

  return TRUE;
}

/* given @entry_n in @stream, calculate info such as timestamps and
 * offsets for the entry. */
static void
gst_avi_demux_get_buffer_info (GstAviDemux * avi, GstAviStream * stream,
    guint entry_n, GstClockTime * timestamp, GstClockTime * ts_end,
    guint64 * offset, guint64 * offset_end)
{
  GstAviIndexEntry *entry;

  entry = &stream->index[entry_n];

  if (stream->is_vbr) {
    /* VBR stream next timestamp */
    if (stream->strh->type == GST_RIFF_FCC_auds) {
      if (timestamp)
        *timestamp =
            avi_stream_convert_frames_to_time_unchecked (stream, entry->total);
      if (ts_end) {
        gint size = 1;
        if (G_LIKELY (entry_n + 1 < stream->idx_n))
          size = stream->index[entry_n + 1].total - entry->total;
        *ts_end = avi_stream_convert_frames_to_time_unchecked (stream,
            entry->total + size);
      }
    } else {
      if (timestamp)
        *timestamp =
            avi_stream_convert_frames_to_time_unchecked (stream, entry_n);
      if (ts_end)
        *ts_end = avi_stream_convert_frames_to_time_unchecked (stream,
            entry_n + 1);
    }
  } else if (stream->strh->type == GST_RIFF_FCC_auds) {
    /* constant rate stream */
    if (timestamp)
      *timestamp =
          avi_stream_convert_bytes_to_time_unchecked (stream, entry->total);
    if (ts_end)
      *ts_end = avi_stream_convert_bytes_to_time_unchecked (stream,
          entry->total + entry->size);
  }
  if (stream->strh->type == GST_RIFF_FCC_vids) {
    /* video offsets are the frame number */
    if (offset)
      *offset = entry_n;
    if (offset_end)
      *offset_end = entry_n + 1;
  } else {
    /* no offsets for audio */
    if (offset)
      *offset = -1;
    if (offset_end)
      *offset_end = -1;
  }
}

/* collect and debug stats about the indexes for all streams.
 * This method is also responsible for filling in the stream duration
 * as measured by the amount of index entries.
 *
 * Returns TRUE if the index is not empty, else FALSE */
static gboolean
gst_avi_demux_do_index_stats (GstAviDemux * avi)
{
  guint total_idx = 0;
  guint i;
#ifndef GST_DISABLE_GST_DEBUG
  guint total_max = 0;
#endif

  /* get stream stats now */
  for (i = 0; i < avi->num_streams; i++) {
    GstAviStream *stream;

    if (G_UNLIKELY (!(stream = &avi->stream[i])))
      continue;
    if (G_UNLIKELY (!stream->strh))
      continue;
    if (G_UNLIKELY (!stream->index || stream->idx_n == 0))
      continue;

    /* we interested in the end_ts of the last entry, which is the total
     * duration of this stream */
    gst_avi_demux_get_buffer_info (avi, stream, stream->idx_n - 1,
        NULL, &stream->idx_duration, NULL, NULL);

    total_idx += stream->idx_n;
#ifndef GST_DISABLE_GST_DEBUG
    total_max += stream->idx_max;
#endif
    GST_INFO_OBJECT (avi, "Stream %d, dur %" GST_TIME_FORMAT ", %6u entries, "
        "%5u keyframes, entry size = %2u, total size = %10u, allocated %10u",
        i, GST_TIME_ARGS (stream->idx_duration), stream->idx_n,
        stream->n_keyframes, (guint) sizeof (GstAviIndexEntry),
        (guint) (stream->idx_n * sizeof (GstAviIndexEntry)),
        (guint) (stream->idx_max * sizeof (GstAviIndexEntry)));

    /* knowing all that we do, that also includes avg bitrate */
    if (!stream->taglist) {
      stream->taglist = gst_tag_list_new_empty ();
    }
    if (stream->total_bytes && stream->idx_duration)
      gst_tag_list_add (stream->taglist, GST_TAG_MERGE_REPLACE,
          GST_TAG_BITRATE,
          (guint) gst_util_uint64_scale (stream->total_bytes * 8,
              GST_SECOND, stream->idx_duration), NULL);
  }
  total_idx *= sizeof (GstAviIndexEntry);
#ifndef GST_DISABLE_GST_DEBUG
  total_max *= sizeof (GstAviIndexEntry);
#endif
  GST_INFO_OBJECT (avi, "%u bytes for index vs %u ideally, %u wasted",
      total_max, total_idx, total_max - total_idx);

  if (total_idx == 0) {
    GST_WARNING_OBJECT (avi, "Index is empty !");
    return FALSE;
  }
  return TRUE;
}

/*
 * gst_avi_demux_parse_subindex:
 * @avi: Avi object
 * @buf: input data to use for parsing.
 * @stream: stream context.
 * @entries_list: a list (returned by the function) containing all the
 *           indexes parsed in this specific subindex. The first
 *           entry is also a pointer to allocated memory that needs
 *           to be free´ed. May be NULL if no supported indexes were
 *           found.
 *
 * Reads superindex (openDML-2 spec stuff) from the provided data.
 * The buffer should contain a GST_RIFF_TAG_ix?? chunk.
 *
 * Returns: TRUE on success, FALSE otherwise. Errors are fatal, we
 *          throw an error, caller should bail out asap.
 */
static gboolean
gst_avi_demux_parse_subindex (GstAviDemux * avi, GstAviStream * stream,
    GstBuffer * buf)
{
  GstMapInfo map;
  guint8 *data;
  guint16 bpe;
  guint32 num, i;
  guint64 baseoff;

  if (buf == NULL)
    return TRUE;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  data = map.data;

  /* check size */
  if (map.size < 24)
    goto too_small;

  /* We don't support index-data yet */
  if (data[3] & 0x80)
    goto not_implemented;

  /* check type of index. The opendml2 specs state that
   * there should be 4 dwords per array entry. Type can be
   * either frame or field (and we don't care). */
  bpe = (data[2] & 0x01) ? 12 : 8;
  if (GST_READ_UINT16_LE (data) != bpe / 4 ||
      (data[2] & 0xfe) != 0x0 || data[3] != 0x1) {
    GST_WARNING_OBJECT (avi,
        "Superindex for stream %d has unexpected "
        "size_entry %d (bytes) or flags 0x%02x/0x%02x",
        stream->num, GST_READ_UINT16_LE (data), data[2], data[3]);
    bpe = GST_READ_UINT16_LE (data) * 4;
  }
  num = GST_READ_UINT32_LE (&data[4]);
  baseoff = GST_READ_UINT64_LE (&data[12]);

  /* If there's nothing, just return ! */
  if (num == 0)
    goto empty_index;

  GST_INFO_OBJECT (avi, "Parsing subindex, nr_entries = %6d", num);

  for (i = 0; i < num; i++) {
    GstAviIndexEntry entry;

    if (map.size < 24 + bpe * (i + 1))
      break;

    /* fill in offset and size. offset contains the keyframe flag in the
     * upper bit*/
    entry.offset = baseoff + GST_READ_UINT32_LE (&data[24 + bpe * i]);
    entry.size = GST_READ_UINT32_LE (&data[24 + bpe * i + 4]);
    /* handle flags */
    if (stream->strh->type == GST_RIFF_FCC_auds) {
      /* all audio frames are keyframes */
      ENTRY_SET_KEYFRAME (&entry);
    } else {
      /* else read flags */
      entry.flags = (entry.size & 0x80000000) ? 0 : GST_AVI_KEYFRAME;
    }
    entry.size &= ~0x80000000;

    /* and add */
    if (G_UNLIKELY (!gst_avi_demux_add_index (avi, stream, num, &entry)))
      goto out_of_mem;
  }
done:
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  return TRUE;

  /* ERRORS */
too_small:
  {
    GST_ERROR_OBJECT (avi,
        "Not enough data to parse subindex (%" G_GSIZE_FORMAT
        " available, 24 needed)", map.size);
    goto done;                  /* continue */
  }
not_implemented:
  {
    GST_ELEMENT_ERROR (avi, STREAM, NOT_IMPLEMENTED, (NULL),
        ("Subindex-is-data is not implemented"));
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return FALSE;
  }
empty_index:
  {
    GST_DEBUG_OBJECT (avi, "the index is empty");
    goto done;                  /* continue */
  }
out_of_mem:
  {
    GST_ELEMENT_ERROR (avi, RESOURCE, NO_SPACE_LEFT, (NULL),
        ("Cannot allocate memory for %u*%u=%u bytes",
            (guint) sizeof (GstAviIndexEntry), num,
            (guint) sizeof (GstAviIndexEntry) * num));
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return FALSE;
  }
}

/*
 * Create and push a flushing seek event upstream
 */
static gboolean
perform_seek_to_offset (GstAviDemux * demux, guint64 offset, guint32 seqnum)
{
  GstEvent *event;
  gboolean res = 0;

  GST_DEBUG_OBJECT (demux, "Seeking to %" G_GUINT64_FORMAT, offset);

  event =
      gst_event_new_seek (1.0, GST_FORMAT_BYTES,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET, offset,
      GST_SEEK_TYPE_NONE, -1);
  gst_event_set_seqnum (event, seqnum);
  res = gst_pad_push_event (demux->sinkpad, event);

  if (res)
    demux->offset = offset;
  return res;
}

/*
 * Read AVI index when streaming
 */
static gboolean
gst_avi_demux_read_subindexes_push (GstAviDemux * avi)
{
  guint32 tag = 0, size;
  GstBuffer *buf = NULL;
  guint odml_stream;

  GST_DEBUG_OBJECT (avi, "read subindexes for %d streams", avi->num_streams);

  if (avi->odml_subidxs[avi->odml_subidx] != avi->offset)
    return FALSE;

  if (!gst_avi_demux_peek_chunk (avi, &tag, &size))
    return TRUE;

  /* this is the ODML chunk we expect */
  odml_stream = avi->odml_stream;

  if ((tag != GST_MAKE_FOURCC ('i', 'x', '0' + odml_stream / 10,
              '0' + odml_stream % 10)) &&
      (tag != GST_MAKE_FOURCC ('0' + odml_stream / 10,
              '0' + odml_stream % 10, 'i', 'x'))) {
    GST_WARNING_OBJECT (avi, "Not an ix## chunk (%" GST_FOURCC_FORMAT ")",
        GST_FOURCC_ARGS (tag));
    return FALSE;
  }

  avi->offset += 8 + GST_ROUND_UP_2 (size);
  /* flush chunk header so we get just the 'size' payload data */
  gst_adapter_flush (avi->adapter, 8);
  buf = gst_adapter_take_buffer (avi->adapter, size);

  if (!gst_avi_demux_parse_subindex (avi, &avi->stream[odml_stream], buf))
    return FALSE;

  /* we parsed the index, go to next subindex */
  avi->odml_subidx++;

  if (avi->odml_subidxs[avi->odml_subidx] == GST_BUFFER_OFFSET_NONE) {
    /* we reached the end of the indexes for this stream, move to the next
     * stream to handle the first index */
    avi->odml_stream++;
    avi->odml_subidx = 0;

    if (avi->odml_stream < avi->num_streams) {
      /* there are more indexes */
      avi->odml_subidxs = avi->stream[avi->odml_stream].indexes;
    } else {
      /* we're done, get stream stats now */
      avi->have_index = gst_avi_demux_do_index_stats (avi);

      return TRUE;
    }
  }

  /* seek to next index */
  return perform_seek_to_offset (avi, avi->odml_subidxs[avi->odml_subidx],
      avi->segment_seqnum);
}

/*
 * Read AVI index
 */
static void
gst_avi_demux_read_subindexes_pull (GstAviDemux * avi)
{
  guint32 tag;
  GstBuffer *buf;
  gint i, n;

  GST_DEBUG_OBJECT (avi, "read subindexes for %d streams", avi->num_streams);

  for (n = 0; n < avi->num_streams; n++) {
    GstAviStream *stream = &avi->stream[n];

    if (stream->indexes == NULL)
      continue;

    for (i = 0; stream->indexes[i] != GST_BUFFER_OFFSET_NONE; i++) {
      if (gst_riff_read_chunk (GST_ELEMENT_CAST (avi), avi->sinkpad,
              &stream->indexes[i], &tag, &buf) != GST_FLOW_OK)
        continue;
      else if ((tag != GST_MAKE_FOURCC ('i', 'x', '0' + stream->num / 10,
                  '0' + stream->num % 10)) &&
          (tag != GST_MAKE_FOURCC ('0' + stream->num / 10,
                  '0' + stream->num % 10, 'i', 'x'))) {
        /* Some ODML files (created by god knows what muxer) have a ##ix format
         * instead of the 'official' ix##. They are still valid though. */
        GST_WARNING_OBJECT (avi, "Not an ix## chunk (%" GST_FOURCC_FORMAT ")",
            GST_FOURCC_ARGS (tag));
        gst_buffer_unref (buf);
        continue;
      }

      if (!gst_avi_demux_parse_subindex (avi, stream, buf))
        continue;
    }

    g_free (stream->indexes);
    stream->indexes = NULL;
  }
  /* get stream stats now */
  avi->have_index = gst_avi_demux_do_index_stats (avi);
}

/*
 * gst_avi_demux_riff_parse_vprp:
 * @element: caller element (used for debugging/error).
 * @buf: input data to be used for parsing, stripped from header.
 * @vprp: a pointer (returned by this function) to a filled-in vprp
 *        structure. Caller should free it.
 *
 * Parses a video stream´s vprp. This function takes ownership of @buf.
 *
 * Returns: TRUE if parsing succeeded, otherwise FALSE. The stream
 *          should be skipped on error, but it is not fatal.
 */
static gboolean
gst_avi_demux_riff_parse_vprp (GstElement * element,
    GstBuffer * buf, gst_riff_vprp ** _vprp)
{
  gst_riff_vprp *vprp;
  gint k;
  gsize size;

  g_return_val_if_fail (buf != NULL, FALSE);
  g_return_val_if_fail (_vprp != NULL, FALSE);

  size = gst_buffer_get_size (buf);

  if (size < G_STRUCT_OFFSET (gst_riff_vprp, field_info))
    goto too_small;

  vprp = g_malloc (size);
  gst_buffer_extract (buf, 0, vprp, size);

#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  vprp->format_token = GUINT32_FROM_LE (vprp->format_token);
  vprp->standard = GUINT32_FROM_LE (vprp->standard);
  vprp->vert_rate = GUINT32_FROM_LE (vprp->vert_rate);
  vprp->hor_t_total = GUINT32_FROM_LE (vprp->hor_t_total);
  vprp->vert_lines = GUINT32_FROM_LE (vprp->vert_lines);
  vprp->aspect = GUINT32_FROM_LE (vprp->aspect);
  vprp->width = GUINT32_FROM_LE (vprp->width);
  vprp->height = GUINT32_FROM_LE (vprp->height);
  vprp->fields = GUINT32_FROM_LE (vprp->fields);
#endif

  /* size checking */
  /* calculate fields based on size */
  k = (size - G_STRUCT_OFFSET (gst_riff_vprp, field_info)) / vprp->fields;
  if (vprp->fields > k) {
    GST_WARNING_OBJECT (element,
        "vprp header indicated %d fields, only %d available", vprp->fields, k);
    vprp->fields = k;
  }
  if (vprp->fields > GST_RIFF_VPRP_VIDEO_FIELDS) {
    GST_WARNING_OBJECT (element,
        "vprp header indicated %d fields, at most %d supported", vprp->fields,
        GST_RIFF_VPRP_VIDEO_FIELDS);
    vprp->fields = GST_RIFF_VPRP_VIDEO_FIELDS;
  }
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  for (k = 0; k < vprp->fields; k++) {
    gst_riff_vprp_video_field_desc *fd;

    fd = &vprp->field_info[k];
    fd->compressed_bm_height = GUINT32_FROM_LE (fd->compressed_bm_height);
    fd->compressed_bm_width = GUINT32_FROM_LE (fd->compressed_bm_width);
    fd->valid_bm_height = GUINT32_FROM_LE (fd->valid_bm_height);
    fd->valid_bm_width = GUINT16_FROM_LE (fd->valid_bm_width);
    fd->valid_bm_x_offset = GUINT16_FROM_LE (fd->valid_bm_x_offset);
    fd->valid_bm_y_offset = GUINT32_FROM_LE (fd->valid_bm_y_offset);
    fd->video_x_t_offset = GUINT32_FROM_LE (fd->video_x_t_offset);
    fd->video_y_start = GUINT32_FROM_LE (fd->video_y_start);
  }
#endif

  /* debug */
  GST_INFO_OBJECT (element, "vprp tag found in context vids:");
  GST_INFO_OBJECT (element, " format_token  %d", vprp->format_token);
  GST_INFO_OBJECT (element, " standard      %d", vprp->standard);
  GST_INFO_OBJECT (element, " vert_rate     %d", vprp->vert_rate);
  GST_INFO_OBJECT (element, " hor_t_total   %d", vprp->hor_t_total);
  GST_INFO_OBJECT (element, " vert_lines    %d", vprp->vert_lines);
  GST_INFO_OBJECT (element, " aspect        %d:%d", vprp->aspect >> 16,
      vprp->aspect & 0xffff);
  GST_INFO_OBJECT (element, " width         %d", vprp->width);
  GST_INFO_OBJECT (element, " height        %d", vprp->height);
  GST_INFO_OBJECT (element, " fields        %d", vprp->fields);
  for (k = 0; k < vprp->fields; k++) {
    gst_riff_vprp_video_field_desc *fd;

    fd = &(vprp->field_info[k]);
    GST_INFO_OBJECT (element, " field %u description:", k);
    GST_INFO_OBJECT (element, "  compressed_bm_height  %d",
        fd->compressed_bm_height);
    GST_INFO_OBJECT (element, "  compressed_bm_width  %d",
        fd->compressed_bm_width);
    GST_INFO_OBJECT (element, "  valid_bm_height       %d",
        fd->valid_bm_height);
    GST_INFO_OBJECT (element, "  valid_bm_width        %d", fd->valid_bm_width);
    GST_INFO_OBJECT (element, "  valid_bm_x_offset     %d",
        fd->valid_bm_x_offset);
    GST_INFO_OBJECT (element, "  valid_bm_y_offset     %d",
        fd->valid_bm_y_offset);
    GST_INFO_OBJECT (element, "  video_x_t_offset      %d",
        fd->video_x_t_offset);
    GST_INFO_OBJECT (element, "  video_y_start         %d", fd->video_y_start);
  }

  gst_buffer_unref (buf);

  *_vprp = vprp;

  return TRUE;

  /* ERRORS */
too_small:
  {
    GST_ERROR_OBJECT (element,
        "Too small vprp (%" G_GSIZE_FORMAT " available, at least %d needed)",
        size, (int) G_STRUCT_OFFSET (gst_riff_vprp, field_info));
    gst_buffer_unref (buf);
    return FALSE;
  }
}

static void
gst_avi_demux_expose_streams (GstAviDemux * avi, gboolean force)
{
  guint i;

  GST_DEBUG_OBJECT (avi, "force : %d", force);

  for (i = 0; i < avi->num_streams; i++) {
    GstAviStream *stream = &avi->stream[i];

    if (force || stream->idx_n != 0) {
      GST_LOG_OBJECT (avi, "Adding pad %s", GST_PAD_NAME (stream->pad));
      gst_element_add_pad ((GstElement *) avi, stream->pad);
      gst_flow_combiner_add_pad (avi->flowcombiner, stream->pad);

#if 0
      if (avi->element_index)
        gst_index_get_writer_id (avi->element_index,
            GST_OBJECT_CAST (stream->pad), &stream->index_id);
#endif

      stream->exposed = TRUE;
      if (avi->main_stream == -1)
        avi->main_stream = i;
    } else {
      GST_WARNING_OBJECT (avi, "Stream #%d doesn't have any entry, removing it",
          i);
      gst_avi_demux_reset_stream (avi, stream);
    }
  }
}

/* buf contains LIST chunk data, and will be padded to even size,
 * since some buggy files do not account for the padding of chunks
 * within a LIST in the size of the LIST */
static inline void
gst_avi_demux_roundup_list (GstAviDemux * avi, GstBuffer ** buf)
{
  gsize size;

  size = gst_buffer_get_size (*buf);

  if (G_UNLIKELY (size & 1)) {
    GstBuffer *obuf;
    GstMapInfo map;

    GST_DEBUG_OBJECT (avi, "rounding up dubious list size %" G_GSIZE_FORMAT,
        size);
    obuf = gst_buffer_new_and_alloc (size + 1);

    gst_buffer_map (obuf, &map, GST_MAP_WRITE);
    gst_buffer_extract (*buf, 0, map.data, size);
    /* assume 0 padding, at least makes outcome deterministic */
    map.data[size] = 0;
    gst_buffer_unmap (obuf, &map);
    gst_buffer_replace (buf, obuf);
  }
}

static GstCaps *
gst_avi_demux_check_caps (GstAviDemux * avi, GstAviStream * stream,
    GstCaps * caps)
{
  GstStructure *s;
  const GValue *val;
  GstBuffer *buf;

  caps = gst_caps_make_writable (caps);

  s = gst_caps_get_structure (caps, 0);
  if (gst_structure_has_name (s, "video/x-raw")) {
    stream->is_raw = TRUE;
    stream->alignment = 32;
    if (!gst_structure_has_field (s, "pixel-aspect-ratio"))
      gst_structure_set (s, "pixel-aspect-ratio", GST_TYPE_FRACTION,
          1, 1, NULL);
    if (gst_structure_has_field_typed (s, "palette_data", GST_TYPE_BUFFER)) {
      gst_structure_get (s, "palette_data", GST_TYPE_BUFFER,
          &stream->rgb8_palette, NULL);
      gst_structure_remove_field (s, "palette_data");
      return caps;
    }
  } else if (!gst_structure_has_name (s, "video/x-h264")) {
    return caps;
  }

  GST_DEBUG_OBJECT (avi, "checking caps %" GST_PTR_FORMAT, caps);

  /* some muxers put invalid bytestream stuff in h264 extra data */
  val = gst_structure_get_value (s, "codec_data");
  if (val && (buf = gst_value_get_buffer (val))) {
    guint8 *data;
    gint size;
    GstMapInfo map;

    gst_buffer_map (buf, &map, GST_MAP_READ);
    data = map.data;
    size = map.size;
    if (size >= 4) {
      guint32 h = GST_READ_UINT32_BE (data);
      gst_buffer_unmap (buf, &map);
      if (h == 0x01) {
        /* can hardly be valid AVC codec data */
        GST_DEBUG_OBJECT (avi,
            "discarding invalid codec_data containing byte-stream");
        /* so do not pretend to downstream that it is packetized avc */
        gst_structure_remove_field (s, "codec_data");
        /* ... but rather properly parsed bytestream */
        gst_structure_set (s, "stream-format", G_TYPE_STRING, "byte-stream",
            "alignment", G_TYPE_STRING, "au", NULL);
      }
    } else {
      gst_buffer_unmap (buf, &map);
    }
  }

  return caps;
}

/*
 * gst_avi_demux_parse_stream:
 * @avi: calling element (used for debugging/errors).
 * @buf: input buffer used to parse the stream.
 *
 * Parses all subchunks in a strl chunk (which defines a single
 * stream). Discards the buffer after use. This function will
 * increment the stream counter internally.
 *
 * Returns: whether the stream was identified successfully.
 *          Errors are not fatal. It does indicate the stream
 *          was skipped.
 */
static gboolean
gst_avi_demux_parse_stream (GstAviDemux * avi, GstBuffer * buf)
{
  GstAviStream *stream;
  GstElementClass *klass;
  GstPadTemplate *templ;
  GstBuffer *sub = NULL;
  guint offset = 4;
  guint32 tag = 0;
  gchar *codec_name = NULL, *padname = NULL;
  const gchar *tag_name;
  GstCaps *caps = NULL;
  GstPad *pad;
  GstElement *element;
  gboolean got_strh = FALSE, got_strf = FALSE, got_vprp = FALSE;
  gst_riff_vprp *vprp = NULL;
  GstEvent *event;
  gchar *stream_id;
  GstMapInfo map;
  gboolean sparse = FALSE;

  element = GST_ELEMENT_CAST (avi);

  GST_DEBUG_OBJECT (avi, "Parsing stream");

  gst_avi_demux_roundup_list (avi, &buf);

  if (avi->num_streams >= GST_AVI_DEMUX_MAX_STREAMS) {
    GST_WARNING_OBJECT (avi,
        "maximum no of streams (%d) exceeded, ignoring stream",
        GST_AVI_DEMUX_MAX_STREAMS);
    gst_buffer_unref (buf);
    /* not a fatal error, let's say */
    return TRUE;
  }

  stream = &avi->stream[avi->num_streams];

  /* initial settings */
  stream->idx_duration = GST_CLOCK_TIME_NONE;
  stream->hdr_duration = GST_CLOCK_TIME_NONE;
  stream->duration = GST_CLOCK_TIME_NONE;

  while (gst_riff_parse_chunk (element, buf, &offset, &tag, &sub)) {
    /* sub can be NULL if the chunk is empty */
    if (sub == NULL) {
      GST_DEBUG_OBJECT (avi, "ignoring empty chunk %" GST_FOURCC_FORMAT,
          GST_FOURCC_ARGS (tag));
      continue;
    }
    switch (tag) {
      case GST_RIFF_TAG_strh:
      {
        gst_riff_strh *strh;

        if (got_strh) {
          GST_WARNING_OBJECT (avi, "Ignoring additional strh chunk");
          break;
        }
        if (!gst_riff_parse_strh (element, sub, &stream->strh)) {
          /* ownership given away */
          sub = NULL;
          GST_WARNING_OBJECT (avi, "Failed to parse strh chunk");
          goto fail;
        }
        sub = NULL;
        strh = stream->strh;
        /* sanity check; stream header frame rate matches global header
         * frame duration */
        if (stream->strh->type == GST_RIFF_FCC_vids) {
          GstClockTime s_dur;
          GstClockTime h_dur = avi->avih->us_frame * GST_USECOND;

          s_dur = gst_util_uint64_scale (GST_SECOND, strh->scale, strh->rate);
          GST_DEBUG_OBJECT (avi, "verifying stream framerate %d/%d, "
              "frame duration = %d ms", strh->rate, strh->scale,
              (gint) (s_dur / GST_MSECOND));
          if (h_dur > (10 * GST_MSECOND) && (s_dur > 10 * h_dur)) {
            strh->rate = GST_SECOND / GST_USECOND;
            strh->scale = h_dur / GST_USECOND;
            GST_DEBUG_OBJECT (avi, "correcting stream framerate to %d/%d",
                strh->rate, strh->scale);
          }
        }
        /* determine duration as indicated by header */
        stream->hdr_duration = gst_util_uint64_scale ((guint64) strh->length *
            strh->scale, GST_SECOND, (guint64) strh->rate);
        GST_INFO ("Stream duration according to header: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (stream->hdr_duration));
        if (stream->hdr_duration == 0)
          stream->hdr_duration = GST_CLOCK_TIME_NONE;

        got_strh = TRUE;
        break;
      }
      case GST_RIFF_TAG_strf:
      {
        gboolean res = FALSE;

        if (got_strf) {
          GST_WARNING_OBJECT (avi, "Ignoring additional strf chunk");
          break;
        }
        if (!got_strh) {
          GST_ERROR_OBJECT (avi, "Found strf chunk before strh chunk");
          goto fail;
        }
        switch (stream->strh->type) {
          case GST_RIFF_FCC_vids:
            stream->is_vbr = TRUE;
            res = gst_riff_parse_strf_vids (element, sub,
                &stream->strf.vids, &stream->extradata);
            sub = NULL;
            GST_DEBUG_OBJECT (element, "marking video as VBR, res %d", res);
            break;
          case GST_RIFF_FCC_auds:
            res =
                gst_riff_parse_strf_auds (element, sub, &stream->strf.auds,
                &stream->extradata);
            sub = NULL;
            if (!res)
              break;
            stream->is_vbr = (stream->strh->samplesize == 0)
                && stream->strh->scale > 1
                && stream->strf.auds->blockalign != 1;
            GST_DEBUG_OBJECT (element, "marking audio as VBR:%d, res %d",
                stream->is_vbr, res);
            /* we need these or we have no way to come up with timestamps */
            if ((!stream->is_vbr && !stream->strf.auds->av_bps) ||
                (stream->is_vbr && (!stream->strh->scale ||
                        !stream->strh->rate))) {
              GST_WARNING_OBJECT (element,
                  "invalid audio header, ignoring stream");
              goto fail;
            }
            /* some more sanity checks */
            if (stream->is_vbr) {
              if (stream->strf.auds->blockalign <= 4) {
                /* that would mean (too) many frames per chunk,
                 * so not likely set as expected */
                GST_DEBUG_OBJECT (element,
                    "suspicious blockalign %d for VBR audio; "
                    "overriding to 1 frame per chunk",
                    stream->strf.auds->blockalign);
                /* this should top any likely value */
                stream->strf.auds->blockalign = (1 << 12);
              }
            }
            break;
          case GST_RIFF_FCC_iavs:
            stream->is_vbr = TRUE;
            res = gst_riff_parse_strf_iavs (element, sub,
                &stream->strf.iavs, &stream->extradata);
            sub = NULL;
            GST_DEBUG_OBJECT (element, "marking iavs as VBR, res %d", res);
            break;
          case GST_RIFF_FCC_txts:
            /* nothing to parse here */
            stream->is_vbr = (stream->strh->samplesize == 0)
                && (stream->strh->scale > 1);
            res = TRUE;
            break;
          default:
            GST_ERROR_OBJECT (avi,
                "Don´t know how to handle stream type %" GST_FOURCC_FORMAT,
                GST_FOURCC_ARGS (stream->strh->type));
            break;
        }
        if (sub) {
          gst_buffer_unref (sub);
          sub = NULL;
        }
        if (!res)
          goto fail;
        got_strf = TRUE;
        break;
      }
      case GST_RIFF_TAG_vprp:
      {
        if (got_vprp) {
          GST_WARNING_OBJECT (avi, "Ignoring additional vprp chunk");
          break;
        }
        if (!got_strh) {
          GST_ERROR_OBJECT (avi, "Found vprp chunk before strh chunk");
          goto fail;
        }
        if (!got_strf) {
          GST_ERROR_OBJECT (avi, "Found vprp chunk before strf chunk");
          goto fail;
        }

        if (!gst_avi_demux_riff_parse_vprp (element, sub, &vprp)) {
          GST_WARNING_OBJECT (avi, "Failed to parse vprp chunk");
          /* not considered fatal */
          g_free (vprp);
          vprp = NULL;
        } else
          got_vprp = TRUE;
        sub = NULL;
        break;
      }
      case GST_RIFF_TAG_strd:
        if (stream->initdata)
          gst_buffer_unref (stream->initdata);
        stream->initdata = sub;
        if (sub != NULL) {
          gst_avi_demux_parse_strd (avi, sub);
          sub = NULL;
        }
        break;
      case GST_RIFF_TAG_strn:
        g_free (stream->name);

        gst_buffer_map (sub, &map, GST_MAP_READ);

        if (avi->globaltags == NULL)
          avi->globaltags = gst_tag_list_new_empty ();
        parse_tag_value (avi, avi->globaltags, GST_TAG_TITLE,
            map.data, map.size);

        if (gst_tag_list_get_string (avi->globaltags, GST_TAG_TITLE,
                &stream->name))
          GST_DEBUG_OBJECT (avi, "stream name: %s", stream->name);

        gst_buffer_unmap (sub, &map);
        gst_buffer_unref (sub);
        sub = NULL;
        break;
      case GST_RIFF_IDIT:
        gst_avi_demux_parse_idit (avi, sub);
        break;
      default:
        if (tag == GST_MAKE_FOURCC ('i', 'n', 'd', 'x') ||
            tag == GST_MAKE_FOURCC ('i', 'x', '0' + avi->num_streams / 10,
                '0' + avi->num_streams % 10)) {
          g_free (stream->indexes);
          gst_avi_demux_parse_superindex (avi, sub, &stream->indexes);
          stream->superindex = TRUE;
          sub = NULL;
          break;
        }
        GST_WARNING_OBJECT (avi,
            "Unknown stream header tag %" GST_FOURCC_FORMAT ", ignoring",
            GST_FOURCC_ARGS (tag));
        /* Only get buffer for debugging if the memdump is needed  */
        if (gst_debug_category_get_threshold (GST_CAT_DEFAULT) >= 9) {
          GstMapInfo map;

          gst_buffer_map (sub, &map, GST_MAP_READ);
          GST_MEMDUMP_OBJECT (avi, "Unknown stream header tag", map.data,
              map.size);
          gst_buffer_unmap (sub, &map);
        }
        /* fall-through */
      case GST_RIFF_TAG_JUNQ:
      case GST_RIFF_TAG_JUNK:
        break;
    }
    if (sub != NULL) {
      gst_buffer_unref (sub);
      sub = NULL;
    }
  }

  if (!got_strh) {
    GST_WARNING_OBJECT (avi, "Failed to find strh chunk");
    goto fail;
  }

  if (!got_strf) {
    GST_WARNING_OBJECT (avi, "Failed to find strf chunk");
    goto fail;
  }

  /* get class to figure out the template */
  klass = GST_ELEMENT_GET_CLASS (avi);

  /* we now have all info, let´s set up a pad and a caps and be done */
  /* create stream name + pad */
  switch (stream->strh->type) {
    case GST_RIFF_FCC_vids:{
      guint32 fourcc;

      fourcc = (stream->strf.vids->compression) ?
          stream->strf.vids->compression : stream->strh->fcc_handler;
      caps = gst_riff_create_video_caps (fourcc, stream->strh,
          stream->strf.vids, stream->extradata, stream->initdata, &codec_name);

      /* DXSB is XSUB, and it is placed inside a vids */
      if (!caps || (fourcc != GST_MAKE_FOURCC ('D', 'X', 'S', 'B') &&
              fourcc != GST_MAKE_FOURCC ('D', 'X', 'S', 'A'))) {
        padname = g_strdup_printf ("video_%u", avi->num_v_streams);
        templ = gst_element_class_get_pad_template (klass, "video_%u");
        if (!caps) {
          caps = gst_caps_new_simple ("video/x-avi-unknown", "fourcc",
              G_TYPE_INT, fourcc, NULL);
        } else if (got_vprp && vprp) {
          guint32 aspect_n, aspect_d;
          gint n, d;

          aspect_n = vprp->aspect >> 16;
          aspect_d = vprp->aspect & 0xffff;
          /* calculate the pixel aspect ratio using w/h and aspect ratio */
          n = aspect_n * stream->strf.vids->height;
          d = aspect_d * stream->strf.vids->width;
          if (n && d)
            gst_caps_set_simple (caps, "pixel-aspect-ratio", GST_TYPE_FRACTION,
                n, d, NULL);
        }
        caps = gst_avi_demux_check_caps (avi, stream, caps);
        tag_name = GST_TAG_VIDEO_CODEC;
        avi->num_v_streams++;
      } else {
        padname = g_strdup_printf ("subpicture_%u", avi->num_sp_streams);
        templ = gst_element_class_get_pad_template (klass, "subpicture_%u");
        tag_name = NULL;
        avi->num_sp_streams++;
        sparse = TRUE;
      }
      break;
    }
    case GST_RIFF_FCC_auds:{
      /* FIXME: Do something with the channel reorder map */
      padname = g_strdup_printf ("audio_%u", avi->num_a_streams);
      templ = gst_element_class_get_pad_template (klass, "audio_%u");
      caps = gst_riff_create_audio_caps (stream->strf.auds->format,
          stream->strh, stream->strf.auds, stream->extradata,
          stream->initdata, &codec_name, NULL);
      if (!caps) {
        caps = gst_caps_new_simple ("audio/x-avi-unknown", "codec_id",
            G_TYPE_INT, stream->strf.auds->format, NULL);
      }
      tag_name = GST_TAG_AUDIO_CODEC;
      avi->num_a_streams++;
      break;
    }
    case GST_RIFF_FCC_iavs:{
      guint32 fourcc = stream->strh->fcc_handler;

      padname = g_strdup_printf ("video_%u", avi->num_v_streams);
      templ = gst_element_class_get_pad_template (klass, "video_%u");
      caps = gst_riff_create_iavs_caps (fourcc, stream->strh,
          stream->strf.iavs, stream->extradata, stream->initdata, &codec_name);
      if (!caps) {
        caps = gst_caps_new_simple ("video/x-avi-unknown", "fourcc",
            G_TYPE_INT, fourcc, NULL);
      }
      tag_name = GST_TAG_VIDEO_CODEC;
      avi->num_v_streams++;
      break;
    }
    case GST_RIFF_FCC_txts:{
      padname = g_strdup_printf ("subtitle_%u", avi->num_t_streams);
      templ = gst_element_class_get_pad_template (klass, "subtitle_%u");
      caps = gst_caps_new_empty_simple ("application/x-subtitle-avi");
      tag_name = NULL;
      avi->num_t_streams++;
      sparse = TRUE;
      break;
    }
    default:
      g_return_val_if_reached (FALSE);
  }

  /* no caps means no stream */
  if (!caps) {
    GST_ERROR_OBJECT (element, "Did not find caps for stream %s", padname);
    goto fail;
  }

  GST_DEBUG_OBJECT (element, "codec-name=%s", codec_name ? codec_name : "NULL");
  GST_DEBUG_OBJECT (element, "caps=%" GST_PTR_FORMAT, caps);

  /* set proper settings and add it */
  if (stream->pad)
    gst_object_unref (stream->pad);
  pad = stream->pad = gst_pad_new_from_template (templ, padname);
  g_free (padname);

  gst_pad_use_fixed_caps (pad);
#if 0
  gst_pad_set_formats_function (pad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_get_src_formats));
  gst_pad_set_event_mask_function (pad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_get_event_mask));
#endif
  gst_pad_set_event_function (pad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_handle_src_event));
  gst_pad_set_query_function (pad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_handle_src_query));
#if 0
  gst_pad_set_convert_function (pad,
      GST_DEBUG_FUNCPTR (gst_avi_demux_src_convert));
#endif

  stream->num = avi->num_streams;

  stream->start_entry = 0;
  stream->step_entry = 0;
  stream->stop_entry = 0;

  stream->current_entry = -1;
  stream->current_total = 0;

  stream->discont = TRUE;

  stream->total_bytes = 0;
  stream->total_blocks = 0;
  stream->n_keyframes = 0;

  stream->idx_n = 0;
  stream->idx_max = 0;

  gst_pad_set_element_private (pad, stream);
  avi->num_streams++;

  gst_pad_set_active (pad, TRUE);
  stream_id =
      gst_pad_create_stream_id_printf (pad, GST_ELEMENT_CAST (avi), "%03u",
      avi->num_streams);

  event = gst_pad_get_sticky_event (avi->sinkpad, GST_EVENT_STREAM_START, 0);
  if (event) {
    if (gst_event_parse_group_id (event, &avi->group_id))
      avi->have_group_id = TRUE;
    else
      avi->have_group_id = FALSE;
    gst_event_unref (event);
  } else if (!avi->have_group_id) {
    avi->have_group_id = TRUE;
    avi->group_id = gst_util_group_id_next ();
  }

  event = gst_event_new_stream_start (stream_id);
  if (avi->have_group_id)
    gst_event_set_group_id (event, avi->group_id);
  if (sparse)
    gst_event_set_stream_flags (event, GST_STREAM_FLAG_SPARSE);

  gst_pad_push_event (pad, event);
  g_free (stream_id);
  gst_pad_set_caps (pad, caps);
  gst_caps_unref (caps);

  /* make tags */
  if (codec_name && tag_name) {
    if (!stream->taglist)
      stream->taglist = gst_tag_list_new_empty ();

    avi->got_tags = TRUE;

    gst_tag_list_add (stream->taglist, GST_TAG_MERGE_APPEND, tag_name,
        codec_name, NULL);
  }

  g_free (vprp);
  g_free (codec_name);
  gst_buffer_unref (buf);

  return TRUE;

  /* ERRORS */
fail:
  {
    /* unref any mem that may be in use */
    if (buf)
      gst_buffer_unref (buf);
    if (sub)
      gst_buffer_unref (sub);
    g_free (vprp);
    g_free (codec_name);
    gst_avi_demux_reset_stream (avi, stream);
    avi->num_streams++;
    return FALSE;
  }
}

/*
 * gst_avi_demux_parse_odml:
 * @avi: calling element (used for debug/error).
 * @buf: input buffer to be used for parsing.
 *
 * Read an openDML-2.0 extension header. Fills in the frame number
 * in the avi demuxer object when reading succeeds.
 */
static void
gst_avi_demux_parse_odml (GstAviDemux * avi, GstBuffer * buf)
{
  guint32 tag = 0;
  guint offset = 4;
  GstBuffer *sub = NULL;

  while (gst_riff_parse_chunk (GST_ELEMENT_CAST (avi), buf, &offset, &tag,
          &sub)) {
    switch (tag) {
      case GST_RIFF_TAG_dmlh:{
        gst_riff_dmlh dmlh, *_dmlh;
        GstMapInfo map;

        /* sub == NULL is possible and means an empty buffer */
        if (sub == NULL)
          goto next;

        gst_buffer_map (sub, &map, GST_MAP_READ);

        /* check size */
        if (map.size < sizeof (gst_riff_dmlh)) {
          GST_ERROR_OBJECT (avi,
              "DMLH entry is too small (%" G_GSIZE_FORMAT " bytes, %d needed)",
              map.size, (int) sizeof (gst_riff_dmlh));
          gst_buffer_unmap (sub, &map);
          goto next;
        }
        _dmlh = (gst_riff_dmlh *) map.data;
        dmlh.totalframes = GST_READ_UINT32_LE (&_dmlh->totalframes);
        gst_buffer_unmap (sub, &map);

        GST_INFO_OBJECT (avi, "dmlh tag found: totalframes: %u",
            dmlh.totalframes);

        avi->avih->tot_frames = dmlh.totalframes;
        goto next;
      }

      default:
        GST_WARNING_OBJECT (avi,
            "Unknown tag %" GST_FOURCC_FORMAT " in ODML header",
            GST_FOURCC_ARGS (tag));
        /* Only get buffer for debugging if the memdump is needed  */
        if (gst_debug_category_get_threshold (GST_CAT_DEFAULT) >= 9) {
          GstMapInfo map;

          gst_buffer_map (sub, &map, GST_MAP_READ);
          GST_MEMDUMP_OBJECT (avi, "Unknown ODML tag", map.data, map.size);
          gst_buffer_unmap (sub, &map);
        }
        /* fall-through */
      case GST_RIFF_TAG_JUNQ:
      case GST_RIFF_TAG_JUNK:
      next:
        /* skip and move to next chunk */
        if (sub) {
          gst_buffer_unref (sub);
          sub = NULL;
        }
        break;
    }
  }
  if (buf)
    gst_buffer_unref (buf);
}

/* Index helper */
static guint
gst_avi_demux_index_last (GstAviDemux * avi, GstAviStream * stream)
{
  return stream->idx_n;
}

/* find a previous entry in the index with the given flags */
static guint
gst_avi_demux_index_prev (GstAviDemux * avi, GstAviStream * stream,
    guint last, gboolean keyframe)
{
  GstAviIndexEntry *entry;
  guint i;

  for (i = last; i > 0; i--) {
    entry = &stream->index[i - 1];
    if (!keyframe || ENTRY_IS_KEYFRAME (entry)) {
      return i - 1;
    }
  }
  return 0;
}

static guint
gst_avi_demux_index_next (GstAviDemux * avi, GstAviStream * stream,
    guint last, gboolean keyframe)
{
  GstAviIndexEntry *entry;
  gint i;

  for (i = last + 1; i < stream->idx_n; i++) {
    entry = &stream->index[i];
    if (!keyframe || ENTRY_IS_KEYFRAME (entry)) {
      return i;
    }
  }
  return stream->idx_n - 1;
}

static guint
gst_avi_demux_index_entry_search (GstAviIndexEntry * entry, guint64 * total)
{
  if (entry->total < *total)
    return -1;
  else if (entry->total > *total)
    return 1;
  return 0;
}

/*
 * gst_avi_demux_index_for_time:
 * @avi: Avi object
 * @stream: the stream
 * @time: a time position
 * @next: whether to look for entry before or after @time
 *
 * Finds the index entry which time is less/more or equal than the requested time.
 * Try to avoid binary search when we can convert the time to an index
 * position directly (for example for video frames with a fixed duration).
 *
 * Returns: the found position in the index.
 */
static guint
gst_avi_demux_index_for_time (GstAviDemux * avi,
    GstAviStream * stream, guint64 time, gboolean next)
{
  guint index = -1;
  guint64 total;

  GST_LOG_OBJECT (avi, "search time:%" GST_TIME_FORMAT, GST_TIME_ARGS (time));

  /* easy (and common) cases */
  if (time == 0 || stream->idx_n == 0)
    return 0;
  if (time >= stream->idx_duration)
    return stream->idx_n - 1;

  /* figure out where we need to go. For that we convert the time to an
   * index entry or we convert it to a total and then do a binary search. */
  if (stream->is_vbr) {
    /* VBR stream next timestamp */
    if (stream->strh->type == GST_RIFF_FCC_auds) {
      total = avi_stream_convert_time_to_frames_unchecked (stream, time);
    } else {
      index = avi_stream_convert_time_to_frames_unchecked (stream, time);
      /* this entry typically undershoots the target time,
       * so check a bit more if next needed */
      if (next && index != -1) {
        GstClockTime itime =
            avi_stream_convert_frames_to_time_unchecked (stream, index);
        if (itime < time && index + 1 < stream->idx_n)
          index++;
      }
    }
  } else if (stream->strh->type == GST_RIFF_FCC_auds) {
    /* constant rate stream */
    total = avi_stream_convert_time_to_bytes_unchecked (stream, time);
  } else
    return -1;

  if (index == -1) {
    GstAviIndexEntry *entry;

    /* no index, find index with binary search on total */
    GST_LOG_OBJECT (avi, "binary search for entry with total %"
        G_GUINT64_FORMAT, total);

    entry = gst_util_array_binary_search (stream->index,
        stream->idx_n, sizeof (GstAviIndexEntry),
        (GCompareDataFunc) gst_avi_demux_index_entry_search,
        next ? GST_SEARCH_MODE_AFTER : GST_SEARCH_MODE_BEFORE, &total, NULL);

    if (entry == NULL) {
      GST_LOG_OBJECT (avi, "not found, assume index 0");
      index = 0;
    } else {
      index = entry - stream->index;
      GST_LOG_OBJECT (avi, "found at %u", index);
    }
  } else {
    GST_LOG_OBJECT (avi, "converted time to index %u", index);
  }

  return index;
}

static inline GstAviStream *
gst_avi_demux_stream_for_id (GstAviDemux * avi, guint32 id)
{
  guint stream_nr;
  GstAviStream *stream;

  /* get the stream for this entry */
  stream_nr = CHUNKID_TO_STREAMNR (id);
  if (G_UNLIKELY (stream_nr >= avi->num_streams)) {
    GST_WARNING_OBJECT (avi,
        "invalid stream nr %d (0x%08x, %" GST_FOURCC_FORMAT ")", stream_nr, id,
        GST_FOURCC_ARGS (id));
    return NULL;
  }
  stream = &avi->stream[stream_nr];
  if (G_UNLIKELY (!stream->strh)) {
    GST_WARNING_OBJECT (avi, "Unhandled stream %d, skipping", stream_nr);
    return NULL;
  }
  return stream;
}

/*
 * gst_avi_demux_parse_index:
 * @avi: calling element (used for debugging/errors).
 * @buf: buffer containing the full index.
 *
 * Read index entries from the provided buffer.
 * The buffer should contain a GST_RIFF_TAG_idx1 chunk.
 */
static gboolean
gst_avi_demux_parse_index (GstAviDemux * avi, GstBuffer * buf)
{
  GstMapInfo map;
  guint i, num, n;
  gst_riff_index_entry *index;
  GstClockTime stamp;
  GstAviStream *stream;
  GstAviIndexEntry entry;
  guint32 id;

  if (!buf)
    return FALSE;

  gst_buffer_map (buf, &map, GST_MAP_READ);

  stamp = gst_util_get_timestamp ();

  /* see how many items in the index */
  num = map.size / sizeof (gst_riff_index_entry);
  if (num == 0)
    goto empty_list;

  GST_INFO_OBJECT (avi, "Parsing index, nr_entries = %6d", num);

  index = (gst_riff_index_entry *) map.data;

  /* figure out if the index is 0 based or relative to the MOVI start */
  entry.offset = GST_READ_UINT32_LE (&index[0].offset);
  if (entry.offset < avi->offset) {
    avi->index_offset = avi->offset + 8;
    GST_DEBUG ("index_offset = %" G_GUINT64_FORMAT, avi->index_offset);
  } else {
    avi->index_offset = 0;
    GST_DEBUG ("index is 0 based");
  }

  for (i = 0, n = 0; i < num; i++) {
    id = GST_READ_UINT32_LE (&index[i].id);
    entry.offset = GST_READ_UINT32_LE (&index[i].offset);

    /* some sanity checks */
    if (G_UNLIKELY (id == GST_RIFF_rec || id == 0 ||
            (entry.offset == 0 && n > 0)))
      continue;

    /* get the stream for this entry */
    stream = gst_avi_demux_stream_for_id (avi, id);
    if (G_UNLIKELY (!stream))
      continue;

    /* handle offset and size */
    entry.offset += avi->index_offset + 8;
    entry.size = GST_READ_UINT32_LE (&index[i].size);

    /* handle flags */
    if (stream->strh->type == GST_RIFF_FCC_auds) {
      /* all audio frames are keyframes */
      ENTRY_SET_KEYFRAME (&entry);
    } else if (stream->strh->type == GST_RIFF_FCC_vids &&
        stream->strf.vids->compression == GST_RIFF_DXSB) {
      /* all xsub frames are keyframes */
      ENTRY_SET_KEYFRAME (&entry);
    } else {
      guint32 flags;
      /* else read flags */
      flags = GST_READ_UINT32_LE (&index[i].flags);
      if (flags & GST_RIFF_IF_KEYFRAME) {
        ENTRY_SET_KEYFRAME (&entry);
      } else {
        ENTRY_UNSET_KEYFRAME (&entry);
      }
    }

    /* and add */
    if (G_UNLIKELY (!gst_avi_demux_add_index (avi, stream, num, &entry)))
      goto out_of_mem;

    n++;
  }
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  /* get stream stats now */
  avi->have_index = gst_avi_demux_do_index_stats (avi);

  stamp = gst_util_get_timestamp () - stamp;
  GST_DEBUG_OBJECT (avi, "index parsing took %" GST_TIME_FORMAT,
      GST_TIME_ARGS (stamp));

  return TRUE;

  /* ERRORS */
empty_list:
  {
    GST_DEBUG_OBJECT (avi, "empty index");
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return FALSE;
  }
out_of_mem:
  {
    GST_ELEMENT_ERROR (avi, RESOURCE, NO_SPACE_LEFT, (NULL),
        ("Cannot allocate memory for %u*%u=%u bytes",
            (guint) sizeof (GstAviIndexEntry), num,
            (guint) sizeof (GstAviIndexEntry) * num));
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return FALSE;
  }
}

/*
 * gst_avi_demux_stream_index:
 * @avi: avi demuxer object.
 *
 * Seeks to index and reads it.
 */
static void
gst_avi_demux_stream_index (GstAviDemux * avi)
{
  GstFlowReturn res;
  guint64 offset = avi->offset;
  GstBuffer *buf = NULL;
  guint32 tag;
  guint32 size;
  GstMapInfo map;

  GST_DEBUG ("demux stream index at offset %" G_GUINT64_FORMAT, offset);

  /* get chunk information */
  res = gst_pad_pull_range (avi->sinkpad, offset, 8, &buf);
  if (res != GST_FLOW_OK)
    goto pull_failed;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  if (map.size < 8)
    goto too_small;

  /* check tag first before blindly trying to read 'size' bytes */
  tag = GST_READ_UINT32_LE (map.data);
  size = GST_READ_UINT32_LE (map.data + 4);
  if (tag == GST_RIFF_TAG_LIST) {
    /* this is the movi tag */
    GST_DEBUG_OBJECT (avi, "skip LIST chunk, size %" G_GUINT32_FORMAT,
        (8 + GST_ROUND_UP_2 (size)));
    offset += 8 + GST_ROUND_UP_2 (size);
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);

    buf = NULL;
    res = gst_pad_pull_range (avi->sinkpad, offset, 8, &buf);
    if (res != GST_FLOW_OK)
      goto pull_failed;

    gst_buffer_map (buf, &map, GST_MAP_READ);
    if (map.size < 8)
      goto too_small;

    tag = GST_READ_UINT32_LE (map.data);
    size = GST_READ_UINT32_LE (map.data + 4);
  }
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  if (tag != GST_RIFF_TAG_idx1)
    goto no_index;
  if (!size)
    goto zero_index;

  GST_DEBUG ("index found at offset %" G_GUINT64_FORMAT, offset);

  /* read chunk, advance offset */
  if (gst_riff_read_chunk (GST_ELEMENT_CAST (avi),
          avi->sinkpad, &offset, &tag, &buf) != GST_FLOW_OK)
    return;

  GST_DEBUG ("will parse index chunk size %" G_GSIZE_FORMAT " for tag %"
      GST_FOURCC_FORMAT, gst_buffer_get_size (buf), GST_FOURCC_ARGS (tag));

  gst_avi_demux_parse_index (avi, buf);

#ifndef GST_DISABLE_GST_DEBUG
  /* debug our indexes */
  {
    gint i;
    GstAviStream *stream;

    for (i = 0; i < avi->num_streams; i++) {
      stream = &avi->stream[i];
      GST_DEBUG_OBJECT (avi, "stream %u: %u frames, %" G_GINT64_FORMAT " bytes",
          i, stream->idx_n, stream->total_bytes);
    }
  }
#endif
  return;

  /* ERRORS */
pull_failed:
  {
    GST_DEBUG_OBJECT (avi,
        "pull range failed: pos=%" G_GUINT64_FORMAT " size=8", offset);
    return;
  }
too_small:
  {
    GST_DEBUG_OBJECT (avi, "Buffer is too small");
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return;
  }
no_index:
  {
    GST_WARNING_OBJECT (avi,
        "No index data (idx1) after movi chunk, but %" GST_FOURCC_FORMAT,
        GST_FOURCC_ARGS (tag));
    return;
  }
zero_index:
  {
    GST_WARNING_OBJECT (avi, "Empty index data (idx1) after movi chunk");
    return;
  }
}

/*
 * gst_avi_demux_stream_index_push:
 * @avi: avi demuxer object.
 *
 * Read index.
 */
static void
gst_avi_demux_stream_index_push (GstAviDemux * avi)
{
  guint64 offset = avi->idx1_offset;
  GstBuffer *buf;
  guint32 tag;
  guint32 size;

  GST_DEBUG ("demux stream index at offset %" G_GUINT64_FORMAT, offset);

  /* get chunk information */
  if (!gst_avi_demux_peek_chunk (avi, &tag, &size))
    return;

  /* check tag first before blindly trying to read 'size' bytes */
  if (tag == GST_RIFF_TAG_LIST) {
    /* this is the movi tag */
    GST_DEBUG_OBJECT (avi, "skip LIST chunk, size %" G_GUINT32_FORMAT,
        (8 + GST_ROUND_UP_2 (size)));
    avi->idx1_offset = offset + 8 + GST_ROUND_UP_2 (size);
    /* issue seek to allow chain function to handle it and return! */
    perform_seek_to_offset (avi, avi->idx1_offset, avi->segment_seqnum);
    return;
  }

  if (tag != GST_RIFF_TAG_idx1)
    goto no_index;

  GST_DEBUG ("index found at offset %" G_GUINT64_FORMAT, offset);

  /* flush chunk header */
  gst_adapter_flush (avi->adapter, 8);
  /* read chunk payload */
  buf = gst_adapter_take_buffer (avi->adapter, size);
  if (!buf)
    goto pull_failed;
  /* advance offset */
  offset += 8 + GST_ROUND_UP_2 (size);

  GST_DEBUG ("will parse index chunk size %" G_GSIZE_FORMAT " for tag %"
      GST_FOURCC_FORMAT, gst_buffer_get_size (buf), GST_FOURCC_ARGS (tag));

  avi->offset = avi->first_movi_offset;
  gst_avi_demux_parse_index (avi, buf);

#ifndef GST_DISABLE_GST_DEBUG
  /* debug our indexes */
  {
    gint i;
    GstAviStream *stream;

    for (i = 0; i < avi->num_streams; i++) {
      stream = &avi->stream[i];
      GST_DEBUG_OBJECT (avi, "stream %u: %u frames, %" G_GINT64_FORMAT " bytes",
          i, stream->idx_n, stream->total_bytes);
    }
  }
#endif
  return;

  /* ERRORS */
pull_failed:
  {
    GST_DEBUG_OBJECT (avi,
        "taking data from adapter failed: pos=%" G_GUINT64_FORMAT " size=%u",
        offset, size);
    return;
  }
no_index:
  {
    GST_WARNING_OBJECT (avi,
        "No index data (idx1) after movi chunk, but %" GST_FOURCC_FORMAT,
        GST_FOURCC_ARGS (tag));
    return;
  }
}

/*
 * gst_avi_demux_peek_tag:
 *
 * Returns the tag and size of the next chunk
 */
static GstFlowReturn
gst_avi_demux_peek_tag (GstAviDemux * avi, guint64 offset, guint32 * tag,
    guint * size)
{
  GstFlowReturn res;
  GstBuffer *buf = NULL;
  GstMapInfo map;

  res = gst_pad_pull_range (avi->sinkpad, offset, 8, &buf);
  if (res != GST_FLOW_OK)
    goto pull_failed;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  if (map.size != 8)
    goto wrong_size;

  *tag = GST_READ_UINT32_LE (map.data);
  *size = GST_READ_UINT32_LE (map.data + 4);

  GST_LOG_OBJECT (avi, "Tag[%" GST_FOURCC_FORMAT "] (size:%d) %"
      G_GINT64_FORMAT " -- %" G_GINT64_FORMAT, GST_FOURCC_ARGS (*tag),
      *size, offset + 8, offset + 8 + (gint64) * size);

done:
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  return res;

  /* ERRORS */
pull_failed:
  {
    GST_DEBUG_OBJECT (avi, "pull_ranged returned %s", gst_flow_get_name (res));
    return res;
  }
wrong_size:
  {
    GST_DEBUG_OBJECT (avi, "got %" G_GSIZE_FORMAT " bytes which is <> 8 bytes",
        map.size);
    res = GST_FLOW_ERROR;
    goto done;
  }
}

/*
 * gst_avi_demux_next_data_buffer:
 *
 * Returns the offset and size of the next buffer
 * Position is the position of the buffer (after tag and size)
 */
static GstFlowReturn
gst_avi_demux_next_data_buffer (GstAviDemux * avi, guint64 * offset,
    guint32 * tag, guint * size)
{
  guint64 off = *offset;
  guint _size = 0;
  GstFlowReturn res;

  do {
    res = gst_avi_demux_peek_tag (avi, off, tag, &_size);
    if (res != GST_FLOW_OK)
      break;
    if (*tag == GST_RIFF_TAG_LIST || *tag == GST_RIFF_TAG_RIFF)
      off += 8 + 4;             /* skip tag + size + subtag */
    else {
      *offset = off + 8;
      *size = _size;
      break;
    }
  } while (TRUE);

  return res;
}

/*
 * gst_avi_demux_stream_scan:
 * @avi: calling element (used for debugging/errors).
 *
 * Scan the file for all chunks to "create" a new index.
 * pull-range based
 */
static gboolean
gst_avi_demux_stream_scan (GstAviDemux * avi)
{
  GstFlowReturn res;
  GstAviStream *stream;
  guint64 pos = 0;
  guint64 length;
  gint64 tmplength;
  guint32 tag = 0;
  guint num;

  /* FIXME:
   * - implement non-seekable source support.
   */
  GST_DEBUG_OBJECT (avi, "Creating index");

  /* get the size of the file */
  if (!gst_pad_peer_query_duration (avi->sinkpad, GST_FORMAT_BYTES, &tmplength))
    return FALSE;
  length = tmplength;

  /* guess the total amount of entries we expect */
  num = 16000;

  while (TRUE) {
    GstAviIndexEntry entry;
    guint size = 0;

    /* start reading data buffers to find the id and offset */
    res = gst_avi_demux_next_data_buffer (avi, &pos, &tag, &size);
    if (G_UNLIKELY (res != GST_FLOW_OK))
      break;

    /* get stream */
    stream = gst_avi_demux_stream_for_id (avi, tag);
    if (G_UNLIKELY (!stream))
      goto next;

    /* we can't figure out the keyframes, assume they all are */
    entry.flags = GST_AVI_KEYFRAME;
    entry.offset = pos;
    entry.size = size;

    /* and add to the index of this stream */
    if (G_UNLIKELY (!gst_avi_demux_add_index (avi, stream, num, &entry)))
      goto out_of_mem;

  next:
    /* update position */
    pos += GST_ROUND_UP_2 (size);
    if (G_UNLIKELY (pos > length)) {
      GST_WARNING_OBJECT (avi,
          "Stopping index lookup since we are further than EOF");
      break;
    }
  }

  /* collect stats */
  avi->have_index = gst_avi_demux_do_index_stats (avi);

  return TRUE;

  /* ERRORS */
out_of_mem:
  {
    GST_ELEMENT_ERROR (avi, RESOURCE, NO_SPACE_LEFT, (NULL),
        ("Cannot allocate memory for %u*%u=%u bytes",
            (guint) sizeof (GstAviIndexEntry), num,
            (guint) sizeof (GstAviIndexEntry) * num));
    return FALSE;
  }
}

static void
gst_avi_demux_calculate_durations_from_index (GstAviDemux * avi)
{
  guint i;
  GstClockTime total;
  GstAviStream *stream;

  total = GST_CLOCK_TIME_NONE;

  /* all streams start at a timestamp 0 */
  for (i = 0; i < avi->num_streams; i++) {
    GstClockTime duration, hduration;
    gst_riff_strh *strh;

    stream = &avi->stream[i];
    if (G_UNLIKELY (!stream || !stream->idx_n || !(strh = stream->strh)))
      continue;

    /* get header duration for the stream */
    hduration = stream->hdr_duration;
    /* index duration calculated during parsing */
    duration = stream->idx_duration;

    /* now pick a good duration */
    if (GST_CLOCK_TIME_IS_VALID (duration)) {
      /* index gave valid duration, use that */
      GST_INFO ("Stream %p duration according to index: %" GST_TIME_FORMAT,
          stream, GST_TIME_ARGS (duration));
    } else {
      /* fall back to header info to calculate a duration */
      duration = hduration;
    }
    GST_INFO ("Setting duration of stream #%d to %" GST_TIME_FORMAT,
        i, GST_TIME_ARGS (duration));
    /* set duration for the stream */
    stream->duration = duration;

    /* find total duration */
    if (total == GST_CLOCK_TIME_NONE ||
        (GST_CLOCK_TIME_IS_VALID (duration) && duration > total))
      total = duration;
  }

  if (GST_CLOCK_TIME_IS_VALID (total) && (total > 0)) {
    /* now update the duration for those streams where we had none */
    for (i = 0; i < avi->num_streams; i++) {
      stream = &avi->stream[i];

      if (!GST_CLOCK_TIME_IS_VALID (stream->duration)
          || stream->duration == 0) {
        stream->duration = total;

        GST_INFO ("Stream %p duration according to total: %" GST_TIME_FORMAT,
            stream, GST_TIME_ARGS (total));
      }
    }
  }

  /* and set the total duration in the segment. */
  GST_INFO ("Setting total duration to: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (total));

  avi->segment.duration = total;
}

/* returns FALSE if there are no pads to deliver event to,
 * otherwise TRUE (whatever the outcome of event sending),
 * takes ownership of the event. */
static gboolean
gst_avi_demux_push_event (GstAviDemux * avi, GstEvent * event)
{
  gboolean result = FALSE;
  gint i;

  GST_DEBUG_OBJECT (avi, "sending %s event to %d streams",
      GST_EVENT_TYPE_NAME (event), avi->num_streams);

  for (i = 0; i < avi->num_streams; i++) {
    GstAviStream *stream = &avi->stream[i];

    if (stream->pad) {
      result = TRUE;
      gst_pad_push_event (stream->pad, gst_event_ref (event));
    }
  }
  gst_event_unref (event);
  return result;
}

static void
gst_avi_demux_check_seekability (GstAviDemux * avi)
{
  GstQuery *query;
  gboolean seekable = FALSE;
  gint64 start = -1, stop = -1;

  query = gst_query_new_seeking (GST_FORMAT_BYTES);
  if (!gst_pad_peer_query (avi->sinkpad, query)) {
    GST_DEBUG_OBJECT (avi, "seeking query failed");
    goto done;
  }

  gst_query_parse_seeking (query, NULL, &seekable, &start, &stop);

  /* try harder to query upstream size if we didn't get it the first time */
  if (seekable && stop == -1) {
    GST_DEBUG_OBJECT (avi, "doing duration query to fix up unset stop");
    gst_pad_peer_query_duration (avi->sinkpad, GST_FORMAT_BYTES, &stop);
  }

  /* if upstream doesn't know the size, it's likely that it's not seekable in
   * practice even if it technically may be seekable */
  if (seekable && (start != 0 || stop <= start)) {
    GST_DEBUG_OBJECT (avi, "seekable but unknown start/stop -> disable");
    seekable = FALSE;
  }

done:
  GST_INFO_OBJECT (avi, "seekable: %d (%" G_GUINT64_FORMAT " - %"
      G_GUINT64_FORMAT ")", seekable, start, stop);
  avi->seekable = seekable;

  gst_query_unref (query);
}

/*
 * Read AVI headers when streaming
 */
static GstFlowReturn
gst_avi_demux_stream_header_push (GstAviDemux * avi)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 tag = 0;
  guint32 ltag = 0;
  guint32 size = 0;
  const guint8 *data;
  GstBuffer *buf = NULL, *sub = NULL;
  guint offset = 4;
  gint i;
  GstTagList *tags = NULL;
  guint8 fourcc[4];

  GST_DEBUG ("Reading and parsing avi headers: %d", avi->header_state);

  switch (avi->header_state) {
    case GST_AVI_DEMUX_HEADER_TAG_LIST:
    again:
      if (gst_avi_demux_peek_chunk (avi, &tag, &size)) {
        avi->offset += 8 + GST_ROUND_UP_2 (size);
        if (tag != GST_RIFF_TAG_LIST)
          goto header_no_list;

        gst_adapter_flush (avi->adapter, 8);
        /* Find the 'hdrl' LIST tag */
        GST_DEBUG ("Reading %d bytes", size);
        buf = gst_adapter_take_buffer (avi->adapter, size);

        gst_buffer_extract (buf, 0, fourcc, 4);

        if (GST_READ_UINT32_LE (fourcc) != GST_RIFF_LIST_hdrl) {
          GST_WARNING_OBJECT (avi, "Invalid AVI header (no hdrl at start): %"
              GST_FOURCC_FORMAT, GST_FOURCC_ARGS (tag));
          gst_buffer_unref (buf);
          goto again;
        }

        /* mind padding */
        if (size & 1)
          gst_adapter_flush (avi->adapter, 1);

        GST_DEBUG ("'hdrl' LIST tag found. Parsing next chunk");

        gst_avi_demux_roundup_list (avi, &buf);

        /* the hdrl starts with a 'avih' header */
        if (!gst_riff_parse_chunk (GST_ELEMENT_CAST (avi), buf, &offset, &tag,
                &sub))
          goto header_no_avih;

        if (tag != GST_RIFF_TAG_avih)
          goto header_no_avih;

        if (!gst_avi_demux_parse_avih (avi, sub, &avi->avih))
          goto header_wrong_avih;

        GST_DEBUG_OBJECT (avi, "AVI header ok, reading elements from header");

        /* now, read the elements from the header until the end */
        while (gst_riff_parse_chunk (GST_ELEMENT_CAST (avi), buf, &offset, &tag,
                &sub)) {
          /* sub can be NULL on empty tags */
          if (!sub)
            continue;

          switch (tag) {
            case GST_RIFF_TAG_LIST:
              if (gst_buffer_get_size (sub) < 4)
                goto next;

              gst_buffer_extract (sub, 0, fourcc, 4);

              switch (GST_READ_UINT32_LE (fourcc)) {
                case GST_RIFF_LIST_strl:
                  if (!(gst_avi_demux_parse_stream (avi, sub))) {
                    sub = NULL;
                    GST_ELEMENT_WARNING (avi, STREAM, DEMUX, (NULL),
                        ("failed to parse stream, ignoring"));
                    goto next;
                  }
                  sub = NULL;
                  goto next;
                case GST_RIFF_LIST_odml:
                  gst_avi_demux_parse_odml (avi, sub);
                  sub = NULL;
                  break;
                default:
                  GST_WARNING_OBJECT (avi,
                      "Unknown list %" GST_FOURCC_FORMAT " in AVI header",
                      GST_FOURCC_ARGS (GST_READ_UINT32_LE (fourcc)));
                  /* fall-through */
                case GST_RIFF_TAG_JUNQ:
                case GST_RIFF_TAG_JUNK:
                  goto next;
              }
              break;
            case GST_RIFF_IDIT:
              gst_avi_demux_parse_idit (avi, sub);
              goto next;
            default:
              GST_WARNING_OBJECT (avi,
                  "Unknown tag %" GST_FOURCC_FORMAT " in AVI header",
                  GST_FOURCC_ARGS (tag));
              /* Only get buffer for debugging if the memdump is needed  */
              if (gst_debug_category_get_threshold (GST_CAT_DEFAULT) >= 9) {
                GstMapInfo map;

                gst_buffer_map (sub, &map, GST_MAP_READ);
                GST_MEMDUMP_OBJECT (avi, "Unknown tag", map.data, map.size);
                gst_buffer_unmap (sub, &map);
              }
              /* fall-through */
            case GST_RIFF_TAG_JUNQ:
            case GST_RIFF_TAG_JUNK:
            next:
              /* move to next chunk */
              if (sub)
                gst_buffer_unref (sub);
              sub = NULL;
              break;
          }
        }
        gst_buffer_unref (buf);
        GST_DEBUG ("elements parsed");

        /* check parsed streams */
        if (avi->num_streams == 0) {
          goto no_streams;
        } else if (avi->num_streams != avi->avih->streams) {
          GST_WARNING_OBJECT (avi,
              "Stream header mentioned %d streams, but %d available",
              avi->avih->streams, avi->num_streams);
        }
        GST_DEBUG ("Get junk and info next");
        avi->header_state = GST_AVI_DEMUX_HEADER_INFO;
      } else {
        /* Need more data */
        return ret;
      }
      /* fall-though */
    case GST_AVI_DEMUX_HEADER_INFO:
      GST_DEBUG_OBJECT (avi, "skipping junk between header and data ...");
      while (TRUE) {
        if (gst_adapter_available (avi->adapter) < 12)
          return GST_FLOW_OK;

        data = gst_adapter_map (avi->adapter, 12);
        tag = GST_READ_UINT32_LE (data);
        size = GST_READ_UINT32_LE (data + 4);
        ltag = GST_READ_UINT32_LE (data + 8);
        gst_adapter_unmap (avi->adapter);

        if (tag == GST_RIFF_TAG_LIST) {
          switch (ltag) {
            case GST_RIFF_LIST_movi:
              gst_adapter_flush (avi->adapter, 12);
              if (!avi->first_movi_offset)
                avi->first_movi_offset = avi->offset;
              avi->offset += 12;
              avi->idx1_offset = avi->offset + size - 4;
              goto skipping_done;
            case GST_RIFF_LIST_INFO:
              GST_DEBUG ("Found INFO chunk");
              if (gst_avi_demux_peek_chunk (avi, &tag, &size)) {
                GST_DEBUG ("got size %d", size);
                avi->offset += 12;
                gst_adapter_flush (avi->adapter, 12);
                if (size > 4) {
                  buf = gst_adapter_take_buffer (avi->adapter, size - 4);
                  /* mind padding */
                  if (size & 1)
                    gst_adapter_flush (avi->adapter, 1);
                  gst_riff_parse_info (GST_ELEMENT_CAST (avi), buf, &tags);
                  if (tags) {
                    if (avi->globaltags) {
                      gst_tag_list_insert (avi->globaltags, tags,
                          GST_TAG_MERGE_REPLACE);
                      gst_tag_list_unref (tags);
                    } else {
                      avi->globaltags = tags;
                    }
                  }
                  tags = NULL;
                  gst_buffer_unref (buf);

                  avi->offset += GST_ROUND_UP_2 (size) - 4;
                } else {
                  GST_DEBUG ("skipping INFO LIST prefix");
                }
              } else {
                /* Need more data */
                return GST_FLOW_OK;
              }
              break;
            default:
              if (gst_avi_demux_peek_chunk (avi, &tag, &size) || size == 0) {
                /* accept 0 size buffer here */
                avi->abort_buffering = FALSE;
                avi->offset += 8 + GST_ROUND_UP_2 (size);
                gst_adapter_flush (avi->adapter, 8 + GST_ROUND_UP_2 (size));
              } else {
                /* Need more data */
                return GST_FLOW_OK;
              }
              break;
          }
        } else {
          if (gst_avi_demux_peek_chunk (avi, &tag, &size) || size == 0) {
            /* accept 0 size buffer here */
            avi->abort_buffering = FALSE;
            avi->offset += 8 + GST_ROUND_UP_2 (size);
            gst_adapter_flush (avi->adapter, 8 + GST_ROUND_UP_2 (size));
          } else {
            /* Need more data */
            return GST_FLOW_OK;
          }
        }
      }
      break;
    default:
      GST_WARNING ("unhandled header state: %d", avi->header_state);
      break;
  }
skipping_done:

  GST_DEBUG_OBJECT (avi, "skipping done ... (streams=%u, stream[0].indexes=%p)",
      avi->num_streams, avi->stream[0].indexes);

  GST_DEBUG ("Found movi chunk. Starting to stream data");
  avi->state = GST_AVI_DEMUX_MOVI;

  /* no indexes in push mode, but it still sets some variables */
  gst_avi_demux_calculate_durations_from_index (avi);

  gst_avi_demux_expose_streams (avi, TRUE);

  /* prepare all streams for index 0 */
  for (i = 0; i < avi->num_streams; i++)
    avi->stream[i].current_entry = 0;

  /* create initial NEWSEGMENT event */
  if (avi->seg_event)
    gst_event_unref (avi->seg_event);
  avi->seg_event = gst_event_new_segment (&avi->segment);
  if (avi->segment_seqnum)
    gst_event_set_seqnum (avi->seg_event, avi->segment_seqnum);

  gst_avi_demux_check_seekability (avi);

  /* at this point we know all the streams and we can signal the no more
   * pads signal */
  GST_DEBUG_OBJECT (avi, "signaling no more pads");
  gst_element_no_more_pads (GST_ELEMENT_CAST (avi));

  return GST_FLOW_OK;

  /* ERRORS */
no_streams:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL), ("No streams found"));
    return GST_FLOW_ERROR;
  }
header_no_list:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("Invalid AVI header (no LIST at start): %"
            GST_FOURCC_FORMAT, GST_FOURCC_ARGS (tag)));
    return GST_FLOW_ERROR;
  }
header_no_avih:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("Invalid AVI header (no avih at start): %"
            GST_FOURCC_FORMAT, GST_FOURCC_ARGS (tag)));
    if (sub)
      gst_buffer_unref (sub);

    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
header_wrong_avih:
  {
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
}

static void
gst_avi_demux_add_date_tag (GstAviDemux * avi, gint y, gint m, gint d,
    gint h, gint min, gint s)
{
  GDate *date;
  GstDateTime *dt;

  date = g_date_new_dmy (d, m, y);
  if (!g_date_valid (date)) {
    /* bogus date */
    GST_WARNING_OBJECT (avi, "Refusing to add invalid date %d-%d-%d", y, m, d);
    g_date_free (date);
    return;
  }

  dt = gst_date_time_new_local_time (y, m, d, h, min, s);

  if (avi->globaltags == NULL)
    avi->globaltags = gst_tag_list_new_empty ();

  gst_tag_list_add (avi->globaltags, GST_TAG_MERGE_REPLACE, GST_TAG_DATE, date,
      NULL);
  g_date_free (date);
  if (dt) {
    gst_tag_list_add (avi->globaltags, GST_TAG_MERGE_REPLACE, GST_TAG_DATE_TIME,
        dt, NULL);
    gst_date_time_unref (dt);
  }
}

static void
gst_avi_demux_parse_idit_nums_only (GstAviDemux * avi, gchar * data)
{
  gint y, m, d;
  gint hr = 0, min = 0, sec = 0;
  gint ret;

  GST_DEBUG ("data : '%s'", data);

  ret = sscanf (data, "%d:%d:%d %d:%d:%d", &y, &m, &d, &hr, &min, &sec);
  if (ret < 3) {
    /* Attempt YYYY/MM/DD/ HH:MM variant (found in CASIO cameras) */
    ret = sscanf (data, "%04d/%02d/%02d/ %d:%d", &y, &m, &d, &hr, &min);
    if (ret < 3) {
      GST_WARNING_OBJECT (avi, "Failed to parse IDIT tag");
      return;
    }
  }
  gst_avi_demux_add_date_tag (avi, y, m, d, hr, min, sec);
}

static gint
get_month_num (gchar * data, guint size)
{
  if (g_ascii_strncasecmp (data, "jan", 3) == 0) {
    return 1;
  } else if (g_ascii_strncasecmp (data, "feb", 3) == 0) {
    return 2;
  } else if (g_ascii_strncasecmp (data, "mar", 3) == 0) {
    return 3;
  } else if (g_ascii_strncasecmp (data, "apr", 3) == 0) {
    return 4;
  } else if (g_ascii_strncasecmp (data, "may", 3) == 0) {
    return 5;
  } else if (g_ascii_strncasecmp (data, "jun", 3) == 0) {
    return 6;
  } else if (g_ascii_strncasecmp (data, "jul", 3) == 0) {
    return 7;
  } else if (g_ascii_strncasecmp (data, "aug", 3) == 0) {
    return 8;
  } else if (g_ascii_strncasecmp (data, "sep", 3) == 0) {
    return 9;
  } else if (g_ascii_strncasecmp (data, "oct", 3) == 0) {
    return 10;
  } else if (g_ascii_strncasecmp (data, "nov", 3) == 0) {
    return 11;
  } else if (g_ascii_strncasecmp (data, "dec", 3) == 0) {
    return 12;
  }

  return 0;
}

static void
gst_avi_demux_parse_idit_text (GstAviDemux * avi, gchar * data)
{
  gint year, month, day;
  gint hour, min, sec;
  gint ret;
  gchar weekday[4];
  gchar monthstr[4];

  ret = sscanf (data, "%3s %3s %d %d:%d:%d %d", weekday, monthstr, &day, &hour,
      &min, &sec, &year);
  if (ret != 7) {
    GST_WARNING_OBJECT (avi, "Failed to parse IDIT tag");
    return;
  }
  month = get_month_num (monthstr, strlen (monthstr));
  gst_avi_demux_add_date_tag (avi, year, month, day, hour, min, sec);
}

static void
gst_avi_demux_parse_idit (GstAviDemux * avi, GstBuffer * buf)
{
  GstMapInfo map;
  gchar *ptr;
  gsize left;
  gchar *safedata = NULL;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  /*
   * According to:
   * http://www.eden-foundation.org/products/code/film_date_stamp/index.html
   *
   * This tag could be in one of the below formats
   * 2005:08:17 11:42:43
   * THU OCT 26 16:46:04 2006
   * Mon Mar  3 09:44:56 2008
   *
   * FIXME: Our date tag doesn't include hours
   */

  /* skip eventual initial whitespace */
  ptr = (gchar *) map.data;
  left = map.size;

  while (left > 0 && g_ascii_isspace (ptr[0])) {
    ptr++;
    left--;
  }

  if (left == 0) {
    goto non_parsable;
  }

  /* make a safe copy to add a \0 to the end of the string */
  safedata = g_strndup (ptr, left);

  /* test if the first char is a alpha or a number */
  if (g_ascii_isdigit (ptr[0])) {
    gst_avi_demux_parse_idit_nums_only (avi, safedata);
    g_free (safedata);
    gst_buffer_unmap (buf, &map);
    return;
  } else if (g_ascii_isalpha (ptr[0])) {
    gst_avi_demux_parse_idit_text (avi, safedata);
    g_free (safedata);
    gst_buffer_unmap (buf, &map);
    return;
  }

  g_free (safedata);

non_parsable:
  GST_WARNING_OBJECT (avi, "IDIT tag has no parsable info");
  gst_buffer_unmap (buf, &map);
}

static void
parse_tag_value (GstAviDemux * avi, GstTagList * taglist, const gchar * type,
    guint8 * ptr, guint tsize)
{
  static const gchar *env_vars[] = { "GST_AVI_TAG_ENCODING",
    "GST_RIFF_TAG_ENCODING", "GST_TAG_ENCODING", NULL
  };
  GType tag_type;
  gchar *val;

  tag_type = gst_tag_get_type (type);
  val = gst_tag_freeform_string_to_utf8 ((gchar *) ptr, tsize, env_vars);

  if (val != NULL) {
    if (tag_type == G_TYPE_STRING) {
      gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, type, val, NULL);
    } else {
      GValue tag_val = { 0, };

      g_value_init (&tag_val, tag_type);
      if (gst_value_deserialize (&tag_val, val)) {
        gst_tag_list_add_value (taglist, GST_TAG_MERGE_APPEND, type, &tag_val);
      } else {
        GST_WARNING_OBJECT (avi, "could not deserialize '%s' into a "
            "tag %s of type %s", val, type, g_type_name (tag_type));
      }
      g_value_unset (&tag_val);
    }
    g_free (val);
  } else {
    GST_WARNING_OBJECT (avi, "could not extract %s tag", type);
  }
}

static void
gst_avi_demux_parse_strd (GstAviDemux * avi, GstBuffer * buf)
{
  GstMapInfo map;
  guint32 tag;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  if (map.size > 4) {
    guint8 *ptr = map.data;
    gsize left = map.size;

    /* parsing based on
     * http://www.eden-foundation.org/products/code/film_date_stamp/index.html
     */
    tag = GST_READ_UINT32_LE (ptr);
    if ((tag == GST_MAKE_FOURCC ('A', 'V', 'I', 'F')) && (map.size > 98)) {
      gsize sub_size;

      ptr += 98;
      left -= 98;
      if (!memcmp (ptr, "FUJIFILM", 8)) {
        GST_MEMDUMP_OBJECT (avi, "fujifim tag", ptr, 48);

        ptr += 10;
        left -= 10;
        sub_size = 0;
        while (ptr[sub_size] && sub_size < left)
          sub_size++;

        if (avi->globaltags == NULL)
          avi->globaltags = gst_tag_list_new_empty ();

        gst_tag_list_add (avi->globaltags, GST_TAG_MERGE_APPEND,
            GST_TAG_DEVICE_MANUFACTURER, "FUJIFILM", NULL);
        parse_tag_value (avi, avi->globaltags, GST_TAG_DEVICE_MODEL, ptr,
            sub_size);

        while (ptr[sub_size] == '\0' && sub_size < left)
          sub_size++;

        ptr += sub_size;
        left -= sub_size;
        sub_size = 0;
        while (ptr[sub_size] && sub_size < left)
          sub_size++;
        if (ptr[4] == ':')
          ptr[4] = '-';
        if (ptr[7] == ':')
          ptr[7] = '-';

        parse_tag_value (avi, avi->globaltags, GST_TAG_DATE_TIME, ptr,
            sub_size);
      }
    }
  }
  gst_buffer_unmap (buf, &map);
}

/*
 * gst_avi_demux_parse_ncdt:
 * @element: caller element (used for debugging/error).
 * @buf: input data to be used for parsing, stripped from header.
 * @taglist: a pointer to a taglist (returned by this function)
 *           containing information about this stream. May be
 *           NULL if no supported tags were found.
 *
 * Parses Nikon metadata from input data.
 */
static void
gst_avi_demux_parse_ncdt (GstAviDemux * avi, GstBuffer * buf,
    GstTagList ** _taglist)
{
  GstMapInfo info;
  guint8 *ptr;
  gsize left;
  guint tsize;
  guint32 tag;
  const gchar *type;
  GstTagList *taglist;

  g_return_if_fail (_taglist != NULL);

  if (!buf) {
    *_taglist = NULL;
    return;
  }
  gst_buffer_map (buf, &info, GST_MAP_READ);

  taglist = gst_tag_list_new_empty ();

  ptr = info.data;
  left = info.size;

  while (left > 8) {
    tag = GST_READ_UINT32_LE (ptr);
    tsize = GST_READ_UINT32_LE (ptr + 4);

    GST_MEMDUMP_OBJECT (avi, "tag chunk", ptr, MIN (tsize + 8, left));

    left -= 8;
    ptr += 8;

    GST_DEBUG_OBJECT (avi, "tag %" GST_FOURCC_FORMAT ", size %u",
        GST_FOURCC_ARGS (tag), tsize);

    if (tsize > left) {
      GST_WARNING_OBJECT (avi,
          "Tagsize %d is larger than available data %" G_GSIZE_FORMAT,
          tsize, left);
      tsize = left;
    }

    /* find out the type of metadata */
    switch (tag) {
      case GST_RIFF_LIST_nctg:
        while (tsize > 4) {
          guint16 sub_tag = GST_READ_UINT16_LE (ptr);
          guint16 sub_size = GST_READ_UINT16_LE (ptr + 2);

          tsize -= 4;
          ptr += 4;
          left -= 4;

          if (sub_size > tsize)
            break;

          GST_DEBUG_OBJECT (avi, "sub-tag %u, size %u", sub_tag, sub_size);
          /* http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/Nikon.html#NCTG
           * for some reason the sub_tag has a +2 offset
           */
          switch (sub_tag) {
            case 0x03:         /* Make */
              type = GST_TAG_DEVICE_MANUFACTURER;
              break;
            case 0x04:         /* Model */
              type = GST_TAG_DEVICE_MODEL;
              break;
              /* TODO: 0x05: is software version, like V1.0 */
            case 0x06:         /* Software */
              type = GST_TAG_ENCODER;
              break;
            case 0x13:         /* CreationDate */
              type = GST_TAG_DATE_TIME;
              if (left > 7) {
                if (ptr[4] == ':')
                  ptr[4] = '-';
                if (ptr[7] == ':')
                  ptr[7] = '-';
              }
              break;
            default:
              type = NULL;
              break;
          }
          if (type != NULL && ptr[0] != '\0') {
            GST_DEBUG_OBJECT (avi, "mapped tag %u to tag %s", sub_tag, type);

            parse_tag_value (avi, taglist, type, ptr, sub_size);
          }

          ptr += sub_size;
          tsize -= sub_size;
          left -= sub_size;
        }
        break;
      default:
        type = NULL;
        GST_WARNING_OBJECT (avi,
            "Unknown ncdt (metadata) tag entry %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (tag));
        GST_MEMDUMP_OBJECT (avi, "Unknown ncdt", ptr, tsize);
        break;
    }

    if (tsize & 1) {
      tsize++;
      if (tsize > left)
        tsize = left;
    }

    ptr += tsize;
    left -= tsize;
  }

  if (!gst_tag_list_is_empty (taglist)) {
    GST_INFO_OBJECT (avi, "extracted tags: %" GST_PTR_FORMAT, taglist);
    *_taglist = taglist;
  } else {
    *_taglist = NULL;
    gst_tag_list_unref (taglist);
  }
  gst_buffer_unmap (buf, &info);

  return;
}

/*
 * Read full AVI headers.
 */
static GstFlowReturn
gst_avi_demux_stream_header_pull (GstAviDemux * avi)
{
  GstFlowReturn res;
  GstBuffer *buf, *sub = NULL;
  guint32 tag;
  guint offset = 4;
  GstElement *element = GST_ELEMENT_CAST (avi);
  GstClockTime stamp;
  GstTagList *tags = NULL;
  guint8 fourcc[4];

  stamp = gst_util_get_timestamp ();

  /* the header consists of a 'hdrl' LIST tag */
  res = gst_riff_read_chunk (element, avi->sinkpad, &avi->offset, &tag, &buf);
  if (res != GST_FLOW_OK)
    goto pull_range_failed;
  else if (tag != GST_RIFF_TAG_LIST)
    goto no_list;
  else if (gst_buffer_get_size (buf) < 4)
    goto no_header;

  GST_DEBUG_OBJECT (avi, "parsing headers");

  /* Find the 'hdrl' LIST tag */
  gst_buffer_extract (buf, 0, fourcc, 4);
  while (GST_READ_UINT32_LE (fourcc) != GST_RIFF_LIST_hdrl) {
    GST_LOG_OBJECT (avi, "buffer contains %" GST_FOURCC_FORMAT,
        GST_FOURCC_ARGS (GST_READ_UINT32_LE (fourcc)));

    /* Eat up */
    gst_buffer_unref (buf);

    /* read new chunk */
    res = gst_riff_read_chunk (element, avi->sinkpad, &avi->offset, &tag, &buf);
    if (res != GST_FLOW_OK)
      goto pull_range_failed;
    else if (tag != GST_RIFF_TAG_LIST)
      goto no_list;
    else if (gst_buffer_get_size (buf) < 4)
      goto no_header;
    gst_buffer_extract (buf, 0, fourcc, 4);
  }

  GST_DEBUG_OBJECT (avi, "hdrl LIST tag found");

  gst_avi_demux_roundup_list (avi, &buf);

  /* the hdrl starts with a 'avih' header */
  if (!gst_riff_parse_chunk (element, buf, &offset, &tag, &sub))
    goto no_avih;
  else if (tag != GST_RIFF_TAG_avih)
    goto no_avih;
  else if (!gst_avi_demux_parse_avih (avi, sub, &avi->avih))
    goto invalid_avih;

  GST_DEBUG_OBJECT (avi, "AVI header ok, reading elements from header");

  /* now, read the elements from the header until the end */
  while (gst_riff_parse_chunk (element, buf, &offset, &tag, &sub)) {
    GstMapInfo map;

    /* sub can be NULL on empty tags */
    if (!sub)
      continue;

    gst_buffer_map (sub, &map, GST_MAP_READ);

    switch (tag) {
      case GST_RIFF_TAG_LIST:
        if (map.size < 4)
          goto next;

        switch (GST_READ_UINT32_LE (map.data)) {
          case GST_RIFF_LIST_strl:
            gst_buffer_unmap (sub, &map);
            if (!(gst_avi_demux_parse_stream (avi, sub))) {
              GST_ELEMENT_WARNING (avi, STREAM, DEMUX, (NULL),
                  ("failed to parse stream, ignoring"));
              sub = NULL;
            }
            sub = NULL;
            goto next;
          case GST_RIFF_LIST_odml:
            gst_buffer_unmap (sub, &map);
            gst_avi_demux_parse_odml (avi, sub);
            sub = NULL;
            break;
          case GST_RIFF_LIST_INFO:
            gst_buffer_unmap (sub, &map);
            gst_buffer_resize (sub, 4, -1);
            gst_riff_parse_info (element, sub, &tags);
            if (tags) {
              if (avi->globaltags) {
                gst_tag_list_insert (avi->globaltags, tags,
                    GST_TAG_MERGE_REPLACE);
                gst_tag_list_unref (tags);
              } else {
                avi->globaltags = tags;
              }
            }
            tags = NULL;
            gst_buffer_unref (sub);
            sub = NULL;
            break;
          case GST_RIFF_LIST_ncdt:
            gst_buffer_unmap (sub, &map);
            gst_buffer_resize (sub, 4, -1);
            gst_avi_demux_parse_ncdt (avi, sub, &tags);
            if (tags) {
              if (avi->globaltags) {
                gst_tag_list_insert (avi->globaltags, tags,
                    GST_TAG_MERGE_REPLACE);
                gst_tag_list_unref (tags);
              } else {
                avi->globaltags = tags;
              }
            }
            tags = NULL;
            gst_buffer_unref (sub);
            sub = NULL;
            break;
          default:
            GST_WARNING_OBJECT (avi,
                "Unknown list %" GST_FOURCC_FORMAT " in AVI header",
                GST_FOURCC_ARGS (GST_READ_UINT32_LE (map.data)));
            GST_MEMDUMP_OBJECT (avi, "Unknown list", map.data, map.size);
            /* fall-through */
          case GST_RIFF_TAG_JUNQ:
          case GST_RIFF_TAG_JUNK:
            goto next;
        }
        break;
      case GST_RIFF_IDIT:
        gst_avi_demux_parse_idit (avi, sub);
        goto next;
      default:
        GST_WARNING_OBJECT (avi,
            "Unknown tag %" GST_FOURCC_FORMAT " in AVI header",
            GST_FOURCC_ARGS (tag));
        GST_MEMDUMP_OBJECT (avi, "Unknown tag", map.data, map.size);
        /* fall-through */
      case GST_RIFF_TAG_JUNQ:
      case GST_RIFF_TAG_JUNK:
      next:
        if (sub) {
          gst_buffer_unmap (sub, &map);
          gst_buffer_unref (sub);
        }
        sub = NULL;
        break;
    }
  }
  gst_buffer_unref (buf);
  GST_DEBUG ("elements parsed");

  /* check parsed streams */
  if (avi->num_streams == 0)
    goto no_streams;
  else if (avi->num_streams != avi->avih->streams) {
    GST_WARNING_OBJECT (avi,
        "Stream header mentioned %d streams, but %d available",
        avi->avih->streams, avi->num_streams);
  }

  GST_DEBUG_OBJECT (avi, "skipping junk between header and data, offset=%"
      G_GUINT64_FORMAT, avi->offset);

  /* Now, find the data (i.e. skip all junk between header and data) */
  do {
    GstMapInfo map;
    guint size;
    guint32 tag, ltag;

    buf = NULL;
    res = gst_pad_pull_range (avi->sinkpad, avi->offset, 12, &buf);
    if (res != GST_FLOW_OK) {
      GST_DEBUG_OBJECT (avi, "pull_range failure while looking for tags");
      goto pull_range_failed;
    } else if (gst_buffer_get_size (buf) < 12) {
      GST_DEBUG_OBJECT (avi,
          "got %" G_GSIZE_FORMAT " bytes which is less than 12 bytes",
          gst_buffer_get_size (buf));
      gst_buffer_unref (buf);
      return GST_FLOW_ERROR;
    }

    gst_buffer_map (buf, &map, GST_MAP_READ);
    tag = GST_READ_UINT32_LE (map.data);
    size = GST_READ_UINT32_LE (map.data + 4);
    ltag = GST_READ_UINT32_LE (map.data + 8);

    GST_DEBUG ("tag %" GST_FOURCC_FORMAT ", size %u",
        GST_FOURCC_ARGS (tag), size);
    GST_MEMDUMP ("Tag content", map.data, map.size);
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);

    switch (tag) {
      case GST_RIFF_TAG_LIST:{
        switch (ltag) {
          case GST_RIFF_LIST_movi:
            GST_DEBUG_OBJECT (avi,
                "Reached the 'movi' tag, we're done with skipping");
            goto skipping_done;
          case GST_RIFF_LIST_INFO:
            res =
                gst_riff_read_chunk (element, avi->sinkpad, &avi->offset, &tag,
                &buf);
            if (res != GST_FLOW_OK) {
              GST_DEBUG_OBJECT (avi, "couldn't read INFO chunk");
              goto pull_range_failed;
            }
            GST_DEBUG ("got size %" G_GSIZE_FORMAT, gst_buffer_get_size (buf));
            if (size < 4) {
              GST_DEBUG ("skipping INFO LIST prefix");
              avi->offset += (4 - GST_ROUND_UP_2 (size));
              gst_buffer_unref (buf);
              continue;
            }

            sub = gst_buffer_copy_region (buf, GST_BUFFER_COPY_ALL, 4, -1);
            gst_riff_parse_info (element, sub, &tags);
            if (tags) {
              if (avi->globaltags) {
                gst_tag_list_insert (avi->globaltags, tags,
                    GST_TAG_MERGE_REPLACE);
                gst_tag_list_unref (tags);
              } else {
                avi->globaltags = tags;
              }
            }
            tags = NULL;
            if (sub) {
              gst_buffer_unref (sub);
              sub = NULL;
            }
            gst_buffer_unref (buf);
            /* gst_riff_read_chunk() has already advanced avi->offset */
            break;
          case GST_RIFF_LIST_ncdt:
            res =
                gst_riff_read_chunk (element, avi->sinkpad, &avi->offset, &tag,
                &buf);
            if (res != GST_FLOW_OK) {
              GST_DEBUG_OBJECT (avi, "couldn't read ncdt chunk");
              goto pull_range_failed;
            }
            GST_DEBUG ("got size %" G_GSIZE_FORMAT, gst_buffer_get_size (buf));
            if (size < 4) {
              GST_DEBUG ("skipping ncdt LIST prefix");
              avi->offset += (4 - GST_ROUND_UP_2 (size));
              gst_buffer_unref (buf);
              continue;
            }

            sub = gst_buffer_copy_region (buf, GST_BUFFER_COPY_ALL, 4, -1);
            gst_avi_demux_parse_ncdt (avi, sub, &tags);
            if (tags) {
              if (avi->globaltags) {
                gst_tag_list_insert (avi->globaltags, tags,
                    GST_TAG_MERGE_REPLACE);
                gst_tag_list_unref (tags);
              } else {
                avi->globaltags = tags;
              }
            }
            tags = NULL;
            if (sub) {
              gst_buffer_unref (sub);
              sub = NULL;
            }
            gst_buffer_unref (buf);
            /* gst_riff_read_chunk() has already advanced avi->offset */
            break;
          default:
            GST_WARNING_OBJECT (avi,
                "Skipping unknown list tag %" GST_FOURCC_FORMAT,
                GST_FOURCC_ARGS (ltag));
            avi->offset += 8 + GST_ROUND_UP_2 (size);
            break;
        }
      }
        break;
      default:
        GST_WARNING_OBJECT (avi, "Skipping unknown tag %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (tag));
        /* Fall-through */
      case GST_MAKE_FOURCC ('J', 'U', 'N', 'Q'):
      case GST_MAKE_FOURCC ('J', 'U', 'N', 'K'):
        /* Only get buffer for debugging if the memdump is needed  */
        if (gst_debug_category_get_threshold (GST_CAT_DEFAULT) >= 9) {
          buf = NULL;
          res = gst_pad_pull_range (avi->sinkpad, avi->offset, size, &buf);
          if (res != GST_FLOW_OK) {
            GST_DEBUG_OBJECT (avi, "couldn't read INFO chunk");
            goto pull_range_failed;
          }
          gst_buffer_map (buf, &map, GST_MAP_READ);
          GST_MEMDUMP ("Junk", map.data, map.size);
          gst_buffer_unmap (buf, &map);
          gst_buffer_unref (buf);
        }
        avi->offset += 8 + GST_ROUND_UP_2 (size);
        break;
    }
  } while (1);
skipping_done:

  GST_DEBUG_OBJECT (avi, "skipping done ... (streams=%u, stream[0].indexes=%p)",
      avi->num_streams, avi->stream[0].indexes);

  /* create or read stream index (for seeking) */
  if (avi->stream[0].indexes != NULL) {
    /* we read a super index already (gst_avi_demux_parse_superindex() ) */
    gst_avi_demux_read_subindexes_pull (avi);
  }
  if (!avi->have_index) {
    if (avi->avih->flags & GST_RIFF_AVIH_HASINDEX)
      gst_avi_demux_stream_index (avi);

    /* still no index, scan */
    if (!avi->have_index) {
      gst_avi_demux_stream_scan (avi);

      /* still no index.. this is a fatal error for now.
       * FIXME, we should switch to plain push mode without seeking
       * instead of failing. */
      if (!avi->have_index)
        goto no_index;
    }
  }
  /* use the indexes now to construct nice durations */
  gst_avi_demux_calculate_durations_from_index (avi);

  gst_avi_demux_expose_streams (avi, FALSE);

  /* do initial seek to the default segment values */
  gst_avi_demux_do_seek (avi, &avi->segment, 0);

  /* create initial NEWSEGMENT event */
  if (avi->seg_event)
    gst_event_unref (avi->seg_event);
  avi->seg_event = gst_event_new_segment (&avi->segment);
  if (avi->segment_seqnum)
    gst_event_set_seqnum (avi->seg_event, avi->segment_seqnum);

  stamp = gst_util_get_timestamp () - stamp;
  GST_DEBUG_OBJECT (avi, "pulling header took %" GST_TIME_FORMAT,
      GST_TIME_ARGS (stamp));

  /* at this point we know all the streams and we can signal the no more
   * pads signal */
  GST_DEBUG_OBJECT (avi, "signaling no more pads");
  gst_element_no_more_pads (GST_ELEMENT_CAST (avi));

  return GST_FLOW_OK;

  /* ERRORS */
no_list:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("Invalid AVI header (no LIST at start): %"
            GST_FOURCC_FORMAT, GST_FOURCC_ARGS (tag)));
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
no_header:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("Invalid AVI header (no hdrl at start): %"
            GST_FOURCC_FORMAT, GST_FOURCC_ARGS (tag)));
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
no_avih:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("Invalid AVI header (no avih at start): %"
            GST_FOURCC_FORMAT, GST_FOURCC_ARGS (tag)));
    if (sub)
      gst_buffer_unref (sub);
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
invalid_avih:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("Invalid AVI header (cannot parse avih at start)"));
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
no_streams:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL), ("No streams found"));
    return GST_FLOW_ERROR;
  }
no_index:
  {
    GST_WARNING ("file without or too big index");
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("Could not get/create index"));
    return GST_FLOW_ERROR;
  }
pull_range_failed:
  {
    if (res == GST_FLOW_FLUSHING)
      return res;
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL),
        ("pull_range flow reading header: %s", gst_flow_get_name (res)));
    return res;
  }
}

/* move a stream to @index */
static void
gst_avi_demux_move_stream (GstAviDemux * avi, GstAviStream * stream,
    GstSegment * segment, guint index)
{
  GST_DEBUG_OBJECT (avi, "Move stream %d to %u", stream->num, index);

  if (segment->rate < 0.0) {
    guint next_key;
    /* Because we don't know the frame order we need to push from the prev keyframe
     * to the next keyframe. If there is a smart decoder downstream he will notice
     * that there are too many encoded frames send and return EOS when there
     * are enough decoded frames to fill the segment. */
    next_key = gst_avi_demux_index_next (avi, stream, index, TRUE);

    /* FIXME, we go back to 0, we should look at segment.start. We will however
     * stop earlier when the see the timestamp < segment.start */
    stream->start_entry = 0;
    stream->step_entry = index;
    stream->current_entry = index;
    stream->stop_entry = next_key;

    GST_DEBUG_OBJECT (avi, "reverse seek: start %u, step %u, stop %u",
        stream->start_entry, stream->step_entry, stream->stop_entry);
  } else {
    stream->start_entry = index;
    stream->step_entry = index;
    stream->stop_entry = gst_avi_demux_index_last (avi, stream);
  }
  if (stream->current_entry != index) {
    GST_DEBUG_OBJECT (avi, "Move DISCONT from %u to %u",
        stream->current_entry, index);
    stream->current_entry = index;
    stream->discont = TRUE;
  }

  /* update the buffer info */
  gst_avi_demux_get_buffer_info (avi, stream, index,
      &stream->current_timestamp, &stream->current_ts_end,
      &stream->current_offset, &stream->current_offset_end);

  GST_DEBUG_OBJECT (avi, "Moved to %u, ts %" GST_TIME_FORMAT
      ", ts_end %" GST_TIME_FORMAT ", off %" G_GUINT64_FORMAT
      ", off_end %" G_GUINT64_FORMAT, index,
      GST_TIME_ARGS (stream->current_timestamp),
      GST_TIME_ARGS (stream->current_ts_end), stream->current_offset,
      stream->current_offset_end);

  GST_DEBUG_OBJECT (avi, "Seeking to offset %" G_GUINT64_FORMAT,
      stream->index[index].offset);
}

/*
 * Do the actual seeking.
 */
static gboolean
gst_avi_demux_do_seek (GstAviDemux * avi, GstSegment * segment,
    GstSeekFlags flags)
{
  GstClockTime seek_time;
  gboolean keyframe, before, after;
  guint i, index;
  GstAviStream *stream;
  gboolean next;

  seek_time = segment->position;
  keyframe = ! !(flags & GST_SEEK_FLAG_KEY_UNIT);
  before = ! !(flags & GST_SEEK_FLAG_SNAP_BEFORE);
  after = ! !(flags & GST_SEEK_FLAG_SNAP_AFTER);

  GST_DEBUG_OBJECT (avi, "seek to: %" GST_TIME_FORMAT
      " keyframe seeking:%d, %s", GST_TIME_ARGS (seek_time), keyframe,
      snap_types[before ? 1 : 0][after ? 1 : 0]);

  /* FIXME, this code assumes the main stream with keyframes is stream 0,
   * which is mostly correct... */
  stream = &avi->stream[avi->main_stream];

  next = after && !before;
  if (segment->rate < 0)
    next = !next;

  /* get the entry index for the requested position */
  index = gst_avi_demux_index_for_time (avi, stream, seek_time, next);
  GST_DEBUG_OBJECT (avi, "Got entry %u", index);
  if (index == -1)
    return FALSE;

  /* check if we are already on a keyframe */
  if (!ENTRY_IS_KEYFRAME (&stream->index[index])) {
    if (next) {
      GST_DEBUG_OBJECT (avi, "not keyframe, searching forward");
      /* now go to the next keyframe, this is where we should start
       * decoding from. */
      index = gst_avi_demux_index_next (avi, stream, index, TRUE);
      GST_DEBUG_OBJECT (avi, "next keyframe at %u", index);
    } else {
      GST_DEBUG_OBJECT (avi, "not keyframe, searching back");
      /* now go to the previous keyframe, this is where we should start
       * decoding from. */
      index = gst_avi_demux_index_prev (avi, stream, index, TRUE);
      GST_DEBUG_OBJECT (avi, "previous keyframe at %u", index);
    }
  }

  /* move the main stream to this position */
  gst_avi_demux_move_stream (avi, stream, segment, index);

  if (keyframe) {
    /* when seeking to a keyframe, we update the result seek time
     * to the time of the keyframe. */
    seek_time = stream->current_timestamp;
    GST_DEBUG_OBJECT (avi, "keyframe adjusted to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (seek_time));
    /* the seek time is always the position ... */
    segment->position = seek_time;
    /* ... and start and stream time when going forwards,
     * otherwise only stop time */
    if (segment->rate > 0.0)
      segment->start = segment->time = seek_time;
    else
      segment->stop = seek_time;
  }

  /* now set DISCONT and align the other streams */
  for (i = 0; i < avi->num_streams; i++) {
    GstAviStream *ostream;

    ostream = &avi->stream[i];
    if ((ostream == stream) || (ostream->index == NULL))
      continue;

    /* get the entry index for the requested position */
    index = gst_avi_demux_index_for_time (avi, ostream, seek_time, FALSE);
    if (index == -1)
      continue;

    /* move to previous keyframe */
    if (!ENTRY_IS_KEYFRAME (&ostream->index[index]))
      index = gst_avi_demux_index_prev (avi, ostream, index, TRUE);

    gst_avi_demux_move_stream (avi, ostream, segment, index);
  }
  GST_DEBUG_OBJECT (avi, "done seek to: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (seek_time));

  return TRUE;
}

/*
 * Handle seek event in pull mode.
 */
static gboolean
gst_avi_demux_handle_seek (GstAviDemux * avi, GstPad * pad, GstEvent * event)
{
  gdouble rate;
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType cur_type = GST_SEEK_TYPE_NONE, stop_type;
  gint64 cur, stop;
  gboolean flush;
  gboolean update;
  GstSegment seeksegment = { 0, };
  gint i;
  guint32 seqnum = 0;

  if (event) {
    GST_DEBUG_OBJECT (avi, "doing seek with event");

    gst_event_parse_seek (event, &rate, &format, &flags,
        &cur_type, &cur, &stop_type, &stop);
    seqnum = gst_event_get_seqnum (event);

    /* we have to have a format as the segment format. Try to convert
     * if not. */
    if (format != GST_FORMAT_TIME) {
      gboolean res = TRUE;

      if (cur_type != GST_SEEK_TYPE_NONE)
        res = gst_pad_query_convert (pad, format, cur, GST_FORMAT_TIME, &cur);
      if (res && stop_type != GST_SEEK_TYPE_NONE)
        res = gst_pad_query_convert (pad, format, stop, GST_FORMAT_TIME, &stop);
      if (!res)
        goto no_format;

      format = GST_FORMAT_TIME;
    }
    GST_DEBUG_OBJECT (avi,
        "seek requested: rate %g cur %" GST_TIME_FORMAT " stop %"
        GST_TIME_FORMAT, rate, GST_TIME_ARGS (cur), GST_TIME_ARGS (stop));
    /* FIXME: can we do anything with rate!=1.0 */
  } else {
    GST_DEBUG_OBJECT (avi, "doing seek without event");
    flags = 0;
    rate = 1.0;
  }

  /* save flush flag */
  flush = flags & GST_SEEK_FLAG_FLUSH;

  if (flush) {
    GstEvent *fevent = gst_event_new_flush_start ();

    if (seqnum)
      gst_event_set_seqnum (fevent, seqnum);
    /* for a flushing seek, we send a flush_start on all pads. This will
     * eventually stop streaming with a WRONG_STATE. We can thus eventually
     * take the STREAM_LOCK. */
    GST_DEBUG_OBJECT (avi, "sending flush start");
    gst_avi_demux_push_event (avi, gst_event_ref (fevent));
    gst_pad_push_event (avi->sinkpad, fevent);
  } else {
    /* a non-flushing seek, we PAUSE the task so that we can take the
     * STREAM_LOCK */
    GST_DEBUG_OBJECT (avi, "non flushing seek, pausing task");
    gst_pad_pause_task (avi->sinkpad);
  }

  /* wait for streaming to stop */
  GST_DEBUG_OBJECT (avi, "wait for streaming to stop");
  GST_PAD_STREAM_LOCK (avi->sinkpad);

  /* copy segment, we need this because we still need the old
   * segment when we close the current segment. */
  memcpy (&seeksegment, &avi->segment, sizeof (GstSegment));

  if (event) {
    GST_DEBUG_OBJECT (avi, "configuring seek");
    gst_segment_do_seek (&seeksegment, rate, format, flags,
        cur_type, cur, stop_type, stop, &update);
  }
  /* do the seek, seeksegment.position contains the new position, this
   * actually never fails. */
  gst_avi_demux_do_seek (avi, &seeksegment, flags);

  if (flush) {
    GstEvent *fevent = gst_event_new_flush_stop (TRUE);

    if (seqnum)
      gst_event_set_seqnum (fevent, seqnum);

    GST_DEBUG_OBJECT (avi, "sending flush stop");
    gst_avi_demux_push_event (avi, gst_event_ref (fevent));
    gst_pad_push_event (avi->sinkpad, fevent);
  }

  /* now update the real segment info */
  memcpy (&avi->segment, &seeksegment, sizeof (GstSegment));

  /* post the SEGMENT_START message when we do segmented playback */
  if (avi->segment.flags & GST_SEEK_FLAG_SEGMENT) {
    GstMessage *segment_start_msg =
        gst_message_new_segment_start (GST_OBJECT_CAST (avi),
        avi->segment.format, avi->segment.position);
    if (seqnum)
      gst_message_set_seqnum (segment_start_msg, seqnum);
    gst_element_post_message (GST_ELEMENT_CAST (avi), segment_start_msg);
  }

  /* queue the segment event for the streaming thread. */
  if (avi->seg_event)
    gst_event_unref (avi->seg_event);
  avi->seg_event = gst_event_new_segment (&avi->segment);
  if (seqnum)
    gst_event_set_seqnum (avi->seg_event, seqnum);
  avi->segment_seqnum = seqnum;

  if (!avi->streaming) {
    gst_pad_start_task (avi->sinkpad, (GstTaskFunction) gst_avi_demux_loop,
        avi->sinkpad, NULL);
  }
  /* reset the last flow and mark discont, seek is always DISCONT */
  for (i = 0; i < avi->num_streams; i++) {
    GST_DEBUG_OBJECT (avi, "marking DISCONT");
    avi->stream[i].discont = TRUE;
  }
  /* likewise for the whole new segment */
  gst_flow_combiner_reset (avi->flowcombiner);
  GST_PAD_STREAM_UNLOCK (avi->sinkpad);

  return TRUE;

  /* ERRORS */
no_format:
  {
    GST_DEBUG_OBJECT (avi, "unsupported format given, seek aborted.");
    return FALSE;
  }
}

/*
 * Handle seek event in push mode.
 */
static gboolean
avi_demux_handle_seek_push (GstAviDemux * avi, GstPad * pad, GstEvent * event)
{
  gdouble rate;
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType cur_type = GST_SEEK_TYPE_NONE, stop_type;
  gint64 cur, stop;
  gboolean keyframe, before, after, next;
  GstAviStream *stream;
  guint index;
  guint n, str_num;
  guint64 min_offset;
  GstSegment seeksegment;
  gboolean update;

  /* check we have the index */
  if (!avi->have_index) {
    GST_DEBUG_OBJECT (avi, "no seek index built, seek aborted.");
    return FALSE;
  } else {
    GST_DEBUG_OBJECT (avi, "doing push-based seek with event");
  }

  gst_event_parse_seek (event, &rate, &format, &flags,
      &cur_type, &cur, &stop_type, &stop);

  if (format != GST_FORMAT_TIME) {
    gboolean res = TRUE;

    if (cur_type != GST_SEEK_TYPE_NONE)
      res = gst_pad_query_convert (pad, format, cur, GST_FORMAT_TIME, &cur);
    if (res && stop_type != GST_SEEK_TYPE_NONE)
      res = gst_pad_query_convert (pad, format, stop, GST_FORMAT_TIME, &stop);
    if (!res) {
      GST_DEBUG_OBJECT (avi, "unsupported format given, seek aborted.");
      return FALSE;
    }

    format = GST_FORMAT_TIME;
  }

  /* let gst_segment handle any tricky stuff */
  GST_DEBUG_OBJECT (avi, "configuring seek");
  memcpy (&seeksegment, &avi->segment, sizeof (GstSegment));
  gst_segment_do_seek (&seeksegment, rate, format, flags,
      cur_type, cur, stop_type, stop, &update);

  keyframe = ! !(flags & GST_SEEK_FLAG_KEY_UNIT);
  cur = seeksegment.position;
  before = ! !(flags & GST_SEEK_FLAG_SNAP_BEFORE);
  after = ! !(flags & GST_SEEK_FLAG_SNAP_AFTER);

  GST_DEBUG_OBJECT (avi,
      "Seek requested: ts %" GST_TIME_FORMAT " stop %" GST_TIME_FORMAT
      ", kf %u, %s, rate %lf", GST_TIME_ARGS (cur), GST_TIME_ARGS (stop),
      keyframe, snap_types[before ? 1 : 0][after ? 1 : 0], rate);

  if (rate < 0) {
    GST_DEBUG_OBJECT (avi, "negative rate seek not supported in push mode");
    return FALSE;
  }

  /* FIXME, this code assumes the main stream with keyframes is stream 0,
   * which is mostly correct... */
  str_num = avi->main_stream;
  stream = &avi->stream[str_num];

  next = after && !before;
  if (seeksegment.rate < 0)
    next = !next;

  /* get the entry index for the requested position */
  index = gst_avi_demux_index_for_time (avi, stream, cur, next);
  GST_DEBUG_OBJECT (avi, "str %u: Found entry %u for %" GST_TIME_FORMAT,
      str_num, index, GST_TIME_ARGS (cur));
  if (index == -1)
    return -1;

  /* check if we are already on a keyframe */
  if (!ENTRY_IS_KEYFRAME (&stream->index[index])) {
    if (next) {
      GST_DEBUG_OBJECT (avi, "Entry is not a keyframe - searching forward");
      /* now go to the next keyframe, this is where we should start
       * decoding from. */
      index = gst_avi_demux_index_next (avi, stream, index, TRUE);
      GST_DEBUG_OBJECT (avi, "Found next keyframe at %u", index);
    } else {
      GST_DEBUG_OBJECT (avi, "Entry is not a keyframe - searching back");
      /* now go to the previous keyframe, this is where we should start
       * decoding from. */
      index = gst_avi_demux_index_prev (avi, stream, index, TRUE);
      GST_DEBUG_OBJECT (avi, "Found previous keyframe at %u", index);
    }
  }

  gst_avi_demux_get_buffer_info (avi, stream, index,
      &stream->current_timestamp, &stream->current_ts_end,
      &stream->current_offset, &stream->current_offset_end);

  /* re-use cur to be the timestamp of the seek as it _will_ be */
  cur = stream->current_timestamp;

  min_offset = stream->index[index].offset;
  avi->seek_kf_offset = min_offset - 8;

  GST_DEBUG_OBJECT (avi,
      "Seek to: ts %" GST_TIME_FORMAT " (on str %u, idx %u, offset %"
      G_GUINT64_FORMAT ")", GST_TIME_ARGS (stream->current_timestamp), str_num,
      index, min_offset);

  for (n = 0; n < avi->num_streams; n++) {
    GstAviStream *str = &avi->stream[n];
    guint idx;

    if (n == avi->main_stream)
      continue;

    /* get the entry index for the requested position */
    idx = gst_avi_demux_index_for_time (avi, str, cur, FALSE);
    GST_DEBUG_OBJECT (avi, "str %u: Found entry %u for %" GST_TIME_FORMAT, n,
        idx, GST_TIME_ARGS (cur));
    if (idx == -1)
      continue;

    /* check if we are already on a keyframe */
    if (!ENTRY_IS_KEYFRAME (&str->index[idx])) {
      if (next) {
        GST_DEBUG_OBJECT (avi, "Entry is not a keyframe - searching forward");
        /* now go to the next keyframe, this is where we should start
         * decoding from. */
        idx = gst_avi_demux_index_next (avi, str, idx, TRUE);
        GST_DEBUG_OBJECT (avi, "Found next keyframe at %u", idx);
      } else {
        GST_DEBUG_OBJECT (avi, "Entry is not a keyframe - searching back");
        /* now go to the previous keyframe, this is where we should start
         * decoding from. */
        idx = gst_avi_demux_index_prev (avi, str, idx, TRUE);
        GST_DEBUG_OBJECT (avi, "Found previous keyframe at %u", idx);
      }
    }

    gst_avi_demux_get_buffer_info (avi, str, idx,
        &str->current_timestamp, &str->current_ts_end,
        &str->current_offset, &str->current_offset_end);

    if (str->index[idx].offset < min_offset) {
      min_offset = str->index[idx].offset;
      GST_DEBUG_OBJECT (avi,
          "Found an earlier offset at %" G_GUINT64_FORMAT ", str %u",
          min_offset, n);
      str_num = n;
      stream = str;
      index = idx;
    }
  }

  GST_DEBUG_OBJECT (avi,
      "Seek performed: str %u, offset %" G_GUINT64_FORMAT ", idx %u, ts %"
      GST_TIME_FORMAT ", ts_end %" GST_TIME_FORMAT ", off %" G_GUINT64_FORMAT
      ", off_end %" G_GUINT64_FORMAT, str_num, min_offset, index,
      GST_TIME_ARGS (stream->current_timestamp),
      GST_TIME_ARGS (stream->current_ts_end), stream->current_offset,
      stream->current_offset_end);

  /* index data refers to data, not chunk header (for pull mode convenience) */
  min_offset -= 8;
  GST_DEBUG_OBJECT (avi, "seeking to chunk at offset %" G_GUINT64_FORMAT,
      min_offset);

  if (!perform_seek_to_offset (avi, min_offset, gst_event_get_seqnum (event))) {
    GST_DEBUG_OBJECT (avi, "seek event failed!");
    return FALSE;
  }

  return TRUE;
}

/*
 * Handle whether we can perform the seek event or if we have to let the chain
 * function handle seeks to build the seek indexes first.
 */
static gboolean
gst_avi_demux_handle_seek_push (GstAviDemux * avi, GstPad * pad,
    GstEvent * event)
{
  /* check for having parsed index already */
  if (!avi->have_index) {
    guint64 offset = 0;
    gboolean building_index;

    GST_OBJECT_LOCK (avi);
    /* handle the seek event in the chain function */
    avi->state = GST_AVI_DEMUX_SEEK;

    /* copy the event */
    if (avi->seek_event)
      gst_event_unref (avi->seek_event);
    avi->seek_event = gst_event_ref (event);

    /* set the building_index flag so that only one thread can setup the
     * structures for index seeking. */
    building_index = avi->building_index;
    if (!building_index) {
      avi->building_index = TRUE;
      if (avi->stream[0].indexes) {
        avi->odml_stream = 0;
        avi->odml_subidxs = avi->stream[avi->odml_stream].indexes;
        offset = avi->odml_subidxs[0];
      } else {
        offset = avi->idx1_offset;
      }
    }
    GST_OBJECT_UNLOCK (avi);

    if (!building_index) {
      /* seek to the first subindex or legacy index */
      GST_INFO_OBJECT (avi,
          "Seeking to legacy index/first subindex at %" G_GUINT64_FORMAT,
          offset);
      return perform_seek_to_offset (avi, offset, gst_event_get_seqnum (event));
    }

    /* FIXME: we have to always return true so that we don't block the seek
     * thread.
     * Note: maybe it is OK to return true if we're still building the index */
    return TRUE;
  }

  return avi_demux_handle_seek_push (avi, pad, event);
}

/*
 * Helper for gst_avi_demux_invert()
 */
static inline void
swap_line (guint8 * d1, guint8 * d2, guint8 * tmp, gint bytes)
{
  memcpy (tmp, d1, bytes);
  memcpy (d1, d2, bytes);
  memcpy (d2, tmp, bytes);
}


#define gst_avi_demux_is_uncompressed(fourcc)		\
  (fourcc == GST_RIFF_DIB ||				\
   fourcc == GST_RIFF_rgb ||				\
   fourcc == GST_RIFF_RGB || fourcc == GST_RIFF_RAW)

/*
 * Invert DIB buffers... Takes existing buffer and
 * returns either the buffer or a new one (with old
 * one dereferenced).
 * FIXME: can't we preallocate tmp? and remember stride, bpp?
 */
static GstBuffer *
gst_avi_demux_invert (GstAviStream * stream, GstBuffer * buf)
{
  gint y, w, h;
  gint bpp, stride;
  guint8 *tmp = NULL;
  GstMapInfo map;
  guint32 fourcc;

  if (stream->strh->type != GST_RIFF_FCC_vids)
    return buf;

  if (stream->strf.vids == NULL) {
    GST_WARNING ("Failed to retrieve vids for stream");
    return buf;
  }

  fourcc = (stream->strf.vids->compression) ?
      stream->strf.vids->compression : stream->strh->fcc_handler;
  if (!gst_avi_demux_is_uncompressed (fourcc)) {
    return buf;                 /* Ignore non DIB buffers */
  }

  /* raw rgb data is stored topdown, but instead of inverting the buffer, */
  /* some tools just negate the height field in the header (e.g. ffmpeg) */
  if (((gint32) stream->strf.vids->height) < 0)
    return buf;

  h = stream->strf.vids->height;
  w = stream->strf.vids->width;
  bpp = stream->strf.vids->bit_cnt ? stream->strf.vids->bit_cnt : 8;
  stride = GST_ROUND_UP_4 (w * (bpp / 8));

  buf = gst_buffer_make_writable (buf);

  gst_buffer_map (buf, &map, GST_MAP_READWRITE);
  if (map.size < (stride * h)) {
    GST_WARNING ("Buffer is smaller than reported Width x Height x Depth");
    gst_buffer_unmap (buf, &map);
    return buf;
  }

  tmp = g_malloc (stride);

  for (y = 0; y < h / 2; y++) {
    swap_line (map.data + stride * y, map.data + stride * (h - 1 - y), tmp,
        stride);
  }

  g_free (tmp);

  gst_buffer_unmap (buf, &map);

  /* append palette to paletted RGB8 buffer data */
  if (stream->rgb8_palette != NULL)
    buf = gst_buffer_append (buf, gst_buffer_ref (stream->rgb8_palette));

  return buf;
}

#if 0
static void
gst_avi_demux_add_assoc (GstAviDemux * avi, GstAviStream * stream,
    GstClockTime timestamp, guint64 offset, gboolean keyframe)
{
  /* do not add indefinitely for open-ended streaming */
  if (G_UNLIKELY (avi->element_index && avi->seekable)) {
    GST_LOG_OBJECT (avi, "adding association %" GST_TIME_FORMAT "-> %"
        G_GUINT64_FORMAT, GST_TIME_ARGS (timestamp), offset);
    gst_index_add_association (avi->element_index, avi->index_id,
        keyframe ? GST_ASSOCIATION_FLAG_KEY_UNIT :
        GST_ASSOCIATION_FLAG_DELTA_UNIT, GST_FORMAT_TIME, timestamp,
        GST_FORMAT_BYTES, offset, NULL);
    /* current_entry is DEFAULT (frame #) */
    gst_index_add_association (avi->element_index, stream->index_id,
        keyframe ? GST_ASSOCIATION_FLAG_KEY_UNIT :
        GST_ASSOCIATION_FLAG_DELTA_UNIT, GST_FORMAT_TIME, timestamp,
        GST_FORMAT_BYTES, offset, GST_FORMAT_DEFAULT, stream->current_entry,
        NULL);
  }
}
#endif

/*
 * Returns the aggregated GstFlowReturn.
 */
static GstFlowReturn
gst_avi_demux_combine_flows (GstAviDemux * avi, GstAviStream * stream,
    GstFlowReturn ret)
{
  GST_LOG_OBJECT (avi, "Stream %s:%s flow return: %s",
      GST_DEBUG_PAD_NAME (stream->pad), gst_flow_get_name (ret));
  ret = gst_flow_combiner_update_pad_flow (avi->flowcombiner, stream->pad, ret);
  GST_LOG_OBJECT (avi, "combined to return %s", gst_flow_get_name (ret));

  return ret;
}

/* move @stream to the next position in its index */
static GstFlowReturn
gst_avi_demux_advance (GstAviDemux * avi, GstAviStream * stream,
    GstFlowReturn ret)
{
  guint old_entry, new_entry;

  old_entry = stream->current_entry;
  /* move forwards */
  new_entry = old_entry + 1;

  /* see if we reached the end */
  if (new_entry >= stream->stop_entry) {
    if (avi->segment.rate < 0.0) {
      if (stream->step_entry == stream->start_entry) {
        /* we stepped all the way to the start, eos */
        GST_DEBUG_OBJECT (avi, "reverse reached start %u", stream->start_entry);
        goto eos;
      }
      /* backwards, stop becomes step, find a new step */
      stream->stop_entry = stream->step_entry;
      stream->step_entry = gst_avi_demux_index_prev (avi, stream,
          stream->stop_entry, TRUE);

      GST_DEBUG_OBJECT (avi,
          "reverse playback jump: start %u, step %u, stop %u",
          stream->start_entry, stream->step_entry, stream->stop_entry);

      /* and start from the previous keyframe now */
      new_entry = stream->step_entry;
    } else {
      /* EOS */
      GST_DEBUG_OBJECT (avi, "forward reached stop %u", stream->stop_entry);
      goto eos;
    }
  }

  if (new_entry != old_entry) {
    stream->current_entry = new_entry;
    stream->current_total = stream->index[new_entry].total;

    if (new_entry == old_entry + 1) {
      GST_DEBUG_OBJECT (avi, "moved forwards from %u to %u",
          old_entry, new_entry);
      /* we simply moved one step forwards, reuse current info */
      stream->current_timestamp = stream->current_ts_end;
      stream->current_offset = stream->current_offset_end;
      gst_avi_demux_get_buffer_info (avi, stream, new_entry,
          NULL, &stream->current_ts_end, NULL, &stream->current_offset_end);
    } else {
      /* we moved DISCONT, full update */
      gst_avi_demux_get_buffer_info (avi, stream, new_entry,
          &stream->current_timestamp, &stream->current_ts_end,
          &stream->current_offset, &stream->current_offset_end);
      /* and MARK discont for this stream */
      stream->discont = TRUE;
      GST_DEBUG_OBJECT (avi, "Moved from %u to %u, ts %" GST_TIME_FORMAT
          ", ts_end %" GST_TIME_FORMAT ", off %" G_GUINT64_FORMAT
          ", off_end %" G_GUINT64_FORMAT, old_entry, new_entry,
          GST_TIME_ARGS (stream->current_timestamp),
          GST_TIME_ARGS (stream->current_ts_end), stream->current_offset,
          stream->current_offset_end);
    }
  }
  return ret;

  /* ERROR */
eos:
  {
    GST_DEBUG_OBJECT (avi, "we are EOS");
    /* setting current_timestamp to -1 marks EOS */
    stream->current_timestamp = -1;
    return GST_FLOW_EOS;
  }
}

/* find the stream with the lowest current position when going forwards or with
 * the highest position when going backwards, this is the stream
 * we should push from next */
static gint
gst_avi_demux_find_next (GstAviDemux * avi, gfloat rate)
{
  guint64 min_time, max_time;
  guint stream_num, i;

  max_time = 0;
  min_time = G_MAXUINT64;
  stream_num = -1;

  for (i = 0; i < avi->num_streams; i++) {
    guint64 position;
    GstAviStream *stream;

    stream = &avi->stream[i];

    /* ignore streams that finished */
    if (stream->pad && GST_PAD_LAST_FLOW_RETURN (stream->pad) == GST_FLOW_EOS)
      continue;

    position = stream->current_timestamp;

    /* position of -1 is EOS */
    if (position != -1) {
      if (rate > 0.0 && position < min_time) {
        min_time = position;
        stream_num = i;
      } else if (rate < 0.0 && position >= max_time) {
        max_time = position;
        stream_num = i;
      }
    }
  }
  return stream_num;
}

static GstBuffer *
gst_avi_demux_align_buffer (GstAviDemux * demux,
    GstBuffer * buffer, gsize alignment)
{
  GstMapInfo map;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  if (map.size < sizeof (guintptr)) {
    gst_buffer_unmap (buffer, &map);
    return buffer;
  }

  if (((guintptr) map.data) & (alignment - 1)) {
    GstBuffer *new_buffer;
    GstAllocationParams params = { 0, alignment - 1, 0, 0, };

    new_buffer = gst_buffer_new_allocate (NULL,
        gst_buffer_get_size (buffer), &params);

    /* Copy data "by hand", so ensure alignment is kept: */
    gst_buffer_fill (new_buffer, 0, map.data, map.size);

    gst_buffer_copy_into (new_buffer, buffer, GST_BUFFER_COPY_METADATA, 0, -1);
    GST_DEBUG_OBJECT (demux,
        "We want output aligned on %" G_GSIZE_FORMAT ", reallocated",
        alignment);

    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);

    return new_buffer;
  }

  gst_buffer_unmap (buffer, &map);
  return buffer;
}

static GstFlowReturn
gst_avi_demux_loop_data (GstAviDemux * avi)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint stream_num;
  GstAviStream *stream;
  gboolean processed = FALSE;
  GstBuffer *buf;
  guint64 offset, size;
  GstClockTime timestamp, duration;
  guint64 out_offset, out_offset_end;
  gboolean keyframe;
  GstAviIndexEntry *entry;

  do {
    stream_num = gst_avi_demux_find_next (avi, avi->segment.rate);

    /* all are EOS */
    if (G_UNLIKELY (stream_num == -1)) {
      GST_DEBUG_OBJECT (avi, "all streams are EOS");
      goto eos;
    }

    /* we have the stream now */
    stream = &avi->stream[stream_num];

    /* skip streams without pads */
    if (!stream->pad) {
      GST_DEBUG_OBJECT (avi, "skipping entry from stream %d without pad",
          stream_num);
      goto next;
    }

    /* get the timing info for the entry */
    timestamp = stream->current_timestamp;
    duration = stream->current_ts_end - timestamp;
    out_offset = stream->current_offset;
    out_offset_end = stream->current_offset_end;

    /* get the entry data info */
    entry = &stream->index[stream->current_entry];
    offset = entry->offset;
    size = entry->size;
    keyframe = ENTRY_IS_KEYFRAME (entry);

    /* skip empty entries */
    if (size == 0) {
      GST_DEBUG_OBJECT (avi, "Skipping entry %u (%" G_GUINT64_FORMAT ", %p)",
          stream->current_entry, size, stream->pad);
      goto next;
    }

    if (avi->segment.rate > 0.0) {
      /* only check this for forwards playback for now */
      if (keyframe && GST_CLOCK_TIME_IS_VALID (avi->segment.stop)
          && (timestamp > avi->segment.stop)) {
        goto eos_stop;
      }
    } else {
      if (keyframe && GST_CLOCK_TIME_IS_VALID (avi->segment.start)
          && (timestamp < avi->segment.start))
        goto eos_stop;
    }

    GST_LOG ("reading buffer (size=%" G_GUINT64_FORMAT "), stream %d, pos %"
        G_GUINT64_FORMAT " (0x%" G_GINT64_MODIFIER "x), kf %d", size,
        stream_num, offset, offset, keyframe);

    /* FIXME, check large chunks and cut them up */

    /* pull in the data */
    buf = NULL;
    ret = gst_pad_pull_range (avi->sinkpad, offset, size, &buf);
    if (ret != GST_FLOW_OK)
      goto pull_failed;

    /* check for short buffers, this is EOS as well */
    if (gst_buffer_get_size (buf) < size)
      goto short_buffer;

    /* invert the picture if needed, and append palette for RGB8P */
    buf = gst_avi_demux_invert (stream, buf);

    /* mark non-keyframes */
    if (keyframe || stream->is_raw) {
      GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DELTA_UNIT);
      GST_BUFFER_PTS (buf) = timestamp;
    } else {
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT);
      GST_BUFFER_PTS (buf) = GST_CLOCK_TIME_NONE;
    }

    GST_BUFFER_DTS (buf) = timestamp;

    GST_BUFFER_DURATION (buf) = duration;
    GST_BUFFER_OFFSET (buf) = out_offset;
    GST_BUFFER_OFFSET_END (buf) = out_offset_end;

    /* mark discont when pending */
    if (stream->discont) {
      GST_DEBUG_OBJECT (avi, "setting DISCONT flag");
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
      stream->discont = FALSE;
    } else {
      GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DISCONT);
    }
#if 0
    gst_avi_demux_add_assoc (avi, stream, timestamp, offset, keyframe);
#endif

    /* update current position in the segment */
    avi->segment.position = timestamp;

    GST_DEBUG_OBJECT (avi, "Pushing buffer of size %" G_GSIZE_FORMAT ", ts %"
        GST_TIME_FORMAT ", dur %" GST_TIME_FORMAT ", off %" G_GUINT64_FORMAT
        ", off_end %" G_GUINT64_FORMAT,
        gst_buffer_get_size (buf), GST_TIME_ARGS (timestamp),
        GST_TIME_ARGS (duration), out_offset, out_offset_end);

    if (stream->alignment > 1)
      buf = gst_avi_demux_align_buffer (avi, buf, stream->alignment);
    ret = gst_pad_push (stream->pad, buf);

    /* mark as processed, we increment the frame and byte counters then
     * leave the while loop and return the GstFlowReturn */
    processed = TRUE;

    if (avi->segment.rate < 0) {
      if (timestamp > avi->segment.stop && ret == GST_FLOW_EOS) {
        /* In reverse playback we can get a GST_FLOW_EOS when
         * we are at the end of the segment, so we just need to jump
         * back to the previous section. */
        GST_DEBUG_OBJECT (avi, "downstream has reached end of segment");
        ret = GST_FLOW_OK;
      }
    }
  next:
    /* move to next item */
    ret = gst_avi_demux_advance (avi, stream, ret);

    /* combine flows */
    ret = gst_avi_demux_combine_flows (avi, stream, ret);
  } while (!processed);

beach:
  return ret;

  /* special cases */
eos:
  {
    GST_DEBUG_OBJECT (avi, "No samples left for any streams - EOS");
    ret = GST_FLOW_EOS;
    goto beach;
  }
eos_stop:
  {
    GST_LOG_OBJECT (avi, "Found keyframe after segment,"
        " setting EOS (%" GST_TIME_FORMAT " > %" GST_TIME_FORMAT ")",
        GST_TIME_ARGS (timestamp), GST_TIME_ARGS (avi->segment.stop));
    ret = GST_FLOW_EOS;
    /* move to next stream */
    goto next;
  }
pull_failed:
  {
    GST_DEBUG_OBJECT (avi, "pull range failed: pos=%" G_GUINT64_FORMAT
        " size=%" G_GUINT64_FORMAT, offset, size);
    goto beach;
  }
short_buffer:
  {
    GST_WARNING_OBJECT (avi, "Short read at offset %" G_GUINT64_FORMAT
        ", only got %" G_GSIZE_FORMAT "/%" G_GUINT64_FORMAT
        " bytes (truncated file?)", offset, gst_buffer_get_size (buf), size);
    gst_buffer_unref (buf);
    ret = GST_FLOW_EOS;
    goto beach;
  }
}

/*
 * Read data. If we have an index it delegates to
 * gst_avi_demux_process_next_entry().
 */
static GstFlowReturn
gst_avi_demux_stream_data (GstAviDemux * avi)
{
  guint32 tag = 0;
  guint32 size = 0;
  gint stream_nr = 0;
  GstFlowReturn res = GST_FLOW_OK;

  if (G_UNLIKELY (avi->have_eos)) {
    /* Clean adapter, we're done */
    gst_adapter_clear (avi->adapter);
    return GST_FLOW_EOS;
  }

  if (G_UNLIKELY (avi->todrop)) {
    guint drop;

    if ((drop = gst_adapter_available (avi->adapter))) {
      if (drop > avi->todrop)
        drop = avi->todrop;
      GST_DEBUG_OBJECT (avi, "Dropping %d bytes", drop);
      gst_adapter_flush (avi->adapter, drop);
      avi->todrop -= drop;
      avi->offset += drop;
    }
  }

  /* Iterate until need more data, so adapter won't grow too much */
  while (1) {
    if (G_UNLIKELY (!gst_avi_demux_peek_chunk_info (avi, &tag, &size))) {
      return GST_FLOW_OK;
    }

    GST_DEBUG ("Trying chunk (%" GST_FOURCC_FORMAT "), size %d",
        GST_FOURCC_ARGS (tag), size);

    if (G_LIKELY ((tag & 0xff) >= '0' && (tag & 0xff) <= '9' &&
            ((tag >> 8) & 0xff) >= '0' && ((tag >> 8) & 0xff) <= '9')) {
      GST_LOG ("Chunk ok");
    } else if ((tag & 0xffff) == (('x' << 8) | 'i')) {
      GST_DEBUG ("Found sub-index tag");
      if (gst_avi_demux_peek_chunk (avi, &tag, &size) || size == 0) {
        /* accept 0 size buffer here */
        avi->abort_buffering = FALSE;
        GST_DEBUG ("  skipping %d bytes for now", size);
        gst_adapter_flush (avi->adapter, 8 + GST_ROUND_UP_2 (size));
      }
      return GST_FLOW_OK;
    } else if (tag == GST_RIFF_TAG_RIFF) {
      /* RIFF tags can appear in ODML files, just jump over them */
      if (gst_adapter_available (avi->adapter) >= 12) {
        GST_DEBUG ("Found RIFF tag, skipping RIFF header");
        gst_adapter_flush (avi->adapter, 12);
        continue;
      }
      return GST_FLOW_OK;
    } else if (tag == GST_RIFF_TAG_idx1) {
      GST_DEBUG ("Found index tag");
      if (gst_avi_demux_peek_chunk (avi, &tag, &size) || size == 0) {
        /* accept 0 size buffer here */
        avi->abort_buffering = FALSE;
        GST_DEBUG ("  skipping %d bytes for now", size);
        gst_adapter_flush (avi->adapter, 8 + GST_ROUND_UP_2 (size));
      }
      return GST_FLOW_OK;
    } else if (tag == GST_RIFF_TAG_LIST) {
      /* movi chunks might be grouped in rec list */
      if (gst_adapter_available (avi->adapter) >= 12) {
        GST_DEBUG ("Found LIST tag, skipping LIST header");
        gst_adapter_flush (avi->adapter, 12);
        continue;
      }
      return GST_FLOW_OK;
    } else if (tag == GST_RIFF_TAG_JUNK || tag == GST_RIFF_TAG_JUNQ) {
      /* rec list might contain JUNK chunks */
      GST_DEBUG ("Found JUNK tag");
      if (gst_avi_demux_peek_chunk (avi, &tag, &size) || size == 0) {
        /* accept 0 size buffer here */
        avi->abort_buffering = FALSE;
        GST_DEBUG ("  skipping %d bytes for now", size);
        gst_adapter_flush (avi->adapter, 8 + GST_ROUND_UP_2 (size));
      }
      return GST_FLOW_OK;
    } else {
      GST_DEBUG ("No more stream chunks, send EOS");
      avi->have_eos = TRUE;
      return GST_FLOW_EOS;
    }

    if (G_UNLIKELY (!gst_avi_demux_peek_chunk (avi, &tag, &size))) {
      /* supposedly one hopes to catch a nicer chunk later on ... */
      /* FIXME ?? give up here rather than possibly ending up going
       * through the whole file */
      if (avi->abort_buffering) {
        avi->abort_buffering = FALSE;
        if (size) {
          gst_adapter_flush (avi->adapter, 8);
          return GST_FLOW_OK;
        }
      } else {
        return GST_FLOW_OK;
      }
    }
    GST_DEBUG ("chunk ID %" GST_FOURCC_FORMAT ", size %u",
        GST_FOURCC_ARGS (tag), size);

    stream_nr = CHUNKID_TO_STREAMNR (tag);

    if (G_UNLIKELY (stream_nr < 0 || stream_nr >= avi->num_streams)) {
      /* recoverable */
      GST_WARNING ("Invalid stream ID %d (%" GST_FOURCC_FORMAT ")",
          stream_nr, GST_FOURCC_ARGS (tag));
      avi->offset += 8 + GST_ROUND_UP_2 (size);
      gst_adapter_flush (avi->adapter, 8 + GST_ROUND_UP_2 (size));
    } else {
      GstAviStream *stream;
      GstClockTime next_ts = 0;
      GstBuffer *buf = NULL;
#if 0
      guint64 offset;
#endif
      gboolean saw_desired_kf = stream_nr != avi->main_stream
          || avi->offset >= avi->seek_kf_offset;

      if (stream_nr == avi->main_stream && avi->offset == avi->seek_kf_offset) {
        GST_DEBUG_OBJECT (avi, "Desired keyframe reached");
        avi->seek_kf_offset = 0;
      }

      if (saw_desired_kf) {
        gst_adapter_flush (avi->adapter, 8);
        /* get buffer */
        if (size) {
          buf = gst_adapter_take_buffer (avi->adapter, GST_ROUND_UP_2 (size));
          /* patch the size */
          gst_buffer_resize (buf, 0, size);
        } else {
          buf = NULL;
        }
      } else {
        GST_DEBUG_OBJECT (avi,
            "Desired keyframe not yet reached, flushing chunk");
        gst_adapter_flush (avi->adapter, 8 + GST_ROUND_UP_2 (size));
      }

#if 0
      offset = avi->offset;
#endif
      avi->offset += 8 + GST_ROUND_UP_2 (size);

      stream = &avi->stream[stream_nr];

      /* set delay (if any)
         if (stream->strh->init_frames == stream->current_frame &&
         stream->delay == 0)
         stream->delay = next_ts;
       */

      /* parsing of corresponding header may have failed */
      if (G_UNLIKELY (!stream->pad)) {
        GST_WARNING_OBJECT (avi, "no pad for stream ID %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (tag));
        if (buf)
          gst_buffer_unref (buf);
      } else {
        /* get time of this buffer */
        gst_pad_query_position (stream->pad, GST_FORMAT_TIME,
            (gint64 *) & next_ts);

#if 0
        gst_avi_demux_add_assoc (avi, stream, next_ts, offset, FALSE);
#endif

        /* increment our positions */
        stream->current_entry++;
        /* as in pull mode, 'total' is either bytes (CBR) or frames (VBR) */
        if (stream->strh->type == GST_RIFF_FCC_auds && stream->is_vbr) {
          gint blockalign = stream->strf.auds->blockalign;
          if (blockalign > 0)
            stream->current_total += DIV_ROUND_UP (size, blockalign);
          else
            stream->current_total++;
        } else {
          stream->current_total += size;
        }
        GST_LOG_OBJECT (avi, "current entry %u, total %u",
            stream->current_entry, stream->current_total);

        /* update current position in the segment */
        avi->segment.position = next_ts;

        if (saw_desired_kf && buf) {
          GstClockTime dur_ts = 0;

          /* invert the picture if needed, and append palette for RGB8P */
          buf = gst_avi_demux_invert (stream, buf);

          gst_pad_query_position (stream->pad, GST_FORMAT_TIME,
              (gint64 *) & dur_ts);

          GST_BUFFER_DTS (buf) = next_ts;
          GST_BUFFER_PTS (buf) = GST_CLOCK_TIME_NONE;
          GST_BUFFER_DURATION (buf) = dur_ts - next_ts;
          if (stream->strh->type == GST_RIFF_FCC_vids) {
            GST_BUFFER_OFFSET (buf) = stream->current_entry - 1;
            GST_BUFFER_OFFSET_END (buf) = stream->current_entry;
          } else {
            GST_BUFFER_OFFSET (buf) = GST_BUFFER_OFFSET_NONE;
            GST_BUFFER_OFFSET_END (buf) = GST_BUFFER_OFFSET_NONE;
          }

          GST_DEBUG_OBJECT (avi,
              "Pushing buffer with time=%" GST_TIME_FORMAT ", duration %"
              GST_TIME_FORMAT ", offset %" G_GUINT64_FORMAT
              " and size %d over pad %s", GST_TIME_ARGS (next_ts),
              GST_TIME_ARGS (GST_BUFFER_DURATION (buf)),
              GST_BUFFER_OFFSET (buf), size, GST_PAD_NAME (stream->pad));

          /* mark discont when pending */
          if (G_UNLIKELY (stream->discont)) {
            GST_DEBUG_OBJECT (avi, "Setting DISCONT");
            GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
            stream->discont = FALSE;
          } else {
            GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DISCONT);
          }

          if (stream->alignment > 1)
            buf = gst_avi_demux_align_buffer (avi, buf, stream->alignment);
          res = gst_pad_push (stream->pad, buf);
          buf = NULL;

          /* combine flows */
          res = gst_avi_demux_combine_flows (avi, stream, res);
          if (G_UNLIKELY (res != GST_FLOW_OK)) {
            GST_DEBUG ("Push failed; %s", gst_flow_get_name (res));
            return res;
          }
        }
      }
    }
  }

  return res;
}

/*
 * Send pending tags.
 */
static void
push_tag_lists (GstAviDemux * avi)
{
  guint i;
  GstTagList *tags;

  if (!avi->got_tags)
    return;

  GST_DEBUG_OBJECT (avi, "Pushing pending tag lists");

  for (i = 0; i < avi->num_streams; i++) {
    GstAviStream *stream = &avi->stream[i];
    GstPad *pad = stream->pad;

    tags = stream->taglist;

    if (pad && tags) {
      GST_DEBUG_OBJECT (pad, "Tags: %" GST_PTR_FORMAT, tags);

      gst_pad_push_event (pad, gst_event_new_tag (tags));
      stream->taglist = NULL;
    }
  }

  if (!(tags = avi->globaltags))
    tags = gst_tag_list_new_empty ();

  gst_tag_list_add (tags, GST_TAG_MERGE_REPLACE,
      GST_TAG_CONTAINER_FORMAT, "AVI", NULL);

  GST_DEBUG_OBJECT (avi, "Global tags: %" GST_PTR_FORMAT, tags);
  gst_tag_list_set_scope (tags, GST_TAG_SCOPE_GLOBAL);
  gst_avi_demux_push_event (avi, gst_event_new_tag (tags));
  avi->globaltags = NULL;
  avi->got_tags = FALSE;
}

static void
gst_avi_demux_loop (GstPad * pad)
{
  GstFlowReturn res;
  GstAviDemux *avi = GST_AVI_DEMUX (GST_PAD_PARENT (pad));

  switch (avi->state) {
    case GST_AVI_DEMUX_START:
      res = gst_avi_demux_stream_init_pull (avi);
      if (G_UNLIKELY (res != GST_FLOW_OK)) {
        GST_WARNING ("stream_init flow: %s", gst_flow_get_name (res));
        goto pause;
      }
      avi->state = GST_AVI_DEMUX_HEADER;
      /* fall-through */
    case GST_AVI_DEMUX_HEADER:
      res = gst_avi_demux_stream_header_pull (avi);
      if (G_UNLIKELY (res != GST_FLOW_OK)) {
        GST_WARNING ("stream_header flow: %s", gst_flow_get_name (res));
        goto pause;
      }
      avi->state = GST_AVI_DEMUX_MOVI;
      break;
    case GST_AVI_DEMUX_MOVI:
      if (G_UNLIKELY (avi->seg_event)) {
        gst_avi_demux_push_event (avi, avi->seg_event);
        avi->seg_event = NULL;
      }
      if (G_UNLIKELY (avi->got_tags)) {
        push_tag_lists (avi);
      }
      /* process each index entry in turn */
      res = gst_avi_demux_loop_data (avi);

      /* pause when error */
      if (G_UNLIKELY (res != GST_FLOW_OK)) {
        GST_INFO ("stream_movi flow: %s", gst_flow_get_name (res));
        goto pause;
      }
      break;
    default:
      GST_ERROR_OBJECT (avi, "unknown state %d", avi->state);
      res = GST_FLOW_ERROR;
      goto pause;
  }

  return;

  /* ERRORS */
pause:{

    gboolean push_eos = FALSE;
    GST_LOG_OBJECT (avi, "pausing task, reason %s", gst_flow_get_name (res));
    gst_pad_pause_task (avi->sinkpad);

    if (res == GST_FLOW_EOS) {
      /* handle end-of-stream/segment */
      /* so align our position with the end of it, if there is one
       * this ensures a subsequent will arrive at correct base/acc time */
      if (avi->segment.rate > 0.0 &&
          GST_CLOCK_TIME_IS_VALID (avi->segment.stop))
        avi->segment.position = avi->segment.stop;
      else if (avi->segment.rate < 0.0)
        avi->segment.position = avi->segment.start;
      if (avi->segment.flags & GST_SEEK_FLAG_SEGMENT) {
        gint64 stop;
        GstEvent *event;
        GstMessage *msg;

        if ((stop = avi->segment.stop) == -1)
          stop = avi->segment.duration;

        GST_INFO_OBJECT (avi, "sending segment_done");

        msg =
            gst_message_new_segment_done (GST_OBJECT_CAST (avi),
            GST_FORMAT_TIME, stop);
        if (avi->segment_seqnum)
          gst_message_set_seqnum (msg, avi->segment_seqnum);
        gst_element_post_message (GST_ELEMENT_CAST (avi), msg);

        event = gst_event_new_segment_done (GST_FORMAT_TIME, stop);
        if (avi->segment_seqnum)
          gst_event_set_seqnum (event, avi->segment_seqnum);
        gst_avi_demux_push_event (avi, event);
      } else {
        push_eos = TRUE;
      }
    } else if (res == GST_FLOW_NOT_LINKED || res < GST_FLOW_EOS) {
      /* for fatal errors we post an error message, wrong-state is
       * not fatal because it happens due to flushes and only means
       * that we should stop now. */
      GST_ELEMENT_FLOW_ERROR (avi, res);
      push_eos = TRUE;
    }
    if (push_eos) {
      GstEvent *event;

      GST_INFO_OBJECT (avi, "sending eos");
      event = gst_event_new_eos ();
      if (avi->segment_seqnum)
        gst_event_set_seqnum (event, avi->segment_seqnum);
      if (!gst_avi_demux_push_event (avi, event) && (res == GST_FLOW_EOS)) {
        GST_ELEMENT_ERROR (avi, STREAM, DEMUX,
            (NULL), ("got eos but no streams (yet)"));
      }
    }
  }
}


static GstFlowReturn
gst_avi_demux_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstFlowReturn res;
  GstAviDemux *avi = GST_AVI_DEMUX (parent);
  gint i;

  if (GST_BUFFER_IS_DISCONT (buf)) {
    GST_DEBUG_OBJECT (avi, "got DISCONT");
    gst_adapter_clear (avi->adapter);
    /* mark all streams DISCONT */
    for (i = 0; i < avi->num_streams; i++)
      avi->stream[i].discont = TRUE;
  }

  GST_DEBUG ("Store %" G_GSIZE_FORMAT " bytes in adapter",
      gst_buffer_get_size (buf));
  gst_adapter_push (avi->adapter, buf);

  switch (avi->state) {
    case GST_AVI_DEMUX_START:
      if ((res = gst_avi_demux_stream_init_push (avi)) != GST_FLOW_OK) {
        GST_WARNING ("stream_init flow: %s", gst_flow_get_name (res));
        break;
      }
      break;
    case GST_AVI_DEMUX_HEADER:
      if ((res = gst_avi_demux_stream_header_push (avi)) != GST_FLOW_OK) {
        GST_WARNING ("stream_header flow: %s", gst_flow_get_name (res));
        break;
      }
      break;
    case GST_AVI_DEMUX_MOVI:
      if (G_UNLIKELY (avi->seg_event)) {
        gst_avi_demux_push_event (avi, avi->seg_event);
        avi->seg_event = NULL;
      }
      if (G_UNLIKELY (avi->got_tags)) {
        push_tag_lists (avi);
      }
      res = gst_avi_demux_stream_data (avi);
      break;
    case GST_AVI_DEMUX_SEEK:
    {
      GstEvent *event;

      res = GST_FLOW_OK;

      /* obtain and parse indexes */
      if (avi->stream[0].indexes && !gst_avi_demux_read_subindexes_push (avi))
        /* seek in subindex read function failed */
        goto index_failed;

      if (!avi->stream[0].indexes && !avi->have_index
          && avi->avih->flags & GST_RIFF_AVIH_HASINDEX)
        gst_avi_demux_stream_index_push (avi);

      if (avi->have_index) {
        /* use the indexes now to construct nice durations */
        gst_avi_demux_calculate_durations_from_index (avi);
      } else {
        /* still parsing indexes */
        break;
      }

      GST_OBJECT_LOCK (avi);
      event = avi->seek_event;
      avi->seek_event = NULL;
      GST_OBJECT_UNLOCK (avi);

      /* calculate and perform seek */
      if (!avi_demux_handle_seek_push (avi, avi->sinkpad, event)) {
        gst_event_unref (event);
        goto seek_failed;
      }

      gst_event_unref (event);
      avi->state = GST_AVI_DEMUX_MOVI;
      break;
    }
    default:
      GST_ELEMENT_ERROR (avi, STREAM, FAILED, (NULL),
          ("Illegal internal state"));
      res = GST_FLOW_ERROR;
      break;
  }

  GST_DEBUG_OBJECT (avi, "state: %d res:%s", avi->state,
      gst_flow_get_name (res));

  if (G_UNLIKELY (avi->abort_buffering))
    goto abort_buffering;

  return res;

  /* ERRORS */
index_failed:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL), ("failed to read indexes"));
    return GST_FLOW_ERROR;
  }
seek_failed:
  {
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL), ("push mode seek failed"));
    return GST_FLOW_ERROR;
  }
abort_buffering:
  {
    avi->abort_buffering = FALSE;
    GST_ELEMENT_ERROR (avi, STREAM, DEMUX, (NULL), ("unhandled buffer size"));
    return GST_FLOW_ERROR;
  }
}

static gboolean
gst_avi_demux_sink_activate (GstPad * sinkpad, GstObject * parent)
{
  GstQuery *query;
  gboolean pull_mode;

  query = gst_query_new_scheduling ();

  if (!gst_pad_peer_query (sinkpad, query)) {
    gst_query_unref (query);
    goto activate_push;
  }

  pull_mode = gst_query_has_scheduling_mode_with_flags (query,
      GST_PAD_MODE_PULL, GST_SCHEDULING_FLAG_SEEKABLE);
  gst_query_unref (query);

  if (!pull_mode)
    goto activate_push;

  GST_DEBUG_OBJECT (sinkpad, "activating pull");
  return gst_pad_activate_mode (sinkpad, GST_PAD_MODE_PULL, TRUE);

activate_push:
  {
    GST_DEBUG_OBJECT (sinkpad, "activating push");
    return gst_pad_activate_mode (sinkpad, GST_PAD_MODE_PUSH, TRUE);
  }
}

static gboolean
gst_avi_demux_sink_activate_mode (GstPad * sinkpad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  gboolean res;
  GstAviDemux *avi = GST_AVI_DEMUX (parent);

  switch (mode) {
    case GST_PAD_MODE_PULL:
      if (active) {
        avi->streaming = FALSE;
        res = gst_pad_start_task (sinkpad, (GstTaskFunction) gst_avi_demux_loop,
            sinkpad, NULL);
      } else {
        res = gst_pad_stop_task (sinkpad);
      }
      break;
    case GST_PAD_MODE_PUSH:
      if (active) {
        GST_DEBUG ("avi: activating push/chain function");
        avi->streaming = TRUE;
      } else {
        GST_DEBUG ("avi: deactivating push/chain function");
      }
      res = TRUE;
      break;
    default:
      res = FALSE;
      break;
  }
  return res;
}

#if 0
static void
gst_avi_demux_set_index (GstElement * element, GstIndex * index)
{
  GstAviDemux *avi = GST_AVI_DEMUX (element);

  GST_OBJECT_LOCK (avi);
  if (avi->element_index)
    gst_object_unref (avi->element_index);
  if (index) {
    avi->element_index = gst_object_ref (index);
  } else {
    avi->element_index = NULL;
  }
  GST_OBJECT_UNLOCK (avi);
  /* object lock might be taken again */
  if (index)
    gst_index_get_writer_id (index, GST_OBJECT_CAST (element), &avi->index_id);
  GST_DEBUG_OBJECT (avi, "Set index %" GST_PTR_FORMAT, avi->element_index);
}

static GstIndex *
gst_avi_demux_get_index (GstElement * element)
{
  GstIndex *result = NULL;
  GstAviDemux *avi = GST_AVI_DEMUX (element);

  GST_OBJECT_LOCK (avi);
  if (avi->element_index)
    result = gst_object_ref (avi->element_index);
  GST_OBJECT_UNLOCK (avi);

  GST_DEBUG_OBJECT (avi, "Returning index %" GST_PTR_FORMAT, result);

  return result;
}
#endif

static GstStateChangeReturn
gst_avi_demux_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstAviDemux *avi = GST_AVI_DEMUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      avi->streaming = FALSE;
      gst_segment_init (&avi->segment, GST_FORMAT_TIME);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto done;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      avi->have_index = FALSE;
      gst_avi_demux_reset (avi);
      break;
    default:
      break;
  }

done:
  return ret;
}
