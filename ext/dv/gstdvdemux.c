/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *               <2005> Wim Taymans <wim@fluendo.com>
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
#include "config.h"
#endif

#include <string.h>
#include <math.h>

#include <gst/audio/audio.h>
#include "gstdvdemux.h"
#include "gstsmptetimecode.h"

/**
 * SECTION:element-dvdemux
 * @title: dvdemux
 *
 * dvdemux splits raw DV into its audio and video components. The audio will be
 * decoded raw samples and the video will be encoded DV video.
 *
 * This element can operate in both push and pull mode depending on the
 * capabilities of the upstream peer.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=test.dv ! dvdemux name=demux ! queue ! audioconvert ! alsasink demux. ! queue ! dvdec ! xvimagesink
 * ]| This pipeline decodes and renders the raw DV stream to an audio and a videosink.
 *
 */

/* DV output has two modes, normal and wide. The resolution is the same in both
 * cases: 720 pixels wide by 576 pixels tall in PAL format, and 720x480 for
 * NTSC.
 *
 * Each of the modes has its own pixel aspect ratio, which is fixed in practice
 * by ITU-R BT.601 (also known as "CCIR-601" or "Rec.601"). Or so claims a
 * reference that I culled from the reliable "internet",
 * http://www.mir.com/DMG/aspect.html. Normal PAL is 59/54 and normal NTSC is
 * 10/11. Because the pixel resolution is the same for both cases, we can get
 * the pixel aspect ratio for wide recordings by multiplying by the ratio of
 * display aspect ratios, 16/9 (for wide) divided by 4/3 (for normal):
 *
 * Wide NTSC: 10/11 * (16/9)/(4/3) = 40/33
 * Wide PAL: 59/54 * (16/9)/(4/3) = 118/81
 *
 * However, the pixel resolution coming out of a DV source does not combine with
 * the standard pixel aspect ratios to give a proper display aspect ratio. An
 * image 480 pixels tall, with a 4:3 display aspect ratio, will be 768 pixels
 * wide. But, if we take the normal PAL aspect ratio of 59/54, and multiply it
 * with the width of the DV image (720 pixels), we get 786.666..., which is
 * nonintegral and too wide. The camera is not outputting a 4:3 image.
 * 
 * If the video sink for this stream has fixed dimensions (such as for
 * fullscreen playback, or for a java applet in a web page), you then have two
 * choices. Either you show the whole image, but pad the image with black
 * borders on the top and bottom (like watching a widescreen video on a 4:3
 * device), or you crop the video to the proper ratio. Apparently the latter is
 * the standard practice.
 *
 * For its part, GStreamer is concerned with accuracy and preservation of
 * information. This element outputs the 720x576 or 720x480 video that it
 * receives, noting the proper aspect ratio. This should not be a problem for
 * windowed applications, which can change size to fit the video. Applications
 * with fixed size requirements should decide whether to crop or pad which
 * an element such as videobox can do.
 */

#define NTSC_HEIGHT 480
#define NTSC_BUFFER 120000
#define NTSC_FRAMERATE_NUMERATOR 30000
#define NTSC_FRAMERATE_DENOMINATOR 1001

#define PAL_HEIGHT 576
#define PAL_BUFFER 144000
#define PAL_FRAMERATE_NUMERATOR 25
#define PAL_FRAMERATE_DENOMINATOR 1

#define PAL_NORMAL_PAR_X        16
#define PAL_NORMAL_PAR_Y        15
#define PAL_WIDE_PAR_X          64
#define PAL_WIDE_PAR_Y          45

#define NTSC_NORMAL_PAR_X       8
#define NTSC_NORMAL_PAR_Y       9
#define NTSC_WIDE_PAR_X         32
#define NTSC_WIDE_PAR_Y         27

GST_DEBUG_CATEGORY_STATIC (dvdemux_debug);
#define GST_CAT_DEFAULT dvdemux_debug

static GstStaticPadTemplate sink_temp = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-dv, systemstream = (boolean) true")
    );

static GstStaticPadTemplate video_src_temp = GST_STATIC_PAD_TEMPLATE ("video",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("video/x-dv, systemstream = (boolean) false")
    );

static GstStaticPadTemplate audio_src_temp = GST_STATIC_PAD_TEMPLATE ("audio",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "rate = (int) { 32000, 44100, 48000 }, " "channels = (int) {2, 4}")
    );


#define gst_dvdemux_parent_class parent_class
G_DEFINE_TYPE (GstDVDemux, gst_dvdemux, GST_TYPE_ELEMENT);

static void gst_dvdemux_finalize (GObject * object);

/* query functions */
static gboolean gst_dvdemux_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query);
static gboolean gst_dvdemux_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query);

/* convert functions */
static gboolean gst_dvdemux_sink_convert (GstDVDemux * demux,
    GstFormat src_format, gint64 src_value, GstFormat dest_format,
    gint64 * dest_value);
static gboolean gst_dvdemux_src_convert (GstDVDemux * demux, GstPad * pad,
    GstFormat src_format, gint64 src_value, GstFormat dest_format,
    gint64 * dest_value);

/* event functions */
static gboolean gst_dvdemux_send_event (GstElement * element, GstEvent * event);
static gboolean gst_dvdemux_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_dvdemux_handle_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

/* scheduling functions */
static void gst_dvdemux_loop (GstPad * pad);
static GstFlowReturn gst_dvdemux_flush (GstDVDemux * dvdemux);
static GstFlowReturn gst_dvdemux_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

/* state change functions */
static gboolean gst_dvdemux_sink_activate (GstPad * sinkpad,
    GstObject * parent);
static gboolean gst_dvdemux_sink_activate_mode (GstPad * sinkpad,
    GstObject * parent, GstPadMode mode, gboolean active);
static GstStateChangeReturn gst_dvdemux_change_state (GstElement * element,
    GstStateChange transition);

static void
gst_dvdemux_class_init (GstDVDemuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->finalize = gst_dvdemux_finalize;

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_dvdemux_change_state);
  gstelement_class->send_event = GST_DEBUG_FUNCPTR (gst_dvdemux_send_event);

  gst_element_class_add_static_pad_template (gstelement_class, &sink_temp);
  gst_element_class_add_static_pad_template (gstelement_class, &video_src_temp);
  gst_element_class_add_static_pad_template (gstelement_class, &audio_src_temp);

  gst_element_class_set_static_metadata (gstelement_class,
      "DV system stream demuxer", "Codec/Demuxer",
      "Uses libdv to separate DV audio from DV video (libdv.sourceforge.net)",
      "Erik Walthinsen <omega@cse.ogi.edu>, Wim Taymans <wim@fluendo.com>");

  GST_DEBUG_CATEGORY_INIT (dvdemux_debug, "dvdemux", 0, "DV demuxer element");
}

static void
gst_dvdemux_init (GstDVDemux * dvdemux)
{
  gint i;

  dvdemux->sinkpad = gst_pad_new_from_static_template (&sink_temp, "sink");
  /* we can operate in pull and push mode so we install
   * a custom activate function */
  gst_pad_set_activate_function (dvdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_dvdemux_sink_activate));
  /* the function to activate in push mode */
  gst_pad_set_activatemode_function (dvdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_dvdemux_sink_activate_mode));
  /* for push mode, this is the chain function */
  gst_pad_set_chain_function (dvdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_dvdemux_chain));
  /* handling events (in push mode only) */
  gst_pad_set_event_function (dvdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_dvdemux_handle_sink_event));
  /* query functions */
  gst_pad_set_query_function (dvdemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_dvdemux_sink_query));

  /* now add the pad */
  gst_element_add_pad (GST_ELEMENT (dvdemux), dvdemux->sinkpad);

  dvdemux->adapter = gst_adapter_new ();

  /* we need 4 temp buffers for audio decoding which are of a static
   * size and which we can allocate here */
  for (i = 0; i < 4; i++) {
    dvdemux->audio_buffers[i] =
        (gint16 *) g_malloc (DV_AUDIO_MAX_SAMPLES * sizeof (gint16));
  }
}

