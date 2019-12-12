/* GStreamer Split Demuxer bin that recombines files created by
 * the splitmuxsink element.
 *
 * Copyright (C) <2014> Jan Schmidt <jan@centricular.com>
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
 * SECTION:element-splitmuxsrc
 * @title: splitmuxsrc
 * @short_description: Split Demuxer bin that recombines files created by
 * the splitmuxsink element.
 *
 * This element reads a set of input files created by the splitmuxsink element
 * containing contiguous elementary streams split across multiple files.
 *
 * This element is similar to splitfilesrc, except that it recombines the
 * streams in each file part at the demuxed elementary level, rather than
 * as a single larger bytestream.
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 splitmuxsrc location=video*.mov ! decodebin ! xvimagesink
 * ]| Demux each file part and output the video stream as one continuous stream
 * |[
 * gst-launch-1.0 playbin uri="splitmux://path/to/foo.mp4.*"
 * ]| Play back a set of files created by splitmuxsink
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstsplitmuxsrc.h"
#include "gstsplitutils.h"

#include "../../gst-libs/gst/gst-i18n-plugin.h"

GST_DEBUG_CATEGORY (splitmux_debug);
#define GST_CAT_DEFAULT splitmux_debug

enum
{
  PROP_0,
  PROP_LOCATION
};

enum
{
  SIGNAL_FORMAT_LOCATION,
  SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static GstStaticPadTemplate video_src_template =
GST_STATIC_PAD_TEMPLATE ("video",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate video_aux_src_template =
GST_STATIC_PAD_TEMPLATE ("video_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate audio_src_template =
GST_STATIC_PAD_TEMPLATE ("audio_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate subtitle_src_template =
GST_STATIC_PAD_TEMPLATE ("subtitle_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStateChangeReturn gst_splitmux_src_change_state (GstElement *
    element, GstStateChange transition);
static void gst_splitmux_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_splitmux_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_splitmux_src_dispose (GObject * object);
static void gst_splitmux_src_finalize (GObject * object);
static gboolean gst_splitmux_src_start (GstSplitMuxSrc * splitmux);
static gboolean gst_splitmux_src_stop (GstSplitMuxSrc * splitmux);
static void splitmux_src_pad_constructed (GObject * pad);
static gboolean splitmux_src_pad_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean splitmux_src_pad_query (GstPad * pad, GstObject * parent,
    GstQuery * query);
static void splitmux_src_uri_handler_init (gpointer g_iface,
    gpointer iface_data);


static GstPad *gst_splitmux_find_output_pad (GstSplitMuxPartReader * part,
    GstPad * pad, GstSplitMuxSrc * splitmux);
static gboolean gst_splitmux_end_of_part (GstSplitMuxSrc * splitmux,
    SplitMuxSrcPad * pad);
static gboolean gst_splitmux_check_new_caps (SplitMuxSrcPad * splitpad,
    GstEvent * event);
static gboolean gst_splitmux_src_prepare_next_part (GstSplitMuxSrc * splitmux);
static gboolean gst_splitmux_src_activate_part (GstSplitMuxSrc * splitmux,
    guint part, GstSeekFlags extra_flags);

#define _do_init \
    G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, splitmux_src_uri_handler_init);
#define gst_splitmux_src_parent_class parent_class

G_DEFINE_TYPE_EXTENDED (GstSplitMuxSrc, gst_splitmux_src, GST_TYPE_BIN, 0,
    _do_init);

static GstURIType
splitmux_src_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
splitmux_src_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "splitmux", NULL };

  return protocols;
}

static gchar *
splitmux_src_uri_get_uri (GstURIHandler * handler)
{
  GstSplitMuxSrc *splitmux = GST_SPLITMUX_SRC (handler);
  gchar *ret = NULL;

  GST_OBJECT_LOCK (splitmux);
  if (splitmux->location)
    ret = g_strdup_printf ("splitmux://%s", splitmux->location);
  GST_OBJECT_UNLOCK (splitmux);
  return ret;
}

static gboolean
splitmux_src_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** err)
{
  GstSplitMuxSrc *splitmux = GST_SPLITMUX_SRC (handler);
  gchar *protocol, *location;

  protocol = gst_uri_get_protocol (uri);
  if (protocol == NULL || !g_str_equal (protocol, "splitmux"))
    goto wrong_uri;
  g_free (protocol);

  location = gst_uri_get_location (uri);
  GST_OBJECT_LOCK (splitmux);
  g_free (splitmux->location);
  splitmux->location = location;
  GST_OBJECT_UNLOCK (splitmux);

  return TRUE;

wrong_uri:
  g_free (protocol);
  GST_ELEMENT_ERROR (splitmux, RESOURCE, READ, (NULL),
      ("Error parsing uri %s", uri));
  g_set_error_literal (err, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
      "Could not parse splitmux URI");
  return FALSE;
}

static void
splitmux_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) (g_iface);

  iface->get_type = splitmux_src_uri_get_type;
  iface->get_protocols = splitmux_src_uri_get_protocols;
  iface->set_uri = splitmux_src_uri_set_uri;
  iface->get_uri = splitmux_src_uri_get_uri;
}


static void
gst_splitmux_src_class_init (GstSplitMuxSrcClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_splitmux_src_set_property;
  gobject_class->get_property = gst_splitmux_src_get_property;
  gobject_class->dispose = gst_splitmux_src_dispose;
  gobject_class->finalize = gst_splitmux_src_finalize;

  gst_element_class_set_static_metadata (gstelement_class,
      "Split File Demuxing Bin", "Generic/Bin/Demuxer",
      "Source that reads a set of files created by splitmuxsink",
      "Jan Schmidt <jan@centricular.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &video_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &video_aux_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &audio_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &subtitle_src_template);

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_splitmux_src_change_state);

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "File Input Pattern",
          "Glob pattern for the location of the files to read", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstSplitMuxSrc::format-location:
   * @splitmux: the #GstSplitMuxSrc
   *
   * Returns: A NULL-terminated sorted array of strings containing the
   *   filenames of the input files. The array will be freed internally
   *   using g_strfreev()
   *
   * Since: 1.8
   */
  signals[SIGNAL_FORMAT_LOCATION] =
      g_signal_new ("format-location", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_STRV, 0);
}

