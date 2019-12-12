/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
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

/*
 * This file was (probably) generated from gstnavseek.c,
 * gstnavseek.c,v 1.7 2003/11/08 02:48:59 dschleef Exp 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstnavseek.h"
#include <string.h>
#include <math.h>

enum
{
  PROP_0,
  PROP_SEEKOFFSET
};

GstStaticPadTemplate navseek_src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GstStaticPadTemplate navseek_sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static gboolean gst_navseek_sink_event (GstBaseTransform * trans,
    GstEvent * event);
static GstFlowReturn gst_navseek_transform_ip (GstBaseTransform * basetrans,
    GstBuffer * buf);
static gboolean gst_navseek_src_event (GstBaseTransform * trans,
    GstEvent * event);
static gboolean gst_navseek_stop (GstBaseTransform * trans);
static gboolean gst_navseek_start (GstBaseTransform * trans);

static void gst_navseek_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_navseek_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

GType gst_navseek_get_type (void);
#define gst_navseek_parent_class parent_class
G_DEFINE_TYPE (GstNavSeek, gst_navseek, GST_TYPE_BASE_TRANSFORM);

static void
gst_navseek_class_init (GstNavSeekClass * klass)
{
  GstBaseTransformClass *gstbasetrans_class;
  GstElementClass *element_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  gstbasetrans_class = GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->set_property = gst_navseek_set_property;
  gobject_class->get_property = gst_navseek_get_property;

  g_object_class_install_property (gobject_class,
      PROP_SEEKOFFSET, g_param_spec_double ("seek-offset", "Seek Offset",
          "Time in seconds to seek by", 0.0, G_MAXDOUBLE, 5.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (element_class,
      &navseek_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &navseek_src_template);

  gst_element_class_set_static_metadata (element_class,
      "Seek based on left-right arrows", "Filter/Video",
      "Seek based on navigation keys left-right",
      "Jan Schmidt <thaytan@mad.scientist.com>");

  gstbasetrans_class->src_event = GST_DEBUG_FUNCPTR (gst_navseek_src_event);
  gstbasetrans_class->sink_event = GST_DEBUG_FUNCPTR (gst_navseek_sink_event);
  gstbasetrans_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_navseek_transform_ip);
  gstbasetrans_class->start = GST_DEBUG_FUNCPTR (gst_navseek_start);
  gstbasetrans_class->stop = GST_DEBUG_FUNCPTR (gst_navseek_stop);
}

static void
gst_navseek_init (GstNavSeek * navseek)
{
  gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (navseek), TRUE);

  navseek->seek_offset = 5.0;
  navseek->loop = FALSE;
  navseek->grab_seg_start = FALSE;
  navseek->grab_seg_end = FALSE;
  navseek->segment_start = GST_CLOCK_TIME_NONE;
  navseek->segment_end = GST_CLOCK_TIME_NONE;
}

static void
gst_navseek_seek (GstNavSeek * navseek, gint64 offset)
{
  gboolean ret;
  GstPad *peer_pad;
  gint64 peer_value;

  /* Query for the current time then attempt to set to time + offset */
  peer_pad = gst_pad_get_peer (GST_BASE_TRANSFORM (navseek)->sinkpad);
  ret = gst_pad_query_position (peer_pad, GST_FORMAT_TIME, &peer_value);

  if (ret) {
    GstEvent *event;

    peer_value += offset;
    if (peer_value < 0)
      peer_value = 0;

    event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, peer_value, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

    gst_pad_send_event (peer_pad, event);
  }

  gst_object_unref (peer_pad);
}

static void
gst_navseek_change_playback_rate (GstNavSeek * navseek, gdouble rate)
{
  gboolean ret;
  GstPad *peer_pad;
  gint64 current_position;

  peer_pad = gst_pad_get_peer (GST_BASE_TRANSFORM (navseek)->sinkpad);
  ret = gst_pad_query_position (peer_pad, GST_FORMAT_TIME, &current_position);

  if (ret) {
    GstEvent *event;
    gint64 start;
    gint64 stop;

    if (rate > 0.0) {
      start = current_position;
      stop = -1;
    } else {
      /* negative rate: we play from stop to start */
      start = 0;
      stop = current_position;
    }

    event = gst_event_new_seek (rate, GST_FORMAT_TIME,
        GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP,
        GST_SEEK_TYPE_SET, start, GST_SEEK_TYPE_SET, stop);

    gst_pad_send_event (peer_pad, event);
  }
  gst_object_unref (peer_pad);
}

static void
gst_navseek_segseek (GstNavSeek * navseek)
{
  GstEvent *event;
  GstPad *peer_pad;

  if ((navseek->segment_start == GST_CLOCK_TIME_NONE) ||
      (navseek->segment_end == GST_CLOCK_TIME_NONE) ||
      (!GST_PAD_IS_LINKED (GST_BASE_TRANSFORM (navseek)->sinkpad))) {
    return;
  }

  if (navseek->loop) {
    event =
        gst_event_new_seek (1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, navseek->segment_start, GST_SEEK_TYPE_SET,
        navseek->segment_end);
  } else {
    event =
        gst_event_new_seek (1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_ACCURATE,
        GST_SEEK_TYPE_SET, navseek->segment_start, GST_SEEK_TYPE_SET,
        navseek->segment_end);
  }

  peer_pad = gst_pad_get_peer (GST_BASE_TRANSFORM (navseek)->sinkpad);
  gst_pad_send_event (peer_pad, event);
  gst_object_unref (peer_pad);
}

