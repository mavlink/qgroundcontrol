/* GStreamer Push File Source
 * Copyright (C) <2007> Tim-Philipp Müller <tim centricular net>
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
 * SECTION:element-pushfilesrc
 * @title: pushfilesrc
 * @see_also: filesrc
 *
 * This element is only useful for debugging purposes. It implements an URI
 * protocol handler for the 'pushfile' protocol and behaves like a file source
 * element that cannot be activated in pull-mode. This makes it very easy to
 * debug demuxers or decoders that can operate both pull and push-based in
 * connection with the playbin element (which creates a source based on the
 * URI passed).
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -m playbin uri=pushfile:///home/you/some/file.ogg
 * ]| This plays back the given file using playbin, with the demuxer operating
 * push-based.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstpushfilesrc.h"

#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC (pushfilesrc_debug);
#define GST_CAT_DEFAULT pushfilesrc_debug

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_TIME_SEGMENT,
  PROP_STREAM_TIME,
  PROP_START_TIME,
  PROP_INITIAL_TIMESTAMP,
  PROP_RATE,
  PROP_APPLIED_RATE
};

#define DEFAULT_TIME_SEGMENT FALSE
#define DEFAULT_STREAM_TIME 0
#define DEFAULT_START_TIME 0
#define DEFAULT_INITIAL_TIMESTAMP GST_CLOCK_TIME_NONE
#define DEFAULT_RATE 1.0
#define DEFAULT_APPLIED_RATE 1.0

static void gst_push_file_src_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_push_file_src_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_push_file_src_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

#define gst_push_file_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstPushFileSrc, gst_push_file_src, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_push_file_src_uri_handler_init));

static void
gst_push_file_src_dispose (GObject * obj)
{
  GstPushFileSrc *src = GST_PUSH_FILE_SRC (obj);

  if (src->srcpad) {
    gst_element_remove_pad (GST_ELEMENT (src), src->srcpad);
    src->srcpad = NULL;
  }
  if (src->filesrc) {
    gst_bin_remove (GST_BIN (src), src->filesrc);
    src->filesrc = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gst_push_file_src_class_init (GstPushFileSrcClass * g_class)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;

  gobject_class = G_OBJECT_CLASS (g_class);
  element_class = GST_ELEMENT_CLASS (g_class);

  GST_DEBUG_CATEGORY_INIT (pushfilesrc_debug, "pushfilesrc", 0,
      "pushfilesrc element");

  gobject_class->dispose = gst_push_file_src_dispose;
  gobject_class->set_property = gst_push_file_src_set_property;
  gobject_class->get_property = gst_push_file_src_get_property;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "File Location",
          "Location of the file to read", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_READY));

  g_object_class_install_property (gobject_class, PROP_TIME_SEGMENT,
      g_param_spec_boolean ("time-segment", "Time Segment",
          "Emit TIME SEGMENTS", DEFAULT_TIME_SEGMENT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_STREAM_TIME,
      g_param_spec_int64 ("stream-time", "Stream Time",
          "Initial Stream Time (if time-segment TRUE)", 0, G_MAXINT64,
          DEFAULT_STREAM_TIME, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_START_TIME,
      g_param_spec_int64 ("start-time", "Start Time",
          "Initial Start Time (if time-segment TRUE)", 0, G_MAXINT64,
          DEFAULT_START_TIME, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INITIAL_TIMESTAMP,
      g_param_spec_uint64 ("initial-timestamp", "Initial Timestamp",
          "Initial Buffer Timestamp (if time-segment TRUE)", 0, G_MAXUINT64,
          DEFAULT_INITIAL_TIMESTAMP, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RATE,
      g_param_spec_double ("rate", "Rate", "Rate to use in TIME SEGMENT",
          G_MINDOUBLE, G_MAXDOUBLE, DEFAULT_RATE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_APPLIED_RATE,
      g_param_spec_double ("applied-rate", "Applied Rate",
          "Applied rate to use in TIME SEGMENT", G_MINDOUBLE, G_MAXDOUBLE,
          DEFAULT_APPLIED_RATE, G_PARAM_READWRITE));

  gst_element_class_add_static_pad_template (element_class, &srctemplate);

  gst_element_class_set_static_metadata (element_class, "Push File Source",
      "Testing",
      "Implements pushfile:// URI-handler for push-based file access",
      "Tim-Philipp Müller <tim centricular net>");
}

static void
gst_push_file_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstPushFileSrc *src = (GstPushFileSrc *) object;

  switch (prop_id) {
    case PROP_LOCATION:
      g_object_set_property (G_OBJECT (src->filesrc), "location", value);
      break;
    case PROP_TIME_SEGMENT:
      src->time_segment = g_value_get_boolean (value);
      break;
    case PROP_STREAM_TIME:
      src->stream_time = g_value_get_int64 (value);
      break;
    case PROP_START_TIME:
      src->start_time = g_value_get_int64 (value);
      break;
    case PROP_INITIAL_TIMESTAMP:
      src->initial_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_RATE:
      src->rate = g_value_get_double (value);
      break;
    case PROP_APPLIED_RATE:
      src->applied_rate = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_push_file_src_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstPushFileSrc *src = (GstPushFileSrc *) object;

  switch (prop_id) {
    case PROP_LOCATION:
      g_object_get_property (G_OBJECT (src->filesrc), "location", value);
      break;
    case PROP_TIME_SEGMENT:
      g_value_set_boolean (value, src->time_segment);
      break;
    case PROP_STREAM_TIME:
      g_value_set_int64 (value, src->stream_time);
      break;
    case PROP_START_TIME:
      g_value_set_int64 (value, src->start_time);
      break;
    case PROP_INITIAL_TIMESTAMP:
      g_value_set_uint64 (value, src->initial_timestamp);
      break;
    case PROP_RATE:
      g_value_set_double (value, src->rate);
      break;
    case PROP_APPLIED_RATE:
      g_value_set_double (value, src->applied_rate);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstPadProbeReturn
gst_push_file_src_ghostpad_buffer_probe (GstPad * pad, GstPadProbeInfo * info,
    GstPushFileSrc * src)
{
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);

  if (src->time_segment && !src->seen_first_buffer) {
    GST_BUFFER_TIMESTAMP (buffer) = src->initial_timestamp;
    src->seen_first_buffer = TRUE;
  }
  return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
gst_push_file_src_ghostpad_event_probe (GstPad * pad, GstPadProbeInfo * info,
    GstPushFileSrc * src)
{
  GstEvent *event = GST_PAD_PROBE_INFO_EVENT (info);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    {
      if (src->time_segment) {
        GstSegment segment;
        GstEvent *replacement;
        GST_DEBUG_OBJECT (src, "Replacing outgoing segment with TIME SEGMENT");
        gst_segment_init (&segment, GST_FORMAT_TIME);
        segment.start = src->start_time;
        segment.time = src->stream_time;
        segment.rate = src->rate;
        segment.applied_rate = src->applied_rate;
        replacement = gst_event_new_segment (&segment);
        gst_event_unref (event);
        GST_PAD_PROBE_INFO_DATA (info) = replacement;
      }
    }
    default:
      break;
  }
  return GST_PAD_PROBE_OK;
}

static gboolean
gst_push_file_src_ghostpad_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstPushFileSrc *src = (GstPushFileSrc *) parent;
  gboolean ret;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      if (src->time_segment) {
        /* When working in time we don't allow seeks */
        GST_DEBUG_OBJECT (src, "Refusing seek event in TIME mode");
        gst_event_unref (event);
        ret = FALSE;
        break;
      }
      /* PASSTHROUGH */
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