static void
gst_splitmux_src_init (GstSplitMuxSrc * splitmux)
{
  g_mutex_init (&splitmux->lock);
  g_rw_lock_init (&splitmux->pads_rwlock);
  splitmux->total_duration = GST_CLOCK_TIME_NONE;
  gst_segment_init (&splitmux->play_segment, GST_FORMAT_TIME);
}

static void
gst_splitmux_src_dispose (GObject * object)
{
  GstSplitMuxSrc *splitmux = GST_SPLITMUX_SRC (object);
  GList *cur;

  SPLITMUX_SRC_PADS_WLOCK (splitmux);

  for (cur = g_list_first (splitmux->pads);
      cur != NULL; cur = g_list_next (cur)) {
    GstPad *pad = GST_PAD (cur->data);
    gst_element_remove_pad (GST_ELEMENT (splitmux), pad);
  }
  g_list_free (splitmux->pads);
  splitmux->n_pads = 0;
  splitmux->pads = NULL;
  SPLITMUX_SRC_PADS_WUNLOCK (splitmux);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_splitmux_src_finalize (GObject * object)
{
  GstSplitMuxSrc *splitmux = GST_SPLITMUX_SRC (object);
  g_mutex_clear (&splitmux->lock);
  g_rw_lock_clear (&splitmux->pads_rwlock);
  g_free (splitmux->location);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_splitmux_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSplitMuxSrc *splitmux = GST_SPLITMUX_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:{
      GST_OBJECT_LOCK (splitmux);
      g_free (splitmux->location);
      splitmux->location = g_value_dup_string (value);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_splitmux_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSplitMuxSrc *splitmux = GST_SPLITMUX_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      GST_OBJECT_LOCK (splitmux);
      g_value_set_string (value, splitmux->location);
      GST_OBJECT_UNLOCK (splitmux);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
do_async_start (GstSplitMuxSrc * splitmux)
{
  GstMessage *message;

  GST_STATE_LOCK (splitmux);
  splitmux->async_pending = TRUE;

  message = gst_message_new_async_start (GST_OBJECT_CAST (splitmux));
  GST_BIN_CLASS (parent_class)->handle_message (GST_BIN_CAST (splitmux),
      message);
  GST_STATE_UNLOCK (splitmux);
}

static void
do_async_done (GstSplitMuxSrc * splitmux)
{
  GstMessage *message;

  GST_STATE_LOCK (splitmux);
  if (splitmux->async_pending) {
    message =
        gst_message_new_async_done (GST_OBJECT_CAST (splitmux),
        GST_CLOCK_TIME_NONE);
    GST_BIN_CLASS (parent_class)->handle_message (GST_BIN_CAST (splitmux),
        message);

    splitmux->async_pending = FALSE;
  }
  GST_STATE_UNLOCK (splitmux);
}

static GstStateChangeReturn
gst_splitmux_src_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstSplitMuxSrc *splitmux = (GstSplitMuxSrc *) element;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:{
      break;
    }
    case GST_STATE_CHANGE_READY_TO_PAUSED:{
      do_async_start (splitmux);

      if (!gst_splitmux_src_start (splitmux)) {
        do_async_done (splitmux);
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    case GST_STATE_CHANGE_READY_TO_NULL:
      /* Make sure the element will shut down */
      if (!gst_splitmux_src_stop (splitmux))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    do_async_done (splitmux);
    return ret;
  }

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      ret = GST_STATE_CHANGE_ASYNC;
      break;
    default:
      break;
  }


  return ret;
}

static void
gst_splitmux_src_activate_first_part (GstSplitMuxSrc * splitmux)
{
  if (!gst_splitmux_src_activate_part (splitmux, 0, GST_SEEK_FLAG_NONE)) {
    GST_ELEMENT_ERROR (splitmux, RESOURCE, OPEN_READ, (NULL),
        ("Failed to activate first part for playback"));
  }
}

static GstBusSyncReply
gst_splitmux_part_bus_handler (GstBus * bus, GstMessage * msg,
    gpointer user_data)
{
  GstSplitMuxSrc *splitmux = user_data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ASYNC_DONE:{
      guint idx = splitmux->num_prepared_parts;
      gboolean need_no_more_pads;

      if (idx >= splitmux->num_parts) {
        /* Shouldn't really happen! */
        do_async_done (splitmux);
        g_warn_if_reached ();
        break;
      }

      GST_DEBUG_OBJECT (splitmux, "Prepared file part %s (%u)",
          splitmux->parts[idx]->path, idx);

      /* signal no-more-pads as we have all pads at this point now */
      SPLITMUX_SRC_LOCK (splitmux);
      need_no_more_pads = !splitmux->pads_complete;
      splitmux->pads_complete = TRUE;
      SPLITMUX_SRC_UNLOCK (splitmux);

      if (need_no_more_pads) {
        GST_DEBUG_OBJECT (splitmux, "Signalling no-more-pads");
        gst_element_no_more_pads (GST_ELEMENT_CAST (splitmux));
      }

      /* Extend our total duration to cover this part */
      GST_OBJECT_LOCK (splitmux);
      splitmux->total_duration +=
          gst_splitmux_part_reader_get_duration (splitmux->parts[idx]);
      splitmux->play_segment.duration = splitmux->total_duration;
      GST_OBJECT_UNLOCK (splitmux);

      splitmux->end_offset =
          gst_splitmux_part_reader_get_end_offset (splitmux->parts[idx]);

      GST_DEBUG_OBJECT (splitmux,
          "Duration %" GST_TIME_FORMAT ", total duration now: %" GST_TIME_FORMAT
          " and end offset %" GST_TIME_FORMAT,
          GST_TIME_ARGS (gst_splitmux_part_reader_get_duration (splitmux->parts
                  [idx])), GST_TIME_ARGS (splitmux->total_duration),
          GST_TIME_ARGS (splitmux->end_offset));

      splitmux->num_prepared_parts++;

      /* If we're done or preparing the next part fails, finish here */
      if (splitmux->num_prepared_parts >= splitmux->num_parts
          || !gst_splitmux_src_prepare_next_part (splitmux)) {
        /* Store how many parts we actually prepared in the end */
        splitmux->num_parts = splitmux->num_prepared_parts;
        do_async_done (splitmux);

        /* All done preparing, activate the first part */
        GST_INFO_OBJECT (splitmux,
            "All parts prepared. Total duration %" GST_TIME_FORMAT
            " Activating first part", GST_TIME_ARGS (splitmux->total_duration));
        gst_element_call_async (GST_ELEMENT_CAST (splitmux),
            (GstElementCallAsyncFunc) gst_splitmux_src_activate_first_part,
            NULL, NULL);
      }

      break;
    }
    case GST_MESSAGE_ERROR:{
      GST_ERROR_OBJECT (splitmux,
          "Got error message from part %" GST_PTR_FORMAT ": %" GST_PTR_FORMAT,
          GST_MESSAGE_SRC (msg), msg);
      if (splitmux->num_prepared_parts < splitmux->num_parts) {
        guint idx = splitmux->num_prepared_parts;

        if (idx == 0) {
          GST_ERROR_OBJECT (splitmux,
              "Failed to prepare first file part %s for playback",
              splitmux->parts[idx]->path);
          GST_ELEMENT_ERROR (splitmux, RESOURCE, OPEN_READ, (NULL),
              ("Failed to prepare first file part %s for playback",
                  splitmux->parts[idx]->path));
        } else {
          GST_WARNING_OBJECT (splitmux,
              "Failed to prepare file part %s. Cannot play past there.",
              splitmux->parts[idx]->path);
          GST_ELEMENT_WARNING (splitmux, RESOURCE, READ, (NULL),
              ("Failed to prepare file part %s. Cannot play past there.",
                  splitmux->parts[idx]->path));
        }

        /* Store how many parts we actually prepared in the end */
        splitmux->num_parts = splitmux->num_prepared_parts;
        do_async_done (splitmux);

        if (idx > 0) {
          /* All done preparing, activate the first part */
          GST_INFO_OBJECT (splitmux,
              "All parts prepared. Total duration %" GST_TIME_FORMAT
              " Activating first part",
              GST_TIME_ARGS (splitmux->total_duration));
          gst_element_call_async (GST_ELEMENT_CAST (splitmux),
              (GstElementCallAsyncFunc) gst_splitmux_src_activate_first_part,
              NULL, NULL);
        }
      } else {
        /* Need to update the message source so that it's part of the element
         * hierarchy the application would expect */
        msg = gst_message_copy (msg);
        gst_object_replace ((GstObject **) & msg->src, (GstObject *) splitmux);
        gst_element_post_message (GST_ELEMENT_CAST (splitmux), msg);
      }
      break;
    }
    default:
      break;
  }

  return GST_BUS_PASS;
}

static GstSplitMuxPartReader *
gst_splitmux_part_create (GstSplitMuxSrc * splitmux, char *filename)
{
  GstSplitMuxPartReader *r;
  GstBus *bus;

  r = g_object_new (GST_TYPE_SPLITMUX_PART_READER, NULL);

  gst_splitmux_part_reader_set_callbacks (r, splitmux,
      (GstSplitMuxPartReaderPadCb) gst_splitmux_find_output_pad);
  gst_splitmux_part_reader_set_location (r, filename);

  bus = gst_element_get_bus (GST_ELEMENT_CAST (r));
  gst_bus_set_sync_handler (bus, gst_splitmux_part_bus_handler, splitmux, NULL);
  gst_object_unref (bus);

  return r;
}

static gboolean
gst_splitmux_check_new_caps (SplitMuxSrcPad * splitpad, GstEvent * event)
{
  GstCaps *curcaps = gst_pad_get_current_caps ((GstPad *) (splitpad));
  GstCaps *newcaps;
  GstCaps *tmpcaps;
  GstCaps *tmpcurcaps;

  GstStructure *s;
  gboolean res = TRUE;

  gst_event_parse_caps (event, &newcaps);

  GST_LOG_OBJECT (splitpad, "Comparing caps %" GST_PTR_FORMAT
      " and %" GST_PTR_FORMAT, curcaps, newcaps);

  if (curcaps == NULL)
    return TRUE;

  /* If caps are exactly equal exit early */
  if (gst_caps_is_equal (curcaps, newcaps)) {
    gst_caps_unref (curcaps);
    return FALSE;
  }

  /* More extensive check, ignore changes in framerate, because
   * demuxers get that wrong */
  tmpcaps = gst_caps_copy (newcaps);
  s = gst_caps_get_structure (tmpcaps, 0);
  gst_structure_remove_field (s, "framerate");

  tmpcurcaps = gst_caps_copy (curcaps);
  gst_caps_unref (curcaps);
  s = gst_caps_get_structure (tmpcurcaps, 0);
  gst_structure_remove_field (s, "framerate");

  /* Now check if these filtered caps are equal */
  if (gst_caps_is_equal (tmpcurcaps, tmpcaps)) {
    GST_INFO_OBJECT (splitpad, "Ignoring framerate-only caps change");
    res = FALSE;
  }

  gst_caps_unref (tmpcaps);
  gst_caps_unref (tmpcurcaps);
  return res;
}

static void
gst_splitmux_handle_event (GstSplitMuxSrc * splitmux,
    SplitMuxSrcPad * splitpad, GstPad * part_pad, GstEvent * event)
{
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_STREAM_START:{
      if (splitpad->sent_stream_start)
        goto drop_event;
      splitpad->sent_stream_start = TRUE;
      break;
    }
    case GST_EVENT_EOS:{
      if (gst_splitmux_end_of_part (splitmux, splitpad))
        // Continuing to next part, drop the EOS
        goto drop_event;
      if (splitmux->segment_seqnum) {
        event = gst_event_make_writable (event);
        gst_event_set_seqnum (event, splitmux->segment_seqnum);
      }
      break;
    }
    case GST_EVENT_SEGMENT:{
      GstClockTime duration;
      GstSegment seg;

      gst_event_copy_segment (event, &seg);

      splitpad->segment.position = seg.position;

      if (splitpad->sent_segment)
        goto drop_event;        /* We already forwarded a segment event */

      /* Calculate output segment */
      GST_LOG_OBJECT (splitpad, "Pad seg %" GST_SEGMENT_FORMAT
          " got seg %" GST_SEGMENT_FORMAT
          " play seg %" GST_SEGMENT_FORMAT,
          &splitpad->segment, &seg, &splitmux->play_segment);

      /* If playing forward, take the stop time from the overall
       * seg or play_segment */
      if (splitmux->play_segment.rate > 0.0) {
        if (splitmux->play_segment.stop != -1)
          seg.stop = splitmux->play_segment.stop;
        else
          seg.stop = splitpad->segment.stop;
      } else {
        /* Reverse playback from stop time to start time */
        /* See if an end point was requested in the seek */
        if (splitmux->play_segment.start != -1) {
          seg.start = splitmux->play_segment.start;
          seg.time = splitmux->play_segment.time;
        } else {
          seg.start = splitpad->segment.start;
          seg.time = splitpad->segment.time;
        }
      }

      GST_OBJECT_LOCK (splitmux);
      duration = splitmux->total_duration;
      GST_OBJECT_UNLOCK (splitmux);

      if (duration > 0)
        seg.duration = duration;
      else
        seg.duration = GST_CLOCK_TIME_NONE;

      GST_INFO_OBJECT (splitpad,
          "Forwarding segment %" GST_SEGMENT_FORMAT, &seg);

      gst_event_unref (event);
      event = gst_event_new_segment (&seg);
      if (splitmux->segment_seqnum)
        gst_event_set_seqnum (event, splitmux->segment_seqnum);
      splitpad->sent_segment = TRUE;
      break;
    }
    case GST_EVENT_CAPS:{
      if (!gst_splitmux_check_new_caps (splitpad, event))
        goto drop_event;
      splitpad->sent_caps = TRUE;
      break;
    }
    default:
      break;
  }

  gst_pad_push_event ((GstPad *) (splitpad), event);
  return;
drop_event:
  gst_event_unref (event);
  return;
}