static void
gst_dvdemux_finalize (GObject * object)
{
  GstDVDemux *dvdemux;
  gint i;

  dvdemux = GST_DVDEMUX (object);

  g_object_unref (dvdemux->adapter);

  /* clean up temp audio buffers */
  for (i = 0; i < 4; i++) {
    g_free (dvdemux->audio_buffers[i]);
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* reset to default values before starting streaming */
static void
gst_dvdemux_reset (GstDVDemux * dvdemux)
{
  dvdemux->frame_offset = 0;
  dvdemux->audio_offset = 0;
  dvdemux->video_offset = 0;
  dvdemux->discont = TRUE;
  g_atomic_int_set (&dvdemux->found_header, 0);
  dvdemux->frame_len = -1;
  dvdemux->need_segment = FALSE;
  dvdemux->new_media = FALSE;
  dvdemux->framerate_numerator = 0;
  dvdemux->framerate_denominator = 0;
  dvdemux->height = 0;
  dvdemux->frequency = 0;
  dvdemux->channels = 0;
  dvdemux->wide = FALSE;
  gst_segment_init (&dvdemux->byte_segment, GST_FORMAT_BYTES);
  gst_segment_init (&dvdemux->time_segment, GST_FORMAT_TIME);
  dvdemux->segment_seqnum = 0;
  dvdemux->upstream_time_segment = FALSE;
  dvdemux->have_group_id = FALSE;
  dvdemux->group_id = G_MAXUINT;
  dvdemux->tag_event = NULL;
}

static gboolean
have_group_id (GstDVDemux * demux)
{
  GstEvent *event;

  event = gst_pad_get_sticky_event (demux->sinkpad, GST_EVENT_STREAM_START, 0);
  if (event) {
    if (gst_event_parse_group_id (event, &demux->group_id))
      demux->have_group_id = TRUE;
    else
      demux->have_group_id = FALSE;
    gst_event_unref (event);
  } else if (!demux->have_group_id) {
    demux->have_group_id = TRUE;
    demux->group_id = gst_util_group_id_next ();
  }

  return demux->have_group_id;
}

static GstEvent *
gst_dvdemux_create_global_tag_event (GstDVDemux * dvdemux)
{
  gchar rec_datetime[40];
  GstDateTime *rec_dt;
  GstTagList *tags;

  tags = gst_tag_list_new (GST_TAG_CONTAINER_FORMAT, "DV", NULL);
  gst_tag_list_set_scope (tags, GST_TAG_SCOPE_GLOBAL);

  if (dv_get_recording_datetime (dvdemux->decoder, rec_datetime)) {
    rec_dt = gst_date_time_new_from_iso8601_string (rec_datetime);
    if (rec_dt) {
      gst_tag_list_add (tags, GST_TAG_MERGE_REPLACE, GST_TAG_DATE_TIME,
          rec_dt, NULL);
      gst_date_time_unref (rec_dt);
    }
  }

  return gst_event_new_tag (tags);
}

static GstPad *
gst_dvdemux_add_pad (GstDVDemux * dvdemux, GstStaticPadTemplate * template,
    GstCaps * caps)
{
  GstPad *pad;
  GstEvent *event;
  gchar *stream_id;

  pad = gst_pad_new_from_static_template (template, template->name_template);

  gst_pad_set_query_function (pad, GST_DEBUG_FUNCPTR (gst_dvdemux_src_query));

  gst_pad_set_event_function (pad,
      GST_DEBUG_FUNCPTR (gst_dvdemux_handle_src_event));
  gst_pad_use_fixed_caps (pad);
  gst_pad_set_active (pad, TRUE);

  stream_id =
      gst_pad_create_stream_id (pad,
      GST_ELEMENT_CAST (dvdemux),
      template == &video_src_temp ? "video" : "audio");
  event = gst_event_new_stream_start (stream_id);
  if (have_group_id (dvdemux))
    gst_event_set_group_id (event, dvdemux->group_id);
  gst_pad_push_event (pad, event);
  g_free (stream_id);

  gst_pad_set_caps (pad, caps);

  gst_pad_push_event (pad, gst_event_new_segment (&dvdemux->time_segment));

  gst_element_add_pad (GST_ELEMENT (dvdemux), pad);

  if (!dvdemux->tag_event) {
    dvdemux->tag_event = gst_dvdemux_create_global_tag_event (dvdemux);
  }

  if (dvdemux->tag_event) {
    gst_pad_push_event (pad, gst_event_ref (dvdemux->tag_event));
  }

  return pad;
}

static void
gst_dvdemux_remove_pads (GstDVDemux * dvdemux)
{
  if (dvdemux->videosrcpad) {
    gst_element_remove_pad (GST_ELEMENT (dvdemux), dvdemux->videosrcpad);
    dvdemux->videosrcpad = NULL;
  }
  if (dvdemux->audiosrcpad) {
    gst_element_remove_pad (GST_ELEMENT (dvdemux), dvdemux->audiosrcpad);
    dvdemux->audiosrcpad = NULL;
  }
}

static gboolean
gst_dvdemux_src_convert (GstDVDemux * dvdemux, GstPad * pad,
    GstFormat src_format, gint64 src_value, GstFormat dest_format,
    gint64 * dest_value)
{
  gboolean res = TRUE;

  if (dest_format == src_format || src_value == -1) {
    *dest_value = src_value;
    goto done;
  }

  if (dvdemux->frame_len <= 0)
    goto error;

  if (dvdemux->decoder == NULL)
    goto error;

  GST_INFO_OBJECT (pad,
      "src_value:%" G_GINT64_FORMAT ", src_format:%d, dest_format:%d",
      src_value, src_format, dest_format);

  switch (src_format) {
    case GST_FORMAT_BYTES:
      switch (dest_format) {
        case GST_FORMAT_DEFAULT:
          if (pad == dvdemux->videosrcpad)
            *dest_value = src_value / dvdemux->frame_len;
          else if (pad == dvdemux->audiosrcpad)
            *dest_value = src_value / (2 * dvdemux->channels);
          break;
        case GST_FORMAT_TIME:
          if (pad == dvdemux->videosrcpad)
            *dest_value = gst_util_uint64_scale (src_value,
                GST_SECOND * dvdemux->framerate_denominator,
                dvdemux->frame_len * dvdemux->framerate_numerator);
          else if (pad == dvdemux->audiosrcpad)
            *dest_value = gst_util_uint64_scale_int (src_value, GST_SECOND,
                2 * dvdemux->frequency * dvdemux->channels);
          break;
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_TIME:
      switch (dest_format) {
        case GST_FORMAT_BYTES:
          if (pad == dvdemux->videosrcpad)
            *dest_value = gst_util_uint64_scale (src_value,
                dvdemux->frame_len * dvdemux->framerate_numerator,
                dvdemux->framerate_denominator * GST_SECOND);
          else if (pad == dvdemux->audiosrcpad)
            *dest_value = gst_util_uint64_scale_int (src_value,
                2 * dvdemux->frequency * dvdemux->channels, GST_SECOND);
          break;
        case GST_FORMAT_DEFAULT:
          if (pad == dvdemux->videosrcpad) {
            if (src_value)
              *dest_value = gst_util_uint64_scale (src_value,
                  dvdemux->framerate_numerator,
                  dvdemux->framerate_denominator * GST_SECOND);
            else
              *dest_value = 0;
          } else if (pad == dvdemux->audiosrcpad) {
            *dest_value = gst_util_uint64_scale (src_value,
                dvdemux->frequency, GST_SECOND);
          }
          break;
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_DEFAULT:
      switch (dest_format) {
        case GST_FORMAT_TIME:
          if (pad == dvdemux->videosrcpad) {
            *dest_value = gst_util_uint64_scale (src_value,
                GST_SECOND * dvdemux->framerate_denominator,
                dvdemux->framerate_numerator);
          } else if (pad == dvdemux->audiosrcpad) {
            if (src_value)
              *dest_value = gst_util_uint64_scale (src_value,
                  GST_SECOND, dvdemux->frequency);
            else
              *dest_value = 0;
          }
          break;
        case GST_FORMAT_BYTES:
          if (pad == dvdemux->videosrcpad) {
            *dest_value = src_value * dvdemux->frame_len;
          } else if (pad == dvdemux->audiosrcpad) {
            *dest_value = src_value * 2 * dvdemux->channels;
          }
          break;
        default:
          res = FALSE;
      }
      break;
    default:
      res = FALSE;
  }

done:
  GST_INFO_OBJECT (pad,
      "Result : dest_format:%d, dest_value:%" G_GINT64_FORMAT ", res:%d",
      dest_format, *dest_value, res);
  return res;

  /* ERRORS */
error:
  {
    GST_INFO ("source conversion failed");
    return FALSE;
  }
}

static gboolean
gst_dvdemux_sink_convert (GstDVDemux * dvdemux, GstFormat src_format,
    gint64 src_value, GstFormat dest_format, gint64 * dest_value)
{
  gboolean res = TRUE;

  GST_DEBUG_OBJECT (dvdemux, "%d -> %d", src_format, dest_format);
  GST_INFO_OBJECT (dvdemux,
      "src_value:%" G_GINT64_FORMAT ", src_format:%d, dest_format:%d",
      src_value, src_format, dest_format);

  if (dest_format == src_format || src_value == -1) {
    *dest_value = src_value;
    goto done;
  }

  if (dvdemux->frame_len <= 0)
    goto error;

  switch (src_format) {
    case GST_FORMAT_BYTES:
      switch (dest_format) {
        case GST_FORMAT_TIME:
        {
          guint64 frame;

          /* get frame number, rounds down so don't combine this
           * line and the next line. */
          frame = src_value / dvdemux->frame_len;

          *dest_value = gst_util_uint64_scale (frame,
              GST_SECOND * dvdemux->framerate_denominator,
              dvdemux->framerate_numerator);
          break;
        }
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_TIME:
      switch (dest_format) {
        case GST_FORMAT_BYTES:
        {
          guint64 frame;

          /* calculate the frame */
          frame =
              gst_util_uint64_scale (src_value, dvdemux->framerate_numerator,
              dvdemux->framerate_denominator * GST_SECOND);

          /* calculate the offset from the rounded frame */
          *dest_value = frame * dvdemux->frame_len;
          break;
        }
        default:
          res = FALSE;
      }
      break;
    default:
      res = FALSE;
  }
  GST_INFO_OBJECT (dvdemux,
      "Result : dest_format:%d, dest_value:%" G_GINT64_FORMAT ", res:%d",
      dest_format, *dest_value, res);

done:
  return res;

error:
  {
    GST_INFO_OBJECT (dvdemux, "sink conversion failed");
    return FALSE;
  }
}

static gboolean
gst_dvdemux_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res = TRUE;
  GstDVDemux *dvdemux;

  dvdemux = GST_DVDEMUX (parent);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
    {
      GstFormat format;
      gint64 cur;

      /* get target format */
      gst_query_parse_position (query, &format, NULL);

      /* bring the position to the requested format. */
      if (!(res = gst_dvdemux_src_convert (dvdemux, pad,
                  GST_FORMAT_TIME, dvdemux->time_segment.position,
                  format, &cur)))
        goto error;
      gst_query_set_position (query, format, cur);
      break;
    }
    case GST_QUERY_DURATION:
    {
      GstFormat format;
      gint64 end;
      GstQuery *pquery;

      /* First ask the peer in the original format */
      if (!gst_pad_peer_query (dvdemux->sinkpad, query)) {
        /* get target format */
        gst_query_parse_duration (query, &format, NULL);

        /* Now ask the peer in BYTES format and try to convert */
        pquery = gst_query_new_duration (GST_FORMAT_BYTES);
        if (!gst_pad_peer_query (dvdemux->sinkpad, pquery)) {
          gst_query_unref (pquery);
          goto error;
        }

        /* get peer total length */
        gst_query_parse_duration (pquery, NULL, &end);
        gst_query_unref (pquery);

        /* convert end to requested format */
        if (end != -1) {
          if (!(res = gst_dvdemux_sink_convert (dvdemux,
                      GST_FORMAT_BYTES, end, format, &end))) {
            goto error;
          }
          gst_query_set_duration (query, format, end);
        }
      }
      break;
    }
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (!(res =
              gst_dvdemux_src_convert (dvdemux, pad, src_fmt, src_val,
                  dest_fmt, &dest_val)))
        goto error;
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      break;
    }
    case GST_QUERY_SEEKING:
    {
      GstFormat fmt;
      GstQuery *peerquery;
      gboolean seekable;

      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);

      /* We can only handle TIME seeks really */
      if (fmt != GST_FORMAT_TIME) {
        gst_query_set_seeking (query, fmt, FALSE, -1, -1);
        break;
      }

      /* First ask upstream */
      if (gst_pad_peer_query (dvdemux->sinkpad, query)) {
        gst_query_parse_seeking (query, NULL, &seekable, NULL, NULL);
        if (seekable) {
          res = TRUE;
          break;
        }
      }

      res = TRUE;

      peerquery = gst_query_new_seeking (GST_FORMAT_BYTES);
      seekable = gst_pad_peer_query (dvdemux->sinkpad, peerquery);

      if (seekable)
        gst_query_parse_seeking (peerquery, NULL, &seekable, NULL, NULL);
      gst_query_unref (peerquery);

      if (seekable) {
        peerquery = gst_query_new_duration (GST_FORMAT_TIME);
        seekable = gst_dvdemux_src_query (pad, parent, peerquery);

        if (seekable) {
          gint64 duration;

          gst_query_parse_duration (peerquery, NULL, &duration);
          gst_query_set_seeking (query, GST_FORMAT_TIME, seekable, 0, duration);
        } else {
          gst_query_set_seeking (query, GST_FORMAT_TIME, FALSE, -1, -1);
        }

        gst_query_unref (peerquery);
      } else {
        gst_query_set_seeking (query, GST_FORMAT_TIME, FALSE, -1, -1);
      }
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;

  /* ERRORS */
error:
  {
    GST_DEBUG ("error source query");
    return FALSE;
  }
}

static gboolean
gst_dvdemux_sink_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res = TRUE;
  GstDVDemux *dvdemux;

  dvdemux = GST_DVDEMUX (parent);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (!(res =
              gst_dvdemux_sink_convert (dvdemux, src_fmt, src_val, dest_fmt,
                  &dest_val)))
        goto error;
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;

  /* ERRORS */
error:
  {
    GST_DEBUG ("error handling sink query");
    return FALSE;
  }
}

/* takes ownership of the event */
static gboolean
gst_dvdemux_push_event (GstDVDemux * dvdemux, GstEvent * event)
{
  gboolean res = FALSE;

  if (dvdemux->videosrcpad) {
    gst_event_ref (event);
    res |= gst_pad_push_event (dvdemux->videosrcpad, event);
  }

  if (dvdemux->audiosrcpad)
    res |= gst_pad_push_event (dvdemux->audiosrcpad, event);
  else {
    gst_event_unref (event);
    res = TRUE;
  }

  return res;
}

static gboolean
gst_dvdemux_handle_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstDVDemux *dvdemux = GST_DVDEMUX (parent);
  gboolean res = TRUE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
      /* we are not blocking on anything exect the push() calls
       * to the peer which will be unblocked by forwarding the
       * event.*/
      res = gst_dvdemux_push_event (dvdemux, event);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_adapter_clear (dvdemux->adapter);
      GST_DEBUG ("cleared adapter");
      gst_segment_init (&dvdemux->byte_segment, GST_FORMAT_BYTES);
      gst_segment_init (&dvdemux->time_segment, GST_FORMAT_TIME);
      dvdemux->discont = TRUE;
      res = gst_dvdemux_push_event (dvdemux, event);
      break;
    case GST_EVENT_SEGMENT:
    {
      const GstSegment *segment;

      gst_event_parse_segment (event, &segment);
      switch (segment->format) {
        case GST_FORMAT_BYTES:
          gst_segment_copy_into (segment, &dvdemux->byte_segment);
          dvdemux->need_segment = TRUE;
          dvdemux->segment_seqnum = gst_event_get_seqnum (event);
          gst_event_unref (event);
          break;
        case GST_FORMAT_TIME:
          gst_segment_copy_into (segment, &dvdemux->time_segment);

          dvdemux->upstream_time_segment = TRUE;
          dvdemux->segment_seqnum = gst_event_get_seqnum (event);

          /* and we can just forward this time event */
          res = gst_dvdemux_push_event (dvdemux, event);
          break;
        default:
          gst_event_unref (event);
          /* cannot accept this format */
          res = FALSE;
          break;
      }
      break;
    }
    case GST_EVENT_EOS:
      /* flush any pending data, should be nothing left. */
      gst_dvdemux_flush (dvdemux);
      /* forward event */
      res = gst_dvdemux_push_event (dvdemux, event);
      /* and clear the adapter */
      gst_adapter_clear (dvdemux->adapter);
      break;
    case GST_EVENT_CAPS:
      gst_event_unref (event);
      break;
    default:
      res = gst_dvdemux_push_event (dvdemux, event);
      break;
  }

  return res;
}