static gboolean
gst_push_file_src_ghostpad_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstPushFileSrc *src = (GstPushFileSrc *) parent;
  gboolean res;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_SCHEDULING:
      /* When working in time we don't allow seeks */
      if (src->time_segment)
        gst_query_set_scheduling (query, GST_SCHEDULING_FLAG_SEQUENTIAL, 1, -1,
            0);
      else
        gst_query_set_scheduling (query, GST_SCHEDULING_FLAG_SEEKABLE, 1, -1,
            0);
      gst_query_add_scheduling_mode (query, GST_PAD_MODE_PUSH);
      res = TRUE;
      break;
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }
  return res;
}

static void
gst_push_file_src_init (GstPushFileSrc * src)
{
  src->time_segment = DEFAULT_TIME_SEGMENT;
  src->stream_time = DEFAULT_STREAM_TIME;
  src->start_time = DEFAULT_START_TIME;
  src->initial_timestamp = DEFAULT_INITIAL_TIMESTAMP;
  src->rate = DEFAULT_RATE;
  src->applied_rate = DEFAULT_APPLIED_RATE;
  src->seen_first_buffer = FALSE;

  src->filesrc = gst_element_factory_make ("filesrc", "real-filesrc");
  if (src->filesrc) {
    GstPad *pad;

    gst_bin_add (GST_BIN (src), src->filesrc);
    pad = gst_element_get_static_pad (src->filesrc, "src");
    g_assert (pad != NULL);
    src->srcpad = gst_ghost_pad_new ("src", pad);
    /* FIXME^H^HCORE: try pushfile:///foo/bar.ext ! typefind ! fakesink without
     * this and watch core bugginess (some pad stays in flushing state) */
    gst_pad_set_query_function (src->srcpad,
        GST_DEBUG_FUNCPTR (gst_push_file_src_ghostpad_query));
    gst_pad_set_event_function (src->srcpad,
        GST_DEBUG_FUNCPTR (gst_push_file_src_ghostpad_event));
    /* Add outgoing event probe to replace segment and buffer timestamp */
    gst_pad_add_probe (src->srcpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        (GstPadProbeCallback) gst_push_file_src_ghostpad_event_probe,
        src, NULL);
    gst_pad_add_probe (src->srcpad, GST_PAD_PROBE_TYPE_BUFFER,
        (GstPadProbeCallback) gst_push_file_src_ghostpad_buffer_probe,
        src, NULL);
    gst_element_add_pad (GST_ELEMENT (src), src->srcpad);
    gst_object_unref (pad);
  }
}

/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_push_file_src_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_push_file_src_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "pushfile", NULL };

  return protocols;
}

static gchar *
gst_push_file_src_uri_get_uri (GstURIHandler * handler)
{
  GstPushFileSrc *src = GST_PUSH_FILE_SRC (handler);
  gchar *fileuri, *pushfileuri;

  if (src->filesrc == NULL)
    return NULL;

  fileuri = gst_uri_handler_get_uri (GST_URI_HANDLER (src->filesrc));
  if (fileuri == NULL)
    return NULL;
  pushfileuri = g_strconcat ("push", fileuri, NULL);
  g_free (fileuri);

  return pushfileuri;
}

static gboolean
gst_push_file_src_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  GstPushFileSrc *src = GST_PUSH_FILE_SRC (handler);

  if (src->filesrc == NULL) {
    g_set_error_literal (error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
        "Could not create file source element");
    return FALSE;
  }

  /* skip 'push' bit */
  return gst_uri_handler_set_uri (GST_URI_HANDLER (src->filesrc), uri + 4,
      error);
}

static void
gst_push_file_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_push_file_src_uri_get_type;
  iface->get_protocols = gst_push_file_src_uri_get_protocols;
  iface->get_uri = gst_push_file_src_uri_get_uri;
  iface->set_uri = gst_push_file_src_uri_set_uri;
}