static GstFlowReturn
gst_splitmux_handle_buffer (GstSplitMuxSrc * splitmux,
    SplitMuxSrcPad * splitpad, GstBuffer * buf)
{
  GstFlowReturn ret;

  if (splitpad->clear_next_discont) {
    GST_LOG_OBJECT (splitpad, "Clearing discont flag on buffer");
    GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DISCONT);
    splitpad->clear_next_discont = FALSE;
  }
  if (splitpad->set_next_discont) {
    GST_LOG_OBJECT (splitpad, "Setting discont flag on buffer");
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
    splitpad->set_next_discont = FALSE;
  }

  ret = gst_pad_push (GST_PAD_CAST (splitpad), buf);

  GST_LOG_OBJECT (splitpad, "Pad push returned %d", ret);
  return ret;
}

static guint
count_not_linked (GstSplitMuxSrc * splitmux)
{
  GList *cur;
  guint ret = 0;

  for (cur = g_list_first (splitmux->pads);
      cur != NULL; cur = g_list_next (cur)) {
    SplitMuxSrcPad *splitpad = (SplitMuxSrcPad *) (cur->data);
    if (GST_PAD_LAST_FLOW_RETURN (splitpad) == GST_FLOW_NOT_LINKED)
      ret++;
  }

  return ret;
}