/* convert a pair of values on the given srcpad */
static gboolean
gst_dvdemux_convert_src_pair (GstDVDemux * dvdemux, GstPad * pad,
    GstFormat src_format, gint64 src_start, gint64 src_stop,
    GstFormat dst_format, gint64 * dst_start, gint64 * dst_stop)
{
  gboolean res;

  GST_INFO ("starting conversion of start");
  /* bring the format to time on srcpad. */
  if (!(res = gst_dvdemux_src_convert (dvdemux, pad,
              src_format, src_start, dst_format, dst_start))) {
    goto done;
  }
  GST_INFO ("Finished conversion of start: %" G_GINT64_FORMAT, *dst_start);

  GST_INFO ("starting conversion of stop");
  /* bring the format to time on srcpad. */
  if (!(res = gst_dvdemux_src_convert (dvdemux, pad,
              src_format, src_stop, dst_format, dst_stop))) {
    /* could not convert seek format to time offset */
    goto done;
  }
  GST_INFO ("Finished conversion of stop: %" G_GINT64_FORMAT, *dst_stop);
done:
  return res;
}

/* convert a pair of values on the sinkpad */
static gboolean
gst_dvdemux_convert_sink_pair (GstDVDemux * dvdemux,
    GstFormat src_format, gint64 src_start, gint64 src_stop,
    GstFormat dst_format, gint64 * dst_start, gint64 * dst_stop)
{
  gboolean res;

  GST_INFO ("starting conversion of start");
  /* bring the format to time on srcpad. */
  if (!(res = gst_dvdemux_sink_convert (dvdemux,
              src_format, src_start, dst_format, dst_start))) {
    goto done;
  }
  GST_INFO ("Finished conversion of start: %" G_GINT64_FORMAT, *dst_start);

  GST_INFO ("starting conversion of stop");
  /* bring the format to time on srcpad. */
  if (!(res = gst_dvdemux_sink_convert (dvdemux,
              src_format, src_stop, dst_format, dst_stop))) {
    /* could not convert seek format to time offset */
    goto done;
  }
  GST_INFO ("Finished conversion of stop: %" G_GINT64_FORMAT, *dst_stop);
done:
  return res;
}

