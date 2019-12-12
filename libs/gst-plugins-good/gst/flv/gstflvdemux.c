/* GStreamer
 * Copyright (C) <2007> Julien Moutte <julien@moutte.net>
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
 * SECTION:element-flvdemux
 * @title: flvdemux
 *
 * flvdemux demuxes an FLV file into the different contained streams.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v filesrc location=/path/to/flv ! flvdemux ! audioconvert ! autoaudiosink
 * ]| This pipeline demuxes an FLV file and outputs the contained raw audio streams.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstflvdemux.h"
#include "gstflvmux.h"

#include <string.h>
#include <stdio.h>
#include <gst/base/gstbytereader.h>
#include <gst/base/gstbytewriter.h>
#include <gst/pbutils/descriptions.h>
#include <gst/pbutils/pbutils.h>
#include <gst/audio/audio.h>
#include <gst/video/video.h>
#include <gst/tag/tag.h>

/* FIXME: don't rely on own GstIndex */
#include "gstindex.c"
#include "gstmemindex.c"
#define GST_ASSOCIATION_FLAG_NONE GST_INDEX_ASSOCIATION_FLAG_NONE
#define GST_ASSOCIATION_FLAG_KEY_UNIT GST_INDEX_ASSOCIATION_FLAG_KEY_UNIT
#define GST_ASSOCIATION_FLAG_DELTA_UNIT GST_INDEX_ASSOCIATION_FLAG_DELTA_UNIT

static GstStaticPadTemplate flv_sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-flv")
    );

static GstStaticPadTemplate audio_src_template =
    GST_STATIC_PAD_TEMPLATE ("audio",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS
    ("audio/x-adpcm, layout = (string) swf, channels = (int) { 1, 2 }, rate = (int) { 5512, 11025, 22050, 44100 }; "
        "audio/mpeg, mpegversion = (int) 1, layer = (int) 3, channels = (int) { 1, 2 }, rate = (int) { 5512, 8000, 11025, 22050, 44100 }, parsed = (boolean) TRUE; "
        "audio/mpeg, mpegversion = (int) 4, stream-format = (string) raw, framed = (boolean) TRUE; "
        "audio/x-nellymoser, channels = (int) { 1, 2 }, rate = (int) { 5512, 8000, 11025, 16000, 22050, 44100 }; "
        "audio/x-raw, format = (string) { U8, S16LE }, layout = (string) interleaved, channels = (int) { 1, 2 }, rate = (int) { 5512, 11025, 22050, 44100 }; "
        "audio/x-alaw, channels = (int) { 1, 2 }, rate = (int) 8000; "
        "audio/x-mulaw, channels = (int) { 1, 2 }, rate = (int) 8000; "
        "audio/x-speex, channels = (int) 1, rate = (int) 16000;")
    );

static GstStaticPadTemplate video_src_template =
    GST_STATIC_PAD_TEMPLATE ("video",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("video/x-flash-video, flvversion=(int) 1; "
        "video/x-flash-screen; "
        "video/x-vp6-flash; " "video/x-vp6-alpha; "
        "video/x-h264, stream-format=avc;")
    );

GST_DEBUG_CATEGORY_STATIC (flvdemux_debug);
#define GST_CAT_DEFAULT flvdemux_debug

#define gst_flv_demux_parent_class parent_class
G_DEFINE_TYPE (GstFlvDemux, gst_flv_demux, GST_TYPE_ELEMENT);

/* 9 bytes of header + 4 bytes of first previous tag size */
#define FLV_HEADER_SIZE 13
/* 1 byte of tag type + 3 bytes of tag data size */
#define FLV_TAG_TYPE_SIZE 4

/* two seconds - consider dts are resynced to another base if this different */
#define RESYNC_THRESHOLD 2000

/* how much stream time to wait for audio tags to appear after we have video, or vice versa */
#define NO_MORE_PADS_THRESHOLD (6 * GST_SECOND)

static gboolean flv_demux_handle_seek_push (GstFlvDemux * demux,
    GstEvent * event);
static gboolean gst_flv_demux_handle_seek_pull (GstFlvDemux * demux,
    GstEvent * event, gboolean seeking);

static gboolean gst_flv_demux_query (GstPad * pad, GstObject * parent,
    GstQuery * query);
static gboolean gst_flv_demux_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

static GstIndex *gst_flv_demux_get_index (GstElement * element);

static void gst_flv_demux_push_tags (GstFlvDemux * demux);

static void
gst_flv_demux_parse_and_add_index_entry (GstFlvDemux * demux, GstClockTime ts,
    guint64 pos, gboolean keyframe)
{
  GstIndexAssociation associations[2];
  GstIndex *index;
  GstIndexEntry *entry;

  GST_LOG_OBJECT (demux,
      "adding key=%d association %" GST_TIME_FORMAT "-> %" G_GUINT64_FORMAT,
      keyframe, GST_TIME_ARGS (ts), pos);

  /* if upstream is not seekable there is no point in building an index */
  if (!demux->upstream_seekable)
    return;

  index = gst_flv_demux_get_index (GST_ELEMENT (demux));

  if (!index)
    return;

  /* entry may already have been added before, avoid adding indefinitely */
  entry = gst_index_get_assoc_entry (index, demux->index_id,
      GST_INDEX_LOOKUP_EXACT, GST_ASSOCIATION_FLAG_NONE, GST_FORMAT_BYTES, pos);

  if (entry) {
#ifndef GST_DISABLE_GST_DEBUG
    gint64 time = 0;
    gboolean key;

    gst_index_entry_assoc_map (entry, GST_FORMAT_TIME, &time);
    key = ! !(GST_INDEX_ASSOC_FLAGS (entry) & GST_ASSOCIATION_FLAG_KEY_UNIT);
    GST_LOG_OBJECT (demux, "position already mapped to time %" GST_TIME_FORMAT
        ", keyframe %d", GST_TIME_ARGS (time), key);
    /* there is not really a way to delete the existing one */
    if (time != ts || key != ! !keyframe)
      GST_DEBUG_OBJECT (demux, "metadata mismatch");
#endif
    gst_object_unref (index);
    return;
  }

  associations[0].format = GST_FORMAT_TIME;
  associations[0].value = ts;
  associations[1].format = GST_FORMAT_BYTES;
  associations[1].value = pos;

  gst_index_add_associationv (index, demux->index_id,
      (keyframe) ? GST_ASSOCIATION_FLAG_KEY_UNIT :
      GST_ASSOCIATION_FLAG_DELTA_UNIT, 2,
      (const GstIndexAssociation *) &associations);

  if (pos > demux->index_max_pos)
    demux->index_max_pos = pos;
  if (ts > demux->index_max_time)
    demux->index_max_time = ts;

  gst_object_unref (index);
}

static gchar *
FLV_GET_STRING (GstByteReader * reader)
{
  guint16 string_size = 0;
  gchar *string = NULL;
  const guint8 *str = NULL;

  g_return_val_if_fail (reader != NULL, NULL);

  if (G_UNLIKELY (!gst_byte_reader_get_uint16_be (reader, &string_size)))
    return NULL;

  if (G_UNLIKELY (string_size > gst_byte_reader_get_remaining (reader)))
    return NULL;

  string = g_try_malloc0 (string_size + 1);
  if (G_UNLIKELY (!string)) {
    return NULL;
  }

  if (G_UNLIKELY (!gst_byte_reader_get_data (reader, string_size, &str))) {
    g_free (string);
    return NULL;
  }

  memcpy (string, str, string_size);
  if (!g_utf8_validate (string, string_size, NULL)) {
    g_free (string);
    return NULL;
  }

  return string;
}

static void
gst_flv_demux_check_seekability (GstFlvDemux * demux)
{
  GstQuery *query;
  gint64 start = -1, stop = -1;

  demux->upstream_seekable = FALSE;

  query = gst_query_new_seeking (GST_FORMAT_BYTES);
  if (!gst_pad_peer_query (demux->sinkpad, query)) {
    GST_DEBUG_OBJECT (demux, "seeking query failed");
    gst_query_unref (query);
    return;
  }

  gst_query_parse_seeking (query, NULL, &demux->upstream_seekable,
      &start, &stop);

  gst_query_unref (query);

  /* try harder to query upstream size if we didn't get it the first time */
  if (demux->upstream_seekable && stop == -1) {
    GST_DEBUG_OBJECT (demux, "doing duration query to fix up unset stop");
    gst_pad_peer_query_duration (demux->sinkpad, GST_FORMAT_BYTES, &stop);
  }

  /* if upstream doesn't know the size, it's likely that it's not seekable in
   * practice even if it technically may be seekable */
  if (demux->upstream_seekable && (start != 0 || stop <= start)) {
    GST_DEBUG_OBJECT (demux, "seekable but unknown start/stop -> disable");
    demux->upstream_seekable = FALSE;
  }

  GST_DEBUG_OBJECT (demux, "upstream seekable: %d", demux->upstream_seekable);
}

static GstDateTime *
parse_flv_demux_parse_date_string (const gchar * s)
{
  static const gchar months[12][4] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
  };
  GstDateTime *dt = NULL;
  gchar **tokens;
  guint64 d;
  gchar *endptr, *stripped;
  gint i, hh, mm, ss;
  gint year = -1, month = -1, day = -1;
  gint hour = -1, minute = -1, seconds = -1;

  stripped = g_strstrip (g_strdup (s));

  /* "Fri Oct 15 15:13:16 2004" needs to be parsed */
  tokens = g_strsplit (stripped, " ", -1);

  g_free (stripped);

  if (g_strv_length (tokens) != 5)
    goto out;

  /* year */
  d = g_ascii_strtoull (tokens[4], &endptr, 10);
  if (d == 0 && *endptr != '\0')
    goto out;

  year = d;

  /* month */
  if (strlen (tokens[1]) != 3)
    goto out;
  for (i = 0; i < 12; i++) {
    if (!strcmp (tokens[1], months[i])) {
      break;
    }
  }
  if (i == 12)
    goto out;

  month = i + 1;

  /* day */
  d = g_ascii_strtoull (tokens[2], &endptr, 10);
  if (d == 0 && *endptr != '\0')
    goto out;

  day = d;

  /* time */
  hh = mm = ss = 0;
  if (sscanf (tokens[3], "%d:%d:%d", &hh, &mm, &ss) < 2)
    goto out;
  if (hh >= 0 && hh < 24 && mm >= 0 && mm < 60 && ss >= 0 && ss < 60) {
    hour = hh;
    minute = mm;
    seconds = ss;
  }

out:

  if (tokens)
    g_strfreev (tokens);

  if (year > 0)
    dt = gst_date_time_new (0.0, year, month, day, hour, minute, seconds);

  return dt;
}

static gboolean
gst_flv_demux_parse_metadata_item (GstFlvDemux * demux, GstByteReader * reader,
    gboolean * end_marker)
{
  gchar *tag_name = NULL;
  guint8 tag_type = 0;

  /* Initialize the end_marker flag to FALSE */
  *end_marker = FALSE;

  /* Name of the tag */
  tag_name = FLV_GET_STRING (reader);
  if (G_UNLIKELY (!tag_name)) {
    GST_WARNING_OBJECT (demux, "failed reading tag name");
    return FALSE;
  }

  /* What kind of object is that */
  if (!gst_byte_reader_get_uint8 (reader, &tag_type))
    goto error;

  GST_DEBUG_OBJECT (demux, "tag name %s, tag type %d", tag_name, tag_type);

  switch (tag_type) {
    case 0:                    /* Double */
    {                           /* Use a union to read the uint64 and then as a double */
      gdouble d = 0;

      if (!gst_byte_reader_get_float64_be (reader, &d))
        goto error;

      GST_DEBUG_OBJECT (demux, "%s => (double) %f", tag_name, d);

      if (!strcmp (tag_name, "duration")) {
        demux->duration = d * GST_SECOND;

        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            GST_TAG_DURATION, demux->duration, NULL);
      } else if (!strcmp (tag_name, "AspectRatioX")) {
        demux->par_x = d;
        demux->got_par = TRUE;
      } else if (!strcmp (tag_name, "AspectRatioY")) {
        demux->par_y = d;
        demux->got_par = TRUE;
      } else if (!strcmp (tag_name, "width")) {
        demux->w = d;
      } else if (!strcmp (tag_name, "height")) {
        demux->h = d;
      } else if (!strcmp (tag_name, "framerate")) {
        demux->framerate = d;
      } else if (!strcmp (tag_name, "audiodatarate")) {
        gst_tag_list_add (demux->audio_tags, GST_TAG_MERGE_REPLACE,
            GST_TAG_NOMINAL_BITRATE, (guint) (d * 1024), NULL);
      } else if (!strcmp (tag_name, "videodatarate")) {
        gst_tag_list_add (demux->video_tags, GST_TAG_MERGE_REPLACE,
            GST_TAG_NOMINAL_BITRATE, (guint) (d * 1024), NULL);
      } else {
        GST_INFO_OBJECT (demux, "Tag \'%s\' not handled", tag_name);
      }

      break;
    }
    case 1:                    /* Boolean */
    {
      guint8 b = 0;

      if (!gst_byte_reader_get_uint8 (reader, &b))
        goto error;

      GST_DEBUG_OBJECT (demux, "%s => (boolean) %d", tag_name, b);

      GST_INFO_OBJECT (demux, "Tag \'%s\' not handled", tag_name);

      break;
    }
    case 2:                    /* String */
    {
      gchar *s = NULL;

      s = FLV_GET_STRING (reader);
      if (s == NULL)
        goto error;

      GST_DEBUG_OBJECT (demux, "%s => (string) %s", tag_name, s);

      if (!strcmp (tag_name, "creationdate")) {
        GstDateTime *dt;

        dt = parse_flv_demux_parse_date_string (s);
        if (dt == NULL) {
          GST_DEBUG_OBJECT (demux, "Failed to parse '%s' into datetime", s);
        } else {
          gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
              GST_TAG_DATE_TIME, dt, NULL);
          gst_date_time_unref (dt);
        }
      } else if (!strcmp (tag_name, "creator")) {
        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            GST_TAG_ARTIST, s, NULL);
      } else if (!strcmp (tag_name, "title")) {
        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            GST_TAG_TITLE, s, NULL);
      } else if (!strcmp (tag_name, "metadatacreator")
          || !strcmp (tag_name, "encoder")) {
        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            GST_TAG_ENCODER, s, NULL);
      } else {
        GST_INFO_OBJECT (demux, "Tag \'%s\' not handled", tag_name);
      }

      g_free (s);

      break;
    }
    case 3:                    /* Object */
    {
      gboolean end_of_object_marker = FALSE;

      while (!end_of_object_marker) {
        gboolean ok = gst_flv_demux_parse_metadata_item (demux, reader,
            &end_of_object_marker);

        if (G_UNLIKELY (!ok)) {
          GST_WARNING_OBJECT (demux, "failed reading a tag, skipping");
          goto error;
        }
      }

      break;
    }
    case 8:                    /* ECMA array */
    {
      guint32 nb_elems = 0;
      gboolean end_of_object_marker = FALSE;

      if (!gst_byte_reader_get_uint32_be (reader, &nb_elems))
        goto error;

      GST_DEBUG_OBJECT (demux, "there are approx. %d elements in the array",
          nb_elems);

      while (!end_of_object_marker) {
        gboolean ok = gst_flv_demux_parse_metadata_item (demux, reader,
            &end_of_object_marker);

        if (G_UNLIKELY (!ok)) {
          GST_WARNING_OBJECT (demux, "failed reading a tag, skipping");
          goto error;
        }
      }

      break;
    }
    case 9:                    /* End marker */
    {
      GST_DEBUG_OBJECT (demux, "end marker ?");
      if (tag_name[0] == '\0') {

        GST_DEBUG_OBJECT (demux, "end marker detected");

        *end_marker = TRUE;
      }

      break;
    }
    case 10:                   /* Array */
    {
      guint32 nb_elems = 0;

      if (!gst_byte_reader_get_uint32_be (reader, &nb_elems))
        goto error;

      GST_DEBUG_OBJECT (demux, "array has %d elements", nb_elems);

      if (!strcmp (tag_name, "times")) {
        if (demux->times) {
          g_array_free (demux->times, TRUE);
        }
        demux->times = g_array_new (FALSE, TRUE, sizeof (gdouble));
      } else if (!strcmp (tag_name, "filepositions")) {
        if (demux->filepositions) {
          g_array_free (demux->filepositions, TRUE);
        }
        demux->filepositions = g_array_new (FALSE, TRUE, sizeof (gdouble));
      }

      while (nb_elems--) {
        guint8 elem_type = 0;

        if (!gst_byte_reader_get_uint8 (reader, &elem_type))
          goto error;

        switch (elem_type) {
          case 0:
          {
            gdouble d;

            if (!gst_byte_reader_get_float64_be (reader, &d))
              goto error;

            GST_DEBUG_OBJECT (demux, "element is a double %f", d);

            if (!strcmp (tag_name, "times") && demux->times) {
              g_array_append_val (demux->times, d);
            } else if (!strcmp (tag_name, "filepositions") &&
                demux->filepositions) {
              g_array_append_val (demux->filepositions, d);
            }
            break;
          }
          default:
            GST_WARNING_OBJECT (demux, "unsupported array element type %d",
                elem_type);
        }
      }

      break;
    }
    case 11:                   /* Date */
    {
      gdouble d = 0;
      gint16 i = 0;

      if (!gst_byte_reader_get_float64_be (reader, &d))
        goto error;

      if (!gst_byte_reader_get_int16_be (reader, &i))
        goto error;

      GST_DEBUG_OBJECT (demux,
          "%s => (date as a double) %f, timezone offset %d", tag_name, d, i);

      GST_INFO_OBJECT (demux, "Tag \'%s\' not handled", tag_name);

      break;
    }
    default:
      GST_WARNING_OBJECT (demux, "unsupported tag type %d", tag_type);
  }

  g_free (tag_name);

  return TRUE;