static void
gst_splitmux_pad_loop (GstPad * pad)
{
  /* Get one event/buffer from the associated part and push */
  SplitMuxSrcPad *splitpad = (SplitMuxSrcPad *) (pad);
  GstSplitMuxSrc *splitmux = (GstSplitMuxSrc *) gst_pad_get_parent (pad);
  GstDataQueueItem *item = NULL;
  GstSplitMuxPartReader *reader;
  GstPad *part_pad;
  GstFlowReturn ret;

  GST_OBJECT_LOCK (splitpad);
  if (splitpad->part_pad == NULL) {
    GST_OBJECT_UNLOCK (splitpad);
    return;
  }
  part_pad = gst_object_ref (splitpad->part_pad);
  reader = splitpad->reader;
  GST_OBJECT_UNLOCK (splitpad);

  GST_LOG_OBJECT (splitpad, "Popping data queue item from %" GST_PTR_FORMAT
      " pad %" GST_PTR_FORMAT, reader, part_pad);
  ret = gst_splitmux_part_reader_pop (reader, part_pad, &item);
  if (ret == GST_FLOW_ERROR)
    goto error;
  if (ret == GST_FLOW_FLUSHING || item == NULL)
    goto flushing;

  GST_DEBUG_OBJECT (splitpad, "Got data queue item %" GST_PTR_FORMAT,
      item->object);

  if (GST_IS_EVENT (item->object)) {
    GstEvent *event = (GstEvent *) (item->object);
    gst_splitmux_handle_event (splitmux, splitpad, part_pad, event);
  } else {
    GstBuffer *buf = (GstBuffer *) (item->object);
    GstFlowReturn ret = gst_splitmux_handle_buffer (splitmux, splitpad, buf);
    if (G_UNLIKELY (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)) {
      /* Stop immediately on error or flushing */
      GST_INFO_OBJECT (splitpad, "Stopping due to pad_push() result %d", ret);
      gst_pad_pause_task (pad);
      if (ret < GST_FLOW_EOS) {
        GST_ELEMENT_FLOW_ERROR (splitmux, ret);
      } else if (ret == GST_FLOW_NOT_LINKED) {
        gboolean post_error;
        guint n_notlinked;

        /* Only post not-linked if all pads are not-linked */
        SPLITMUX_SRC_PADS_RLOCK (splitmux);
        n_notlinked = count_not_linked (splitmux);
        post_error = (splitmux->pads_complete
            && n_notlinked == splitmux->n_pads);
        SPLITMUX_SRC_PADS_RUNLOCK (splitmux);

        if (post_error)
          GST_ELEMENT_FLOW_ERROR (splitmux, ret);
      }
    }
  }
  g_slice_free (GstDataQueueItem, item);

  gst_object_unref (part_pad);
  gst_object_unref (splitmux);
  return;

error:
  /* Fall through */
  GST_ELEMENT_ERROR (splitmux, RESOURCE, OPEN_READ, (NULL),
      ("Error reading part file %s", GST_STR_NULL (reader->path)));
flushing:
  gst_pad_pause_task (pad);
  gst_object_unref (part_pad);
  gst_object_unref (splitmux);
  return;
}

