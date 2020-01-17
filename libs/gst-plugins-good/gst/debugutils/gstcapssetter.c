/* GStreamer Element
 * Copyright (C) 2006-2009 Mark Nauwelaerts <mnauw@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1307, USA.
 */

/**
 * SECTION:element-capssetter
 * @title: capssetter
 *
 * Sets or merges caps on a stream's buffers. That is, a buffer's caps are
 * updated using (fields of) #GstCapsSetter:caps. Note that this may contain
 * multiple structures (though not likely recommended), but each of these must
 * be fixed (or will otherwise be rejected).
 *
 * If #GstCapsSetter:join is %TRUE, then the incoming caps' mime-type is
 * compared to the mime-type(s) of provided caps and only matching structure(s)
 * are considered for updating.
 *
 * If #GstCapsSetter:replace is %TRUE, then any caps update is preceded by
 * clearing existing fields, making provided fields (as a whole) replace
 * incoming ones. Otherwise, no clearing is performed, in which case provided
 * fields are added/merged onto incoming caps
 *
 * Although this element might mainly serve as debug helper,
 * it can also practically be used to correct a faulty pixel-aspect-ratio,
 * or to modify a yuv fourcc value to effectively swap chroma components or such
 * alike.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstcapssetter.h"

#include <string.h>


GST_DEBUG_CATEGORY_STATIC (caps_setter_debug);
#define GST_CAT_DEFAULT caps_setter_debug


/* signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CAPS,
  PROP_JOIN,
  PROP_REPLACE
      /* FILL ME */
};

#define DEFAULT_JOIN              TRUE
#define DEFAULT_REPLACE           FALSE

static GstStaticPadTemplate gst_caps_setter_src_template =
GST_STATIC_PAD_TEMPLATE (GST_BASE_TRANSFORM_SRC_NAME,
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_caps_setter_sink_template =
GST_STATIC_PAD_TEMPLATE (GST_BASE_TRANSFORM_SINK_NAME,
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);


static gboolean gst_caps_setter_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size,
    GstCaps * othercaps, gsize * othersize);
static GstCaps *gst_caps_setter_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * cfilter);
static GstFlowReturn gst_caps_setter_transform_ip (GstBaseTransform * btrans,
    GstBuffer * in);

static void gst_caps_setter_finalize (GObject * object);

static void gst_caps_setter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_caps_setter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

#define gst_caps_setter_parent_class parent_class
G_DEFINE_TYPE (GstCapsSetter, gst_caps_setter, GST_TYPE_BASE_TRANSFORM);

static void
gst_caps_setter_class_init (GstCapsSetterClass * g_class)
{
  GObjectClass *gobject_class = (GObjectClass *) g_class;
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) g_class;

  GST_DEBUG_CATEGORY_INIT (caps_setter_debug, "capssetter", 0, "capssetter");

  gobject_class->set_property = gst_caps_setter_set_property;
  gobject_class->get_property = gst_caps_setter_get_property;

  gobject_class->finalize = gst_caps_setter_finalize;

  g_object_class_install_property (gobject_class, PROP_CAPS,
      g_param_spec_boxed ("caps", "Merge caps",
          "Merge these caps (thereby overwriting) in the stream",
          GST_TYPE_CAPS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_JOIN,
      g_param_spec_boolean ("join", "Join",
          "Match incoming caps' mime-type to mime-type of provided caps",
          DEFAULT_JOIN, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_REPLACE,
      g_param_spec_boolean ("replace", "Replace",
          "Drop fields of incoming caps", DEFAULT_REPLACE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class, "CapsSetter",
      "Generic",
      "Set/merge caps on stream",
      "Mark Nauwelaerts <mnauw@users.sourceforge.net>");

  gst_element_class_add_static_pad_template (element_class,
      &gst_caps_setter_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_caps_setter_src_template);

  trans_class->transform_size =
      GST_DEBUG_FUNCPTR (gst_caps_setter_transform_size);
  trans_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_caps_setter_transform_caps);
  /* dummy seems needed */
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_caps_setter_transform_ip);
}

static void
gst_caps_setter_init (GstCapsSetter * filter)
{
  filter->caps = gst_caps_new_any ();
  filter->join = DEFAULT_JOIN;
  filter->replace = DEFAULT_REPLACE;
}

