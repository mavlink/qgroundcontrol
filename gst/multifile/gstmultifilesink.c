/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2006 Wim Taymans <wim@fluendo.com>
 *                    2006 David A. Schleef <ds@schleef.org>
 *                    2011 Collabora Ltd. <tim.muller@collabora.co.uk>
 *                    2015 Tim-Philipp Müller <tim@centricular.com>
 *
 * gstmultifilesink.c:
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
 * SECTION:element-multifilesink
 * @title: multifilesink
 * @see_also: #GstFileSrc
 *
 * Write incoming data to a series of sequentially-named files.
 *
 * This element is usually used with data where each buffer is an
 * independent unit of data in its own right (e.g. raw video buffers or
 * encoded JPEG or PNG images) or with streamable container formats such
 * as MPEG-TS or MPEG-PS.
 *
 * It is not possible to use this element to create independently playable
 * mp4 files, use the splitmuxsink element for that instead.
 *
 * The filename property should contain a string with a \%d placeholder that will
 * be substituted with the index for each filename.
 *
 * If the #GstMultiFileSink:post-messages property is %TRUE, it sends an application
 * message named `GstMultiFileSink` after writing each buffer.
 *
 * The message's structure contains these fields:
 *
 * * #gchararray `filename`: the filename where the buffer was written.
 * * #gint `index`: index of the buffer.
 * * #GstClockTime `timestamp`: the timestamp of the buffer.
 * * #GstClockTime `stream-time`: the stream time of the buffer.
 * * #GstClockTime running-time`: the running_time of the buffer.
 * * #GstClockTime `duration`: the duration of the buffer.
 * * #guint64 `offset`: the offset of the buffer that triggered the message.
 * * #guint64 `offset-end`: the offset-end of the buffer that triggered the message.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc ! multifilesink
 * gst-launch-1.0 videotestsrc ! multifilesink post-messages=true location="frame%d"
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <glib/gstdio.h>
#include "gstmultifilesink.h"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_multi_file_sink_debug);
#define GST_CAT_DEFAULT gst_multi_file_sink_debug

#define DEFAULT_LOCATION "%05d"
#define DEFAULT_INDEX 0
#define DEFAULT_POST_MESSAGES FALSE
#define DEFAULT_NEXT_FILE GST_MULTI_FILE_SINK_NEXT_BUFFER
#define DEFAULT_MAX_FILES 0
#define DEFAULT_MAX_FILE_SIZE G_GUINT64_CONSTANT(2*1024*1024*1024)
#define DEFAULT_MAX_FILE_DURATION GST_CLOCK_TIME_NONE
#define DEFAULT_AGGREGATE_GOPS FALSE

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_INDEX,
  PROP_POST_MESSAGES,
  PROP_NEXT_FILE,
  PROP_MAX_FILES,
  PROP_MAX_FILE_SIZE,
  PROP_MAX_FILE_DURATION,
  PROP_AGGREGATE_GOPS
};

static void gst_multi_file_sink_finalize (GObject * object);