static gboolean
gst_splitmux_src_activate_part (GstSplitMuxSrc * splitmux, guint part,
    GstSeekFlags extra_flags)
{
  GList *cur;

  GST_DEBUG_OBJECT (splitmux, "Activating part %d", part);

  splitmux->cur_part = part;
  if (!gst_splitmux_part_reader_activate (splitmux->parts[part],
          &splitmux->play_segment, extra_flags))
    return FALSE;

  SPLITMUX_SRC_PADS_RLOCK (splitmux);
  for (cur = g_list_first (splitmux->pads);
      cur != NULL; cur = g_list_next (cur)) {
    SplitMuxSrcPad *splitpad = (SplitMuxSrcPad *) (cur->data);
    GST_OBJECT_LOCK (splitpad);
    splitpad->cur_part = part;
    splitpad->reader = splitmux->parts[splitpad->cur_part];
    if (splitpad->part_pad)
      gst_object_unref (splitpad->part_pad);
    splitpad->part_pad =
        gst_splitmux_part_reader_lookup_pad (splitpad->reader,
        (GstPad *) (splitpad));
    GST_OBJECT_UNLOCK (splitpad);

    /* Make sure we start with a DISCONT */
    splitpad->set_next_discont = TRUE;
    splitpad->clear_next_discont = FALSE;

    gst_pad_start_task (GST_PAD (splitpad),
        (GstTaskFunction) gst_splitmux_pad_loop, splitpad, NULL);
  }
  SPLITMUX_SRC_PADS_RUNLOCK (splitmux);

  return TRUE;
}