/* convert a pair of values on the srcpad to a pair of
 * values on the sinkpad 
 */
static gboolean
gst_dvdemux_convert_src_to_sink (GstDVDemux * dvdemux, GstPad * pad,
    GstFormat src_format, gint64 src_start, gint64 src_stop,
    GstFormat dst_format, gint64 * dst_start, gint64 * dst_stop)
{
  GstFormat conv;
  gboolean res;

  conv = GST_FORMAT_TIME;
  /* convert to TIME intermediate format */
  if (!(res = gst_dvdemux_convert_src_pair (dvdemux, pad,
              src_format, src_start, src_stop, conv, dst_start, dst_stop))) {
    /* could not convert format to time offset */
    goto done;
  }
  /* convert to dst format on sinkpad */
  if (!(res = gst_dvdemux_convert_sink_pair (dvdemux,
              conv, *dst_start, *dst_stop, dst_format, dst_start, dst_stop))) {
    /* could not convert format to time offset */
    goto done;
  }
done:
  return res;
}

#if 0
static gboolean
gst_dvdemux_convert_segment (GstDVDemux * dvdemux, GstSegment * src,
    GstSegment * dest)
{
  dest->rate = src->rate;
  dest->abs_rate = src->abs_rate;
  dest->flags = src->flags;

  return TRUE;
}
#endif

/* handle seek in push base mode.
 *
 * Convert the time seek to a bytes seek and send it
 * upstream
 * Does not take ownership of the event.
 */
static gboolean
gst_dvdemux_handle_push_seek (GstDVDemux * dvdemux, GstPad * pad,
    GstEvent * event)
{
  gboolean res = FALSE;
  gdouble rate;
  GstSeekFlags flags;
  GstFormat format;
  GstSeekType cur_type, stop_type;
  gint64 cur, stop;
  gint64 start_position, end_position;
  GstEvent *newevent;

  gst_event_parse_seek (event, &rate, &format, &flags,
      &cur_type, &cur, &stop_type, &stop);

  /* First try if upstream can handle time based seeks */
  if (format == GST_FORMAT_TIME)
    res = gst_pad_push_event (dvdemux->sinkpad, gst_event_ref (event));

  if (!res) {
    /* we convert the start/stop on the srcpad to the byte format
     * on the sinkpad and forward the event */
    res = gst_dvdemux_convert_src_to_sink (dvdemux, pad,
        format, cur, stop, GST_FORMAT_BYTES, &start_position, &end_position);
    if (!res)
      goto done;

    /* now this is the updated seek event on bytes */
    newevent = gst_event_new_seek (rate, GST_FORMAT_BYTES, flags,
        cur_type, start_position, stop_type, end_position);
    gst_event_set_seqnum (newevent, gst_event_get_seqnum (event));

    res = gst_pad_push_event (dvdemux->sinkpad, newevent);
  }
done:
  return res;
}

static void
gst_dvdemux_update_frame_offsets (GstDVDemux * dvdemux, GstClockTime timestamp)
{
  /* calculate current frame number */
  gst_dvdemux_src_convert (dvdemux, dvdemux->videosrcpad,
      dvdemux->time_segment.format, timestamp,
      GST_FORMAT_DEFAULT, &dvdemux->video_offset);

  /* calculate current audio number */
  gst_dvdemux_src_convert (dvdemux, dvdemux->audiosrcpad,
      dvdemux->time_segment.format, timestamp,
      GST_FORMAT_DEFAULT, &dvdemux->audio_offset);

  /* every DV frame corresponts with one video frame */
  dvdemux->frame_offset = dvdemux->video_offset;
}

/* position ourselves to the configured segment, used in pull mode.
 * The input segment is in TIME format. We convert the time values
 * to bytes values into our byte_segment which we use to pull data from
 * the sinkpad peer.
 */
static gboolean
gst_dvdemux_do_seek (GstDVDemux * demux, GstSegment * segment)
{
  gboolean res;
  GstFormat format;

  /* position to value configured is last_stop, this will round down
   * to the byte position where the frame containing the given 
   * timestamp can be found. */
  format = GST_FORMAT_BYTES;
  res = gst_dvdemux_sink_convert (demux,
      segment->format, segment->position,
      format, (gint64 *) & demux->byte_segment.position);
  if (!res)
    goto done;

  /* update byte segment start */
  gst_dvdemux_sink_convert (demux,
      segment->format, segment->start, format,
      (gint64 *) & demux->byte_segment.start);

  /* update byte segment stop */
  gst_dvdemux_sink_convert (demux,
      segment->format, segment->stop, format,
      (gint64 *) & demux->byte_segment.stop);

  /* update byte segment time */
  gst_dvdemux_sink_convert (demux,
      segment->format, segment->time, format,
      (gint64 *) & demux->byte_segment.time);

  gst_dvdemux_update_frame_offsets (demux, segment->start);

  demux->discont = TRUE;

done:
  return res;
}

/* handle seek in pull base mode.
 *
 * Does not take ownership of the event.
 */