static void
gst_caps_setter_finalize (GObject * object)
{
  GstCapsSetter *filter = GST_CAPS_SETTER (object);

  gst_caps_replace (&filter->caps, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_caps_setter_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size,
    GstCaps * othercaps, gsize * othersize)
{
  *othersize = size;

  return TRUE;
}

static GstCaps *
gst_caps_setter_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * cfilter)
{
  GstCapsSetter *filter = GST_CAPS_SETTER (trans);
  GstCaps *ret = NULL, *filter_caps = NULL;
  GstStructure *structure, *merge;
  const gchar *name;
  gint i, j, k;

  GST_DEBUG_OBJECT (trans,
      "receiving caps: %" GST_PTR_FORMAT ", with filter: %" GST_PTR_FORMAT,
      caps, cfilter);

  /* pass filter caps upstream, or any if no filter */
  if (direction != GST_PAD_SINK) {
    if (!cfilter || gst_caps_is_empty (cfilter)) {
      return gst_caps_ref (GST_CAPS_ANY);
    } else {
      return gst_caps_ref (cfilter);
    }
  }

  ret = gst_caps_copy (caps);

  GST_OBJECT_LOCK (filter);
  filter_caps = gst_caps_ref (filter->caps);
  GST_OBJECT_UNLOCK (filter);

  for (k = 0; k < gst_caps_get_size (ret); k++) {
    structure = gst_caps_get_structure (ret, k);
    name = gst_structure_get_name (structure);

    for (i = 0; i < gst_caps_get_size (filter_caps); ++i) {
      merge = gst_caps_get_structure (filter_caps, i);
      if (gst_structure_has_name (merge, name) || !filter->join) {

        if (!filter->join)
          gst_structure_set_name (structure, gst_structure_get_name (merge));

        if (filter->replace)
          gst_structure_remove_all_fields (structure);

        for (j = 0; j < gst_structure_n_fields (merge); ++j) {
          const gchar *fname;

          fname = gst_structure_nth_field_name (merge, j);
          gst_structure_set_value (structure, fname,
              gst_structure_get_value (merge, fname));
        }
      }
    }
  }

  GST_DEBUG_OBJECT (trans, "returning caps: %" GST_PTR_FORMAT, ret);

  gst_caps_unref (filter_caps);

  return ret;
}

static GstFlowReturn
gst_caps_setter_transform_ip (GstBaseTransform * btrans, GstBuffer * in)
{
  return GST_FLOW_OK;
}

static gboolean
gst_caps_is_fixed_foreach (GQuark field_id, const GValue * value,
    gpointer unused)
{
  return gst_value_is_fixed (value);
}

static void
gst_caps_setter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCapsSetter *filter = GST_CAPS_SETTER (object);

  switch (prop_id) {
    case PROP_CAPS:{
      GstCaps *new_caps;
      const GstCaps *new_caps_val = gst_value_get_caps (value);
      gint i;

      if (new_caps_val == NULL) {
        new_caps = gst_caps_new_any ();
      } else {
        new_caps = gst_caps_copy (new_caps_val);
      }

      for (i = 0; new_caps && (i < gst_caps_get_size (new_caps)); ++i) {
        GstStructure *s;

        s = gst_caps_get_structure (new_caps, i);
        if (!gst_structure_foreach (s, gst_caps_is_fixed_foreach, NULL)) {
          GST_ERROR_OBJECT (filter, "rejected unfixed caps: %" GST_PTR_FORMAT,
              new_caps);
          gst_caps_unref (new_caps);
          new_caps = NULL;
          break;
        }
      }

      if (new_caps) {
        GST_OBJECT_LOCK (filter);
        gst_caps_replace (&filter->caps, new_caps);
        /* drop extra ref */
        gst_caps_unref (new_caps);
        GST_OBJECT_UNLOCK (filter);

        GST_DEBUG_OBJECT (filter, "set new caps %" GST_PTR_FORMAT, new_caps);
      }

      /* try to activate these new caps next time around */
      gst_base_transform_reconfigure_src (GST_BASE_TRANSFORM (filter));
      break;
    }
    case PROP_JOIN:
      filter->join = g_value_get_boolean (value);
      break;
    case PROP_REPLACE:
      filter->replace = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_caps_setter_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstCapsSetter *filter = GST_CAPS_SETTER (object);

  switch (prop_id) {
    case PROP_CAPS:
      gst_value_set_caps (value, filter->caps);
      break;
    case PROP_JOIN:
      g_value_set_boolean (value, filter->join);
      break;
    case PROP_REPLACE:
      g_value_set_boolean (value, filter->replace);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