error:
  g_free (tag_name);

  return FALSE;
}

static void
gst_flv_demux_clear_tags (GstFlvDemux * demux)
{
  GST_DEBUG_OBJECT (demux, "clearing taglist");

  if (demux->taglist) {
    gst_tag_list_unref (demux->taglist);
  }
  demux->taglist = gst_tag_list_new_empty ();
  gst_tag_list_set_scope (demux->taglist, GST_TAG_SCOPE_GLOBAL);

  if (demux->audio_tags) {
    gst_tag_list_unref (demux->audio_tags);
  }
  demux->audio_tags = gst_tag_list_new_empty ();

  if (demux->video_tags) {
    gst_tag_list_unref (demux->video_tags);
  }
  demux->video_tags = gst_tag_list_new_empty ();
}

static GstFlowReturn
gst_flv_demux_parse_tag_script (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstByteReader reader;
  guint8 type = 0;
  GstMapInfo map;

  g_return_val_if_fail (gst_buffer_get_size (buffer) >= 7, GST_FLOW_ERROR);

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  gst_byte_reader_init (&reader, map.data, map.size);

  gst_byte_reader_skip_unchecked (&reader, 7);

  GST_LOG_OBJECT (demux, "parsing a script tag");

  if (!gst_byte_reader_get_uint8 (&reader, &type))
    goto cleanup;

  /* Must be string */
  if (type == 2) {
    gchar *function_name;
    guint i;

    function_name = FLV_GET_STRING (&reader);

    GST_LOG_OBJECT (demux, "function name is %s", GST_STR_NULL (function_name));

    if (function_name != NULL && strcmp (function_name, "onMetaData") == 0) {
      gboolean end_marker = FALSE;
      GST_DEBUG_OBJECT (demux, "we have a metadata script object");

      gst_flv_demux_clear_tags (demux);

      if (!gst_byte_reader_get_uint8 (&reader, &type)) {
        g_free (function_name);
        goto cleanup;
      }

      switch (type) {
        case 8:
        {
          guint32 nb_elems = 0;

          /* ECMA array */
          if (!gst_byte_reader_get_uint32_be (&reader, &nb_elems)) {
            g_free (function_name);
            goto cleanup;
          }

          /* The number of elements is just a hint, some files have
             nb_elements == 0 and actually contain items. */
          GST_DEBUG_OBJECT (demux, "there are approx. %d elements in the array",
              nb_elems);
        }
          /* fallthrough to read data */
        case 3:
        {
          /* Object */
          while (!end_marker) {
            gboolean ok =
                gst_flv_demux_parse_metadata_item (demux, &reader, &end_marker);

            if (G_UNLIKELY (!ok)) {
              GST_WARNING_OBJECT (demux, "failed reading a tag, skipping");
              break;
            }
          }
        }
          break;
        default:
          GST_DEBUG_OBJECT (demux, "Unhandled script data type : %d", type);
          g_free (function_name);
          goto cleanup;
      }

      gst_flv_demux_push_tags (demux);
    }

    g_free (function_name);

    if (demux->times && demux->filepositions) {
      guint num;

      /* If an index was found, insert associations */
      num = MIN (demux->times->len, demux->filepositions->len);
      for (i = 0; i < num; i++) {
        guint64 time, fileposition;

        time = g_array_index (demux->times, gdouble, i) * GST_SECOND;
        fileposition = g_array_index (demux->filepositions, gdouble, i);
        gst_flv_demux_parse_and_add_index_entry (demux, time, fileposition,
            TRUE);
      }
      demux->indexed = TRUE;
    }
  }

cleanup:
  gst_buffer_unmap (buffer, &map);

  return ret;
}

static gboolean
have_group_id (GstFlvDemux * demux)
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

static gboolean
gst_flv_demux_audio_negotiate (GstFlvDemux * demux, guint32 codec_tag,
    guint32 rate, guint32 channels, guint32 width)
{
  GstCaps *caps = NULL, *old_caps;
  gboolean ret = FALSE;
  guint adjusted_rate = rate;
  guint adjusted_channels = channels;
  GstEvent *event;
  gchar *stream_id;

  switch (codec_tag) {
    case 1:
      caps = gst_caps_new_simple ("audio/x-adpcm", "layout", G_TYPE_STRING,
          "swf", NULL);
      break;
    case 2:
    case 14:
      caps = gst_caps_new_simple ("audio/mpeg",
          "mpegversion", G_TYPE_INT, 1, "layer", G_TYPE_INT, 3,
          "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
      break;
    case 0:
    case 3:
    {
      GstAudioFormat format;

      /* Assuming little endian for 0 (aka endianness of the
       * system on which the file was created) as most people
       * are probably using little endian machines */
      format = gst_audio_format_build_integer ((width == 8) ? FALSE : TRUE,
          G_LITTLE_ENDIAN, width, width);

      caps = gst_caps_new_simple ("audio/x-raw",
          "format", G_TYPE_STRING, gst_audio_format_to_string (format),
          "layout", G_TYPE_STRING, "interleaved", NULL);
      break;
    }
    case 4:
    case 5:
    case 6:
      caps = gst_caps_new_empty_simple ("audio/x-nellymoser");
      break;
    case 10:
    {
      GstMapInfo map;
      if (!demux->audio_codec_data) {
        GST_DEBUG_OBJECT (demux, "don't have AAC codec data yet");
        ret = TRUE;
        goto done;
      }

      gst_buffer_map (demux->audio_codec_data, &map, GST_MAP_READ);

      /* use codec-data to extract and verify samplerate */
      if (map.size >= 2) {
        gint freq_index;

        freq_index = GST_READ_UINT16_BE (map.data);
        freq_index = (freq_index & 0x0780) >> 7;
        adjusted_rate =
            gst_codec_utils_aac_get_sample_rate_from_index (freq_index);

        if (adjusted_rate && (rate != adjusted_rate)) {
          GST_LOG_OBJECT (demux, "Ajusting AAC sample rate %d -> %d", rate,
              adjusted_rate);
        } else {
          adjusted_rate = rate;
        }

        adjusted_channels =
            gst_codec_utils_aac_get_channels (map.data, map.size);

        if (adjusted_channels && (channels != adjusted_channels)) {
          GST_LOG_OBJECT (demux, "Ajusting AAC channels %d -> %d", channels,
              adjusted_channels);
        } else {
          adjusted_channels = channels;
        }
      }
      gst_buffer_unmap (demux->audio_codec_data, &map);

      caps = gst_caps_new_simple ("audio/mpeg",
          "mpegversion", G_TYPE_INT, 4, "framed", G_TYPE_BOOLEAN, TRUE,
          "stream-format", G_TYPE_STRING, "raw", NULL);
      break;
    }
    case 7:
      caps = gst_caps_new_empty_simple ("audio/x-alaw");
      break;
    case 8:
      caps = gst_caps_new_empty_simple ("audio/x-mulaw");
      break;
    case 11:
    {
      GValue streamheader = G_VALUE_INIT;
      GValue value = G_VALUE_INIT;
      GstByteWriter w;
      GstStructure *structure;
      GstBuffer *buf;
      GstTagList *tags;

      caps = gst_caps_new_empty_simple ("audio/x-speex");
      structure = gst_caps_get_structure (caps, 0);

      GST_DEBUG_OBJECT (demux, "generating speex header");

      /* Speex decoder expects streamheader to be { [header], [comment] } */
      g_value_init (&streamheader, GST_TYPE_ARRAY);

      /* header part */
      gst_byte_writer_init_with_size (&w, 80, TRUE);
      gst_byte_writer_put_data (&w, (guint8 *) "Speex   ", 8);
      gst_byte_writer_put_data (&w, (guint8 *) "1.1.12", 7);
      gst_byte_writer_fill (&w, 0, 13);
      gst_byte_writer_put_uint32_le (&w, 1);    /* version */
      gst_byte_writer_put_uint32_le (&w, 80);   /* header_size */
      gst_byte_writer_put_uint32_le (&w, 16000);        /* rate */
      gst_byte_writer_put_uint32_le (&w, 1);    /* mode: Wideband */
      gst_byte_writer_put_uint32_le (&w, 4);    /* mode_bitstream_version */
      gst_byte_writer_put_uint32_le (&w, 1);    /* nb_channels: 1 */
      gst_byte_writer_put_uint32_le (&w, -1);   /* bitrate */
      gst_byte_writer_put_uint32_le (&w, 0x50); /* frame_size */
      gst_byte_writer_put_uint32_le (&w, 0);    /* VBR */
      gst_byte_writer_put_uint32_le (&w, 1);    /* frames_per_packet */
      gst_byte_writer_put_uint32_le (&w, 0);    /* extra_headers */
      gst_byte_writer_put_uint32_le (&w, 0);    /* reserved1 */
      gst_byte_writer_put_uint32_le (&w, 0);    /* reserved2 */
      g_assert (gst_byte_writer_get_size (&w) == 80);

      g_value_init (&value, GST_TYPE_BUFFER);
      g_value_take_boxed (&value, gst_byte_writer_reset_and_get_buffer (&w));
      gst_value_array_append_value (&streamheader, &value);
      g_value_unset (&value);

      /* comment part */
      g_value_init (&value, GST_TYPE_BUFFER);
      tags = gst_tag_list_new_empty ();
      buf = gst_tag_list_to_vorbiscomment_buffer (tags, NULL, 0, "No comments");
      gst_tag_list_unref (tags);
      g_value_take_boxed (&value, buf);
      gst_value_array_append_value (&streamheader, &value);
      g_value_unset (&value);

      gst_structure_take_value (structure, "streamheader", &streamheader);

      channels = 1;
      adjusted_rate = 16000;
      break;
    }
    default:
      GST_WARNING_OBJECT (demux, "unsupported audio codec tag %u", codec_tag);
      break;
  }

  if (G_UNLIKELY (!caps)) {
    GST_WARNING_OBJECT (demux, "failed creating caps for audio pad");
    goto beach;
  }

  gst_caps_set_simple (caps, "rate", G_TYPE_INT, adjusted_rate,
      "channels", G_TYPE_INT, adjusted_channels, NULL);

  if (demux->audio_codec_data) {
    gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER,
        demux->audio_codec_data, NULL);
  }

  old_caps = gst_pad_get_current_caps (demux->audio_pad);
  if (!old_caps) {
    stream_id =
        gst_pad_create_stream_id (demux->audio_pad, GST_ELEMENT_CAST (demux),
        "audio");

    event = gst_event_new_stream_start (stream_id);
    if (have_group_id (demux))
      gst_event_set_group_id (event, demux->group_id);
    gst_pad_push_event (demux->audio_pad, event);
    g_free (stream_id);
  }
  if (!old_caps || !gst_caps_is_equal (old_caps, caps))
    ret = gst_pad_set_caps (demux->audio_pad, caps);
  else
    ret = TRUE;

  if (old_caps)
    gst_caps_unref (old_caps);

done:
  if (G_LIKELY (ret)) {
    /* Store the caps we got from tags */
    demux->audio_codec_tag = codec_tag;
    demux->rate = rate;
    demux->channels = channels;
    demux->width = width;

    if (caps) {
      GST_DEBUG_OBJECT (demux->audio_pad, "successfully negotiated caps %"
          GST_PTR_FORMAT, caps);

      gst_flv_demux_push_tags (demux);
    } else {
      GST_DEBUG_OBJECT (demux->audio_pad, "delayed setting caps");
    }
  } else {
    GST_WARNING_OBJECT (demux->audio_pad, "failed negotiating caps %"
        GST_PTR_FORMAT, caps);
  }

  if (caps)
    gst_caps_unref (caps);

beach:
  return ret;
}