static gboolean
gst_dvdemux_handle_pull_seek (GstDVDemux * demux, GstPad * pad,
    GstEvent * event)
{
  gboolean res;
  gdouble rate;
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType cur_type, stop_type;
  gint64 cur, stop;
  gboolean flush;
  gboolean update;
  GstSegment seeksegment;
  GstEvent *new_event;

  GST_DEBUG_OBJECT (demux, "doing seek");

  /* first bring the event format to TIME, our native format
   * to perform the seek on */
  if (event) {
    GstFormat conv;

    gst_event_parse_seek (event, &rate, &format, &flags,
        &cur_type, &cur, &stop_type, &stop);

    /* can't seek backwards yet */
    if (rate <= 0.0)
      goto wrong_rate;

    /* convert input format to TIME */
    conv = GST_FORMAT_TIME;
    if (!(gst_dvdemux_convert_src_pair (demux, pad,
                format, cur, stop, conv, &cur, &stop)))
      goto no_format;

    format = GST_FORMAT_TIME;
  } else {
    flags = 0;
  }

  demux->segment_seqnum = gst_event_get_seqnum (event);

  flush = flags & GST_SEEK_FLAG_FLUSH;

  /* send flush start */
  if (flush) {
    new_event = gst_event_new_flush_start ();
    gst_event_set_seqnum (new_event, demux->segment_seqnum);
    gst_dvdemux_push_event (demux, new_event);
  } else {
    gst_pad_pause_task (demux->sinkpad);
  }

  /* grab streaming lock, this should eventually be possible, either
   * because the task is paused or our streaming thread stopped
   * because our peer is flushing. */
  GST_PAD_STREAM_LOCK (demux->sinkpad);

  /* make copy into temp structure, we can only update the main one
   * when the subclass actually could to the seek. */
  memcpy (&seeksegment, &demux->time_segment, sizeof (GstSegment));

  /* now configure the seek segment */
  if (event) {
    gst_segment_do_seek (&seeksegment, rate, format, flags,
        cur_type, cur, stop_type, stop, &update);
  }

  GST_DEBUG_OBJECT (demux, "segment configured from %" G_GINT64_FORMAT
      " to %" G_GINT64_FORMAT ", position %" G_GINT64_FORMAT,
      seeksegment.start, seeksegment.stop, seeksegment.position);

  /* do the seek, segment.position contains new position. */
  res = gst_dvdemux_do_seek (demux, &seeksegment);

  /* and prepare to continue streaming */
  if (flush) {
    /* send flush stop, peer will accept data and events again. We
     * are not yet providing data as we still have the STREAM_LOCK. */
    new_event = gst_event_new_flush_stop (TRUE);
    gst_event_set_seqnum (new_event, demux->segment_seqnum);
    gst_dvdemux_push_event (demux, new_event);
  }

  /* if successful seek, we update our real segment and push
   * out the new segment. */
  if (res) {
    memcpy (&demux->time_segment, &seeksegment, sizeof (GstSegment));

    if (demux->time_segment.flags & GST_SEEK_FLAG_SEGMENT) {
      GstMessage *message;

      message = gst_message_new_segment_start (GST_OBJECT_CAST (demux),
          demux->time_segment.format, demux->time_segment.position);
      gst_message_set_seqnum (message, demux->segment_seqnum);
      gst_element_post_message (GST_ELEMENT_CAST (demux), message);
    }
    if ((stop = demux->time_segment.stop) == -1)
      stop = demux->time_segment.duration;

    GST_INFO_OBJECT (demux,
        "Saving newsegment event to be sent in streaming thread");

    if (demux->pending_segment)
      gst_event_unref (demux->pending_segment);

    demux->pending_segment = gst_event_new_segment (&demux->time_segment);
    gst_event_set_seqnum (demux->pending_segment, demux->segment_seqnum);

    demux->need_segment = FALSE;
  }

  /* and restart the task in case it got paused explicitly or by
   * the FLUSH_START event we pushed out. */
  gst_pad_start_task (demux->sinkpad, (GstTaskFunction) gst_dvdemux_loop,
      demux->sinkpad, NULL);

  /* and release the lock again so we can continue streaming */
  GST_PAD_STREAM_UNLOCK (demux->sinkpad);

  return TRUE;

  /* ERRORS */
wrong_rate:
  {
    GST_DEBUG_OBJECT (demux, "negative playback rate %lf not supported.", rate);
    return FALSE;
  }
no_format:
  {
    GST_DEBUG_OBJECT (demux, "cannot convert to TIME format, seek aborted.");
    return FALSE;
  }
}

static gboolean
gst_dvdemux_send_event (GstElement * element, GstEvent * event)
{
  GstDVDemux *dvdemux = GST_DVDEMUX (element);
  gboolean res = FALSE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
    {
      /* checking header and configuring the seek must be atomic */
      GST_OBJECT_LOCK (dvdemux);
      if (g_atomic_int_get (&dvdemux->found_header) == 0) {
        GstEvent **event_p;

        event_p = &dvdemux->seek_event;

        /* We don't have pads yet. Keep the event. */
        GST_INFO_OBJECT (dvdemux, "Keeping the seek event for later");

        gst_event_replace (event_p, event);
        GST_OBJECT_UNLOCK (dvdemux);

        res = TRUE;
      } else {
        GST_OBJECT_UNLOCK (dvdemux);

        if (dvdemux->seek_handler)
          res = dvdemux->seek_handler (dvdemux, dvdemux->videosrcpad, event);
        gst_event_unref (event);
      }
      break;
    }
    default:
      res = GST_ELEMENT_CLASS (parent_class)->send_event (element, event);
      break;
  }

  return res;
}

/* handle an event on the source pad, it's most likely a seek */
static gboolean
gst_dvdemux_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res = FALSE;
  GstDVDemux *dvdemux;

  dvdemux = GST_DVDEMUX (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      /* seek handler is installed based on scheduling mode */
      if (dvdemux->seek_handler)
        res = dvdemux->seek_handler (dvdemux, pad, event);
      gst_event_unref (event);
      break;
    default:
      res = gst_pad_push_event (dvdemux->sinkpad, event);
      break;
  }

  return res;
}

/* does not take ownership of buffer */
static GstFlowReturn
gst_dvdemux_demux_audio (GstDVDemux * dvdemux, GstBuffer * buffer,
    guint64 duration)
{
  gint num_samples;
  GstFlowReturn ret;
  GstMapInfo map;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  dv_decode_full_audio (dvdemux->decoder, map.data, dvdemux->audio_buffers);
  gst_buffer_unmap (buffer, &map);

  if (G_LIKELY ((num_samples = dv_get_num_samples (dvdemux->decoder)) > 0)) {
    gint16 *a_ptr;
    gint i, j;
    GstBuffer *outbuf;
    gint frequency, channels;

    /* get initial format or check if format changed */
    frequency = dv_get_frequency (dvdemux->decoder);
    channels = dv_get_num_channels (dvdemux->decoder);

    if (G_UNLIKELY ((dvdemux->audiosrcpad == NULL)
            || (frequency != dvdemux->frequency)
            || (channels != dvdemux->channels))) {
      GstCaps *caps;
      GstAudioInfo info;

      dvdemux->frequency = frequency;
      dvdemux->channels = channels;

      gst_audio_info_init (&info);
      gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16LE,
          frequency, channels, NULL);
      caps = gst_audio_info_to_caps (&info);
      if (G_UNLIKELY (dvdemux->audiosrcpad == NULL)) {
        dvdemux->audiosrcpad =
            gst_dvdemux_add_pad (dvdemux, &audio_src_temp, caps);

        if (dvdemux->videosrcpad && dvdemux->audiosrcpad)
          gst_element_no_more_pads (GST_ELEMENT (dvdemux));

      } else {
        gst_pad_set_caps (dvdemux->audiosrcpad, caps);
      }
      gst_caps_unref (caps);
    }

    outbuf = gst_buffer_new_and_alloc (num_samples *
        sizeof (gint16) * dvdemux->channels);

    gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
    a_ptr = (gint16 *) map.data;

    for (i = 0; i < num_samples; i++) {
      for (j = 0; j < dvdemux->channels; j++) {
        *(a_ptr++) = dvdemux->audio_buffers[j][i];
      }
    }
    gst_buffer_unmap (outbuf, &map);

    GST_DEBUG ("pushing audio %" GST_TIME_FORMAT,
        GST_TIME_ARGS (dvdemux->time_segment.position));

    GST_BUFFER_TIMESTAMP (outbuf) = dvdemux->time_segment.position;
    GST_BUFFER_DURATION (outbuf) = duration;
    GST_BUFFER_OFFSET (outbuf) = dvdemux->audio_offset;
    dvdemux->audio_offset += num_samples;
    GST_BUFFER_OFFSET_END (outbuf) = dvdemux->audio_offset;

    if (dvdemux->new_media || dvdemux->discont)
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
    ret = gst_pad_push (dvdemux->audiosrcpad, outbuf);
  } else {
    /* no samples */
    ret = GST_FLOW_OK;
  }

  return ret;
}