static gboolean
gst_splitmux_src_prepare_next_part (GstSplitMuxSrc * splitmux)
{
  guint idx = splitmux->num_prepared_parts;

  g_assert (idx < splitmux->num_parts);

  GST_DEBUG_OBJECT (splitmux, "Preparing file part %s (%u)",
      splitmux->parts[idx]->path, idx);

  gst_splitmux_part_reader_set_start_offset (splitmux->parts[idx],
      splitmux->end_offset);
  if (!gst_splitmux_part_reader_prepare (splitmux->parts[idx])) {
    GST_WARNING_OBJECT (splitmux,
        "Failed to prepare file part %s. Cannot play past there.",
        splitmux->parts[idx]->path);
    GST_ELEMENT_WARNING (splitmux, RESOURCE, READ, (NULL),
        ("Failed to prepare file part %s. Cannot play past there.",
            splitmux->parts[idx]->path));
    gst_splitmux_part_reader_unprepare (splitmux->parts[idx]);
    g_object_unref (splitmux->parts[idx]);
    splitmux->parts[idx] = NULL;
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_splitmux_src_start (GstSplitMuxSrc * splitmux)
{
  gboolean ret = FALSE;
  GError *err = NULL;
  gchar *basename = NULL;
  gchar *dirname = NULL;
  gchar **files;
  guint i;

  GST_DEBUG_OBJECT (splitmux, "Starting");

  g_signal_emit (splitmux, signals[SIGNAL_FORMAT_LOCATION], 0, &files);

  if (files == NULL || *files == NULL) {
    GST_OBJECT_LOCK (splitmux);
    if (splitmux->location != NULL && splitmux->location[0] != '\0') {
      basename = g_path_get_basename (splitmux->location);
      dirname = g_path_get_dirname (splitmux->location);
    }
    GST_OBJECT_UNLOCK (splitmux);

    g_strfreev (files);
    files = gst_split_util_find_files (dirname, basename, &err);

    if (files == NULL || *files == NULL)
      goto no_files;
  }

  SPLITMUX_SRC_LOCK (splitmux);
  splitmux->pads_complete = FALSE;
  splitmux->running = TRUE;
  SPLITMUX_SRC_UNLOCK (splitmux);

  splitmux->num_parts = g_strv_length (files);

  splitmux->parts = g_new0 (GstSplitMuxPartReader *, splitmux->num_parts);

  /* Create all part pipelines */
  for (i = 0; i < splitmux->num_parts; i++) {
    splitmux->parts[i] = gst_splitmux_part_create (splitmux, files[i]);
    if (splitmux->parts[i] == NULL)
      break;
  }

  /* Store how many parts we actually created */
  splitmux->num_created_parts = splitmux->num_parts = i;
  splitmux->num_prepared_parts = 0;

  /* Update total_duration state variable */
  GST_OBJECT_LOCK (splitmux);
  splitmux->total_duration = 0;
  splitmux->end_offset = 0;
  GST_OBJECT_UNLOCK (splitmux);

  /* Then start the first: it will asynchronously go to PAUSED
   * or error out and then we can proceed with the next one
   */
  if (!gst_splitmux_src_prepare_next_part (splitmux) || splitmux->num_parts < 1)
    goto failed_part;

  /* All good now: we have to wait for all parts to be asynchronously
   * prepared to know the total duration we can play */
  ret = TRUE;

done:
  if (err != NULL)
    g_error_free (err);
  g_strfreev (files);
  g_free (basename);
  g_free (dirname);

  return ret;

/* ERRORS */
no_files:
  {
    GST_ELEMENT_ERROR (splitmux, RESOURCE, OPEN_READ, ("%s", err->message),
        ("Failed to find files in '%s' for pattern '%s'",
            GST_STR_NULL (dirname), GST_STR_NULL (basename)));
    goto done;
  }
failed_part:
  {
    GST_ELEMENT_ERROR (splitmux, RESOURCE, OPEN_READ, (NULL),
        ("Failed to open any files for reading"));
    goto done;
  }
}

static gboolean
gst_splitmux_src_stop (GstSplitMuxSrc * splitmux)
{
  gboolean ret = TRUE;
  guint i;
  GList *cur, *pads_list;

  SPLITMUX_SRC_LOCK (splitmux);
  if (!splitmux->running)
    goto out;

  GST_DEBUG_OBJECT (splitmux, "Stopping");

  /* Stop and destroy all parts  */
  for (i = 0; i < splitmux->num_created_parts; i++) {
    if (splitmux->parts[i] == NULL)
      continue;
    gst_splitmux_part_reader_unprepare (splitmux->parts[i]);
    g_object_unref (splitmux->parts[i]);
    splitmux->parts[i] = NULL;
  }

  SPLITMUX_SRC_PADS_WLOCK (splitmux);
  pads_list = splitmux->pads;
  splitmux->pads = NULL;
  SPLITMUX_SRC_PADS_WUNLOCK (splitmux);

  SPLITMUX_SRC_UNLOCK (splitmux);
  for (cur = g_list_first (pads_list); cur != NULL; cur = g_list_next (cur)) {
    SplitMuxSrcPad *tmp = (SplitMuxSrcPad *) (cur->data);
    gst_pad_stop_task (GST_PAD (tmp));
    gst_element_remove_pad (GST_ELEMENT (splitmux), GST_PAD (tmp));
  }
  g_list_free (pads_list);
  SPLITMUX_SRC_LOCK (splitmux);

  g_free (splitmux->parts);
  splitmux->parts = NULL;
  splitmux->num_parts = 0;
  splitmux->num_prepared_parts = 0;
  splitmux->num_created_parts = 0;
  splitmux->running = FALSE;
  splitmux->total_duration = GST_CLOCK_TIME_NONE;
  /* Reset playback segment */
  gst_segment_init (&splitmux->play_segment, GST_FORMAT_TIME);
out:
  SPLITMUX_SRC_UNLOCK (splitmux);
  return ret;
}

typedef struct
{
  GstSplitMuxSrc *splitmux;
  SplitMuxSrcPad *splitpad;
} SplitMuxAndPad;

static gboolean
handle_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  SplitMuxAndPad *splitmux_and_pad;
  GstSplitMuxSrc *splitmux;
  SplitMuxSrcPad *splitpad;

  splitmux_and_pad = user_data;
  splitmux = splitmux_and_pad->splitmux;
  splitpad = splitmux_and_pad->splitpad;

  GST_DEBUG_OBJECT (splitpad, "handle sticky event %" GST_PTR_FORMAT, *event);
  gst_event_ref (*event);
  gst_splitmux_handle_event (splitmux, splitpad, pad, *event);

  return TRUE;
}

static GstPad *
gst_splitmux_find_output_pad (GstSplitMuxPartReader * part, GstPad * pad,
    GstSplitMuxSrc * splitmux)
{
  GList *cur;
  gchar *pad_name = gst_pad_get_name (pad);
  GstPad *target = NULL;
  gboolean is_new_pad = FALSE;

  SPLITMUX_SRC_LOCK (splitmux);
  SPLITMUX_SRC_PADS_WLOCK (splitmux);
  for (cur = g_list_first (splitmux->pads);
      cur != NULL; cur = g_list_next (cur)) {
    GstPad *tmp = (GstPad *) (cur->data);
    if (g_str_equal (GST_PAD_NAME (tmp), pad_name)) {
      target = tmp;
      break;
    }
  }

  if (target == NULL && !splitmux->pads_complete) {
    SplitMuxAndPad splitmux_and_pad;

    /* No pad found, create one */
    target = g_object_new (SPLITMUX_TYPE_SRC_PAD,
        "name", pad_name, "direction", GST_PAD_SRC, NULL);
    splitmux->pads = g_list_prepend (splitmux->pads, target);
    splitmux->n_pads++;

    gst_pad_set_active (target, TRUE);

    splitmux_and_pad.splitmux = splitmux;
    splitmux_and_pad.splitpad = (SplitMuxSrcPad *) target;
    gst_pad_sticky_events_foreach (pad, handle_sticky_events,
        &splitmux_and_pad);
    is_new_pad = TRUE;
  }
  SPLITMUX_SRC_PADS_WUNLOCK (splitmux);
  SPLITMUX_SRC_UNLOCK (splitmux);

  g_free (pad_name);

  if (target == NULL)
    goto pad_not_found;

  if (is_new_pad)
    gst_element_add_pad (GST_ELEMENT_CAST (splitmux), target);

  return target;

pad_not_found:
  GST_ELEMENT_ERROR (splitmux, STREAM, FAILED, (NULL),
      ("Stream part %s contains extra unknown pad %" GST_PTR_FORMAT,
          part->path, pad));
  return NULL;
}

static void
gst_splitmux_push_event (GstSplitMuxSrc * splitmux, GstEvent * e,
    guint32 seqnum)
{
  GList *cur;

  if (seqnum) {
    e = gst_event_make_writable (e);
    gst_event_set_seqnum (e, seqnum);
  }

  SPLITMUX_SRC_PADS_RLOCK (splitmux);
  for (cur = g_list_first (splitmux->pads);
      cur != NULL; cur = g_list_next (cur)) {
    GstPad *pad = GST_PAD_CAST (cur->data);
    gst_event_ref (e);
    gst_pad_push_event (pad, e);
  }
  SPLITMUX_SRC_PADS_RUNLOCK (splitmux);

  gst_event_unref (e);
}

static void
gst_splitmux_push_flush_stop (GstSplitMuxSrc * splitmux, guint32 seqnum)
{
  GstEvent *e = gst_event_new_flush_stop (TRUE);
  GList *cur;

  if (seqnum) {
    e = gst_event_make_writable (e);
    gst_event_set_seqnum (e, seqnum);
  }

  SPLITMUX_SRC_PADS_RLOCK (splitmux);
  for (cur = g_list_first (splitmux->pads);
      cur != NULL; cur = g_list_next (cur)) {
    SplitMuxSrcPad *target = (SplitMuxSrcPad *) (cur->data);

    gst_event_ref (e);
    gst_pad_push_event (GST_PAD_CAST (target), e);
    target->sent_caps = FALSE;
    target->sent_stream_start = FALSE;
    target->sent_segment = FALSE;
  }
  SPLITMUX_SRC_PADS_RUNLOCK (splitmux);

  gst_event_unref (e);
}

/* Callback for when a part finishes and we need to move to the next */
static gboolean
gst_splitmux_end_of_part (GstSplitMuxSrc * splitmux, SplitMuxSrcPad * splitpad)
{
  gint next_part = -1;
  gint cur_part = splitpad->cur_part;
  gboolean res = FALSE;

  if (splitmux->play_segment.rate >= 0.0) {
    if (cur_part + 1 < splitmux->num_parts)
      next_part = cur_part + 1;
    /* Make sure the transition is seamless */
    splitpad->set_next_discont = FALSE;
    splitpad->clear_next_discont = TRUE;
  } else {
    /* Reverse play - move to previous segment */
    if (cur_part > 0) {
      next_part = cur_part - 1;
      /* Non-seamless transition in reverse */
      splitpad->set_next_discont = TRUE;
      splitpad->clear_next_discont = FALSE;
    }
  }

  SPLITMUX_SRC_LOCK (splitmux);

  /* If all pads are done with this part, deactivate it */
  if (gst_splitmux_part_is_eos (splitmux->parts[splitpad->cur_part]))
    gst_splitmux_part_reader_deactivate (splitmux->parts[cur_part]);

  if (splitmux->play_segment.rate >= 0.0) {
    if (splitmux->play_segment.stop != -1) {
      GstClockTime part_end =
          gst_splitmux_part_reader_get_end_offset (splitmux->parts[cur_part]);
      if (part_end >= splitmux->play_segment.stop) {
        GST_DEBUG_OBJECT (splitmux,
            "Stop position was within that part. Finishing");
        next_part = -1;
      }
    }
  } else {
    if (splitmux->play_segment.start != -1) {
      GstClockTime part_start =
          gst_splitmux_part_reader_get_start_offset (splitmux->parts[cur_part]);
      if (part_start <= splitmux->play_segment.start) {
        GST_DEBUG_OBJECT (splitmux,
            "Start position %" GST_TIME_FORMAT
            " was within that part. Finishing",
            GST_TIME_ARGS (splitmux->play_segment.start));
        next_part = -1;
      }
    }
  }

  if (next_part != -1) {
    GST_DEBUG_OBJECT (splitmux, "At EOS on pad %" GST_PTR_FORMAT
        " moving to part %d", splitpad, next_part);
    splitpad->cur_part = next_part;
    splitpad->reader = splitmux->parts[splitpad->cur_part];
    if (splitpad->part_pad)
      gst_object_unref (splitpad->part_pad);
    splitpad->part_pad =
        gst_splitmux_part_reader_lookup_pad (splitpad->reader,
        (GstPad *) (splitpad));

    if (splitmux->cur_part != next_part) {
      if (!gst_splitmux_part_reader_is_active (splitpad->reader)) {
        GstSegment tmp;
        /* If moving backward into a new part, set stop
         * to -1 to ensure we play the entire file - workaround
         * a bug in qtdemux that misses bits at the end */
        gst_segment_copy_into (&splitmux->play_segment, &tmp);
        if (tmp.rate < 0)
          tmp.stop = -1;

        /* This is the first pad to move to the new part, activate it */
        GST_DEBUG_OBJECT (splitpad,
            "First pad to change part. Activating part %d with seg %"
            GST_SEGMENT_FORMAT, next_part, &tmp);
        if (!gst_splitmux_part_reader_activate (splitpad->reader, &tmp,
                GST_SEEK_FLAG_NONE))
          goto error;
      }
      splitmux->cur_part = next_part;
    }
    res = TRUE;
  }

  SPLITMUX_SRC_UNLOCK (splitmux);
  return res;
error:
  SPLITMUX_SRC_UNLOCK (splitmux);
  GST_ELEMENT_ERROR (splitmux, RESOURCE, READ, (NULL),
      ("Failed to activate part %d", splitmux->cur_part));
  return FALSE;
}

G_DEFINE_TYPE (SplitMuxSrcPad, splitmux_src_pad, GST_TYPE_PAD);

static void
splitmux_src_pad_constructed (GObject * pad)
{
  gst_pad_set_event_function (GST_PAD (pad),
      GST_DEBUG_FUNCPTR (splitmux_src_pad_event));
  gst_pad_set_query_function (GST_PAD (pad),
      GST_DEBUG_FUNCPTR (splitmux_src_pad_query));

  G_OBJECT_CLASS (splitmux_src_pad_parent_class)->constructed (pad);
}

static void
gst_splitmux_src_pad_dispose (GObject * object)
{
  SplitMuxSrcPad *pad = (SplitMuxSrcPad *) (object);

  GST_OBJECT_LOCK (pad);
  if (pad->part_pad) {
    gst_object_unref (pad->part_pad);
    pad->part_pad = NULL;
  }
  GST_OBJECT_UNLOCK (pad);

  G_OBJECT_CLASS (splitmux_src_pad_parent_class)->dispose (object);
}

static void
splitmux_src_pad_class_init (SplitMuxSrcPadClass * klass)
{
  GObjectClass *gobject_klass = (GObjectClass *) (klass);

  gobject_klass->constructed = splitmux_src_pad_constructed;
  gobject_klass->dispose = gst_splitmux_src_pad_dispose;
}

static void
splitmux_src_pad_init (SplitMuxSrcPad * pad)
{
}

/* Event handler for source pads. Proxy events into the child
 * parts as needed
 */
static gboolean
splitmux_src_pad_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstSplitMuxSrc *splitmux = GST_SPLITMUX_SRC (parent);
  gboolean ret = FALSE;

  GST_DEBUG_OBJECT (parent, "event %" GST_PTR_FORMAT
      " on %" GST_PTR_FORMAT, event, pad);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:{
      GstFormat format;
      gdouble rate;
      GstSeekFlags flags;
      GstSeekType start_type, stop_type;
      gint64 start, stop;
      guint32 seqnum;
      gint i;
      GstClockTime part_start, position;
      GList *cur;
      GstSegment tmp;

      gst_event_parse_seek (event, &rate, &format, &flags,
          &start_type, &start, &stop_type, &stop);

      if (format != GST_FORMAT_TIME) {
        GST_DEBUG_OBJECT (splitmux, "can only seek on TIME");
        goto error;
      }
      /* FIXME: Support non-flushing seeks, which might never wake up */
      if (!(flags & GST_SEEK_FLAG_FLUSH)) {
        GST_DEBUG_OBJECT (splitmux, "Only flushing seeks supported");
        goto error;
      }
      seqnum = gst_event_get_seqnum (event);

      SPLITMUX_SRC_LOCK (splitmux);
      if (!splitmux->running || splitmux->num_parts < 1) {
        /* Not started yet */
        SPLITMUX_SRC_UNLOCK (splitmux);
        goto error;
      }
      if (splitmux->segment_seqnum == seqnum) {
        GST_DEBUG_OBJECT (splitmux, "Ignoring duplicate seek event");
        SPLITMUX_SRC_UNLOCK (splitmux);
        ret = TRUE;
        goto done;
      }

      gst_segment_copy_into (&splitmux->play_segment, &tmp);

      if (!gst_segment_do_seek (&tmp, rate,
              format, flags, start_type, start, stop_type, stop, NULL)) {
        /* Invalid seek requested, ignore it */
        SPLITMUX_SRC_UNLOCK (splitmux);
        goto error;
      }
      position = tmp.position;

      GST_DEBUG_OBJECT (splitmux, "Performing seek with seg %"
          GST_SEGMENT_FORMAT, &tmp);

      GST_DEBUG_OBJECT (splitmux,
          "Handling flushing seek. Sending flush start");

      /* Send flush_start */
      gst_splitmux_push_event (splitmux, gst_event_new_flush_start (), seqnum);

      /* Stop all parts, which will work because of the flush */
      SPLITMUX_SRC_PADS_RLOCK (splitmux);
      SPLITMUX_SRC_UNLOCK (splitmux);
      for (cur = g_list_first (splitmux->pads);
          cur != NULL; cur = g_list_next (cur)) {
        SplitMuxSrcPad *target = (SplitMuxSrcPad *) (cur->data);
        GstSplitMuxPartReader *reader = splitmux->parts[target->cur_part];
        gst_splitmux_part_reader_deactivate (reader);
      }

      /* Shut down pad tasks */
      GST_DEBUG_OBJECT (splitmux, "Pausing pad tasks");
      for (cur = g_list_first (splitmux->pads);
          cur != NULL; cur = g_list_next (cur)) {
        GstPad *splitpad = (GstPad *) (cur->data);
        gst_pad_pause_task (GST_PAD (splitpad));
      }
      SPLITMUX_SRC_PADS_RUNLOCK (splitmux);
      SPLITMUX_SRC_LOCK (splitmux);

      /* Send flush stop */
      GST_DEBUG_OBJECT (splitmux, "Sending flush stop");
      gst_splitmux_push_flush_stop (splitmux, seqnum);

      /* Everything is stopped, so update the play_segment */
      gst_segment_copy_into (&tmp, &splitmux->play_segment);
      splitmux->segment_seqnum = seqnum;

      /* Work out where to start from now */
      for (i = 0; i < splitmux->num_parts; i++) {
        GstSplitMuxPartReader *reader = splitmux->parts[i];
        GstClockTime part_end =
            gst_splitmux_part_reader_get_end_offset (reader);

        if (part_end > position)
          break;
      }
      if (i == splitmux->num_parts)
        i = splitmux->num_parts - 1;

      part_start =
          gst_splitmux_part_reader_get_start_offset (splitmux->parts[i]);

      GST_DEBUG_OBJECT (splitmux,
          "Seek to time %" GST_TIME_FORMAT " landed in part %d offset %"
          GST_TIME_FORMAT, GST_TIME_ARGS (position),
          i, GST_TIME_ARGS (position - part_start));

      ret = gst_splitmux_src_activate_part (splitmux, i, flags);
      SPLITMUX_SRC_UNLOCK (splitmux);
    }
    case GST_EVENT_RECONFIGURE:{
      GST_DEBUG_OBJECT (splitmux, "reconfigure event on pad %" GST_PTR_FORMAT,
          pad);

      SPLITMUX_SRC_PADS_RLOCK (splitmux);
      /* Restart the task on this pad */
      gst_pad_start_task (GST_PAD (pad),
          (GstTaskFunction) gst_splitmux_pad_loop, pad, NULL);
      SPLITMUX_SRC_PADS_RUNLOCK (splitmux);
      break;
    }
    default:
      break;
  }