static void
gst_navseek_toggle_play_pause (GstNavSeek * navseek)
{
  GstStateChangeReturn sret;
  GstState current, pending, state;

  sret = gst_element_get_state (GST_ELEMENT (navseek), &current, &pending, 0);
  if (sret == GST_STATE_CHANGE_FAILURE)
    return;

  state = (pending != GST_STATE_VOID_PENDING) ? pending : current;

  gst_element_post_message (GST_ELEMENT (navseek),
      gst_message_new_request_state (GST_OBJECT (navseek),
          (state == GST_STATE_PLAYING) ? GST_STATE_PAUSED : GST_STATE_PLAYING));
}

static gboolean
gst_navseek_src_event (GstBaseTransform * trans, GstEvent * event)
{
  GstNavSeek *navseek;
  gboolean ret = TRUE;

  navseek = GST_NAVSEEK (trans);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NAVIGATION:
    {
      /* Check for a keyup and convert left/right to a seek event */
      const GstStructure *structure;
      const gchar *event_type;

      structure = gst_event_get_structure (event);
      g_return_val_if_fail (structure != NULL, FALSE);

      event_type = gst_structure_get_string (structure, "event");
      g_return_val_if_fail (event_type != NULL, FALSE);

      if (strcmp (event_type, "key-press") == 0) {
        const gchar *key;

        key = gst_structure_get_string (structure, "key");
        g_return_val_if_fail (key != NULL, FALSE);

        if (strcmp (key, "Left") == 0) {
          /* Seek backward by 5 secs */
          gst_navseek_seek (navseek, -1.0 * navseek->seek_offset * GST_SECOND);
        } else if (strcmp (key, "Right") == 0) {
          /* Seek forward */
          gst_navseek_seek (navseek, navseek->seek_offset * GST_SECOND);
        } else if (strcmp (key, "s") == 0) {
          /* Grab the next frame as the start frame of a segment */
          navseek->grab_seg_start = TRUE;
        } else if (strcmp (key, "e") == 0) {
          /* Grab the next frame as the end frame of a segment */
          navseek->grab_seg_end = TRUE;
        } else if (strcmp (key, "l") == 0) {
          /* Toggle the loop flag. If we have both start and end segment times send a seek */
          navseek->loop = !navseek->loop;
          gst_navseek_segseek (navseek);
        } else if (strcmp (key, "f") == 0) {
          /* fast forward */
          gst_navseek_change_playback_rate (navseek, 2.0);
        } else if (strcmp (key, "r") == 0) {
          /* rewind */
          gst_navseek_change_playback_rate (navseek, -2.0);
        } else if (strcmp (key, "n") == 0) {
          /* normal speed */
          gst_navseek_change_playback_rate (navseek, 1.0);
        } else if (strcmp (key, "space") == 0) {
          gst_navseek_toggle_play_pause (navseek);
        }
      } else {
        break;
      }
      gst_event_unref (event);
      event = NULL;
      break;
    }
    default:
      break;
  }

  if (event)
    ret = GST_BASE_TRANSFORM_CLASS (parent_class)->src_event (trans, event);

  return ret;
}

static void
gst_navseek_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstNavSeek *navseek = GST_NAVSEEK (object);

  switch (prop_id) {
    case PROP_SEEKOFFSET:
      GST_OBJECT_LOCK (navseek);
      navseek->seek_offset = g_value_get_double (value);
      GST_OBJECT_UNLOCK (navseek);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_navseek_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstNavSeek *navseek = GST_NAVSEEK (object);

  switch (prop_id) {
    case PROP_SEEKOFFSET:
      GST_OBJECT_LOCK (navseek);
      g_value_set_double (value, navseek->seek_offset);
      GST_OBJECT_UNLOCK (navseek);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_navseek_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstNavSeek *navseek = GST_NAVSEEK (trans);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      GST_OBJECT_LOCK (navseek);
      if (navseek->loop)
        gst_navseek_segseek (navseek);
      GST_OBJECT_UNLOCK (navseek);
      break;
    default:
      break;
  }
  return GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);
}

static GstFlowReturn
gst_navseek_transform_ip (GstBaseTransform * basetrans, GstBuffer * buf)
{
  GstNavSeek *navseek = GST_NAVSEEK (basetrans);

  GST_OBJECT_LOCK (navseek);

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
    if (navseek->grab_seg_start) {
      navseek->segment_start = GST_BUFFER_TIMESTAMP (buf);
      navseek->segment_end = GST_CLOCK_TIME_NONE;
      navseek->grab_seg_start = FALSE;
    }

    if (navseek->grab_seg_end) {
      navseek->segment_end = GST_BUFFER_TIMESTAMP (buf);
      navseek->grab_seg_end = FALSE;
      gst_navseek_segseek (navseek);
    }
  }

  GST_OBJECT_UNLOCK (navseek);

  return GST_FLOW_OK;
}

static gboolean
gst_navseek_start (GstBaseTransform * trans)
{
  /* anything we should be doing here? */
  return TRUE;
}

static gboolean
gst_navseek_stop (GstBaseTransform * trans)
{
  /* anything we should be doing here? */
  return TRUE;
}