/* takes ownership of buffer */
static GstFlowReturn
gst_dvdemux_demux_video (GstDVDemux * dvdemux, GstBuffer * buffer,
    guint64 duration)
{
  GstBuffer *outbuf;
  gint height;
  gboolean wide;
  GstFlowReturn ret = GST_FLOW_OK;

  /* get params */
  /* framerate is already up-to-date */
  height = dvdemux->decoder->height;
  wide = dv_format_wide (dvdemux->decoder);

  /* see if anything changed */
  if (G_UNLIKELY ((dvdemux->videosrcpad == NULL) || (dvdemux->height != height)
          || dvdemux->wide != wide)) {
    gint par_x, par_y;
    GstCaps *caps;

    dvdemux->height = height;
    dvdemux->wide = wide;

    if (dvdemux->decoder->system == e_dv_system_625_50) {
      if (wide) {
        par_x = PAL_WIDE_PAR_X;
        par_y = PAL_WIDE_PAR_Y;
      } else {
        par_x = PAL_NORMAL_PAR_X;
        par_y = PAL_NORMAL_PAR_Y;
      }
    } else {
      if (wide) {
        par_x = NTSC_WIDE_PAR_X;
        par_y = NTSC_WIDE_PAR_Y;
      } else {
        par_x = NTSC_NORMAL_PAR_X;
        par_y = NTSC_NORMAL_PAR_Y;
      }
    }

    caps = gst_caps_new_simple ("video/x-dv",
        "systemstream", G_TYPE_BOOLEAN, FALSE,
        "width", G_TYPE_INT, 720,
        "height", G_TYPE_INT, height,
        "framerate", GST_TYPE_FRACTION, dvdemux->framerate_numerator,
        dvdemux->framerate_denominator,
        "pixel-aspect-ratio", GST_TYPE_FRACTION, par_x, par_y, NULL);

    if (G_UNLIKELY (dvdemux->videosrcpad == NULL)) {
      dvdemux->videosrcpad =
          gst_dvdemux_add_pad (dvdemux, &video_src_temp, caps);

      if (dvdemux->videosrcpad && dvdemux->audiosrcpad)
        gst_element_no_more_pads (GST_ELEMENT (dvdemux));

    } else {
      gst_pad_set_caps (dvdemux->videosrcpad, caps);
    }
    gst_caps_unref (caps);
  }

  /* takes ownership of buffer here, we just need to modify
   * the metadata. */
  outbuf = gst_buffer_make_writable (buffer);

  GST_BUFFER_TIMESTAMP (outbuf) = dvdemux->time_segment.position;
  GST_BUFFER_OFFSET (outbuf) = dvdemux->video_offset;
  GST_BUFFER_OFFSET_END (outbuf) = dvdemux->video_offset + 1;
  GST_BUFFER_DURATION (outbuf) = duration;

  if (dvdemux->new_media || dvdemux->discont)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);

  GST_DEBUG ("pushing video %" GST_TIME_FORMAT,
      GST_TIME_ARGS (dvdemux->time_segment.position));

  ret = gst_pad_push (dvdemux->videosrcpad, outbuf);

  dvdemux->video_offset++;

  return ret;
}

static int
get_ssyb_offset (int dif, int ssyb)
{
  int offset;

  offset = dif * 12000;         /* to dif */
  offset += 80 * (1 + (ssyb / 6));      /* to subcode pack */
  offset += 3;                  /* past header */
  offset += 8 * (ssyb % 6);     /* to ssyb */

  return offset;
}

static gboolean
gst_dvdemux_get_timecode (GstDVDemux * dvdemux, GstBuffer * buffer,
    GstSMPTETimeCode * timecode)
{
  guint8 *data;
  GstMapInfo map;
  int offset;
  int dif;
  int n_difs = dvdemux->decoder->num_dif_seqs;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  data = map.data;
  for (dif = 0; dif < n_difs; dif++) {
    offset = get_ssyb_offset (dif, 3);
    if (data[offset + 3] == 0x13) {
      timecode->frames = ((data[offset + 4] >> 4) & 0x3) * 10 +
          (data[offset + 4] & 0xf);
      timecode->seconds = ((data[offset + 5] >> 4) & 0x3) * 10 +
          (data[offset + 5] & 0xf);
      timecode->minutes = ((data[offset + 6] >> 4) & 0x3) * 10 +
          (data[offset + 6] & 0xf);
      timecode->hours = ((data[offset + 7] >> 4) & 0x3) * 10 +
          (data[offset + 7] & 0xf);
      GST_DEBUG ("got timecode %" GST_SMPTE_TIME_CODE_FORMAT,
          GST_SMPTE_TIME_CODE_ARGS (timecode));
      gst_buffer_unmap (buffer, &map);
      return TRUE;
    }
  }

  gst_buffer_unmap (buffer, &map);
  return FALSE;
}

static gboolean
gst_dvdemux_is_new_media (GstDVDemux * dvdemux, GstBuffer * buffer)
{
  guint8 *data;
  GstMapInfo map;
  int aaux_offset;
  int dif;
  int n_difs;

  n_difs = dvdemux->decoder->num_dif_seqs;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  data = map.data;
  for (dif = 0; dif < n_difs; dif++) {
    if (dif & 1) {
      aaux_offset = (dif * 12000) + (6 + 16 * 1) * 80 + 3;
    } else {
      aaux_offset = (dif * 12000) + (6 + 16 * 4) * 80 + 3;
    }
    if (data[aaux_offset + 0] == 0x51) {
      if ((data[aaux_offset + 2] & 0x80) == 0) {
        gst_buffer_unmap (buffer, &map);
        return TRUE;
      }
    }
  }

  gst_buffer_unmap (buffer, &map);
  return FALSE;
}