static gboolean
gst_flv_demux_push_src_event (GstFlvDemux * demux, GstEvent * event)
{
  gboolean ret = TRUE;

  if (demux->audio_pad)
    ret |= gst_pad_push_event (demux->audio_pad, gst_event_ref (event));

  if (demux->video_pad)
    ret |= gst_pad_push_event (demux->video_pad, gst_event_ref (event));

  gst_event_unref (event);

  return ret;
}

static void
gst_flv_demux_add_codec_tag (GstFlvDemux * demux, const gchar * tag,
    GstPad * pad)
{
  if (pad) {
    GstCaps *caps = gst_pad_get_current_caps (pad);

    if (caps) {
      gchar *codec_name = gst_pb_utils_get_codec_description (caps);

      if (codec_name) {
        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            tag, codec_name, NULL);
        g_free (codec_name);
      }

      gst_caps_unref (caps);
    }
  }
}

static void
gst_flv_demux_push_tags (GstFlvDemux * demux)
{
  gst_flv_demux_add_codec_tag (demux, GST_TAG_AUDIO_CODEC, demux->audio_pad);
  gst_flv_demux_add_codec_tag (demux, GST_TAG_VIDEO_CODEC, demux->video_pad);

  GST_DEBUG_OBJECT (demux, "pushing %" GST_PTR_FORMAT, demux->taglist);

  gst_flv_demux_push_src_event (demux,
      gst_event_new_tag (gst_tag_list_copy (demux->taglist)));

  if (demux->audio_pad) {
    GST_DEBUG_OBJECT (demux->audio_pad, "pushing audio %" GST_PTR_FORMAT,
        demux->audio_tags);
    gst_pad_push_event (demux->audio_pad,
        gst_event_new_tag (gst_tag_list_copy (demux->audio_tags)));
  }

  if (demux->video_pad) {
    GST_DEBUG_OBJECT (demux->video_pad, "pushing video %" GST_PTR_FORMAT,
        demux->video_tags);
    gst_pad_push_event (demux->video_pad,
        gst_event_new_tag (gst_tag_list_copy (demux->video_tags)));
  }
}

static gboolean
gst_flv_demux_update_resync (GstFlvDemux * demux, guint32 dts, gboolean discont,
    guint32 * last, GstClockTime * offset)
{
  gboolean ret = FALSE;
  gint32 ddts = dts - *last;
  if (!discont && ddts <= -RESYNC_THRESHOLD) {
    /* Theoretically, we should use subtract the duration of the last buffer,
       but this demuxer sends no durations on buffers, not sure if it cannot
       know, or just does not care to calculate. */
    *offset -= ddts * GST_MSECOND;
    GST_WARNING_OBJECT (demux,
        "Large dts gap (%" G_GINT32_FORMAT " ms), assuming resync, offset now %"
        GST_TIME_FORMAT "", ddts, GST_TIME_ARGS (*offset));

    ret = TRUE;
  }
  *last = dts;

  return ret;
}

static GstFlowReturn
gst_flv_demux_parse_tag_audio (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 pts = 0, codec_tag = 0, rate = 5512, width = 8, channels = 1;
  guint32 codec_data = 0, pts_ext = 0;
  guint8 flags = 0;
  GstMapInfo map;
  GstBuffer *outbuf;
  guint8 *data;

  GST_LOG_OBJECT (demux, "parsing an audio tag");

  if (G_UNLIKELY (!demux->audio_pad && demux->no_more_pads)) {
#ifndef GST_DISABLE_DEBUG
    if (G_UNLIKELY (!demux->no_audio_warned)) {
      GST_WARNING_OBJECT (demux,
          "Signaled no-more-pads already but had no audio pad -- ignoring");
      demux->no_audio_warned = TRUE;
    }
#endif
    return GST_FLOW_OK;
  }

  g_return_val_if_fail (gst_buffer_get_size (buffer) == demux->tag_size,
      GST_FLOW_ERROR);

  /* Error out on tags with too small headers */
  if (gst_buffer_get_size (buffer) < 11) {
    GST_ERROR_OBJECT (demux, "Too small tag size (%" G_GSIZE_FORMAT ")",
        gst_buffer_get_size (buffer));
    return GST_FLOW_ERROR;
  }

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  data = map.data;

  /* Grab information about audio tag */
  pts = GST_READ_UINT24_BE (data);
  /* read the pts extension to 32 bits integer */
  pts_ext = GST_READ_UINT8 (data + 3);
  /* Combine them */
  pts |= pts_ext << 24;

  GST_LOG_OBJECT (demux, "pts bytes %02X %02X %02X %02X (%d)", data[0], data[1],
      data[2], data[3], pts);

  /* Skip the stream id and go directly to the flags */
  flags = GST_READ_UINT8 (data + 7);

  /* Silently skip buffers with no data */
  if (map.size == 11)
    goto beach;

  /* Channels */
  if (flags & 0x01) {
    channels = 2;
  }
  /* Width */
  if (flags & 0x02) {
    width = 16;
  }
  /* Sampling rate */
  if ((flags & 0x0C) == 0x0C) {
    rate = 44100;
  } else if ((flags & 0x0C) == 0x08) {
    rate = 22050;
  } else if ((flags & 0x0C) == 0x04) {
    rate = 11025;
  }
  /* Codec tag */
  codec_tag = flags >> 4;
  if (codec_tag == 10) {        /* AAC has an extra byte for packet type */
    codec_data = 2;
  } else {
    codec_data = 1;
  }

  /* codec tags with special rates */
  if (codec_tag == 5 || codec_tag == 14 || codec_tag == 7 || codec_tag == 8)
    rate = 8000;
  else if ((codec_tag == 4) || (codec_tag == 11))
    rate = 16000;

  GST_LOG_OBJECT (demux, "audio tag with %d channels, %dHz sampling rate, "
      "%d bits width, codec tag %u (flags %02X)", channels, rate, width,
      codec_tag, flags);

  if (codec_tag == 10) {
    guint8 aac_packet_type = GST_READ_UINT8 (data + 8);

    switch (aac_packet_type) {
      case 0:
      {
        /* AudioSpecificConfig data */
        GST_LOG_OBJECT (demux, "got an AAC codec data packet");
        if (demux->audio_codec_data) {
          gst_buffer_unref (demux->audio_codec_data);
        }
        demux->audio_codec_data =
            gst_buffer_copy_region (buffer, GST_BUFFER_COPY_MEMORY,
            7 + codec_data, demux->tag_data_size - codec_data);

        /* Use that buffer data in the caps */
        if (demux->audio_pad)
          gst_flv_demux_audio_negotiate (demux, codec_tag, rate, channels,
              width);
        goto beach;
      }
      case 1:
        if (!demux->audio_codec_data) {
          GST_ERROR_OBJECT (demux, "got AAC audio packet before codec data");
          ret = GST_FLOW_OK;
          goto beach;
        }
        /* AAC raw packet */
        GST_LOG_OBJECT (demux, "got a raw AAC audio packet");
        break;
      default:
        GST_WARNING_OBJECT (demux, "invalid AAC packet type %u",
            aac_packet_type);
    }
  }

  /* If we don't have our audio pad created, then create it. */
  if (G_UNLIKELY (!demux->audio_pad)) {
    demux->audio_pad =
        gst_pad_new_from_template (gst_element_class_get_pad_template
        (GST_ELEMENT_GET_CLASS (demux), "audio"), "audio");
    if (G_UNLIKELY (!demux->audio_pad)) {
      GST_WARNING_OBJECT (demux, "failed creating audio pad");
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    /* Set functions on the pad */
    gst_pad_set_query_function (demux->audio_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_query));
    gst_pad_set_event_function (demux->audio_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_src_event));

    gst_pad_use_fixed_caps (demux->audio_pad);

    /* Make it active */
    gst_pad_set_active (demux->audio_pad, TRUE);

    /* Negotiate caps */
    if (!gst_flv_demux_audio_negotiate (demux, codec_tag, rate, channels,
            width)) {
      gst_object_unref (demux->audio_pad);
      demux->audio_pad = NULL;
      ret = GST_FLOW_ERROR;
      goto beach;
    }
#ifndef GST_DISABLE_GST_DEBUG
    {
      GstCaps *caps;

      caps = gst_pad_get_current_caps (demux->audio_pad);
      GST_DEBUG_OBJECT (demux, "created audio pad with caps %" GST_PTR_FORMAT,
          caps);
      if (caps)
        gst_caps_unref (caps);
    }
#endif

    /* We need to set caps before adding */
    gst_element_add_pad (GST_ELEMENT (demux),
        gst_object_ref (demux->audio_pad));
    gst_flow_combiner_add_pad (demux->flowcombiner, demux->audio_pad);

    /* We only emit no more pads when we have audio and video. Indeed we can
     * not trust the FLV header to tell us if there will be only audio or
     * only video and we would just break discovery of some files */
    if (demux->audio_pad && demux->video_pad) {
      GST_DEBUG_OBJECT (demux, "emitting no more pads");
      gst_element_no_more_pads (GST_ELEMENT (demux));
      demux->no_more_pads = TRUE;
    }
  }

  /* Check if caps have changed */
  if (G_UNLIKELY (rate != demux->rate || channels != demux->channels ||
          codec_tag != demux->audio_codec_tag || width != demux->width)) {
    GST_DEBUG_OBJECT (demux, "audio settings have changed, changing caps");

    gst_buffer_replace (&demux->audio_codec_data, NULL);

    /* Negotiate caps */
    if (!gst_flv_demux_audio_negotiate (demux, codec_tag, rate, channels,
            width)) {
      ret = GST_FLOW_ERROR;
      goto beach;
    }
  }

  /* Check if we have anything to push */
  if (demux->tag_data_size <= codec_data) {
    GST_LOG_OBJECT (demux, "Nothing left in this tag, returning");
    goto beach;
  }

  /* Create buffer from pad */
  outbuf = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_MEMORY,
      7 + codec_data, demux->tag_data_size - codec_data);

  /* detect (and deem to be resyncs)  large pts gaps */
  if (gst_flv_demux_update_resync (demux, pts, demux->audio_need_discont,
          &demux->last_audio_pts, &demux->audio_time_offset)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
  }

  /* Fill buffer with data */
  GST_BUFFER_PTS (outbuf) = pts * GST_MSECOND + demux->audio_time_offset;
  GST_BUFFER_DTS (outbuf) = GST_BUFFER_PTS (outbuf);
  GST_BUFFER_DURATION (outbuf) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_OFFSET (outbuf) = demux->audio_offset++;
  GST_BUFFER_OFFSET_END (outbuf) = demux->audio_offset;

  if (demux->duration == GST_CLOCK_TIME_NONE ||
      demux->duration < GST_BUFFER_TIMESTAMP (outbuf))
    demux->duration = GST_BUFFER_TIMESTAMP (outbuf);

  /* Only add audio frames to the index if we have no video,
   * and if the index is not yet complete */
  if (!demux->has_video && !demux->indexed) {
    gst_flv_demux_parse_and_add_index_entry (demux,
        GST_BUFFER_TIMESTAMP (outbuf), demux->cur_tag_offset, TRUE);
  }

  if (G_UNLIKELY (demux->audio_need_discont)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
    demux->audio_need_discont = FALSE;
  }

  demux->segment.position = GST_BUFFER_TIMESTAMP (outbuf);

  /* Do we need a newsegment event ? */
  if (G_UNLIKELY (demux->audio_need_segment)) {
    if (!demux->new_seg_event) {
      GST_DEBUG_OBJECT (demux, "pushing newsegment from %"
          GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->segment.position),
          GST_TIME_ARGS (demux->segment.stop));
      demux->segment.start = demux->segment.time = demux->segment.position;
      demux->new_seg_event = gst_event_new_segment (&demux->segment);
    } else {
      GST_DEBUG_OBJECT (demux, "pushing pre-generated newsegment event");
    }

    gst_pad_push_event (demux->audio_pad, gst_event_ref (demux->new_seg_event));

    demux->audio_need_segment = FALSE;
  }

  GST_LOG_OBJECT (demux,
      "pushing %" G_GSIZE_FORMAT " bytes buffer at pts %" GST_TIME_FORMAT
      " with duration %" GST_TIME_FORMAT ", offset %" G_GUINT64_FORMAT,
      gst_buffer_get_size (outbuf),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)), GST_BUFFER_OFFSET (outbuf));

  if (!GST_CLOCK_TIME_IS_VALID (demux->audio_start)) {
    demux->audio_start = GST_BUFFER_TIMESTAMP (outbuf);
  }
  if (!GST_CLOCK_TIME_IS_VALID (demux->audio_first_ts)) {
    demux->audio_first_ts = GST_BUFFER_TIMESTAMP (outbuf);
  }

  if (G_UNLIKELY (!demux->no_more_pads
          && (GST_CLOCK_DIFF (demux->audio_start,
                  GST_BUFFER_TIMESTAMP (outbuf)) > NO_MORE_PADS_THRESHOLD))) {
    GST_DEBUG_OBJECT (demux,
        "Signalling no-more-pads because no video stream was found"
        " after 6 seconds of audio");
    gst_element_no_more_pads (GST_ELEMENT_CAST (demux));
    demux->no_more_pads = TRUE;
  }

  /* Push downstream */
  ret = gst_pad_push (demux->audio_pad, outbuf);

  if (G_UNLIKELY (ret != GST_FLOW_OK) &&
      demux->segment.rate < 0.0 && ret == GST_FLOW_EOS &&
      demux->segment.position > demux->segment.stop) {
    /* In reverse playback we can get a GST_FLOW_EOS when
     * we are at the end of the segment, so we just need to jump
     * back to the previous section. */
    GST_DEBUG_OBJECT (demux, "downstream has reached end of segment");
    demux->audio_done = TRUE;
    ret = GST_FLOW_OK;
    goto beach;
  }

  ret = gst_flow_combiner_update_pad_flow (demux->flowcombiner,
      demux->audio_pad, ret);

beach:
  gst_buffer_unmap (buffer, &map);

  return ret;
}

