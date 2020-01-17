/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <otte@gnome.org>
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
 * SECTION:element-breakmydata
 * @title: breakmydata
 *
 * This element modifies the contents of the buffer it is passed randomly
 * according to the parameters set.
 * It otherwise acts as an identity.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

GST_DEBUG_CATEGORY_STATIC (gst_break_my_data_debug);
#define GST_CAT_DEFAULT gst_break_my_data_debug

#define GST_TYPE_BREAK_MY_DATA \
  (gst_break_my_data_get_type())
#define GST_BREAK_MY_DATA(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_BREAK_MY_DATA,GstBreakMyData))
#define GST_BREAK_MY_DATA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_BREAK_MY_DATA,GstBreakMyDataClass))
#define GST_IS_BREAK_MY_DATA(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_BREAK_MY_DATA))
#define GST_IS_BREAK_MY_DATA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_BREAK_MY_DATA))

GType gst_break_my_data_get_type (void);

enum
{
  PROP_0,
  PROP_SEED,
  PROP_SET_TO,
  PROP_SKIP,
  PROP_PROBABILITY
};

typedef struct _GstBreakMyData GstBreakMyData;
typedef struct _GstBreakMyDataClass GstBreakMyDataClass;

struct _GstBreakMyData
{
  GstBaseTransform basetransform;

  GRand *rand;
  guint skipped;

  guint32 seed;
  gint set;
  guint skip;
  gdouble probability;
};

struct _GstBreakMyDataClass
{
  GstBaseTransformClass parent_class;
};

static void gst_break_my_data_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_break_my_data_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_break_my_data_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);
static gboolean gst_break_my_data_stop (GstBaseTransform * trans);
static gboolean gst_break_my_data_start (GstBaseTransform * trans);

GstStaticPadTemplate bmd_src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GstStaticPadTemplate bmd_sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

#define gst_break_my_data_parent_class parent_class
G_DEFINE_TYPE (GstBreakMyData, gst_break_my_data, GST_TYPE_BASE_TRANSFORM);


static void
gst_break_my_data_class_init (GstBreakMyDataClass * klass)
{
  GstBaseTransformClass *gstbasetrans_class;
  GstElementClass *gstelement_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);
  gstbasetrans_class = GST_BASE_TRANSFORM_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_break_my_data_debug, "breakmydata", 0,
      "debugging category for breakmydata element");

  gobject_class->set_property = gst_break_my_data_set_property;
  gobject_class->get_property = gst_break_my_data_get_property;

  g_object_class_install_property (gobject_class, PROP_SEED,
      g_param_spec_uint ("seed", "seed",
          "seed for randomness (initialized when going from READY to PAUSED)",
          0, G_MAXUINT32, 0,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SET_TO,
      g_param_spec_int ("set-to", "set-to",
          "set changed bytes to this value (-1 means random value",
          -1, G_MAXUINT8, -1,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SKIP,
      g_param_spec_uint ("skip", "skip",
          "amount of bytes skipped at the beginning of stream",
          0, G_MAXUINT, 0,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROBABILITY,
      g_param_spec_double ("probability", "probability",
          "probability for each byte in the buffer to be changed", 0.0, 1.0,
          0.0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class,
      &bmd_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &bmd_src_template);

  gst_element_class_set_static_metadata (gstelement_class, "Break my data",
      "Testing",
      "randomly change data in the stream", "Benjamin Otte <otte@gnome>");

  gstbasetrans_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_break_my_data_transform_ip);
  gstbasetrans_class->start = GST_DEBUG_FUNCPTR (gst_break_my_data_start);
  gstbasetrans_class->stop = GST_DEBUG_FUNCPTR (gst_break_my_data_stop);
}

static void
gst_break_my_data_init (GstBreakMyData * bmd)
{
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (bmd), TRUE);
}

static void
gst_break_my_data_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBreakMyData *bmd = GST_BREAK_MY_DATA (object);

  GST_OBJECT_LOCK (bmd);

  switch (prop_id) {
    case PROP_SEED:
      bmd->seed = g_value_get_uint (value);
      break;
    case PROP_SET_TO:
      bmd->set = g_value_get_int (value);
      break;
    case PROP_SKIP:
      bmd->skip = g_value_get_uint (value);
      break;
    case PROP_PROBABILITY:
      bmd->probability = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (bmd);
}

static void
gst_break_my_data_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstBreakMyData *bmd = GST_BREAK_MY_DATA (object);

  GST_OBJECT_LOCK (bmd);

  switch (prop_id) {
    case PROP_SEED:
      g_value_set_uint (value, bmd->seed);
      break;
    case PROP_SET_TO:
      g_value_set_int (value, bmd->set);
      break;
    case PROP_SKIP:
      g_value_set_uint (value, bmd->skip);
      break;
    case PROP_PROBABILITY:
      g_value_set_double (value, bmd->probability);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (bmd);
}

static GstFlowReturn
gst_break_my_data_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstBreakMyData *bmd = GST_BREAK_MY_DATA (trans);
  GstMapInfo map;
  gsize i;

  g_return_val_if_fail (gst_buffer_is_writable (buf), GST_FLOW_ERROR);

  GST_OBJECT_LOCK (bmd);

  if (bmd->skipped < bmd->skip) {
    i = bmd->skip - bmd->skipped;
  } else {
    i = 0;
  }

  gst_buffer_map (buf, &map, GST_MAP_READWRITE);

  GST_LOG_OBJECT (bmd,
      "got buffer %p (size %" G_GSIZE_FORMAT ", timestamp %" G_GUINT64_FORMAT
      ", offset %" G_GUINT64_FORMAT "", buf, map.size,
      GST_BUFFER_TIMESTAMP (buf), GST_BUFFER_OFFSET (buf));

  for (; i < map.size; i++) {
    if (g_rand_double_range (bmd->rand, 0, 1.0) <= bmd->probability) {
      guint8 new;

      if (bmd->set < 0) {
        new = g_rand_int_range (bmd->rand, 0, 256);
      } else {
        new = bmd->set;
      }
      GST_INFO_OBJECT (bmd,
          "changing byte %" G_GSIZE_FORMAT " from 0x%02X to 0x%02X", i,
          (guint) GST_READ_UINT8 (map.data + i), (guint) ((guint8) new));
      map.data[i] = new;
    }
  }
  /* don't overflow */
  bmd->skipped += MIN (G_MAXUINT - bmd->skipped, map.size);

  gst_buffer_unmap (buf, &map);

  GST_OBJECT_UNLOCK (bmd);

  return GST_FLOW_OK;
}

static gboolean
gst_break_my_data_start (GstBaseTransform * trans)
{
  GstBreakMyData *bmd = GST_BREAK_MY_DATA (trans);

  GST_OBJECT_LOCK (bmd);
  bmd->rand = g_rand_new_with_seed (bmd->seed);
  bmd->skipped = 0;
  GST_OBJECT_UNLOCK (bmd);

  return TRUE;
}

static gboolean
gst_break_my_data_stop (GstBaseTransform * trans)
{
  GstBreakMyData *bmd = GST_BREAK_MY_DATA (trans);

  GST_OBJECT_LOCK (bmd);
  g_rand_free (bmd->rand);
  bmd->rand = NULL;
  GST_OBJECT_UNLOCK (bmd);

  return TRUE;
}
