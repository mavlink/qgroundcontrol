/* GStreamer
 * Copyright (C) 2007 Nokia Corporation (contact <stefan.kost@nokia.com>)
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
 * SECTION:element-rndbuffersize
 * @title: rndbuffersize
 *
 * This element pulls buffers with random sizes from the source.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

GST_DEBUG_CATEGORY_STATIC (gst_rnd_buffer_size_debug);
#define GST_CAT_DEFAULT gst_rnd_buffer_size_debug

#define GST_TYPE_RND_BUFFER_SIZE            (gst_rnd_buffer_size_get_type())
#define GST_RND_BUFFER_SIZE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RND_BUFFER_SIZE,GstRndBufferSize))
#define GST_RND_BUFFER_SIZE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RND_BUFFER_SIZE,GstRndBufferSizeClass))
#define GST_IS_RND_BUFFER_SIZE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RND_BUFFER_SIZE))
#define GST_IS_RND_BUFFER_SIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RND_BUFFER_SIZE))

typedef struct _GstRndBufferSize GstRndBufferSize;
typedef struct _GstRndBufferSizeClass GstRndBufferSizeClass;

struct _GstRndBufferSize
{
  GstElement parent;

  /*< private > */
  GRand *rand;
  guint seed;
  gint min, max;

  GstPad *sinkpad, *srcpad;
  guint64 offset;

  gboolean need_newsegment;

  GstAdapter *adapter;
};

struct _GstRndBufferSizeClass
{
  GstElementClass parent_class;
};

enum
{
  PROP_SEED = 1,
  PROP_MINIMUM,
  PROP_MAXIMUM
};

#define DEFAULT_SEED 0
#define DEFAULT_MIN  1
#define DEFAULT_MAX  (8*1024)

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_rnd_buffer_size_finalize (GObject * object);
static void gst_rnd_buffer_size_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rnd_buffer_size_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rnd_buffer_size_activate (GstPad * pad, GstObject * parent);
static gboolean gst_rnd_buffer_size_activate_mode (GstPad * pad,
    GstObject * parent, GstPadMode mode, gboolean active);
static void gst_rnd_buffer_size_loop (GstRndBufferSize * self);
static GstStateChangeReturn gst_rnd_buffer_size_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_rnd_buffer_size_src_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_rnd_buffer_size_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rnd_buffer_size_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);

GType gst_rnd_buffer_size_get_type (void);
#define gst_rnd_buffer_size_parent_class parent_class
G_DEFINE_TYPE (GstRndBufferSize, gst_rnd_buffer_size, GST_TYPE_ELEMENT);

static void
gst_rnd_buffer_size_class_init (GstRndBufferSizeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_rnd_buffer_size_debug, "rndbuffersize", 0,
      "rndbuffersize element");

  gobject_class->set_property = gst_rnd_buffer_size_set_property;
  gobject_class->get_property = gst_rnd_buffer_size_get_property;
  gobject_class->finalize = gst_rnd_buffer_size_finalize;

  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &src_template);

  gst_element_class_set_static_metadata (gstelement_class, "Random buffer size",
      "Testing", "pull random sized buffers",
      "Stefan Kost <stefan.kost@nokia.com>");

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rnd_buffer_size_change_state);

  g_object_class_install_property (gobject_class, PROP_SEED,
      g_param_spec_uint ("seed", "random number seed",
          "seed for randomness (initialized when going from READY to PAUSED)",
          0, G_MAXUINT32, DEFAULT_SEED,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MINIMUM,
      g_param_spec_int ("min", "minimum", "minimum buffer size",
          0, G_MAXINT32, DEFAULT_MIN,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MAXIMUM,
      g_param_spec_int ("max", "maximum", "maximum buffer size",
          1, G_MAXINT32, DEFAULT_MAX,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
}

static void
gst_rnd_buffer_size_init (GstRndBufferSize * self)
{
  self->sinkpad = gst_pad_new_from_static_template (&sink_template, "sink");
  gst_pad_set_activate_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rnd_buffer_size_activate));
  gst_pad_set_activatemode_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rnd_buffer_size_activate_mode));
  gst_pad_set_event_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rnd_buffer_size_sink_event));
  gst_pad_set_chain_function (self->sinkpad,
      GST_DEBUG_FUNCPTR (gst_rnd_buffer_size_chain));
  GST_OBJECT_FLAG_SET (self->sinkpad, GST_PAD_FLAG_PROXY_CAPS);
  GST_OBJECT_FLAG_SET (self->sinkpad, GST_PAD_FLAG_PROXY_ALLOCATION);
  GST_OBJECT_FLAG_SET (self->sinkpad, GST_PAD_FLAG_PROXY_SCHEDULING);
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  self->srcpad = gst_pad_new_from_static_template (&src_template, "src");
  gst_pad_set_event_function (self->srcpad,
      GST_DEBUG_FUNCPTR (gst_rnd_buffer_size_src_event));
  GST_OBJECT_FLAG_SET (self->srcpad, GST_PAD_FLAG_PROXY_CAPS);
  GST_OBJECT_FLAG_SET (self->srcpad, GST_PAD_FLAG_PROXY_ALLOCATION);
  GST_OBJECT_FLAG_SET (self->srcpad, GST_PAD_FLAG_PROXY_SCHEDULING);
  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);
}