static gboolean
gst_flv_demux_video_negotiate (GstFlvDemux * demux, guint32 codec_tag)
{
  gboolean ret = FALSE;
  GstCaps *caps = NULL, *old_caps;
  GstEvent *event;
  gchar *stream_id;

  /* Generate caps for that pad */
  switch (codec_tag) {
    case 2:
      caps =
          gst_caps_new_simple ("video/x-flash-video", "flvversion", G_TYPE_INT,
          1, NULL);
      break;
    case 3:
      caps = gst_caps_new_empty_simple ("video/x-flash-screen");
      break;
    case 4:
      caps = gst_caps_new_empty_simple ("video/x-vp6-flash");
      break;
    case 5:
      caps = gst_caps_new_empty_simple ("video/x-vp6-alpha");
      break;
    case 7:
      if (!demux->video_codec_data) {
        GST_DEBUG_OBJECT (demux, "don't have h264 codec data yet");
        ret = TRUE;
        goto done;
      }
      caps =
          gst_caps_new_simple ("video/x-h264", "stream-format", G_TYPE_STRING,
          "avc", NULL);
      break;
      /* The following two are non-standard but apparently used, see in ffmpeg
       * https://git.videolan.org/?p=ffmpeg.git;a=blob;f=libavformat/flvdec.c;h=2bf1e059e1cbeeb79e4af9542da23f4560e1cf59;hb=b18d6c58000beed872d6bb1fe7d0fbe75ae26aef#l254
       * https://git.videolan.org/?p=ffmpeg.git;a=blob;f=libavformat/flvdec.c;h=2bf1e059e1cbeeb79e4af9542da23f4560e1cf59;hb=b18d6c58000beed872d6bb1fe7d0fbe75ae26aef#l282
       */
    case 8:
      caps = gst_caps_new_empty_simple ("video/x-h263");
      break;
    case 9:
      caps =
          gst_caps_new_simple ("video/mpeg", "mpegversion", G_TYPE_INT, 4,
          "systemstream", G_TYPE_BOOLEAN, FALSE, NULL);
      break;
    default:
      GST_WARNING_OBJECT (demux, "unsupported video codec tag %u", codec_tag);
  }

  if (G_UNLIKELY (!caps)) {
    GST_WARNING_OBJECT (demux, "failed creating caps for video pad");
    goto beach;
  }

  if (demux->got_par) {
    gst_caps_set_simple (caps, "pixel-aspect-ratio", GST_TYPE_FRACTION,
        demux->par_x, demux->par_y, NULL);
  }

  if (G_LIKELY (demux->w)) {
    gst_caps_set_simple (caps, "width", G_TYPE_INT, demux->w, NULL);
  }

  if (G_LIKELY (demux->h)) {
    gst_caps_set_simple (caps, "height", G_TYPE_INT, demux->h, NULL);
  }

  if (G_LIKELY (demux->framerate)) {
    gint num = 0, den = 0;

    gst_video_guess_framerate (GST_SECOND / demux->framerate, &num, &den);
    GST_DEBUG_OBJECT (demux->video_pad,
        "fps to be used on caps %f (as a fraction = %d/%d)", demux->framerate,
        num, den);

    gst_caps_set_simple (caps, "framerate", GST_TYPE_FRACTION, num, den, NULL);
  }

  if (demux->video_codec_data) {
    gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER,
        demux->video_codec_data, NULL);
  }

  old_caps = gst_pad_get_current_caps (demux->video_pad);
  if (!old_caps) {
    stream_id =
        gst_pad_create_stream_id (demux->video_pad, GST_ELEMENT_CAST (demux),
        "video");
    event = gst_event_new_stream_start (stream_id);
    g_free (stream_id);

    if (have_group_id (demux))
      gst_event_set_group_id (event, demux->group_id);
    gst_pad_push_event (demux->video_pad, event);
  }

  if (!old_caps || !gst_caps_is_equal (old_caps, caps))
    ret = gst_pad_set_caps (demux->video_pad, caps);
  else
    ret = TRUE;

  if (old_caps)
    gst_caps_unref (old_caps);

done:
  if (G_LIKELY (ret)) {
    /* Store the caps we have set */
    demux->video_codec_tag = codec_tag;

    if (caps) {
      GST_DEBUG_OBJECT (demux->video_pad, "successfully negotiated caps %"
          GST_PTR_FORMAT, caps);

      gst_flv_demux_push_tags (demux);
    } else {
      GST_DEBUG_OBJECT (demux->video_pad, "delayed setting caps");
    }
  } else {
    GST_WARNING_OBJECT (demux->video_pad, "failed negotiating caps %"
        GST_PTR_FORMAT, caps);
  }

  if (caps)
    gst_caps_unref (caps);

beach:
  return ret;
}

static GstFlowReturn
gst_flv_demux_parse_tag_video (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 dts = 0, codec_data = 1, dts_ext = 0;
  gint32 cts = 0;
  gboolean keyframe = FALSE;
  guint8 flags = 0, codec_tag = 0;
  GstBuffer *outbuf;
  GstMapInfo map;
  guint8 *data;

  g_return_val_if_fail (gst_buffer_get_size (buffer) == demux->tag_size,
      GST_FLOW_ERROR);

  GST_LOG_OBJECT (demux, "parsing a video tag");

  if G_UNLIKELY
    (!demux->video_pad && demux->no_more_pads) {
#ifndef GST_DISABLE_DEBUG
    if G_UNLIKELY
      (!demux->no_video_warned) {
      GST_WARNING_OBJECT (demux,
          "Signaled no-more-pads already but had no video pad -- ignoring");
      demux->no_video_warned = TRUE;
      }
#endif
    return GST_FLOW_OK;
    }

  if (gst_buffer_get_size (buffer) < 12) {
    GST_ERROR_OBJECT (demux, "Too small tag size");
    return GST_FLOW_ERROR;
  }

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  data = map.data;

  /* Grab information about video tag */
  dts = GST_READ_UINT24_BE (data);
  /* read the dts extension to 32 bits integer */
  dts_ext = GST_READ_UINT8 (data + 3);
  /* Combine them */
  dts |= dts_ext << 24;

  GST_LOG_OBJECT (demux, "dts bytes %02X %02X %02X %02X (%d)", data[0], data[1],
      data[2], data[3], dts);

  /* Skip the stream id and go directly to the flags */
  flags = GST_READ_UINT8 (data + 7);

  /* Keyframe */
  if ((flags >> 4) == 1) {
    keyframe = TRUE;
  }
  /* Codec tag */
  codec_tag = flags & 0x0F;
  if (codec_tag == 4 || codec_tag == 5) {
    codec_data = 2;
  } else if (codec_tag == 7) {
    codec_data = 5;

    cts = GST_READ_UINT24_BE (data + 9);
    cts = (cts + 0xff800000) ^ 0xff800000;

    if (cts < 0 && ABS (cts) > dts) {
      GST_ERROR_OBJECT (demux, "Detected a negative composition time offset "
          "'%d' that would lead to negative PTS, fixing", cts);
      cts += ABS (cts) - dts;
    }

    GST_LOG_OBJECT (demux, "got cts %d", cts);
  }

  GST_LOG_OBJECT (demux, "video tag with codec tag %u, keyframe (%d) "
      "(flags %02X)", codec_tag, keyframe, flags);

  if (codec_tag == 7) {
    guint8 avc_packet_type = GST_READ_UINT8 (data + 8);

    switch (avc_packet_type) {
      case 0:
      {
        if (demux->tag_data_size < codec_data) {
          GST_ERROR_OBJECT (demux, "Got invalid H.264 codec, ignoring.");
          break;
        }

        /* AVCDecoderConfigurationRecord data */
        GST_LOG_OBJECT (demux, "got an H.264 codec data packet");
        if (demux->video_codec_data) {
          gst_buffer_unref (demux->video_codec_data);
        }
        demux->video_codec_data = gst_buffer_copy_region (buffer,
            GST_BUFFER_COPY_MEMORY, 7 + codec_data,
            demux->tag_data_size - codec_data);;
        /* Use that buffer data in the caps */
        if (demux->video_pad)
          gst_flv_demux_video_negotiate (demux, codec_tag);
        goto beach;
      }
      case 1:
        /* H.264 NALU packet */
        if (!demux->video_codec_data) {
          GST_ERROR_OBJECT (demux, "got H.264 video packet before codec data");
          ret = GST_FLOW_OK;
          goto beach;
        }
        GST_LOG_OBJECT (demux, "got a H.264 NALU video packet");
        break;
      default:
        GST_WARNING_OBJECT (demux, "invalid video packet type %u",
            avc_packet_type);
    }
  }

  /* If we don't have our video pad created, then create it. */
  if (G_UNLIKELY (!demux->video_pad)) {
    demux->video_pad =
        gst_pad_new_from_template (gst_element_class_get_pad_template
        (GST_ELEMENT_GET_CLASS (demux), "video"), "video");
    if (G_UNLIKELY (!demux->video_pad)) {
      GST_WARNING_OBJECT (demux, "failed creating video pad");
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    /* Set functions on the pad */
    gst_pad_set_query_function (demux->video_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_query));
    gst_pad_set_event_function (demux->video_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_src_event));

    gst_pad_use_fixed_caps (demux->video_pad);

    /* Make it active */
    gst_pad_set_active (demux->video_pad, TRUE);

    /* Needs to be active before setting caps */
    if (!gst_flv_demux_video_negotiate (demux, codec_tag)) {
      gst_object_unref (demux->video_pad);
      demux->video_pad = NULL;
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    /* When we ve set pixel-aspect-ratio we use that boolean to detect a
     * metadata tag that would come later and trigger a caps change */
    demux->got_par = FALSE;

#ifndef GST_DISABLE_GST_DEBUG
    {
      GstCaps *caps;

      caps = gst_pad_get_current_caps (demux->video_pad);
      GST_DEBUG_OBJECT (demux, "created video pad with caps %" GST_PTR_FORMAT,
          caps);
      if (caps)
        gst_caps_unref (caps);
    }
#endif

    /* We need to set caps before adding */
    gst_element_add_pad (GST_ELEMENT (demux),
        gst_object_ref (demux->video_pad));
    gst_flow_combiner_add_pad (demux->flowcombiner, demux->video_pad);

    /* We only emit no more pads when we have audio and video. Indeed we can
     * not trust the FLV header to tell us if there will be only audio or
     * only video and we would just break discovery of some files */
    if (demux->audio_pad && demux->video_pad) {
      GST_DEBUG_OBJECT (demux, "emitting no more pads");
      gst_element_no_more_pads (GST_ELEMENT (demux));
      demux->no_more_pads = TRUE;
    }
  }

  /* Check if caps have changed */
  if (G_UNLIKELY (codec_tag != demux->video_codec_tag || demux->got_par)) {
    GST_DEBUG_OBJECT (demux, "video settings have changed, changing caps");
    gst_buffer_replace (&demux->video_codec_data, NULL);

    if (!gst_flv_demux_video_negotiate (demux, codec_tag)) {
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    /* When we ve set pixel-aspect-ratio we use that boolean to detect a
     * metadata tag that would come later and trigger a caps change */
    demux->got_par = FALSE;
  }

  /* Check if we have anything to push */
  if (demux->tag_data_size <= codec_data) {
    GST_LOG_OBJECT (demux, "Nothing left in this tag, returning");
    goto beach;
  }

  /* Create buffer from pad */
  outbuf = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_MEMORY,
      7 + codec_data, demux->tag_data_size - codec_data);

  /* detect (and deem to be resyncs)  large dts gaps */
  if (gst_flv_demux_update_resync (demux, dts, demux->video_need_discont,
          &demux->last_video_dts, &demux->video_time_offset)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_RESYNC);
  }

  /* Fill buffer with data */
  GST_LOG_OBJECT (demux, "dts %u pts %u cts %d", dts, dts + cts, cts);

  GST_BUFFER_PTS (outbuf) =
      (dts + cts) * GST_MSECOND + demux->video_time_offset;
  GST_BUFFER_DTS (outbuf) = dts * GST_MSECOND + demux->video_time_offset;
  GST_BUFFER_DURATION (outbuf) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_OFFSET (outbuf) = demux->video_offset++;
  GST_BUFFER_OFFSET_END (outbuf) = demux->video_offset;

  if (demux->duration == GST_CLOCK_TIME_NONE ||
      demux->duration < GST_BUFFER_TIMESTAMP (outbuf))
    demux->duration = GST_BUFFER_TIMESTAMP (outbuf);

  if (!keyframe)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

  if (!demux->indexed) {
    gst_flv_demux_parse_and_add_index_entry (demux,
        GST_BUFFER_TIMESTAMP (outbuf), demux->cur_tag_offset, keyframe);
  }

  if (G_UNLIKELY (demux->video_need_discont)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
    demux->video_need_discont = FALSE;
  }

  demux->segment.position = GST_BUFFER_TIMESTAMP (outbuf);

  /* Do we need a newsegment event ? */
  if (G_UNLIKELY (demux->video_need_segment)) {
    if (!demux->new_seg_event) {
      GST_DEBUG_OBJECT (demux, "pushing newsegment from %"
          GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->segment.position),
          GST_TIME_ARGS (demux->segment.stop));
      demux->segment.start = demux->segment.time = demux->segment.position;
      demux->new_seg_event = gst_event_new_segment (&demux->segment);
    } else {
      GST_DEBUG_OBJECT (demux, "pushing pre-generated newsegment event");
    }

    gst_pad_push_event (demux->video_pad, gst_event_ref (demux->new_seg_event));

    demux->video_need_segment = FALSE;
  }

  GST_LOG_OBJECT (demux,
      "pushing %" G_GSIZE_FORMAT " bytes buffer at dts %" GST_TIME_FORMAT
      " with duration %" GST_TIME_FORMAT ", offset %" G_GUINT64_FORMAT
      ", keyframe (%d)", gst_buffer_get_size (outbuf),
      GST_TIME_ARGS (GST_BUFFER_DTS (outbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)), GST_BUFFER_OFFSET (outbuf),
      keyframe);

  if (!GST_CLOCK_TIME_IS_VALID (demux->video_start)) {
    demux->video_start = GST_BUFFER_TIMESTAMP (outbuf);
  }
  if (!GST_CLOCK_TIME_IS_VALID (demux->audio_first_ts)) {
    demux->video_first_ts = GST_BUFFER_TIMESTAMP (outbuf);
  }

  if (G_UNLIKELY (!demux->no_more_pads
          && (GST_CLOCK_DIFF (demux->video_start,
                  GST_BUFFER_TIMESTAMP (outbuf)) > NO_MORE_PADS_THRESHOLD))) {
    GST_DEBUG_OBJECT (demux,
        "Signalling no-more-pads because no audio stream was found"
        " after 6 seconds of video");
    gst_element_no_more_pads (GST_ELEMENT_CAST (demux));
    demux->no_more_pads = TRUE;
  }

  /* Push downstream */
  ret = gst_pad_push (demux->video_pad, outbuf);

  if (G_UNLIKELY (ret != GST_FLOW_OK) &&
      demux->segment.rate < 0.0 && ret == GST_FLOW_EOS &&
      demux->segment.position > demux->segment.stop) {
    /* In reverse playback we can get a GST_FLOW_EOS when
     * we are at the end of the segment, so we just need to jump
     * back to the previous section. */
    GST_DEBUG_OBJECT (demux, "downstream has reached end of segment");
    demux->video_done = TRUE;
    ret = GST_FLOW_OK;
    goto beach;
  }

  ret = gst_flow_combiner_update_pad_flow (demux->flowcombiner,
      demux->video_pad, ret);