static void gst_multi_file_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_multi_file_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_multi_file_sink_start (GstBaseSink * bsink);
static gboolean gst_multi_file_sink_stop (GstBaseSink * sink);
static GstFlowReturn gst_multi_file_sink_render (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn gst_multi_file_sink_render_list (GstBaseSink * sink,
    GstBufferList * buffer_list);
static gboolean gst_multi_file_sink_set_caps (GstBaseSink * sink,
    GstCaps * caps);
static gboolean gst_multi_file_sink_open_next_file (GstMultiFileSink *
    multifilesink);
static void gst_multi_file_sink_close_file (GstMultiFileSink * multifilesink,
    GstBuffer * buffer);
static void gst_multi_file_sink_add_old_file (GstMultiFileSink * multifilesink,
    gchar * fn);
static void gst_multi_file_sink_ensure_max_files (GstMultiFileSink *
    multifilesink);
static gboolean gst_multi_file_sink_event (GstBaseSink * sink,
    GstEvent * event);

#define GST_TYPE_MULTI_FILE_SINK_NEXT (gst_multi_file_sink_next_get_type ())
static GType
gst_multi_file_sink_next_get_type (void)
{
  static GType multi_file_sink_next_type = 0;
  static const GEnumValue next_types[] = {
    {GST_MULTI_FILE_SINK_NEXT_BUFFER, "New file for each buffer", "buffer"},
    {GST_MULTI_FILE_SINK_NEXT_DISCONT, "New file after each discontinuity",
        "discont"},
    {GST_MULTI_FILE_SINK_NEXT_KEY_FRAME, "New file at each key frame "
          "(Useful for MPEG-TS segmenting)", "key-frame"},
    {GST_MULTI_FILE_SINK_NEXT_KEY_UNIT_EVENT,
        "New file after a force key unit event", "key-unit-event"},
    {GST_MULTI_FILE_SINK_NEXT_MAX_SIZE, "New file when the configured maximum "
          "file size would be exceeded with the next buffer or buffer list",
        "max-size"},
    {GST_MULTI_FILE_SINK_NEXT_MAX_DURATION,
          "New file when the configured maximum "
          "file duration would be exceeded with the next buffer or buffer list",
        "max-duration"},
    {0, NULL, NULL}
  };

  if (!multi_file_sink_next_type) {
    multi_file_sink_next_type =
        g_enum_register_static ("GstMultiFileSinkNext", next_types);
  }

  return multi_file_sink_next_type;
}

#define gst_multi_file_sink_parent_class parent_class
G_DEFINE_TYPE (GstMultiFileSink, gst_multi_file_sink, GST_TYPE_BASE_SINK);

static void
gst_multi_file_sink_class_init (GstMultiFileSinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstBaseSinkClass *gstbasesink_class = GST_BASE_SINK_CLASS (klass);

  gobject_class->set_property = gst_multi_file_sink_set_property;
  gobject_class->get_property = gst_multi_file_sink_get_property;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "File Location",
          "Location of the file to write", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INDEX,
      g_param_spec_int ("index", "Index",
          "Index to use with location property to create file names.  The "
          "index is incremented by one for each buffer written.",
          0, G_MAXINT, DEFAULT_INDEX,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstMultiFileSink:post-messages:
   *
   * Post a message on the GstBus for each file.
   */
  g_object_class_install_property (gobject_class, PROP_POST_MESSAGES,
      g_param_spec_boolean ("post-messages", "Post Messages",
          "Post a message for each file with information of the buffer",
          DEFAULT_POST_MESSAGES, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstMultiFileSink:next-file:
   *
   * When to start a new file.
   */
  g_object_class_install_property (gobject_class, PROP_NEXT_FILE,
      g_param_spec_enum ("next-file", "Next File",
          "When to start a new file",
          GST_TYPE_MULTI_FILE_SINK_NEXT, DEFAULT_NEXT_FILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


  /**
   * GstMultiFileSink:max-files:
   *
   * Maximum number of files to keep on disk. Once the maximum is reached, old
   * files start to be deleted to make room for new ones.
   */
  g_object_class_install_property (gobject_class, PROP_MAX_FILES,
      g_param_spec_uint ("max-files", "Max files",
          "Maximum number of files to keep on disk. Once the maximum is reached,"
          "old files start to be deleted to make room for new ones.",
          0, G_MAXUINT, DEFAULT_MAX_FILES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstMultiFileSink:max-file-size:
   *
   * Maximum file size before starting a new file in max-size mode.
   */
  g_object_class_install_property (gobject_class, PROP_MAX_FILE_SIZE,
      g_param_spec_uint64 ("max-file-size", "Maximum File Size",
          "Maximum file size before starting a new file in max-size mode",
          0, G_MAXUINT64, DEFAULT_MAX_FILE_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstMultiFileSink:max-file-duration:
   *
   * Maximum file size before starting a new file in max-size mode.
   */
  g_object_class_install_property (gobject_class, PROP_MAX_FILE_DURATION,
      g_param_spec_uint64 ("max-file-duration", "Maximum File Duration",
          "Maximum file duration before starting a new file in max-duration mode "
          "(in nanoseconds)", 0, G_MAXUINT64, DEFAULT_MAX_FILE_DURATION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstMultiFileSink:aggregate-gops:
   *
   * Whether to aggregate complete GOPs before doing any processing. Set this
   * to TRUE to make sure each new file starts with a keyframe. This requires
   * the upstream element to flag buffers containing key units and delta
   * units correctly. At least the MPEG-PS and MPEG-TS muxers should be doing
   * this.
   *
   * Since: 1.6
   */
  g_object_class_install_property (gobject_class, PROP_AGGREGATE_GOPS,
      g_param_spec_boolean ("aggregate-gops", "Aggregate GOPs",
          "Whether to aggregate GOPs and process them as a whole without "
          "splitting", DEFAULT_AGGREGATE_GOPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gobject_class->finalize = gst_multi_file_sink_finalize;

  gstbasesink_class->start = GST_DEBUG_FUNCPTR (gst_multi_file_sink_start);
  gstbasesink_class->stop = GST_DEBUG_FUNCPTR (gst_multi_file_sink_stop);
  gstbasesink_class->render = GST_DEBUG_FUNCPTR (gst_multi_file_sink_render);
  gstbasesink_class->render_list =
      GST_DEBUG_FUNCPTR (gst_multi_file_sink_render_list);
  gstbasesink_class->set_caps =
      GST_DEBUG_FUNCPTR (gst_multi_file_sink_set_caps);
  gstbasesink_class->event = GST_DEBUG_FUNCPTR (gst_multi_file_sink_event);

  GST_DEBUG_CATEGORY_INIT (gst_multi_file_sink_debug, "multifilesink", 0,
      "multifilesink element");

  gst_element_class_add_static_pad_template (gstelement_class, &sinktemplate);
  gst_element_class_set_static_metadata (gstelement_class, "Multi-File Sink",
      "Sink/File",
      "Write buffers to a sequentially named set of files",
      "David Schleef <ds@schleef.org>");
}

static void
gst_multi_file_sink_init (GstMultiFileSink * multifilesink)
{
  multifilesink->filename = g_strdup (DEFAULT_LOCATION);
  multifilesink->index = DEFAULT_INDEX;
  multifilesink->post_messages = DEFAULT_POST_MESSAGES;
  multifilesink->max_files = DEFAULT_MAX_FILES;
  multifilesink->max_file_size = DEFAULT_MAX_FILE_SIZE;
  multifilesink->max_file_duration = DEFAULT_MAX_FILE_DURATION;

  multifilesink->aggregate_gops = DEFAULT_AGGREGATE_GOPS;
  multifilesink->gop_adapter = NULL;

  gst_base_sink_set_sync (GST_BASE_SINK (multifilesink), FALSE);

  multifilesink->next_segment = GST_CLOCK_TIME_NONE;
  multifilesink->force_key_unit_count = -1;
}

static void
gst_multi_file_sink_finalize (GObject * object)
{
  GstMultiFileSink *sink = GST_MULTI_FILE_SINK (object);

  g_free (sink->filename);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_multi_file_sink_set_location (GstMultiFileSink * sink,
    const gchar * location)
{
  g_free (sink->filename);
  /* FIXME: validate location to have just one %d */
  sink->filename = g_strdup (location);

  return TRUE;
}

static void
gst_multi_file_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMultiFileSink *sink = GST_MULTI_FILE_SINK (object);

  switch (prop_id) {
    case PROP_LOCATION:
      gst_multi_file_sink_set_location (sink, g_value_get_string (value));
      break;
    case PROP_INDEX:
      sink->index = g_value_get_int (value);
      break;
    case PROP_POST_MESSAGES:
      sink->post_messages = g_value_get_boolean (value);
      break;
    case PROP_NEXT_FILE:
      sink->next_file = g_value_get_enum (value);
      break;
    case PROP_MAX_FILES:
      sink->max_files = g_value_get_uint (value);
      break;
    case PROP_MAX_FILE_SIZE:
      sink->max_file_size = g_value_get_uint64 (value);
      break;
    case PROP_MAX_FILE_DURATION:
      sink->max_file_duration = g_value_get_uint64 (value);
      break;
    case PROP_AGGREGATE_GOPS:
      sink->aggregate_gops = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_multi_file_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstMultiFileSink *sink = GST_MULTI_FILE_SINK (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_set_string (value, sink->filename);
      break;
    case PROP_INDEX:
      g_value_set_int (value, sink->index);
      break;
    case PROP_POST_MESSAGES:
      g_value_set_boolean (value, sink->post_messages);
      break;
    case PROP_NEXT_FILE:
      g_value_set_enum (value, sink->next_file);
      break;
    case PROP_MAX_FILES:
      g_value_set_uint (value, sink->max_files);
      break;
    case PROP_MAX_FILE_SIZE:
      g_value_set_uint64 (value, sink->max_file_size);
      break;
    case PROP_MAX_FILE_DURATION:
      g_value_set_uint64 (value, sink->max_file_duration);
      break;
    case PROP_AGGREGATE_GOPS:
      g_value_set_boolean (value, sink->aggregate_gops);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_multi_file_sink_start (GstBaseSink * bsink)
{
  GstMultiFileSink *sink = GST_MULTI_FILE_SINK (bsink);

  if (sink->aggregate_gops)
    sink->gop_adapter = gst_adapter_new ();
  sink->potential_next_gop = NULL;
  sink->file_pts = GST_CLOCK_TIME_NONE;

  g_queue_init (&sink->old_files);

  return TRUE;
}

static gboolean
gst_multi_file_sink_stop (GstBaseSink * sink)
{
  GstMultiFileSink *multifilesink;
  int i;

  multifilesink = GST_MULTI_FILE_SINK (sink);

  if (multifilesink->file != NULL) {
    fclose (multifilesink->file);
    multifilesink->file = NULL;
  }

  if (multifilesink->streamheaders) {
    for (i = 0; i < multifilesink->n_streamheaders; i++) {
      gst_buffer_unref (multifilesink->streamheaders[i]);
    }
    g_free (multifilesink->streamheaders);
    multifilesink->streamheaders = NULL;
  }

  if (multifilesink->gop_adapter != NULL) {
    g_object_unref (multifilesink->gop_adapter);
    multifilesink->gop_adapter = NULL;
  }

  if (multifilesink->potential_next_gop != NULL) {
    g_list_free_full (multifilesink->potential_next_gop,
        (GDestroyNotify) gst_buffer_unref);
    multifilesink->potential_next_gop = NULL;
  }

  multifilesink->force_key_unit_count = -1;

  g_queue_foreach (&multifilesink->old_files, (GFunc) g_free, NULL);
  g_queue_clear (&multifilesink->old_files);

  return TRUE;
}


static void
gst_multi_file_sink_post_message_full (GstMultiFileSink * multifilesink,
    GstClockTime timestamp, GstClockTime duration, GstClockTime offset,
    GstClockTime offset_end, GstClockTime running_time,
    GstClockTime stream_time, const char *filename)
{
  GstStructure *s;

  if (!multifilesink->post_messages)
    return;

  s = gst_structure_new ("GstMultiFileSink",
      "filename", G_TYPE_STRING, filename,
      "index", G_TYPE_INT, multifilesink->index,
      "timestamp", G_TYPE_UINT64, timestamp,
      "stream-time", G_TYPE_UINT64, stream_time,
      "running-time", G_TYPE_UINT64, running_time,
      "duration", G_TYPE_UINT64, duration,
      "offset", G_TYPE_UINT64, offset,
      "offset-end", G_TYPE_UINT64, offset_end, NULL);

  gst_element_post_message (GST_ELEMENT_CAST (multifilesink),
      gst_message_new_element (GST_OBJECT_CAST (multifilesink), s));
}

static void
gst_multi_file_sink_post_message_from_time (GstMultiFileSink * multifilesink,
    GstClockTime timestamp, GstClockTime duration, const char *filename)
{
  GstClockTime running_time, stream_time;
  guint64 offset, offset_end;
  GstSegment *segment;
  GstFormat format;

  if (!multifilesink->post_messages)
    return;

  segment = &GST_BASE_SINK (multifilesink)->segment;
  format = segment->format;

  offset = -1;
  offset_end = -1;

  running_time = gst_segment_to_running_time (segment, format, timestamp);
  stream_time = gst_segment_to_stream_time (segment, format, timestamp);

  gst_multi_file_sink_post_message_full (multifilesink, timestamp, duration,
      offset, offset_end, running_time, stream_time, filename);
}

static void
gst_multi_file_sink_post_message (GstMultiFileSink * multifilesink,
    GstBuffer * buffer, const char *filename)
{
  GstClockTime duration, timestamp;
  GstClockTime running_time, stream_time;
  guint64 offset, offset_end;
  GstSegment *segment;
  GstFormat format;

  if (!multifilesink->post_messages)
    return;

  segment = &GST_BASE_SINK (multifilesink)->segment;
  format = segment->format;

  timestamp = GST_BUFFER_TIMESTAMP (buffer);
  duration = GST_BUFFER_DURATION (buffer);
  offset = GST_BUFFER_OFFSET (buffer);
  offset_end = GST_BUFFER_OFFSET_END (buffer);

  running_time = gst_segment_to_running_time (segment, format, timestamp);
  stream_time = gst_segment_to_stream_time (segment, format, timestamp);

  gst_multi_file_sink_post_message_full (multifilesink, timestamp, duration,
      offset, offset_end, running_time, stream_time, filename);
}

static gboolean
gst_multi_file_sink_write_stream_headers (GstMultiFileSink * sink)
{
  int i;

  if (sink->streamheaders == NULL)
    return TRUE;

  /* we want to write these at the beginning */
  g_assert (sink->cur_file_size == 0);

  for (i = 0; i < sink->n_streamheaders; i++) {
    GstBuffer *hdr;
    GstMapInfo map;
    int ret;

    hdr = sink->streamheaders[i];
    gst_buffer_map (hdr, &map, GST_MAP_READ);
    ret = fwrite (map.data, map.size, 1, sink->file);
    gst_buffer_unmap (hdr, &map);

    if (ret != 1)
      return FALSE;

    sink->cur_file_size += map.size;
  }

  return TRUE;
}

static GstFlowReturn
gst_multi_file_sink_write_buffer (GstMultiFileSink * multifilesink,
    GstBuffer * buffer)
{
  GstMapInfo map;
  gchar *filename;
  gboolean ret;
  GError *error = NULL;
  gboolean first_file = TRUE;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  switch (multifilesink->next_file) {
    case GST_MULTI_FILE_SINK_NEXT_BUFFER:
      gst_multi_file_sink_ensure_max_files (multifilesink);

      filename = g_strdup_printf (multifilesink->filename,
          multifilesink->index);
      ret = g_file_set_contents (filename, (char *) map.data, map.size, &error);
      if (!ret)
        goto write_error;

      gst_multi_file_sink_post_message (multifilesink, buffer, filename);

      gst_multi_file_sink_add_old_file (multifilesink, filename);

      multifilesink->index++;

      break;
    case GST_MULTI_FILE_SINK_NEXT_DISCONT:
      if (GST_BUFFER_IS_DISCONT (buffer)) {
        if (multifilesink->file)
          gst_multi_file_sink_close_file (multifilesink, buffer);
      }

      if (multifilesink->file == NULL) {
        if (!gst_multi_file_sink_open_next_file (multifilesink))
          goto stdio_write_error;
      }

      ret = fwrite (map.data, map.size, 1, multifilesink->file);
      if (ret != 1)
        goto stdio_write_error;

      break;
    case GST_MULTI_FILE_SINK_NEXT_KEY_FRAME:
      if (multifilesink->next_segment == GST_CLOCK_TIME_NONE) {
        if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer)) {
          multifilesink->next_segment = GST_BUFFER_TIMESTAMP (buffer) +
              10 * GST_SECOND;
        }
      }

      if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer) &&
          GST_BUFFER_TIMESTAMP (buffer) >= multifilesink->next_segment &&
          !GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
        if (multifilesink->file) {
          first_file = FALSE;
          gst_multi_file_sink_close_file (multifilesink, buffer);
        }
        multifilesink->next_segment += 10 * GST_SECOND;
      }

      if (multifilesink->file == NULL) {
        if (!gst_multi_file_sink_open_next_file (multifilesink))
          goto stdio_write_error;

        if (!first_file)
          gst_multi_file_sink_write_stream_headers (multifilesink);
      }

      ret = fwrite (map.data, map.size, 1, multifilesink->file);
      if (ret != 1)
        goto stdio_write_error;

      break;
    case GST_MULTI_FILE_SINK_NEXT_KEY_UNIT_EVENT:
      if (multifilesink->file == NULL) {
        if (!gst_multi_file_sink_open_next_file (multifilesink))
          goto stdio_write_error;

        /* we don't need to write stream headers here, they will be inserted in
         * the stream by upstream elements if key unit events have
         * all_headers=true set
         */
      }

      ret = fwrite (map.data, map.size, 1, multifilesink->file);

      if (ret != 1)
        goto stdio_write_error;

      break;
    case GST_MULTI_FILE_SINK_NEXT_MAX_SIZE:{
      guint64 new_size;

      new_size = multifilesink->cur_file_size + map.size;
      if (new_size > multifilesink->max_file_size) {

        GST_INFO_OBJECT (multifilesink, "current size: %" G_GUINT64_FORMAT
            ", new_size: %" G_GUINT64_FORMAT ", max. size %" G_GUINT64_FORMAT,
            multifilesink->cur_file_size, new_size,
            multifilesink->max_file_size);

        if (multifilesink->file != NULL) {
          first_file = FALSE;
          gst_multi_file_sink_close_file (multifilesink, buffer);
        }
      }

      if (multifilesink->file == NULL) {
        if (!gst_multi_file_sink_open_next_file (multifilesink))
          goto stdio_write_error;

        if (!first_file)
          gst_multi_file_sink_write_stream_headers (multifilesink);
      }

      ret = fwrite (map.data, map.size, 1, multifilesink->file);

      if (ret != 1)
        goto stdio_write_error;

      multifilesink->cur_file_size += map.size;
      break;
    }
    case GST_MULTI_FILE_SINK_NEXT_MAX_DURATION:{
      GstClockTime new_duration = 0;

      if (GST_BUFFER_PTS_IS_VALID (buffer)
          && GST_CLOCK_TIME_IS_VALID (multifilesink->file_pts)) {
        /* The new duration will extend to this new buffer pts ... */
        new_duration = GST_BUFFER_PTS (buffer) - multifilesink->file_pts;
        /* ... and duration (if it has one) */
        if (GST_BUFFER_DURATION_IS_VALID (buffer))
          new_duration += GST_BUFFER_DURATION (buffer);
      }

      if (new_duration > multifilesink->max_file_duration) {

        GST_INFO_OBJECT (multifilesink,
            "new_duration: %" G_GUINT64_FORMAT ", max. duration %"
            G_GUINT64_FORMAT, new_duration, multifilesink->max_file_duration);

        if (multifilesink->file != NULL) {
          first_file = FALSE;
          gst_multi_file_sink_close_file (multifilesink, buffer);
        }
      }

      if (multifilesink->file == NULL) {
        if (!gst_multi_file_sink_open_next_file (multifilesink))
          goto stdio_write_error;

        multifilesink->file_pts = GST_BUFFER_PTS (buffer);
        if (!first_file)
          gst_multi_file_sink_write_stream_headers (multifilesink);
      }

      ret = fwrite (map.data, map.size, 1, multifilesink->file);

      if (ret != 1)
        goto stdio_write_error;

      break;
    }
    default:
      g_assert_not_reached ();
  }

  gst_buffer_unmap (buffer, &map);
  return GST_FLOW_OK;

  /* ERRORS */
write_error:
  {
    switch (error->code) {
      case G_FILE_ERROR_NOSPC:{
        GST_ELEMENT_ERROR (multifilesink, RESOURCE, NO_SPACE_LEFT, (NULL),
            (NULL));
        break;
      }
      default:{
        GST_ELEMENT_ERROR (multifilesink, RESOURCE, WRITE,
            ("Error while writing to file \"%s\".", filename),
            ("%s", g_strerror (errno)));
      }
    }
    g_error_free (error);
    g_free (filename);

    gst_buffer_unmap (buffer, &map);
    return GST_FLOW_ERROR;
  }
stdio_write_error:
  switch (errno) {
    case ENOSPC:
      GST_ELEMENT_ERROR (multifilesink, RESOURCE, NO_SPACE_LEFT,
          ("Error while writing to file."), ("%s", g_strerror (errno)));
      break;
    default:
      GST_ELEMENT_ERROR (multifilesink, RESOURCE, WRITE,
          ("Error while writing to file."), ("%s", g_strerror (errno)));
  }
  gst_buffer_unmap (buffer, &map);
  return GST_FLOW_ERROR;
}

static GstFlowReturn
gst_multi_file_sink_render (GstBaseSink * bsink, GstBuffer * buffer)
{
  GstMultiFileSink *sink = GST_MULTI_FILE_SINK (bsink);
  GstFlowReturn flow = GST_FLOW_OK;
  gboolean key_unit, header;

  header = GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_HEADER);
  key_unit = !GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);

  if (sink->aggregate_gops) {
    GstBuffer *gop_buffer = NULL;
    guint avail;

    avail = gst_adapter_available (sink->gop_adapter);

    GST_LOG_OBJECT (sink, "aggregate GOP: received %s%s unit buffer: "
        "%" GST_PTR_FORMAT,
        (key_unit) ? "key" : "delta", (header) ? " header" : "", buffer);

    /* If it's a header buffer, it might potentially be for the next GOP */
    if (header) {
      GST_LOG_OBJECT (sink, "Accumulating buffer to potential next GOP");
      sink->potential_next_gop =
          g_list_append (sink->potential_next_gop, gst_buffer_ref (buffer));
    } else {
      if (key_unit && avail > 0) {
        GstClockTime pts, dts;
        GST_LOG_OBJECT (sink, "Grabbing pending completed GOP");
        pts = gst_adapter_prev_pts_at_offset (sink->gop_adapter, 0, NULL);
        dts = gst_adapter_prev_dts_at_offset (sink->gop_adapter, 0, NULL);
        gop_buffer = gst_adapter_take_buffer (sink->gop_adapter, avail);
        GST_BUFFER_PTS (gop_buffer) = pts;
        GST_BUFFER_DTS (gop_buffer) = dts;
      }

      /* just accumulate the buffer */
      if (sink->potential_next_gop) {
        GList *tmp;
        GST_LOG_OBJECT (sink,
            "Carrying over pending next GOP data into adapter");
        /* If we have pending data, put that first in the adapter */
        for (tmp = sink->potential_next_gop; tmp; tmp = tmp->next) {
          GstBuffer *tmpb = (GstBuffer *) tmp->data;
          gst_adapter_push (sink->gop_adapter, tmpb);
        }
        g_list_free (sink->potential_next_gop);
        sink->potential_next_gop = NULL;
      }
      GST_LOG_OBJECT (sink, "storing buffer in adapter");
      gst_adapter_push (sink->gop_adapter, gst_buffer_ref (buffer));

      if (gop_buffer != NULL) {
        GST_DEBUG_OBJECT (sink, "writing out pending GOP, %u bytes", avail);
        GST_DEBUG_OBJECT (sink,
            "gop buffer pts:%" GST_TIME_FORMAT " dts:%" GST_TIME_FORMAT
            " duration:%" GST_TIME_FORMAT,
            GST_TIME_ARGS (GST_BUFFER_PTS (gop_buffer)),
            GST_TIME_ARGS (GST_BUFFER_DTS (gop_buffer)),
            GST_TIME_ARGS (GST_BUFFER_DURATION (gop_buffer)));
        flow = gst_multi_file_sink_write_buffer (sink, gop_buffer);
        gst_buffer_unref (gop_buffer);
      }
    }
  } else {
    flow = gst_multi_file_sink_write_buffer (sink, buffer);
  }
  return flow;
}

static gboolean
buffer_list_copy_data (GstBuffer ** buf, guint idx, gpointer data)
{
  GstBuffer *dest = data;
  guint num, i;

  if (idx == 0)
    gst_buffer_copy_into (dest, *buf, GST_BUFFER_COPY_METADATA, 0, -1);

  num = gst_buffer_n_memory (*buf);
  for (i = 0; i < num; ++i) {
    GstMemory *mem;

    mem = gst_buffer_get_memory (*buf, i);
    gst_buffer_append_memory (dest, mem);
  }

  return TRUE;
}

/* Our assumption for now is that the buffers in a buffer list should always
 * end up in the same file. If someone wants different behaviour, they'll just
 * have to add a property for that. */
static GstFlowReturn
gst_multi_file_sink_render_list (GstBaseSink * sink, GstBufferList * list)
{
  GstBuffer *buf;
  guint size;

  size = gst_buffer_list_calculate_size (list);
  GST_LOG_OBJECT (sink, "total size of buffer list %p: %u", list, size);

  /* copy all buffers in the list into one single buffer, so we can use
   * the normal render function (FIXME: optimise to avoid the memcpy) */
  buf = gst_buffer_new ();
  gst_buffer_list_foreach (list, buffer_list_copy_data, buf);
  g_assert (gst_buffer_get_size (buf) == size);

  gst_multi_file_sink_render (sink, buf);
  gst_buffer_unref (buf);

  return GST_FLOW_OK;
}

static gboolean
gst_multi_file_sink_set_caps (GstBaseSink * sink, GstCaps * caps)
{
  GstMultiFileSink *multifilesink;
  GstStructure *structure;

  multifilesink = GST_MULTI_FILE_SINK (sink);

  structure = gst_caps_get_structure (caps, 0);
  if (structure) {
    const GValue *value;

    value = gst_structure_get_value (structure, "streamheader");

    if (GST_VALUE_HOLDS_ARRAY (value)) {
      int i;

      if (multifilesink->streamheaders) {
        for (i = 0; i < multifilesink->n_streamheaders; i++) {
          gst_buffer_unref (multifilesink->streamheaders[i]);
        }
        g_free (multifilesink->streamheaders);
      }

      multifilesink->n_streamheaders = gst_value_array_get_size (value);
      multifilesink->streamheaders =
          g_malloc (sizeof (GstBuffer *) * multifilesink->n_streamheaders);

      for (i = 0; i < multifilesink->n_streamheaders; i++) {
        multifilesink->streamheaders[i] =
            gst_buffer_ref (gst_value_get_buffer (gst_value_array_get_value
                (value, i)));
      }
    }
  }

  return TRUE;
}

/* Takes ownership of the filename string */
static void
gst_multi_file_sink_add_old_file (GstMultiFileSink * multifilesink, gchar * fn)
{
  /* Only add file to the list if a max_files limit is set, otherwise we never
   * prune the list and memory just builds up until the pipeline is stopped. */
  if (multifilesink->max_files > 0) {
    g_queue_push_tail (&multifilesink->old_files, fn);
  } else {
    g_free (fn);
  }
}

static void
gst_multi_file_sink_ensure_max_files (GstMultiFileSink * multifilesink)
{
  guint max_files = multifilesink->max_files;

  if (max_files == 0)
    return;

  while (g_queue_get_length (&multifilesink->old_files) >= max_files) {
    gchar *filename;

    filename = g_queue_pop_head (&multifilesink->old_files);
    g_remove (filename);
    g_free (filename);
  }
}

static gboolean
gst_multi_file_sink_event (GstBaseSink * sink, GstEvent * event)
{
  GstMultiFileSink *multifilesink;
  gchar *filename;

  multifilesink = GST_MULTI_FILE_SINK (sink);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_DOWNSTREAM:
    {
      GstClockTime timestamp, duration;
      GstClockTime running_time, stream_time;
      guint64 offset, offset_end;
      gboolean all_headers;
      guint count;

      if (multifilesink->next_file != GST_MULTI_FILE_SINK_NEXT_KEY_UNIT_EVENT ||
          !gst_video_event_is_force_key_unit (event))
        goto out;

      gst_video_event_parse_downstream_force_key_unit (event, &timestamp,
          &stream_time, &running_time, &all_headers, &count);

      if (multifilesink->force_key_unit_count != -1 &&
          multifilesink->force_key_unit_count == count)
        goto out;

      multifilesink->force_key_unit_count = count;

      if (multifilesink->file) {
        duration = GST_CLOCK_TIME_NONE;
        offset = offset_end = -1;
        filename = g_strdup_printf (multifilesink->filename,
            multifilesink->index);

        gst_multi_file_sink_close_file (multifilesink, NULL);

        gst_multi_file_sink_post_message_full (multifilesink, timestamp,
            duration, offset, offset_end, running_time, stream_time, filename);
        g_free (filename);
      }

      if (multifilesink->file == NULL) {
        if (!gst_multi_file_sink_open_next_file (multifilesink))
          goto stdio_write_error;
      }

      break;
    }
    case GST_EVENT_EOS:
      if (multifilesink->aggregate_gops) {
        GstBuffer *buf = gst_buffer_new ();

        /* push key unit buffer to force writing out the pending GOP data */
        GST_INFO_OBJECT (sink, "EOS, write pending GOP data");
        GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DELTA_UNIT);
        gst_multi_file_sink_render (sink, buf);
        gst_buffer_unref (buf);
      }
      if (multifilesink->file) {
        gchar *filename;

        filename = g_strdup_printf (multifilesink->filename,
            multifilesink->index);

        gst_multi_file_sink_close_file (multifilesink, NULL);

        gst_multi_file_sink_post_message_from_time (multifilesink,
            GST_BASE_SINK (multifilesink)->segment.position, -1, filename);
        g_free (filename);
      }
      break;
    default:
      break;
  }

out:
  return GST_BASE_SINK_CLASS (parent_class)->event (sink, event);

  /* ERRORS */
stdio_write_error:
  {
    GST_ELEMENT_ERROR (multifilesink, RESOURCE, WRITE,
        ("Error while writing to file."), (NULL));
    gst_event_unref (event);
    return FALSE;
  }
}

static gboolean
gst_multi_file_sink_open_next_file (GstMultiFileSink * multifilesink)
{
  char *filename;

  g_return_val_if_fail (multifilesink->file == NULL, FALSE);

  gst_multi_file_sink_ensure_max_files (multifilesink);

  filename = g_strdup_printf (multifilesink->filename, multifilesink->index);
  multifilesink->file = g_fopen (filename, "wb");
  if (multifilesink->file == NULL) {
    g_free (filename);
    return FALSE;
  }

  GST_INFO_OBJECT (multifilesink, "opening file %s", filename);

  gst_multi_file_sink_add_old_file (multifilesink, filename);

  multifilesink->cur_file_size = 0;
  return TRUE;
}

static void
gst_multi_file_sink_close_file (GstMultiFileSink * multifilesink,
    GstBuffer * buffer)
{
  char *filename;

  fclose (multifilesink->file);
  multifilesink->file = NULL;

  if (buffer) {
    filename = g_strdup_printf (multifilesink->filename, multifilesink->index);
    gst_multi_file_sink_post_message (multifilesink, buffer, filename);
    g_free (filename);
  }

  multifilesink->index++;
}