static void
gst_rnd_buffer_size_finalize (GObject * object)
{
  GstRndBufferSize *self = GST_RND_BUFFER_SIZE (object);

  if (self->rand) {
    g_rand_free (self->rand);
    self->rand = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gst_rnd_buffer_size_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRndBufferSize *self = GST_RND_BUFFER_SIZE (object);

  switch (prop_id) {
    case PROP_SEED:
      self->seed = g_value_get_uint (value);
      break;
    case PROP_MINIMUM:
      self->min = g_value_get_int (value);
      break;
    case PROP_MAXIMUM:
      self->max = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_rnd_buffer_size_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRndBufferSize *self = GST_RND_BUFFER_SIZE (object);

  switch (prop_id) {
    case PROP_SEED:
      g_value_set_uint (value, self->seed);
      break;
    case PROP_MINIMUM:
      g_value_set_int (value, self->min);
      break;
    case PROP_MAXIMUM:
      g_value_set_int (value, self->max);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static gboolean
gst_rnd_buffer_size_activate (GstPad * pad, GstObject * parent)
{
  GstQuery *query;
  gboolean pull_mode;

  query = gst_query_new_scheduling ();

  if (gst_pad_peer_query (pad, query))
    pull_mode = gst_query_has_scheduling_mode_with_flags (query,
        GST_PAD_MODE_PULL, GST_SCHEDULING_FLAG_SEEKABLE);
  else
    pull_mode = FALSE;

  gst_query_unref (query);

  if (pull_mode) {
    GST_DEBUG_OBJECT (pad, "activating pull");
    return gst_pad_activate_mode (pad, GST_PAD_MODE_PULL, TRUE);
  } else {
    GST_DEBUG_OBJECT (pad, "activating push");
    return gst_pad_activate_mode (pad, GST_PAD_MODE_PUSH, TRUE);
  }
}


static gboolean
gst_rnd_buffer_size_activate_mode (GstPad * pad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  gboolean res;
  GstRndBufferSize *self = GST_RND_BUFFER_SIZE (parent);

  switch (mode) {
    case GST_PAD_MODE_PULL:
      if (active) {
        GST_INFO_OBJECT (self, "starting pull");
        res =
            gst_pad_start_task (pad, (GstTaskFunction) gst_rnd_buffer_size_loop,
            self, NULL);
        self->need_newsegment = TRUE;
      } else {
        GST_INFO_OBJECT (self, "stopping pull");
        res = gst_pad_stop_task (pad);
      }
      break;
    case GST_PAD_MODE_PUSH:
      GST_INFO_OBJECT (self, "%sactivating in push mode", (active) ? "" : "de");
      res = TRUE;
      break;
    default:
      res = FALSE;
      break;
  }
  return res;
}

static gboolean
gst_rnd_buffer_size_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRndBufferSize *self;
  GstSeekType start_type;
  GstSeekFlags flags;
  GstFormat format;
  gint64 start;

  if (GST_EVENT_TYPE (event) != GST_EVENT_SEEK) {
    return gst_pad_event_default (pad, parent, event);
  }

  self = GST_RND_BUFFER_SIZE (parent);
  gst_event_parse_seek (event, NULL, &format, &flags, &start_type, &start,
      NULL, NULL);

  if (format != GST_FORMAT_BYTES) {
    GST_WARNING_OBJECT (pad, "only BYTE format supported");
    return FALSE;
  }
  if (start_type != GST_SEEK_TYPE_SET) {
    GST_WARNING_OBJECT (pad, "only SEEK_TYPE_SET supported");
    return FALSE;
  }

  if ((flags & GST_SEEK_FLAG_FLUSH)) {
    gst_pad_push_event (self->srcpad, gst_event_new_flush_start ());
    gst_pad_push_event (self->sinkpad, gst_event_new_flush_start ());
  } else {
    gst_pad_pause_task (self->sinkpad);
  }

  GST_PAD_STREAM_LOCK (self->sinkpad);

  if ((flags & GST_SEEK_FLAG_FLUSH)) {
    gst_pad_push_event (self->srcpad, gst_event_new_flush_stop (TRUE));
    gst_pad_push_event (self->sinkpad, gst_event_new_flush_stop (TRUE));
  }

  GST_INFO_OBJECT (pad, "seeking to offset %" G_GINT64_FORMAT, start);

  self->offset = start;
  self->need_newsegment = TRUE;

  gst_pad_start_task (self->sinkpad, (GstTaskFunction) gst_rnd_buffer_size_loop,
      self, NULL);

  GST_PAD_STREAM_UNLOCK (self->sinkpad);
  return TRUE;
}

static GstFlowReturn
gst_rnd_buffer_size_drain_adapter (GstRndBufferSize * self, gboolean eos)
{
  GstFlowReturn flow;
  GstBuffer *buf;
  guint num_bytes, avail;

  flow = GST_FLOW_OK;

  if (G_UNLIKELY (self->min > self->max))
    goto bogus_minmax;

  do {
    if (self->min != self->max) {
      num_bytes = g_rand_int_range (self->rand, self->min, self->max);
    } else {
      num_bytes = self->min;
    }

    GST_LOG_OBJECT (self, "pulling %u bytes out of adapter", num_bytes);

    buf = gst_adapter_take_buffer (self->adapter, num_bytes);

    if (buf == NULL) {
      if (!eos) {
        GST_LOG_OBJECT (self, "not enough bytes in adapter");
        break;
      }

      avail = gst_adapter_available (self->adapter);

      if (avail == 0)
        break;

      if (avail < self->min) {
        GST_WARNING_OBJECT (self, "discarding %u bytes at end (min=%u)",
            avail, self->min);
        gst_adapter_clear (self->adapter);
        break;
      }
      buf = gst_adapter_take_buffer (self->adapter, avail);
      g_assert (buf != NULL);
    }

    flow = gst_pad_push (self->srcpad, buf);
  }
  while (flow == GST_FLOW_OK);

  return flow;

/* ERRORS */
bogus_minmax:
  {
    GST_ELEMENT_ERROR (self, LIBRARY, SETTINGS,
        ("The minimum buffer size is smaller than the maximum buffer size."),
        ("buffer sizes: max=%d, min=%d", self->min, self->max));
    return GST_FLOW_ERROR;
  }
}

static gboolean
gst_rnd_buffer_size_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstRndBufferSize *rnd = GST_RND_BUFFER_SIZE (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      gst_rnd_buffer_size_drain_adapter (rnd, TRUE);
      break;
    case GST_EVENT_FLUSH_STOP:
      if (rnd->adapter != NULL)
        gst_adapter_clear (rnd->adapter);
      break;
    default:
      break;
  }

  return gst_pad_event_default (pad, parent, event);
}

static GstFlowReturn
gst_rnd_buffer_size_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstRndBufferSize *rnd = GST_RND_BUFFER_SIZE (parent);
  GstFlowReturn flow;

  if (rnd->adapter == NULL)
    rnd->adapter = gst_adapter_new ();

  gst_adapter_push (rnd->adapter, buf);

  flow = gst_rnd_buffer_size_drain_adapter (rnd, FALSE);

  if (flow != GST_FLOW_OK)
    GST_INFO_OBJECT (rnd, "flow: %s", gst_flow_get_name (flow));

  return flow;
}

static void
gst_rnd_buffer_size_loop (GstRndBufferSize * self)
{
  GstBuffer *buf = NULL;
  GstFlowReturn ret;
  guint num_bytes, size;

  if (G_UNLIKELY (self->min > self->max))
    goto bogus_minmax;

  if (G_UNLIKELY (self->min != self->max)) {
    num_bytes = g_rand_int_range (self->rand, self->min, self->max);
  } else {
    num_bytes = self->min;
  }

  GST_LOG_OBJECT (self, "pulling %u bytes at offset %" G_GUINT64_FORMAT,
      num_bytes, self->offset);

  ret = gst_pad_pull_range (self->sinkpad, self->offset, num_bytes, &buf);

  if (ret != GST_FLOW_OK)
    goto pull_failed;

  size = gst_buffer_get_size (buf);

  if (size < num_bytes) {
    GST_WARNING_OBJECT (self, "short buffer: %u bytes", size);
  }

  if (self->need_newsegment) {
    GstSegment segment;

    gst_segment_init (&segment, GST_FORMAT_BYTES);
    segment.start = self->offset;
    gst_pad_push_event (self->srcpad, gst_event_new_segment (&segment));
    self->need_newsegment = FALSE;
  }

  self->offset += size;

  ret = gst_pad_push (self->srcpad, buf);

  if (ret != GST_FLOW_OK)
    goto push_failed;

  return;

pause_task:
  {
    GST_DEBUG_OBJECT (self, "pausing task");
    gst_pad_pause_task (self->sinkpad);
    return;
  }

pull_failed:
  {
    if (ret == GST_FLOW_EOS) {
      GST_DEBUG_OBJECT (self, "eos");
      gst_pad_push_event (self->srcpad, gst_event_new_eos ());
    } else {
      GST_WARNING_OBJECT (self, "pull_range flow: %s", gst_flow_get_name (ret));
    }
    goto pause_task;
  }

push_failed:
  {
    GST_DEBUG_OBJECT (self, "push flow: %s", gst_flow_get_name (ret));
    if (ret == GST_FLOW_EOS) {
      GST_DEBUG_OBJECT (self, "eos");
      gst_pad_push_event (self->srcpad, gst_event_new_eos ());
    } else if (ret < GST_FLOW_EOS || ret == GST_FLOW_NOT_LINKED) {
      GST_ELEMENT_FLOW_ERROR (self, ret);
    }
    goto pause_task;
  }

bogus_minmax:
  {
    GST_ELEMENT_ERROR (self, LIBRARY, SETTINGS,
        ("The minimum buffer size is smaller than the maximum buffer size."),
        ("buffer sizes: max=%d, min=%d", self->min, self->max));
    goto pause_task;
  }
}

static GstStateChangeReturn
gst_rnd_buffer_size_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRndBufferSize *self = GST_RND_BUFFER_SIZE (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      self->offset = 0;
      if (!self->rand) {
        self->rand = g_rand_new_with_seed (self->seed);
      }
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
      if (self->rand) {
        g_rand_free (self->rand);
        self->rand = NULL;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      if (self->adapter) {
        g_object_unref (self->adapter);
        self->adapter = NULL;
      }
      break;
    default:
      break;
  }

  return ret;
}