beach:
  gst_buffer_unmap (buffer, &map);
  return ret;
}

static GstClockTime
gst_flv_demux_parse_tag_timestamp (GstFlvDemux * demux, gboolean index,
    GstBuffer * buffer, size_t * tag_size)
{
  guint32 dts = 0, dts_ext = 0;
  guint32 tag_data_size;
  guint8 type;
  gboolean keyframe = TRUE;
  GstClockTime ret = GST_CLOCK_TIME_NONE;
  GstMapInfo map;
  guint8 *data;
  gsize size;

  g_return_val_if_fail (gst_buffer_get_size (buffer) >= 12,
      GST_CLOCK_TIME_NONE);

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  data = map.data;
  size = map.size;

  type = data[0];

  if (type != 9 && type != 8 && type != 18) {
    GST_WARNING_OBJECT (demux, "Unsupported tag type %u", data[0]);
    goto exit;
  }

  if (type == 9)
    demux->has_video = TRUE;
  else if (type == 8)
    demux->has_audio = TRUE;

  tag_data_size = GST_READ_UINT24_BE (data + 1);

  if (size >= tag_data_size + 11 + 4) {
    if (GST_READ_UINT32_BE (data + tag_data_size + 11) != tag_data_size + 11) {
      GST_WARNING_OBJECT (demux, "Invalid tag size");
      goto exit;
    }
  }

  if (tag_size)
    *tag_size = tag_data_size + 11 + 4;

  data += 4;

  GST_LOG_OBJECT (demux, "dts bytes %02X %02X %02X %02X", data[0], data[1],
      data[2], data[3]);

  /* Grab timestamp of tag tag */
  dts = GST_READ_UINT24_BE (data);
  /* read the dts extension to 32 bits integer */
  dts_ext = GST_READ_UINT8 (data + 3);
  /* Combine them */
  dts |= dts_ext << 24;

  if (type == 9) {
    data += 7;

    keyframe = ((data[0] >> 4) == 1);
  }

  ret = dts * GST_MSECOND;
  GST_LOG_OBJECT (demux, "dts: %" GST_TIME_FORMAT, GST_TIME_ARGS (ret));

  if (index && !demux->indexed && (type == 9 || (type == 8
              && !demux->has_video))) {
    gst_flv_demux_parse_and_add_index_entry (demux, ret, demux->offset,
        keyframe);
  }

  if (demux->duration == GST_CLOCK_TIME_NONE || demux->duration < ret)
    demux->duration = ret;

exit:
  gst_buffer_unmap (buffer, &map);
  return ret;
}

static GstFlowReturn
gst_flv_demux_parse_tag_type (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint8 tag_type = 0;
  GstMapInfo map;

  g_return_val_if_fail (gst_buffer_get_size (buffer) >= 4, GST_FLOW_ERROR);

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  tag_type = map.data[0];

  /* Tag size is 1 byte of type + 3 bytes of size + 7 bytes + tag data size +
   * 4 bytes of previous tag size */
  demux->tag_data_size = GST_READ_UINT24_BE (map.data + 1);
  demux->tag_size = demux->tag_data_size + 11;

  GST_LOG_OBJECT (demux, "tag data size is %" G_GUINT64_FORMAT,
      demux->tag_data_size);

  gst_buffer_unmap (buffer, &map);

  switch (tag_type) {
    case 9:
      demux->state = FLV_STATE_TAG_VIDEO;
      demux->has_video = TRUE;
      break;
    case 8:
      demux->state = FLV_STATE_TAG_AUDIO;
      demux->has_audio = TRUE;
      break;
    case 18:
      demux->state = FLV_STATE_TAG_SCRIPT;
      break;
    default:
      GST_WARNING_OBJECT (demux, "unsupported tag type %u", tag_type);
      demux->state = FLV_STATE_SKIP;
  }

  return ret;
}

static GstFlowReturn
gst_flv_demux_parse_header (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstMapInfo map;

  g_return_val_if_fail (gst_buffer_get_size (buffer) >= 9, GST_FLOW_ERROR);

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  /* Check for the FLV tag */
  if (map.data[0] == 'F' && map.data[1] == 'L' && map.data[2] == 'V') {
    GST_DEBUG_OBJECT (demux, "FLV header detected");
  } else {
    if (G_UNLIKELY (demux->strict)) {
      GST_WARNING_OBJECT (demux, "invalid header tag detected");
      ret = GST_FLOW_EOS;
      goto beach;
    }
  }

  if (map.data[3] == '1') {
    GST_DEBUG_OBJECT (demux, "FLV version 1 detected");
  } else {
    if (G_UNLIKELY (demux->strict)) {
      GST_WARNING_OBJECT (demux, "invalid header version detected");
      ret = GST_FLOW_EOS;
      goto beach;
    }

  }

  /* Now look at audio/video flags */
  {
    guint8 flags = map.data[4];

    demux->has_video = demux->has_audio = FALSE;

    if (flags & 1) {
      GST_DEBUG_OBJECT (demux, "there is a video stream");
      demux->has_video = TRUE;
    }
    if (flags & 4) {
      GST_DEBUG_OBJECT (demux, "there is an audio stream");
      demux->has_audio = TRUE;
    }
  }

  /* do a one-time seekability check */
  gst_flv_demux_check_seekability (demux);

  /* We don't care about the rest */
  demux->need_header = FALSE;

beach:
  gst_buffer_unmap (buffer, &map);
  return ret;
}


static void
gst_flv_demux_flush (GstFlvDemux * demux, gboolean discont)
{
  GST_DEBUG_OBJECT (demux, "flushing queued data in the FLV demuxer");

  gst_adapter_clear (demux->adapter);

  demux->audio_need_discont = TRUE;
  demux->video_need_discont = TRUE;

  demux->flushing = FALSE;

  /* Only in push mode and if we're not during a seek */
  if (!demux->random_access && demux->state != FLV_STATE_SEEK) {
    /* After a flush we expect a tag_type */
    demux->state = FLV_STATE_TAG_TYPE;
    /* We reset the offset and will get one from first push */
    demux->offset = 0;
  }
}

static void
gst_flv_demux_cleanup (GstFlvDemux * demux)
{
  GST_DEBUG_OBJECT (demux, "cleaning up FLV demuxer");

  demux->state = FLV_STATE_HEADER;

  demux->have_group_id = FALSE;
  demux->group_id = G_MAXUINT;

  demux->flushing = FALSE;
  demux->need_header = TRUE;
  demux->audio_need_segment = TRUE;
  demux->video_need_segment = TRUE;
  demux->audio_need_discont = TRUE;
  demux->video_need_discont = TRUE;

  demux->has_audio = FALSE;
  demux->has_video = FALSE;
  demux->got_par = FALSE;

  demux->indexed = FALSE;
  demux->upstream_seekable = FALSE;
  demux->file_size = 0;

  demux->index_max_pos = 0;
  demux->index_max_time = 0;

  demux->audio_start = demux->video_start = GST_CLOCK_TIME_NONE;
  demux->last_audio_pts = demux->last_video_dts = 0;
  demux->audio_time_offset = demux->video_time_offset = 0;

  demux->no_more_pads = FALSE;

#ifndef GST_DISABLE_DEBUG
  demux->no_audio_warned = FALSE;
  demux->no_video_warned = FALSE;
#endif

  gst_segment_init (&demux->segment, GST_FORMAT_TIME);

  demux->w = demux->h = 0;
  demux->framerate = 0.0;
  demux->par_x = demux->par_y = 1;
  demux->video_offset = 0;
  demux->audio_offset = 0;
  demux->offset = demux->cur_tag_offset = 0;
  demux->tag_size = demux->tag_data_size = 0;
  demux->duration = GST_CLOCK_TIME_NONE;

  if (demux->new_seg_event) {
    gst_event_unref (demux->new_seg_event);
    demux->new_seg_event = NULL;
  }

  gst_adapter_clear (demux->adapter);

  if (demux->audio_codec_data) {
    gst_buffer_unref (demux->audio_codec_data);
    demux->audio_codec_data = NULL;
  }

  if (demux->video_codec_data) {
    gst_buffer_unref (demux->video_codec_data);
    demux->video_codec_data = NULL;
  }

  if (demux->audio_pad) {
    gst_flow_combiner_remove_pad (demux->flowcombiner, demux->audio_pad);
    gst_element_remove_pad (GST_ELEMENT (demux), demux->audio_pad);
    gst_object_unref (demux->audio_pad);
    demux->audio_pad = NULL;
  }

  if (demux->video_pad) {
    gst_flow_combiner_remove_pad (demux->flowcombiner, demux->video_pad);
    gst_element_remove_pad (GST_ELEMENT (demux), demux->video_pad);
    gst_object_unref (demux->video_pad);
    demux->video_pad = NULL;
  }

  if (demux->times) {
    g_array_free (demux->times, TRUE);
    demux->times = NULL;
  }

  if (demux->filepositions) {
    g_array_free (demux->filepositions, TRUE);
    demux->filepositions = NULL;
  }

  gst_flv_demux_clear_tags (demux);
}

/*
 * Create and push a flushing seek event upstream
 */
static gboolean
flv_demux_seek_to_offset (GstFlvDemux * demux, guint64 offset)
{
  GstEvent *event;
  gboolean res = 0;

  GST_DEBUG_OBJECT (demux, "Seeking to %" G_GUINT64_FORMAT, offset);

  event =
      gst_event_new_seek (1.0, GST_FORMAT_BYTES,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET, offset,
      GST_SEEK_TYPE_NONE, -1);

  res = gst_pad_push_event (demux->sinkpad, event);

  if (res)
    demux->offset = offset;
  return res;
}

static GstFlowReturn
gst_flv_demux_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstFlvDemux *demux = NULL;

  demux = GST_FLV_DEMUX (parent);

  GST_LOG_OBJECT (demux,
      "received buffer of %" G_GSIZE_FORMAT " bytes at offset %"
      G_GUINT64_FORMAT, gst_buffer_get_size (buffer),
      GST_BUFFER_OFFSET (buffer));

  if (G_UNLIKELY (GST_BUFFER_OFFSET (buffer) == 0)) {
    GST_DEBUG_OBJECT (demux, "beginning of file, expect header");
    demux->state = FLV_STATE_HEADER;
    demux->offset = 0;
  }

  if (G_UNLIKELY (demux->offset == 0 && GST_BUFFER_OFFSET (buffer) != 0)) {
    GST_DEBUG_OBJECT (demux, "offset was zero, synchronizing with buffer's");
    demux->offset = GST_BUFFER_OFFSET (buffer);
  }

  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT)) {
    GST_DEBUG_OBJECT (demux, "Discontinuity");
    gst_adapter_clear (demux->adapter);
  }

  gst_adapter_push (demux->adapter, buffer);

  if (demux->seeking) {
    demux->state = FLV_STATE_SEEK;
    GST_OBJECT_LOCK (demux);
    demux->seeking = FALSE;
    GST_OBJECT_UNLOCK (demux);
  }