done:
  gst_event_unref (event);
error:
  return ret;
}

static gboolean
splitmux_src_pad_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  /* Query handler for source pads. Proxy queries into the child
   * parts as needed
   */
  GstSplitMuxSrc *splitmux = GST_SPLITMUX_SRC (parent);
  gboolean ret = FALSE;

  GST_LOG_OBJECT (parent, "query %" GST_PTR_FORMAT
      " on %" GST_PTR_FORMAT, query, pad);
  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    case GST_QUERY_POSITION:{
      GstSplitMuxPartReader *part;
      SplitMuxSrcPad *anypad;

      SPLITMUX_SRC_LOCK (splitmux);
      SPLITMUX_SRC_PADS_RLOCK (splitmux);
      anypad = (SplitMuxSrcPad *) (splitmux->pads->data);
      part = splitmux->parts[anypad->cur_part];
      ret = gst_splitmux_part_reader_src_query (part, pad, query);
      SPLITMUX_SRC_PADS_RUNLOCK (splitmux);
      SPLITMUX_SRC_UNLOCK (splitmux);
      break;
    }
    case GST_QUERY_DURATION:{
      GstClockTime duration;
      GstFormat fmt;

      gst_query_parse_duration (query, &fmt, NULL);
      if (fmt != GST_FORMAT_TIME)
        break;

      GST_OBJECT_LOCK (splitmux);
      duration = splitmux->total_duration;
      GST_OBJECT_UNLOCK (splitmux);

      if (duration > 0 && duration != GST_CLOCK_TIME_NONE) {
        gst_query_set_duration (query, GST_FORMAT_TIME, duration);
        ret = TRUE;
      }
      break;
    }
    case GST_QUERY_SEEKING:{
      GstFormat format;

      gst_query_parse_seeking (query, &format, NULL, NULL, NULL);
      if (format != GST_FORMAT_TIME)
        break;

      GST_OBJECT_LOCK (splitmux);
      gst_query_set_seeking (query, GST_FORMAT_TIME, TRUE, 0,
          splitmux->total_duration);
      ret = TRUE;
      GST_OBJECT_UNLOCK (splitmux);

      break;
    }
    default:
      break;
  }
  return ret;
}


gboolean
register_splitmuxsrc (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (splitmux_debug, "splitmuxsrc", 0,
      "Split File Demuxing Source");

  return gst_element_register (plugin, "splitmuxsrc", GST_RANK_NONE,
      GST_TYPE_SPLITMUX_SRC);
}