/* takes ownership of buffer */
static GstFlowReturn
gst_dvdemux_demux_frame (GstDVDemux * dvdemux, GstBuffer * buffer)
{
  GstClockTime next_ts;
  GstFlowReturn aret, vret, ret;
  GstMapInfo map;
  guint64 duration;
  GstSMPTETimeCode timecode;
  int frame_number;

  if (dvdemux->need_segment) {
    GstFormat format;
    GstEvent *event;

    g_assert (!dvdemux->upstream_time_segment);

    /* convert to time and store as start/end_timestamp */
    format = GST_FORMAT_TIME;
    if (!(gst_dvdemux_convert_sink_pair (dvdemux,
                GST_FORMAT_BYTES, dvdemux->byte_segment.start,
                dvdemux->byte_segment.stop, format,
                (gint64 *) & dvdemux->time_segment.start,
                (gint64 *) & dvdemux->time_segment.stop)))
      goto segment_error;

    dvdemux->time_segment.time = dvdemux->time_segment.start;
    dvdemux->time_segment.rate = dvdemux->byte_segment.rate;

    gst_dvdemux_sink_convert (dvdemux,
        GST_FORMAT_BYTES, dvdemux->byte_segment.position,
        GST_FORMAT_TIME, (gint64 *) & dvdemux->time_segment.position);

    gst_dvdemux_update_frame_offsets (dvdemux, dvdemux->time_segment.position);

    GST_DEBUG_OBJECT (dvdemux, "sending segment start: %" GST_TIME_FORMAT
        ", stop: %" GST_TIME_FORMAT ", time: %" GST_TIME_FORMAT,
        GST_TIME_ARGS (dvdemux->time_segment.start),
        GST_TIME_ARGS (dvdemux->time_segment.stop),
        GST_TIME_ARGS (dvdemux->time_segment.start));

    event = gst_event_new_segment (&dvdemux->time_segment);
    if (dvdemux->segment_seqnum)
      gst_event_set_seqnum (event, dvdemux->segment_seqnum);
    gst_dvdemux_push_event (dvdemux, event);

    dvdemux->need_segment = FALSE;
  }

  gst_dvdemux_get_timecode (dvdemux, buffer, &timecode);
  gst_smpte_time_code_get_frame_number (
      (dvdemux->decoder->system == e_dv_system_625_50) ?
      GST_SMPTE_TIME_CODE_SYSTEM_25 : GST_SMPTE_TIME_CODE_SYSTEM_30,
      &frame_number, &timecode);

  if (dvdemux->time_segment.rate < 0) {
    next_ts = gst_util_uint64_scale_int (
        (dvdemux->frame_offset >
            0 ? dvdemux->frame_offset - 1 : 0) * GST_SECOND,
        dvdemux->framerate_denominator, dvdemux->framerate_numerator);
    duration = dvdemux->time_segment.position - next_ts;
  } else {
    next_ts = gst_util_uint64_scale_int (
        (dvdemux->frame_offset + 1) * GST_SECOND,
        dvdemux->framerate_denominator, dvdemux->framerate_numerator);
    duration = next_ts - dvdemux->time_segment.position;
  }

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  dv_parse_packs (dvdemux->decoder, map.data);
  gst_buffer_unmap (buffer, &map);
  dvdemux->new_media = FALSE;
  if (gst_dvdemux_is_new_media (dvdemux, buffer) &&
      dvdemux->frames_since_new_media > 2) {
    dvdemux->new_media = TRUE;
    dvdemux->frames_since_new_media = 0;
  }
  dvdemux->frames_since_new_media++;

  /* does not take ownership of buffer */
  aret = ret = gst_dvdemux_demux_audio (dvdemux, buffer, duration);
  if (G_UNLIKELY (ret != GST_FLOW_OK && ret != GST_FLOW_NOT_LINKED)) {
    gst_buffer_unref (buffer);
    goto done;
  }

  /* takes ownership of buffer */
  vret = ret = gst_dvdemux_demux_video (dvdemux, buffer, duration);
  if (G_UNLIKELY (ret != GST_FLOW_OK && ret != GST_FLOW_NOT_LINKED))
    goto done;

  /* if both are not linked, we stop */
  if (G_UNLIKELY (aret == GST_FLOW_NOT_LINKED && vret == GST_FLOW_NOT_LINKED)) {
    ret = GST_FLOW_NOT_LINKED;
    goto done;
  }

  dvdemux->discont = FALSE;
  dvdemux->time_segment.position = next_ts;

  if (dvdemux->time_segment.rate < 0) {
    if (dvdemux->frame_offset > 0)
      dvdemux->frame_offset--;
    else
      GST_WARNING_OBJECT (dvdemux,
          "Got before frame offset 0 in reverse playback");
  } else {
    dvdemux->frame_offset++;
  }

  /* check for the end of the segment */
  if ((dvdemux->time_segment.rate > 0 && dvdemux->time_segment.stop != -1
          && next_ts > dvdemux->time_segment.stop)
      || (dvdemux->time_segment.rate < 0
          && dvdemux->time_segment.start > next_ts))
    ret = GST_FLOW_EOS;
  else
    ret = GST_FLOW_OK;

done:
  return ret;

  /* ERRORS */
segment_error:
  {
    GST_DEBUG ("error generating new_segment event");
    gst_buffer_unref (buffer);
    return GST_FLOW_ERROR;
  }
}

/* flush any remaining data in the adapter, used in chain based scheduling mode */
static GstFlowReturn
gst_dvdemux_flush (GstDVDemux * dvdemux)
{
  GstFlowReturn ret = GST_FLOW_OK;

  while (gst_adapter_available (dvdemux->adapter) >= dvdemux->frame_len) {
    const guint8 *data;
    gint length;

    /* get the accumulated bytes */
    data = gst_adapter_map (dvdemux->adapter, dvdemux->frame_len);

    /* parse header to know the length and other params */
    if (G_UNLIKELY (dv_parse_header (dvdemux->decoder, data) < 0)) {
      gst_adapter_unmap (dvdemux->adapter);
      goto parse_header_error;
    }
    gst_adapter_unmap (dvdemux->adapter);

    /* after parsing the header we know the length of the data */
    length = dvdemux->frame_len = dvdemux->decoder->frame_size;
    if (dvdemux->decoder->system == e_dv_system_625_50) {
      dvdemux->framerate_numerator = PAL_FRAMERATE_NUMERATOR;
      dvdemux->framerate_denominator = PAL_FRAMERATE_DENOMINATOR;
    } else {
      dvdemux->framerate_numerator = NTSC_FRAMERATE_NUMERATOR;
      dvdemux->framerate_denominator = NTSC_FRAMERATE_DENOMINATOR;
    }
    g_atomic_int_set (&dvdemux->found_header, 1);

    /* let demux_video set the height, it needs to detect when things change so
     * it can reset caps */

    /* if we still have enough for a frame, start decoding */
    if (G_LIKELY (gst_adapter_available (dvdemux->adapter) >= length)) {
      GstBuffer *buffer;

      buffer = gst_adapter_take_buffer (dvdemux->adapter, length);

      /* and decode the buffer, takes ownership */
      ret = gst_dvdemux_demux_frame (dvdemux, buffer);
      if (G_UNLIKELY (ret != GST_FLOW_OK))
        goto done;
    }
  }
done:
  return ret;

  /* ERRORS */
parse_header_error:
  {
    GST_ELEMENT_ERROR (dvdemux, STREAM, DECODE,
        (NULL), ("Error parsing DV header"));
    return GST_FLOW_ERROR;
  }
}

/* streaming operation: 
 *
 * accumulate data until we have a frame, then decode. 
 */
static GstFlowReturn
gst_dvdemux_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstDVDemux *dvdemux;
  GstFlowReturn ret;
  GstClockTime timestamp;

  dvdemux = GST_DVDEMUX (parent);

  /* a discontinuity in the stream, we need to get rid of
   * accumulated data in the adapter and assume a new frame
   * starts after the discontinuity */
  if (G_UNLIKELY (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT))) {
    gst_adapter_clear (dvdemux->adapter);
    dvdemux->discont = TRUE;

    /* Should recheck where we are */
    if (!dvdemux->upstream_time_segment)
      dvdemux->need_segment = TRUE;
  }

  /* a timestamp always should be respected */
  timestamp = GST_BUFFER_TIMESTAMP (buffer);
  if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
    dvdemux->time_segment.position = timestamp;

    if (dvdemux->discont)
      gst_dvdemux_update_frame_offsets (dvdemux,
          dvdemux->time_segment.position);
  } else if (dvdemux->upstream_time_segment && dvdemux->discont) {
    /* This will probably fail later to provide correct
     * timestamps and/or durations but also should not happen */
    GST_ERROR_OBJECT (dvdemux,
        "Upstream provides TIME segment but no PTS after discont");
  }

  gst_adapter_push (dvdemux->adapter, buffer);

  /* Apparently dv_parse_header can read from the body of the frame
   * too, so it needs more than header_size bytes. Wacky!
   */
  if (G_UNLIKELY (dvdemux->frame_len == -1)) {
    /* if we don't know the length of a frame, we assume it is
     * the NTSC_BUFFER length, as this is enough to figure out 
     * if this is PAL or NTSC */
    dvdemux->frame_len = NTSC_BUFFER;
  }

  /* and try to flush pending frames */
  ret = gst_dvdemux_flush (dvdemux);

  return ret;
}

/* pull based operation.
 *
 * Read header first to figure out the frame size. Then read
 * and decode full frames.
 */