parse:
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    GST_DEBUG_OBJECT (demux, "got flow return %s", gst_flow_get_name (ret));
    goto beach;
  }

  if (G_UNLIKELY (demux->flushing)) {
    GST_DEBUG_OBJECT (demux, "we are now flushing, exiting parser loop");
    ret = GST_FLOW_FLUSHING;
    goto beach;
  }

  switch (demux->state) {
    case FLV_STATE_HEADER:
    {
      if (gst_adapter_available (demux->adapter) >= FLV_HEADER_SIZE) {
        GstBuffer *buffer;

        buffer = gst_adapter_take_buffer (demux->adapter, FLV_HEADER_SIZE);

        ret = gst_flv_demux_parse_header (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += FLV_HEADER_SIZE;

        demux->state = FLV_STATE_TAG_TYPE;
        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_TAG_TYPE:
    {
      if (gst_adapter_available (demux->adapter) >= FLV_TAG_TYPE_SIZE) {
        GstBuffer *buffer;

        /* Remember the tag offset in bytes */
        demux->cur_tag_offset = demux->offset;

        buffer = gst_adapter_take_buffer (demux->adapter, FLV_TAG_TYPE_SIZE);

        ret = gst_flv_demux_parse_tag_type (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += FLV_TAG_TYPE_SIZE;

        /* last tag is not an index => no index/don't know where the index is
         * seek back to the beginning */
        if (demux->seek_event && demux->state != FLV_STATE_TAG_SCRIPT)
          goto no_index;

        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_TAG_VIDEO:
    {
      if (gst_adapter_available (demux->adapter) >= demux->tag_size) {
        GstBuffer *buffer;

        buffer = gst_adapter_take_buffer (demux->adapter, demux->tag_size);

        ret = gst_flv_demux_parse_tag_video (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += demux->tag_size;

        demux->state = FLV_STATE_TAG_TYPE;
        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_TAG_AUDIO:
    {
      if (gst_adapter_available (demux->adapter) >= demux->tag_size) {
        GstBuffer *buffer;

        buffer = gst_adapter_take_buffer (demux->adapter, demux->tag_size);

        ret = gst_flv_demux_parse_tag_audio (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += demux->tag_size;

        demux->state = FLV_STATE_TAG_TYPE;
        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_TAG_SCRIPT:
    {
      if (gst_adapter_available (demux->adapter) >= demux->tag_size) {
        GstBuffer *buffer;

        buffer = gst_adapter_take_buffer (demux->adapter, demux->tag_size);

        ret = gst_flv_demux_parse_tag_script (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += demux->tag_size;

        demux->state = FLV_STATE_TAG_TYPE;

        /* if there's a seek event we're here for the index so if we don't have it
         * we seek back to the beginning */
        if (demux->seek_event) {
          if (demux->indexed)
            demux->state = FLV_STATE_SEEK;
          else
            goto no_index;
        }

        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_SEEK:
    {
      GstEvent *event;

      ret = GST_FLOW_OK;

      if (!demux->indexed) {
        if (demux->offset == demux->file_size - sizeof (guint32)) {
          guint64 seek_offset;
          guint8 *data;

          data = gst_adapter_take (demux->adapter, 4);
          if (!data)
            goto no_index;

          seek_offset = demux->file_size - sizeof (guint32) -
              GST_READ_UINT32_BE (data);
          g_free (data);

          GST_INFO_OBJECT (demux,
              "Seeking to beginning of last tag at %" G_GUINT64_FORMAT,
              seek_offset);
          demux->state = FLV_STATE_TAG_TYPE;
          flv_demux_seek_to_offset (demux, seek_offset);
          goto beach;
        } else
          goto no_index;
      }

      GST_OBJECT_LOCK (demux);
      event = demux->seek_event;
      demux->seek_event = NULL;
      GST_OBJECT_UNLOCK (demux);

      /* calculate and perform seek */
      if (!flv_demux_handle_seek_push (demux, event))
        goto seek_failed;

      gst_event_unref (event);
      demux->state = FLV_STATE_TAG_TYPE;
      goto beach;
    }
    case FLV_STATE_SKIP:
      /* Skip unknown tags (set in _parse_tag_type()) */
      if (gst_adapter_available (demux->adapter) >= demux->tag_size) {
        gst_adapter_flush (demux->adapter, demux->tag_size);
        demux->offset += demux->tag_size;
        demux->state = FLV_STATE_TAG_TYPE;
        goto parse;
      } else {
        goto beach;
      }
    default:
      GST_DEBUG_OBJECT (demux, "unexpected demuxer state");
  }

beach:
  return ret;

/* ERRORS */
no_index:
  {
    GST_OBJECT_LOCK (demux);
    demux->seeking = FALSE;
    gst_event_unref (demux->seek_event);
    demux->seek_event = NULL;
    GST_OBJECT_UNLOCK (demux);
    GST_WARNING_OBJECT (demux,
        "failed to find an index, seeking back to beginning");
    flv_demux_seek_to_offset (demux, 0);
    return GST_FLOW_OK;
  }
seek_failed:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL), ("seek failed"));
    return GST_FLOW_ERROR;
  }

}

static GstFlowReturn
gst_flv_demux_pull_range (GstFlvDemux * demux, GstPad * pad, guint64 offset,
    guint size, GstBuffer ** buffer)
{
  GstFlowReturn ret;

  ret = gst_pad_pull_range (pad, offset, size, buffer);
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    GST_WARNING_OBJECT (demux,
        "failed when pulling %d bytes from offset %" G_GUINT64_FORMAT ": %s",
        size, offset, gst_flow_get_name (ret));
    *buffer = NULL;
    return ret;
  }

  if (G_UNLIKELY (*buffer && gst_buffer_get_size (*buffer) != size)) {
    GST_WARNING_OBJECT (demux,
        "partial pull got %" G_GSIZE_FORMAT " when expecting %d from offset %"
        G_GUINT64_FORMAT, gst_buffer_get_size (*buffer), size, offset);
    gst_buffer_unref (*buffer);
    ret = GST_FLOW_EOS;
    *buffer = NULL;
    return ret;
  }

  return ret;
}

static GstFlowReturn
gst_flv_demux_pull_tag (GstPad * pad, GstFlvDemux * demux)
{
  GstBuffer *buffer = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  /* Store tag offset */
  demux->cur_tag_offset = demux->offset;

  /* Get the first 4 bytes to identify tag type and size */
  if (G_UNLIKELY ((ret = gst_flv_demux_pull_range (demux, pad, demux->offset,
                  FLV_TAG_TYPE_SIZE, &buffer)) != GST_FLOW_OK))
    goto beach;

  /* Identify tag type */
  ret = gst_flv_demux_parse_tag_type (demux, buffer);

  gst_buffer_unref (buffer);

  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto beach;

  /* Jump over tag type + size */
  demux->offset += FLV_TAG_TYPE_SIZE;

  /* Pull the whole tag */
  buffer = NULL;
  if (G_UNLIKELY ((ret = gst_flv_demux_pull_range (demux, pad, demux->offset,
                  demux->tag_size, &buffer)) != GST_FLOW_OK))
    goto beach;

  switch (demux->state) {
    case FLV_STATE_TAG_VIDEO:
      ret = gst_flv_demux_parse_tag_video (demux, buffer);
      break;
    case FLV_STATE_TAG_AUDIO:
      ret = gst_flv_demux_parse_tag_audio (demux, buffer);
      break;
    case FLV_STATE_TAG_SCRIPT:
      ret = gst_flv_demux_parse_tag_script (demux, buffer);
      break;
    default:
      GST_WARNING_OBJECT (demux, "unexpected state %d", demux->state);
  }

  gst_buffer_unref (buffer);

  /* Jump over that part we've just parsed */
  demux->offset += demux->tag_size;

  /* Make sure we reinitialize the tag size */
  demux->tag_size = 0;

  /* Ready for the next tag */
  demux->state = FLV_STATE_TAG_TYPE;

  if (G_UNLIKELY (ret == GST_FLOW_NOT_LINKED)) {
    GST_WARNING_OBJECT (demux, "parsing this tag returned not-linked and "
        "neither video nor audio are linked");
  }

beach:
  return ret;
}

static GstFlowReturn
gst_flv_demux_pull_header (GstPad * pad, GstFlvDemux * demux)
{
  GstBuffer *buffer = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  /* Get the first 9 bytes */
  if (G_UNLIKELY ((ret = gst_flv_demux_pull_range (demux, pad, demux->offset,
                  FLV_HEADER_SIZE, &buffer)) != GST_FLOW_OK))
    goto beach;

  ret = gst_flv_demux_parse_header (demux, buffer);

  gst_buffer_unref (buffer);

  /* Jump over the header now */
  demux->offset += FLV_HEADER_SIZE;
  demux->state = FLV_STATE_TAG_TYPE;

beach:
  return ret;
}

static void
gst_flv_demux_move_to_offset (GstFlvDemux * demux, gint64 offset,
    gboolean reset)
{
  demux->offset = offset;

  /* Tell all the stream we moved to a different position (discont) */
  demux->audio_need_discont = TRUE;
  demux->video_need_discont = TRUE;

  /* next section setup */
  demux->from_offset = -1;
  demux->audio_done = demux->video_done = FALSE;
  demux->audio_first_ts = demux->video_first_ts = GST_CLOCK_TIME_NONE;

  if (reset) {
    demux->from_offset = -1;
    demux->to_offset = G_MAXINT64;
  }

  /* If we seeked at the beginning of the file parse the header again */
  if (G_UNLIKELY (!demux->offset)) {
    demux->state = FLV_STATE_HEADER;
  } else {                      /* or parse a tag */
    demux->state = FLV_STATE_TAG_TYPE;
  }
}

static GstFlowReturn
gst_flv_demux_seek_to_prev_keyframe (GstFlvDemux * demux)
{
  GstFlowReturn ret = GST_FLOW_EOS;
  GstIndex *index;
  GstIndexEntry *entry = NULL;

  GST_DEBUG_OBJECT (demux,
      "terminated section started at offset %" G_GINT64_FORMAT,
      demux->from_offset);

  /* we are done if we got all audio and video */
  if ((!GST_CLOCK_TIME_IS_VALID (demux->audio_first_ts) ||
          demux->audio_first_ts < demux->segment.start) &&
      (!GST_CLOCK_TIME_IS_VALID (demux->video_first_ts) ||
          demux->video_first_ts < demux->segment.start))
    goto done;

  if (demux->from_offset <= 0)
    goto done;

  GST_DEBUG_OBJECT (demux, "locating previous position");

  index = gst_flv_demux_get_index (GST_ELEMENT (demux));

  /* locate index entry before previous start position */
  if (index) {
    entry = gst_index_get_assoc_entry (index, demux->index_id,
        GST_INDEX_LOOKUP_BEFORE, GST_ASSOCIATION_FLAG_KEY_UNIT,
        GST_FORMAT_BYTES, demux->from_offset - 1);

    if (entry) {
      gint64 bytes = 0, time = 0;

      gst_index_entry_assoc_map (entry, GST_FORMAT_BYTES, &bytes);
      gst_index_entry_assoc_map (entry, GST_FORMAT_TIME, &time);

      GST_DEBUG_OBJECT (demux, "found index entry for %" G_GINT64_FORMAT
          " at %" GST_TIME_FORMAT ", seeking to %" G_GINT64_FORMAT,
          demux->offset - 1, GST_TIME_ARGS (time), bytes);

      /* setup for next section */
      demux->to_offset = demux->from_offset;
      gst_flv_demux_move_to_offset (demux, bytes, FALSE);
      ret = GST_FLOW_OK;
    }

    gst_object_unref (index);
  }

done:
  return ret;
}

static GstFlowReturn
gst_flv_demux_create_index (GstFlvDemux * demux, gint64 pos, GstClockTime ts)
{
  gint64 size;
  size_t tag_size;
  guint64 old_offset;
  GstBuffer *buffer;
  GstClockTime tag_time;
  GstFlowReturn ret = GST_FLOW_OK;

  if (!gst_pad_peer_query_duration (demux->sinkpad, GST_FORMAT_BYTES, &size))
    return GST_FLOW_OK;

  GST_DEBUG_OBJECT (demux, "building index at %" G_GINT64_FORMAT
      " looking for time %" GST_TIME_FORMAT, pos, GST_TIME_ARGS (ts));

  old_offset = demux->offset;
  demux->offset = pos;

  buffer = NULL;
  while ((ret = gst_flv_demux_pull_range (demux, demux->sinkpad, demux->offset,
              12, &buffer)) == GST_FLOW_OK) {
    tag_time =
        gst_flv_demux_parse_tag_timestamp (demux, TRUE, buffer, &tag_size);

    gst_buffer_unref (buffer);
    buffer = NULL;

    if (G_UNLIKELY (tag_time == GST_CLOCK_TIME_NONE || tag_time > ts))
      goto exit;

    demux->offset += tag_size;
  }

  if (ret == GST_FLOW_EOS) {
    /* file ran out, so mark we have complete index */
    demux->indexed = TRUE;
    ret = GST_FLOW_OK;
  }

exit:
  demux->offset = old_offset;

  return ret;
}

static gint64
gst_flv_demux_get_metadata (GstFlvDemux * demux)
{
  gint64 ret = 0, offset;
  size_t tag_size, size;
  GstBuffer *buffer = NULL;
  GstMapInfo map;

  if (!gst_pad_peer_query_duration (demux->sinkpad, GST_FORMAT_BYTES, &offset))
    goto exit;

  ret = offset;
  GST_DEBUG_OBJECT (demux, "upstream size: %" G_GINT64_FORMAT, offset);
  if (G_UNLIKELY (offset < 4))
    goto exit;

  offset -= 4;
  if (GST_FLOW_OK != gst_flv_demux_pull_range (demux, demux->sinkpad, offset,
          4, &buffer))
    goto exit;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  tag_size = GST_READ_UINT32_BE (map.data);
  gst_buffer_unmap (buffer, &map);
  GST_DEBUG_OBJECT (demux, "last tag size: %" G_GSIZE_FORMAT, tag_size);
  gst_buffer_unref (buffer);
  buffer = NULL;

  if (G_UNLIKELY (offset < tag_size))
    goto exit;

  offset -= tag_size;
  if (GST_FLOW_OK != gst_flv_demux_pull_range (demux, demux->sinkpad, offset,
          12, &buffer))
    goto exit;

  /* a consistency check */
  gst_buffer_map (buffer, &map, GST_MAP_READ);
  size = GST_READ_UINT24_BE (map.data + 1);
  if (size != tag_size - 11) {
    gst_buffer_unmap (buffer, &map);
    GST_DEBUG_OBJECT (demux,
        "tag size %" G_GSIZE_FORMAT ", expected %" G_GSIZE_FORMAT
        ", corrupt or truncated file", size, tag_size - 11);
    goto exit;
  }

  /* try to update duration with timestamp in any case */
  gst_flv_demux_parse_tag_timestamp (demux, FALSE, buffer, &size);

  /* maybe get some more metadata */
  if (map.data[0] == 18) {
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);
    buffer = NULL;
    GST_DEBUG_OBJECT (demux, "script tag, pulling it to parse");
    offset += 4;
    if (GST_FLOW_OK == gst_flv_demux_pull_range (demux, demux->sinkpad, offset,
            tag_size, &buffer))
      gst_flv_demux_parse_tag_script (demux, buffer);
  } else {
    gst_buffer_unmap (buffer, &map);
  }

exit:
  if (buffer)
    gst_buffer_unref (buffer);

  return ret;
}

static void
gst_flv_demux_loop (GstPad * pad)
{
  GstFlvDemux *demux = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  demux = GST_FLV_DEMUX (gst_pad_get_parent (pad));

  /* pull in data */
  switch (demux->state) {
    case FLV_STATE_TAG_TYPE:
      if (demux->from_offset == -1)
        demux->from_offset = demux->offset;
      ret = gst_flv_demux_pull_tag (pad, demux);
      /* if we have seen real data, we probably passed a possible metadata
       * header located at start.  So if we do not yet have an index,
       * try to pick up metadata (index, duration) at the end */
      if (G_UNLIKELY (!demux->file_size && !demux->indexed &&
              (demux->has_video || demux->has_audio)))
        demux->file_size = gst_flv_demux_get_metadata (demux);
      break;
    case FLV_STATE_DONE:
      ret = GST_FLOW_EOS;
      break;
    case FLV_STATE_SEEK:
      /* seek issued with insufficient index;
       * scan for index in task thread from current maximum offset to
       * desired time and then perform seek */
      /* TODO maybe some buffering message or so to indicate scan progress */
      ret = gst_flv_demux_create_index (demux, demux->index_max_pos,
          demux->seek_time);
      if (ret != GST_FLOW_OK)
        goto pause;
      /* position and state arranged by seek,
       * also unrefs event */
      gst_flv_demux_handle_seek_pull (demux, demux->seek_event, FALSE);
      demux->seek_event = NULL;
      break;
    default:
      ret = gst_flv_demux_pull_header (pad, demux);
      /* index scans start after header */
      demux->index_max_pos = demux->offset;
      break;
  }

  if (demux->segment.rate < 0.0) {
    /* check end of section */
    if ((gint64) demux->offset >= demux->to_offset ||
        demux->segment.position >= demux->segment.stop + 2 * GST_SECOND ||
        (demux->audio_done && demux->video_done))
      ret = gst_flv_demux_seek_to_prev_keyframe (demux);
  } else {
    /* check EOS condition */
    if ((demux->segment.stop != -1) &&
        (demux->segment.position >= demux->segment.stop)) {
      ret = GST_FLOW_EOS;
    }
  }

  /* pause if something went wrong or at end */
  if (G_UNLIKELY (ret != GST_FLOW_OK) && !(ret == GST_FLOW_NOT_LINKED
          && !demux->no_more_pads))
    goto pause;

  gst_object_unref (demux);

  return;

pause:
  {
    const gchar *reason = gst_flow_get_name (ret);

    GST_LOG_OBJECT (demux, "pausing task, reason %s", reason);
    gst_pad_pause_task (pad);

    if (ret == GST_FLOW_EOS) {
      /* handle end-of-stream/segment */
      /* so align our position with the end of it, if there is one
       * this ensures a subsequent will arrive at correct base/acc time */
      if (demux->segment.rate > 0.0 &&
          GST_CLOCK_TIME_IS_VALID (demux->segment.stop))
        demux->segment.position = demux->segment.stop;
      else if (demux->segment.rate < 0.0)
        demux->segment.position = demux->segment.start;

      /* perform EOS logic */
      if (!demux->no_more_pads) {
        gst_element_no_more_pads (GST_ELEMENT_CAST (demux));
        demux->no_more_pads = TRUE;
      }

      if (demux->segment.flags & GST_SEGMENT_FLAG_SEGMENT) {
        gint64 stop;

        /* for segment playback we need to post when (in stream time)
         * we stopped, this is either stop (when set) or the duration. */
        if ((stop = demux->segment.stop) == -1)
          stop = demux->segment.duration;

        if (demux->segment.rate >= 0) {
          GST_LOG_OBJECT (demux, "Sending segment done, at end of segment");
          gst_element_post_message (GST_ELEMENT_CAST (demux),
              gst_message_new_segment_done (GST_OBJECT_CAST (demux),
                  GST_FORMAT_TIME, stop));
          gst_flv_demux_push_src_event (demux,
              gst_event_new_segment_done (GST_FORMAT_TIME, stop));
        } else {                /* Reverse playback */
          GST_LOG_OBJECT (demux, "Sending segment done, at beginning of "
              "segment");
          gst_element_post_message (GST_ELEMENT_CAST (demux),
              gst_message_new_segment_done (GST_OBJECT_CAST (demux),
                  GST_FORMAT_TIME, demux->segment.start));
          gst_flv_demux_push_src_event (demux,
              gst_event_new_segment_done (GST_FORMAT_TIME,
                  demux->segment.start));
        }
      } else {
        /* normal playback, send EOS to all linked pads */
        if (!demux->no_more_pads) {
          gst_element_no_more_pads (GST_ELEMENT (demux));
          demux->no_more_pads = TRUE;
        }

        GST_LOG_OBJECT (demux, "Sending EOS, at end of stream");
        if (!demux->audio_pad && !demux->video_pad)
          GST_ELEMENT_ERROR (demux, STREAM, FAILED,
              ("Internal data stream error."), ("Got EOS before any data"));
        else if (!gst_flv_demux_push_src_event (demux, gst_event_new_eos ()))
          GST_WARNING_OBJECT (demux, "failed pushing EOS on streams");
      }
    } else if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_EOS) {
      GST_ELEMENT_FLOW_ERROR (demux, ret);
      gst_flv_demux_push_src_event (demux, gst_event_new_eos ());
    }
    gst_object_unref (demux);
    return;
  }
}

static guint64
gst_flv_demux_find_offset (GstFlvDemux * demux, GstSegment * segment,
    GstSeekFlags seek_flags)
{
  gint64 bytes = 0;
  gint64 time = 0;
  GstIndex *index;
  GstIndexEntry *entry;

  g_return_val_if_fail (segment != NULL, 0);

  time = segment->position;

  index = gst_flv_demux_get_index (GST_ELEMENT (demux));

  if (index) {
    /* Let's check if we have an index entry for that seek time */
    entry = gst_index_get_assoc_entry (index, demux->index_id,
        seek_flags & GST_SEEK_FLAG_SNAP_AFTER ?
        GST_INDEX_LOOKUP_AFTER : GST_INDEX_LOOKUP_BEFORE,
        GST_ASSOCIATION_FLAG_KEY_UNIT, GST_FORMAT_TIME, time);

    if (entry) {
      gst_index_entry_assoc_map (entry, GST_FORMAT_BYTES, &bytes);
      gst_index_entry_assoc_map (entry, GST_FORMAT_TIME, &time);

      GST_DEBUG_OBJECT (demux, "found index entry for %" GST_TIME_FORMAT
          " at %" GST_TIME_FORMAT ", seeking to %" G_GINT64_FORMAT,
          GST_TIME_ARGS (segment->position), GST_TIME_ARGS (time), bytes);

      /* Key frame seeking */
      if (seek_flags & GST_SEEK_FLAG_KEY_UNIT) {
        /* Adjust the segment so that the keyframe fits in */
        segment->start = segment->time = time;
        segment->position = time;
      }
    } else {
      GST_DEBUG_OBJECT (demux, "no index entry found for %" GST_TIME_FORMAT,
          GST_TIME_ARGS (segment->start));
    }

    gst_object_unref (index);
  }

  return bytes;
}

static gboolean
flv_demux_handle_seek_push (GstFlvDemux * demux, GstEvent * event)
{
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType start_type, stop_type;
  gint64 start, stop;
  gdouble rate;
  gboolean update, flush, ret;
  GstSegment seeksegment;

  gst_event_parse_seek (event, &rate, &format, &flags,
      &start_type, &start, &stop_type, &stop);

  if (format != GST_FORMAT_TIME)
    goto wrong_format;

  flush = ! !(flags & GST_SEEK_FLAG_FLUSH);

  /* Work on a copy until we are sure the seek succeeded. */
  memcpy (&seeksegment, &demux->segment, sizeof (GstSegment));

  GST_DEBUG_OBJECT (demux, "segment before configure %" GST_SEGMENT_FORMAT,
      &demux->segment);

  /* Apply the seek to our segment */
  gst_segment_do_seek (&seeksegment, rate, format, flags,
      start_type, start, stop_type, stop, &update);

  GST_DEBUG_OBJECT (demux, "segment configured %" GST_SEGMENT_FORMAT,
      &seeksegment);

  if (flush || seeksegment.position != demux->segment.position) {
    /* Do the actual seeking */
    guint64 offset = gst_flv_demux_find_offset (demux, &seeksegment, flags);

    GST_DEBUG_OBJECT (demux, "generating an upstream seek at position %"
        G_GUINT64_FORMAT, offset);
    ret = gst_pad_push_event (demux->sinkpad,
        gst_event_new_seek (seeksegment.rate, GST_FORMAT_BYTES,
            flags | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
            offset, GST_SEEK_TYPE_NONE, 0));
    if (G_UNLIKELY (!ret)) {
      GST_WARNING_OBJECT (demux, "upstream seek failed");
    }

    gst_flow_combiner_reset (demux->flowcombiner);
    /* Tell all the stream we moved to a different position (discont) */
    demux->audio_need_discont = TRUE;
    demux->video_need_discont = TRUE;
  } else {
    ret = TRUE;
  }

  if (ret) {
    /* Ok seek succeeded, take the newly configured segment */
    memcpy (&demux->segment, &seeksegment, sizeof (GstSegment));

    /* Tell all the stream a new segment is needed */
    demux->audio_need_segment = TRUE;
    demux->video_need_segment = TRUE;
    /* Clean any potential newsegment event kept for the streams. The first
     * stream needing a new segment will create a new one. */
    if (G_UNLIKELY (demux->new_seg_event)) {
      gst_event_unref (demux->new_seg_event);
      demux->new_seg_event = NULL;
    }
    GST_DEBUG_OBJECT (demux, "preparing newsegment from %"
        GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (demux->segment.start),
        GST_TIME_ARGS (demux->segment.stop));
    demux->new_seg_event = gst_event_new_segment (&demux->segment);
    gst_event_unref (event);
  } else {
    ret = gst_pad_push_event (demux->sinkpad, event);
  }

  return ret;

/* ERRORS */
wrong_format:
  {
    GST_WARNING_OBJECT (demux, "we only support seeking in TIME format");
    gst_event_unref (event);
    return FALSE;
  }
}

static gboolean
gst_flv_demux_handle_seek_push (GstFlvDemux * demux, GstEvent * event)
{
  GstFormat format;

  gst_event_parse_seek (event, NULL, &format, NULL, NULL, NULL, NULL, NULL);

  if (format != GST_FORMAT_TIME) {
    GST_WARNING_OBJECT (demux, "we only support seeking in TIME format");
    gst_event_unref (event);
    return FALSE;
  }

  /* First try upstream */
  if (gst_pad_push_event (demux->sinkpad, gst_event_ref (event))) {
    GST_DEBUG_OBJECT (demux, "Upstream successfully seeked");
    gst_event_unref (event);
    return TRUE;
  }

  if (!demux->indexed) {
    guint64 seek_offset = 0;
    gboolean building_index;

    GST_OBJECT_LOCK (demux);
    /* handle the seek in the chain function */
    demux->seeking = TRUE;
    demux->state = FLV_STATE_SEEK;

    /* copy the event */
    if (demux->seek_event)
      gst_event_unref (demux->seek_event);
    demux->seek_event = gst_event_ref (event);

    /* set the building_index flag so that only one thread can setup the
     * structures for index seeking. */
    building_index = demux->building_index;
    if (!building_index) {
      demux->building_index = TRUE;
      if (!demux->file_size
          && !gst_pad_peer_query_duration (demux->sinkpad, GST_FORMAT_BYTES,
              &demux->file_size)) {
        GST_WARNING_OBJECT (demux, "Failed to query upstream file size");
        GST_OBJECT_UNLOCK (demux);
        return FALSE;
      }

      /* we hope the last tag is a scriptdataobject containing an index
       * the size of the last tag is given in the last guint32 bits
       * then we seek to the beginning of the tag, parse it and hopefully obtain an index */
      seek_offset = demux->file_size - sizeof (guint32);
      GST_DEBUG_OBJECT (demux,
          "File size obtained, seeking to %" G_GUINT64_FORMAT, seek_offset);
    }
    GST_OBJECT_UNLOCK (demux);

    if (!building_index) {
      GST_INFO_OBJECT (demux, "Seeking to last 4 bytes at %" G_GUINT64_FORMAT,
          seek_offset);
      return flv_demux_seek_to_offset (demux, seek_offset);
    }

    /* FIXME: we have to always return true so that we don't block the seek
     * thread.
     * Note: maybe it is OK to return true if we're still building the index */
    return TRUE;
  }

  return flv_demux_handle_seek_push (demux, event);
}

static gboolean
gst_flv_demux_handle_seek_pull (GstFlvDemux * demux, GstEvent * event,
    gboolean seeking)
{
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType start_type, stop_type;
  gint64 start, stop;
  gdouble rate;
  gboolean update, flush, ret = FALSE;
  GstSegment seeksegment;

  gst_event_parse_seek (event, &rate, &format, &flags,
      &start_type, &start, &stop_type, &stop);

  if (format != GST_FORMAT_TIME)
    goto wrong_format;

  /* mark seeking thread entering flushing/pausing */
  GST_OBJECT_LOCK (demux);
  if (seeking)
    demux->seeking = seeking;
  GST_OBJECT_UNLOCK (demux);

  flush = ! !(flags & GST_SEEK_FLAG_FLUSH);

  if (flush) {
    /* Flush start up and downstream to make sure data flow and loops are
       idle */
    gst_flv_demux_push_src_event (demux, gst_event_new_flush_start ());
    gst_pad_push_event (demux->sinkpad, gst_event_new_flush_start ());
  } else {
    /* Pause the pulling task */
    gst_pad_pause_task (demux->sinkpad);
  }

  /* Take the stream lock */
  GST_PAD_STREAM_LOCK (demux->sinkpad);

  if (flush) {
    /* Stop flushing upstream we need to pull */
    gst_pad_push_event (demux->sinkpad, gst_event_new_flush_stop (TRUE));
  }

  /* Work on a copy until we are sure the seek succeeded. */
  memcpy (&seeksegment, &demux->segment, sizeof (GstSegment));

  GST_DEBUG_OBJECT (demux, "segment before configure %" GST_SEGMENT_FORMAT,
      &demux->segment);

  /* Apply the seek to our segment */
  gst_segment_do_seek (&seeksegment, rate, format, flags,
      start_type, start, stop_type, stop, &update);

  GST_DEBUG_OBJECT (demux, "segment configured %" GST_SEGMENT_FORMAT,
      &seeksegment);

  if (flush || seeksegment.position != demux->segment.position) {
    /* Do the actual seeking */
    /* index is reliable if it is complete or we do not go to far ahead */
    if (seeking && !demux->indexed &&
        seeksegment.position > demux->index_max_time + 10 * GST_SECOND) {
      GST_DEBUG_OBJECT (demux, "delaying seek to post-scan; "
          " index only up to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->index_max_time));
      /* stop flushing for now */
      if (flush)
        gst_flv_demux_push_src_event (demux, gst_event_new_flush_stop (TRUE));
      /* delegate scanning and index building to task thread to avoid
       * occupying main (UI) loop */
      if (demux->seek_event)
        gst_event_unref (demux->seek_event);
      demux->seek_event = gst_event_ref (event);
      demux->seek_time = seeksegment.position;
      demux->state = FLV_STATE_SEEK;
      /* do not know about success yet, but we did care and handled it */
      ret = TRUE;
      goto exit;
    }

    /* now index should be as reliable as it can be for current purpose */
    gst_flv_demux_move_to_offset (demux,
        gst_flv_demux_find_offset (demux, &seeksegment, flags), TRUE);
    ret = TRUE;
  } else {
    ret = TRUE;
  }

  if (flush) {
    /* Stop flushing, the sinks are at time 0 now */
    gst_flv_demux_push_src_event (demux, gst_event_new_flush_stop (TRUE));
  }

  if (ret) {
    /* Ok seek succeeded, take the newly configured segment */
    memcpy (&demux->segment, &seeksegment, sizeof (GstSegment));

    /* Notify about the start of a new segment */
    if (demux->segment.flags & GST_SEGMENT_FLAG_SEGMENT) {
      gst_element_post_message (GST_ELEMENT (demux),
          gst_message_new_segment_start (GST_OBJECT (demux),
              demux->segment.format, demux->segment.position));
    }

    gst_flow_combiner_reset (demux->flowcombiner);
    /* Tell all the stream a new segment is needed */
    demux->audio_need_segment = TRUE;
    demux->video_need_segment = TRUE;
    /* Clean any potential newsegment event kept for the streams. The first
     * stream needing a new segment will create a new one. */
    if (G_UNLIKELY (demux->new_seg_event)) {
      gst_event_unref (demux->new_seg_event);
      demux->new_seg_event = NULL;
    }
    GST_DEBUG_OBJECT (demux, "preparing newsegment from %"
        GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (demux->segment.start),
        GST_TIME_ARGS (demux->segment.stop));
    demux->new_seg_event = gst_event_new_segment (&demux->segment);
  }

exit:
  GST_OBJECT_LOCK (demux);
  seeking = demux->seeking && !seeking;
  demux->seeking = FALSE;
  GST_OBJECT_UNLOCK (demux);

  /* if we detect an external seek having started (and possibly already having
   * flushed), do not restart task to give it a chance.
   * Otherwise external one's flushing will take care to pause task */
  if (seeking) {
    gst_pad_pause_task (demux->sinkpad);
  } else {
    gst_pad_start_task (demux->sinkpad,
        (GstTaskFunction) gst_flv_demux_loop, demux->sinkpad, NULL);
  }

  GST_PAD_STREAM_UNLOCK (demux->sinkpad);

  gst_event_unref (event);
  return ret;

  /* ERRORS */
wrong_format:
  {
    GST_WARNING_OBJECT (demux, "we only support seeking in TIME format");
    gst_event_unref (event);
    return ret;
  }
}

/* If we can pull that's preferred */
static gboolean
gst_flv_demux_sink_activate (GstPad * sinkpad, GstObject * parent)
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
gst_flv_demux_sink_activate_mode (GstPad * sinkpad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  gboolean res;
  GstFlvDemux *demux;

  demux = GST_FLV_DEMUX (parent);

  switch (mode) {
    case GST_PAD_MODE_PUSH:
      demux->random_access = FALSE;
      res = TRUE;
      break;
    case GST_PAD_MODE_PULL:
      if (active) {
        demux->random_access = TRUE;
        res = gst_pad_start_task (sinkpad, (GstTaskFunction) gst_flv_demux_loop,
            sinkpad, NULL);
      } else {
        demux->random_access = FALSE;
        res = gst_pad_stop_task (sinkpad);
      }
      break;
    default:
      res = FALSE;
      break;
  }
  return res;
}

static gboolean
gst_flv_demux_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstFlvDemux *demux;
  gboolean ret = FALSE;

  demux = GST_FLV_DEMUX (parent);

  GST_DEBUG_OBJECT (demux, "handling event %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
      GST_DEBUG_OBJECT (demux, "trying to force chain function to exit");
      demux->flushing = TRUE;
      ret = gst_flv_demux_push_src_event (demux, event);
      break;
    case GST_EVENT_FLUSH_STOP:
      GST_DEBUG_OBJECT (demux, "flushing FLV demuxer");
      gst_flv_demux_flush (demux, TRUE);
      ret = gst_flv_demux_push_src_event (demux, event);
      break;
    case GST_EVENT_EOS:
    {
      GstIndex *index;

      GST_DEBUG_OBJECT (demux, "received EOS");

      index = gst_flv_demux_get_index (GST_ELEMENT (demux));

      if (index) {
        GST_DEBUG_OBJECT (demux, "committing index");
        gst_index_commit (index, demux->index_id);
        gst_object_unref (index);
      }

      if (!demux->audio_pad && !demux->video_pad) {
        GST_ELEMENT_ERROR (demux, STREAM, FAILED,
            ("Internal data stream error."), ("Got EOS before any data"));
        gst_event_unref (event);
      } else {
        if (!demux->no_more_pads) {
          gst_element_no_more_pads (GST_ELEMENT (demux));
          demux->no_more_pads = TRUE;
        }

        if (!gst_flv_demux_push_src_event (demux, event))
          GST_WARNING_OBJECT (demux, "failed pushing EOS on streams");
      }
      ret = TRUE;
      break;
    }
    case GST_EVENT_SEGMENT:
    {
      GstSegment in_segment;

      GST_DEBUG_OBJECT (demux, "received new segment");

      gst_event_copy_segment (event, &in_segment);

      if (in_segment.format == GST_FORMAT_TIME) {
        /* time segment, this is perfect, copy over the values. */
        memcpy (&demux->segment, &in_segment, sizeof (in_segment));

        GST_DEBUG_OBJECT (demux, "NEWSEGMENT: %" GST_SEGMENT_FORMAT,
            &demux->segment);

        /* and forward */
        ret = gst_flv_demux_push_src_event (demux, event);
      } else {
        /* non-time format */
        demux->audio_need_segment = TRUE;
        demux->video_need_segment = TRUE;
        ret = TRUE;
        gst_event_unref (event);
        if (demux->new_seg_event) {
          gst_event_unref (demux->new_seg_event);
          demux->new_seg_event = NULL;
        }
      }
      gst_flow_combiner_reset (demux->flowcombiner);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

static gboolean
gst_flv_demux_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstFlvDemux *demux;
  gboolean ret = FALSE;

  demux = GST_FLV_DEMUX (parent);

  GST_DEBUG_OBJECT (demux, "handling event %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      /* Try to push upstream first */
      gst_event_ref (event);
      ret = gst_pad_push_event (demux->sinkpad, event);
      if (ret) {
        gst_event_unref (event);
        break;
      }
      if (demux->random_access) {
        ret = gst_flv_demux_handle_seek_pull (demux, event, TRUE);
      } else {
        ret = gst_flv_demux_handle_seek_push (demux, event);
      }
      break;
    default:
      ret = gst_pad_push_event (demux->sinkpad, event);
      break;
  }

  return ret;
}

static gboolean
gst_flv_demux_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res = TRUE;
  GstFlvDemux *demux;

  demux = GST_FLV_DEMUX (parent);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      /* duration is time only */
      if (format != GST_FORMAT_TIME) {
        GST_DEBUG_OBJECT (demux, "duration query only supported for time "
            "format");
        res = FALSE;
        goto beach;
      }

      /* Try to push upstream first */
      res = gst_pad_peer_query (demux->sinkpad, query);
      if (res)
        goto beach;

      GST_DEBUG_OBJECT (pad, "duration query, replying %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->duration));

      gst_query_set_duration (query, GST_FORMAT_TIME, demux->duration);
      res = TRUE;
      break;
    }
    case GST_QUERY_POSITION:
    {
      GstFormat format;

      gst_query_parse_position (query, &format, NULL);

      /* position is time only */
      if (format != GST_FORMAT_TIME) {
        GST_DEBUG_OBJECT (demux, "position query only supported for time "
            "format");
        res = FALSE;
        goto beach;
      }

      GST_DEBUG_OBJECT (pad, "position query, replying %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->segment.position));

      gst_query_set_position (query, GST_FORMAT_TIME, demux->segment.position);

      break;
    }

    case GST_QUERY_SEEKING:{
      GstFormat fmt;

      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);

      /* First ask upstream */
      if (fmt == GST_FORMAT_TIME && gst_pad_peer_query (demux->sinkpad, query)) {
        gboolean seekable;

        gst_query_parse_seeking (query, NULL, &seekable, NULL, NULL);
        if (seekable) {
          res = TRUE;
          break;
        }
      }
      res = TRUE;
      /* FIXME, check index this way is not thread safe */
      if (fmt != GST_FORMAT_TIME || !demux->index) {
        gst_query_set_seeking (query, fmt, FALSE, -1, -1);
      } else if (demux->random_access) {
        gst_query_set_seeking (query, GST_FORMAT_TIME, TRUE, 0,
            demux->duration);
      } else {
        GstQuery *peerquery = gst_query_new_seeking (GST_FORMAT_BYTES);
        gboolean seekable = gst_pad_peer_query (demux->sinkpad, peerquery);

        if (seekable)
          gst_query_parse_seeking (peerquery, NULL, &seekable, NULL, NULL);
        gst_query_unref (peerquery);

        if (seekable)
          gst_query_set_seeking (query, GST_FORMAT_TIME, seekable, 0,
              demux->duration);
        else
          gst_query_set_seeking (query, GST_FORMAT_TIME, FALSE, -1, -1);
      }
      break;
    }
    case GST_QUERY_SEGMENT:
    {
      GstFormat format;
      gint64 start, stop;

      format = demux->segment.format;

      start =
          gst_segment_to_stream_time (&demux->segment, format,
          demux->segment.start);
      if ((stop = demux->segment.stop) == -1)
        stop = demux->segment.duration;
      else
        stop = gst_segment_to_stream_time (&demux->segment, format, stop);

      gst_query_set_segment (query, demux->segment.rate, format, start, stop);
      res = TRUE;
      break;
    }
    case GST_QUERY_LATENCY:
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

beach:

  return res;
}

static GstStateChangeReturn
gst_flv_demux_change_state (GstElement * element, GstStateChange transition)
{
  GstFlvDemux *demux;
  GstStateChangeReturn ret;

  demux = GST_FLV_DEMUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      /* If this is our own index destroy it as the
       * old entries might be wrong for the new stream */
      if (demux->own_index) {
        gst_object_unref (demux->index);
        demux->index = NULL;
        demux->own_index = FALSE;
      }

      /* If no index was created, generate one */
      if (G_UNLIKELY (!demux->index)) {
        GST_DEBUG_OBJECT (demux, "no index provided creating our own");

        demux->index = g_object_new (gst_mem_index_get_type (), NULL);

        gst_index_get_writer_id (demux->index, GST_OBJECT (demux),
            &demux->index_id);
        demux->own_index = TRUE;
      }
      gst_flv_demux_cleanup (demux);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_flv_demux_cleanup (demux);
      break;
    default:
      break;
  }

  return ret;
}

#if 0
static void
gst_flv_demux_set_index (GstElement * element, GstIndex * index)
{
  GstFlvDemux *demux = GST_FLV_DEMUX (element);
  GstIndex *old_index;

  GST_OBJECT_LOCK (demux);

  old_index = demux->index;

  if (index) {
    demux->index = gst_object_ref (index);
    demux->own_index = FALSE;
  } else
    demux->index = NULL;

  if (old_index)
    gst_object_unref (demux->index);

  gst_object_ref (index);

  GST_OBJECT_UNLOCK (demux);

  /* object lock might be taken again */
  if (index)
    gst_index_get_writer_id (index, GST_OBJECT (element), &demux->index_id);

  GST_DEBUG_OBJECT (demux, "Set index %" GST_PTR_FORMAT, demux->index);

  gst_object_unref (index);
}
#endif

static GstIndex *
gst_flv_demux_get_index (GstElement * element)
{
  GstIndex *result = NULL;

  GstFlvDemux *demux = GST_FLV_DEMUX (element);

  GST_OBJECT_LOCK (demux);
  if (demux->index)
    result = gst_object_ref (demux->index);
  GST_OBJECT_UNLOCK (demux);

  return result;
}

static void
gst_flv_demux_dispose (GObject * object)
{
  GstFlvDemux *demux = GST_FLV_DEMUX (object);

  GST_DEBUG_OBJECT (demux, "disposing FLV demuxer");

  if (demux->adapter) {
    gst_adapter_clear (demux->adapter);
    g_object_unref (demux->adapter);
    demux->adapter = NULL;
  }

  if (demux->taglist) {
    gst_tag_list_unref (demux->taglist);
    demux->taglist = NULL;
  }

  if (demux->audio_tags) {
    gst_tag_list_unref (demux->audio_tags);
    demux->audio_tags = NULL;
  }

  if (demux->video_tags) {
    gst_tag_list_unref (demux->video_tags);
    demux->video_tags = NULL;
  }

  if (demux->flowcombiner) {
    gst_flow_combiner_free (demux->flowcombiner);
    demux->flowcombiner = NULL;
  }

  if (demux->new_seg_event) {
    gst_event_unref (demux->new_seg_event);
    demux->new_seg_event = NULL;
  }

  if (demux->audio_codec_data) {
    gst_buffer_unref (demux->audio_codec_data);
    demux->audio_codec_data = NULL;
  }

  if (demux->video_codec_data) {
    gst_buffer_unref (demux->video_codec_data);
    demux->video_codec_data = NULL;
  }

  if (demux->audio_pad) {
    gst_object_unref (demux->audio_pad);
    demux->audio_pad = NULL;
  }

  if (demux->video_pad) {
    gst_object_unref (demux->video_pad);
    demux->video_pad = NULL;
  }

  if (demux->index) {
    gst_object_unref (demux->index);
    demux->index = NULL;
  }

  if (demux->times) {
    g_array_free (demux->times, TRUE);
    demux->times = NULL;
  }

  if (demux->filepositions) {
    g_array_free (demux->filepositions, TRUE);
    demux->filepositions = NULL;
  }

  GST_CALL_PARENT (G_OBJECT_CLASS, dispose, (object));
}

static void
gst_flv_demux_class_init (GstFlvDemuxClass * klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = gst_flv_demux_dispose;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_flv_demux_change_state);

#if 0
  gstelement_class->set_index = GST_DEBUG_FUNCPTR (gst_flv_demux_set_index);
  gstelement_class->get_index = GST_DEBUG_FUNCPTR (gst_flv_demux_get_index);
#endif

  gst_element_class_add_static_pad_template (gstelement_class,
      &flv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &audio_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &video_src_template);
  gst_element_class_set_static_metadata (gstelement_class, "FLV Demuxer",
      "Codec/Demuxer", "Demux FLV feeds into digital streams",
      "Julien Moutte <julien@moutte.net>");
}

static void
gst_flv_demux_init (GstFlvDemux * demux)
{
  demux->sinkpad =
      gst_pad_new_from_static_template (&flv_sink_template, "sink");

  gst_pad_set_event_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_sink_event));
  gst_pad_set_chain_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_chain));
  gst_pad_set_activate_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_sink_activate));
  gst_pad_set_activatemode_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_sink_activate_mode));

  gst_element_add_pad (GST_ELEMENT (demux), demux->sinkpad);

  demux->adapter = gst_adapter_new ();
  demux->flowcombiner = gst_flow_combiner_new ();

  demux->own_index = FALSE;

  GST_OBJECT_FLAG_SET (demux, GST_ELEMENT_FLAG_INDEXABLE);

  gst_flv_demux_cleanup (demux);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (flvdemux_debug, "flvdemux", 0, "FLV demuxer");

  if (!gst_element_register (plugin, "flvdemux", GST_RANK_PRIMARY,
          gst_flv_demux_get_type ()) ||
      !gst_element_register (plugin, "flvmux", GST_RANK_PRIMARY,
          gst_flv_mux_get_type ()))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR,
    flv, "FLV muxing and demuxing plugin",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