static void
gst_dvdemux_loop (GstPad * pad)
{
  GstFlowReturn ret;
  GstDVDemux *dvdemux;
  GstBuffer *buffer = NULL;
  GstMapInfo map;

  dvdemux = GST_DVDEMUX (gst_pad_get_parent (pad));

  if (G_UNLIKELY (g_atomic_int_get (&dvdemux->found_header) == 0)) {
    GST_DEBUG_OBJECT (dvdemux, "pulling first buffer");
    /* pull in NTSC sized buffer to figure out the frame
     * length */
    ret = gst_pad_pull_range (dvdemux->sinkpad,
        dvdemux->byte_segment.position, NTSC_BUFFER, &buffer);
    if (G_UNLIKELY (ret != GST_FLOW_OK))
      goto pause;

    /* check buffer size, don't want to read small buffers */
    if (G_UNLIKELY (gst_buffer_get_size (buffer) < NTSC_BUFFER))
      goto small_buffer;

    gst_buffer_map (buffer, &map, GST_MAP_READ);
    /* parse header to know the length and other params */
    if (G_UNLIKELY (dv_parse_header (dvdemux->decoder, map.data) < 0)) {
      gst_buffer_unmap (buffer, &map);
      goto parse_header_error;
    }
    gst_buffer_unmap (buffer, &map);

    /* after parsing the header we know the length of the data */
    dvdemux->frame_len = dvdemux->decoder->frame_size;
    if (dvdemux->decoder->system == e_dv_system_625_50) {
      dvdemux->framerate_numerator = PAL_FRAMERATE_NUMERATOR;
      dvdemux->framerate_denominator = PAL_FRAMERATE_DENOMINATOR;
    } else {
      dvdemux->framerate_numerator = NTSC_FRAMERATE_NUMERATOR;
      dvdemux->framerate_denominator = NTSC_FRAMERATE_DENOMINATOR;
    }
    dvdemux->need_segment = TRUE;

    /* see if we need to read a larger part */
    if (dvdemux->frame_len != NTSC_BUFFER) {
      gst_buffer_unref (buffer);
      buffer = NULL;
    }

    {
      GstEvent *event;

      /* setting header and prrforming the seek must be atomic */
      GST_OBJECT_LOCK (dvdemux);
      /* got header now */
      g_atomic_int_set (&dvdemux->found_header, 1);

      /* now perform pending seek if any. */
      event = dvdemux->seek_event;
      if (event)
        gst_event_ref (event);
      GST_OBJECT_UNLOCK (dvdemux);

      if (event) {
        if (!gst_dvdemux_handle_pull_seek (dvdemux, dvdemux->videosrcpad,
                event)) {
          GST_ELEMENT_WARNING (dvdemux, STREAM, DECODE, (NULL),
              ("Error performing initial seek"));
        }
        gst_event_unref (event);

        /* and we need to pull a new buffer in all cases. */
        if (buffer) {
          gst_buffer_unref (buffer);
          buffer = NULL;
        }
      }
    }
  }

  if (G_UNLIKELY (dvdemux->pending_segment)) {

    /* now send the newsegment */
    GST_DEBUG_OBJECT (dvdemux, "Sending newsegment from");

    gst_dvdemux_push_event (dvdemux, dvdemux->pending_segment);
    dvdemux->pending_segment = NULL;
  }

  if (G_LIKELY (buffer == NULL)) {
    GST_DEBUG_OBJECT (dvdemux, "pulling buffer at offset %" G_GINT64_FORMAT,
        dvdemux->byte_segment.position);

    ret = gst_pad_pull_range (dvdemux->sinkpad,
        dvdemux->byte_segment.position, dvdemux->frame_len, &buffer);
    if (ret != GST_FLOW_OK)
      goto pause;

    /* check buffer size, don't want to read small buffers */
    if (gst_buffer_get_size (buffer) < dvdemux->frame_len)
      goto small_buffer;
  }
  /* and decode the buffer */
  ret = gst_dvdemux_demux_frame (dvdemux, buffer);
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto pause;

  /* and position ourselves for the next buffer */
  dvdemux->byte_segment.position += dvdemux->frame_len;

done:
  gst_object_unref (dvdemux);

  return;

  /* ERRORS */
parse_header_error:
  {
    GstEvent *event;

    GST_ELEMENT_ERROR (dvdemux, STREAM, DECODE,
        (NULL), ("Error parsing DV header"));
    gst_buffer_unref (buffer);
    gst_pad_pause_task (dvdemux->sinkpad);
    event = gst_event_new_eos ();
    if (dvdemux->segment_seqnum)
      gst_event_set_seqnum (event, dvdemux->segment_seqnum);
    gst_dvdemux_push_event (dvdemux, event);
    goto done;
  }
small_buffer:
  {
    GstEvent *event;

    GST_ELEMENT_ERROR (dvdemux, STREAM, DECODE,
        (NULL), ("Error reading buffer"));
    gst_buffer_unref (buffer);
    gst_pad_pause_task (dvdemux->sinkpad);
    event = gst_event_new_eos ();
    if (dvdemux->segment_seqnum)
      gst_event_set_seqnum (event, dvdemux->segment_seqnum);
    gst_dvdemux_push_event (dvdemux, event);
    goto done;
  }
pause:
  {
    GST_INFO_OBJECT (dvdemux, "pausing task, %s", gst_flow_get_name (ret));
    gst_pad_pause_task (dvdemux->sinkpad);
    if (ret == GST_FLOW_EOS) {
      GST_LOG_OBJECT (dvdemux, "got eos");
      /* so align our position with the end of it, if there is one
       * this ensures a subsequent will arrive at correct base/acc time */
      if (dvdemux->time_segment.rate > 0.0 &&
          GST_CLOCK_TIME_IS_VALID (dvdemux->time_segment.stop))
        dvdemux->time_segment.position = dvdemux->time_segment.stop;
      else if (dvdemux->time_segment.rate < 0.0)
        dvdemux->time_segment.position = dvdemux->time_segment.start;
      /* perform EOS logic */
      if (dvdemux->time_segment.flags & GST_SEEK_FLAG_SEGMENT) {
        GstMessage *message;
        GstEvent *event;

        event = gst_event_new_segment_done (dvdemux->time_segment.format,
            dvdemux->time_segment.position);
        if (dvdemux->segment_seqnum)
          gst_event_set_seqnum (event, dvdemux->segment_seqnum);

        message = gst_message_new_segment_done (GST_OBJECT_CAST (dvdemux),
            dvdemux->time_segment.format, dvdemux->time_segment.position);
        if (dvdemux->segment_seqnum)
          gst_message_set_seqnum (message, dvdemux->segment_seqnum);

        gst_element_post_message (GST_ELEMENT (dvdemux), message);
        gst_dvdemux_push_event (dvdemux, event);
      } else {
        GstEvent *event = gst_event_new_eos ();
        if (dvdemux->segment_seqnum)
          gst_event_set_seqnum (event, dvdemux->segment_seqnum);
        gst_dvdemux_push_event (dvdemux, event);
      }
    } else if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_EOS) {
      GstEvent *event = gst_event_new_eos ();
      /* for fatal errors or not-linked we post an error message */
      GST_ELEMENT_FLOW_ERROR (dvdemux, ret);
      if (dvdemux->segment_seqnum)
        gst_event_set_seqnum (event, dvdemux->segment_seqnum);
      gst_dvdemux_push_event (dvdemux, event);
    }
    goto done;
  }
}

static gboolean
gst_dvdemux_sink_activate_mode (GstPad * sinkpad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  gboolean res;
  GstDVDemux *demux = GST_DVDEMUX (parent);

  switch (mode) {
    case GST_PAD_MODE_PULL:
      if (active) {
        demux->seek_handler = gst_dvdemux_handle_pull_seek;
        res = gst_pad_start_task (sinkpad,
            (GstTaskFunction) gst_dvdemux_loop, sinkpad, NULL);
      } else {
        demux->seek_handler = NULL;
        res = gst_pad_stop_task (sinkpad);
      }
      break;
    case GST_PAD_MODE_PUSH:
      if (active) {
        GST_DEBUG_OBJECT (demux, "activating push/chain function");
        demux->seek_handler = gst_dvdemux_handle_push_seek;
      } else {
        GST_DEBUG_OBJECT (demux, "deactivating push/chain function");
        demux->seek_handler = NULL;
      }
      res = TRUE;
      break;
    default:
      res = FALSE;
      break;
  }
  return res;
}

/* decide on push or pull based scheduling */
static gboolean
gst_dvdemux_sink_activate (GstPad * sinkpad, GstObject * parent)
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

static GstStateChangeReturn
gst_dvdemux_change_state (GstElement * element, GstStateChange transition)
{
  GstDVDemux *dvdemux = GST_DVDEMUX (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      dvdemux->decoder = dv_decoder_new (0, FALSE, FALSE);
      dv_set_error_log (dvdemux->decoder, NULL);
      gst_dvdemux_reset (dvdemux);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_adapter_clear (dvdemux->adapter);
      dv_decoder_free (dvdemux->decoder);
      dvdemux->decoder = NULL;

      gst_dvdemux_remove_pads (dvdemux);

      if (dvdemux->tag_event) {
        gst_event_unref (dvdemux->tag_event);
        dvdemux->tag_event = NULL;
      }

      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
    {
      GstEvent **event_p;

      event_p = &dvdemux->seek_event;
      gst_event_replace (event_p, NULL);
      if (dvdemux->pending_segment)
        gst_event_unref (dvdemux->pending_segment);
      dvdemux->pending_segment = NULL;
      break;
    }
    default:
      break;
  }
  return ret;
}
