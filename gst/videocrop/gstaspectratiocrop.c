/* GStreamer video frame cropping to aspect-ratio
 * Copyright (C) 2009 Thijs Vermeir <thijsvermeir@gmail.com>
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
 * SECTION:element-aspectratiocrop
 * @title: aspectratiocrop
 * @see_also: #GstVideoCrop
 *
 * This element crops video frames to a specified #GstAspectRatioCrop:aspect-ratio.
 *
 * If the aspect-ratio is already correct, the element will operate
 * in pass-through mode.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! video/x-raw,height=640,width=480 ! aspectratiocrop aspect-ratio=16/9 ! ximagesink
 * ]| This pipeline generates a videostream in 4/3 and crops it to 16/9.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>

#include "gstaspectratiocrop.h"

#include "gst/glib-compat-private.h"

GST_DEBUG_CATEGORY_STATIC (aspect_ratio_crop_debug);
#define GST_CAT_DEFAULT aspect_ratio_crop_debug

enum
{
  PROP_0,
  PROP_ASPECT_RATIO_CROP,
};

/* we support the same caps as videocrop (sync changes) */
#define ASPECT_RATIO_CROP_CAPS                        \
  GST_VIDEO_CAPS_MAKE ("{ RGBx, xRGB, BGRx, xBGR, "    \
      "RGBA, ARGB, BGRA, ABGR, RGB, BGR, AYUV, YUY2, " \
      "YVYU, UYVY, I420, YV12, RGB16, RGB15, GRAY8, "  \
      "NV12, NV21, GRAY16_LE, GRAY16_BE }")

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (ASPECT_RATIO_CROP_CAPS)
    );

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (ASPECT_RATIO_CROP_CAPS)
    );

#define gst_aspect_ratio_crop_parent_class parent_class
G_DEFINE_TYPE (GstAspectRatioCrop, gst_aspect_ratio_crop, GST_TYPE_BIN);

static void gst_aspect_ratio_crop_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_aspect_ratio_crop_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_aspect_ratio_crop_set_cropping (GstAspectRatioCrop *
    aspect_ratio_crop, gint top, gint right, gint bottom, gint left);
static GstCaps *gst_aspect_ratio_crop_get_caps (GstPad * pad, GstCaps * filter);
static gboolean gst_aspect_ratio_crop_src_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static gboolean gst_aspect_ratio_crop_set_caps (GstAspectRatioCrop *
    aspect_ratio_crop, GstCaps * caps);
static gboolean gst_aspect_ratio_crop_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * evt);
static void gst_aspect_ratio_crop_finalize (GObject * object);
static void gst_aspect_ratio_transform_structure (GstAspectRatioCrop *
    aspect_ratio_crop, GstStructure * structure, GstStructure ** new_structure,
    gboolean set_videocrop);

static void
gst_aspect_ratio_crop_set_cropping (GstAspectRatioCrop * aspect_ratio_crop,
    gint top, gint right, gint bottom, gint left)
{
  GValue value = { 0 };
  if (G_UNLIKELY (!aspect_ratio_crop->videocrop)) {
    GST_WARNING_OBJECT (aspect_ratio_crop,
        "Can't set the settings if there is no cropping element");
    return;
  }

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, top);
  GST_DEBUG_OBJECT (aspect_ratio_crop, "set top cropping to: %d", top);
  g_object_set_property (G_OBJECT (aspect_ratio_crop->videocrop), "top",
      &value);
  g_value_set_int (&value, right);
  GST_DEBUG_OBJECT (aspect_ratio_crop, "set right cropping to: %d", right);
  g_object_set_property (G_OBJECT (aspect_ratio_crop->videocrop), "right",
      &value);
  g_value_set_int (&value, bottom);
  GST_DEBUG_OBJECT (aspect_ratio_crop, "set bottom cropping to: %d", bottom);
  g_object_set_property (G_OBJECT (aspect_ratio_crop->videocrop), "bottom",
      &value);
  g_value_set_int (&value, left);
  GST_DEBUG_OBJECT (aspect_ratio_crop, "set left cropping to: %d", left);
  g_object_set_property (G_OBJECT (aspect_ratio_crop->videocrop), "left",
      &value);

  g_value_unset (&value);
}

static gboolean
gst_aspect_ratio_crop_set_caps (GstAspectRatioCrop * aspect_ratio_crop,
    GstCaps * caps)
{
  GstPad *peer_pad;
  GstStructure *structure;
  gboolean ret;

  g_mutex_lock (&aspect_ratio_crop->crop_lock);

  structure = gst_caps_get_structure (caps, 0);
  gst_aspect_ratio_transform_structure (aspect_ratio_crop, structure, NULL,
      TRUE);
  peer_pad =
      gst_element_get_static_pad (GST_ELEMENT (aspect_ratio_crop->videocrop),
      "sink");
  ret = gst_pad_set_caps (peer_pad, caps);
  gst_object_unref (peer_pad);
  g_mutex_unlock (&aspect_ratio_crop->crop_lock);
  return ret;
}

static gboolean
gst_aspect_ratio_crop_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * evt)
{
  GstAspectRatioCrop *aspect_ratio_crop = GST_ASPECT_RATIO_CROP (parent);

  switch (GST_EVENT_TYPE (evt)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (evt, &caps);
      gst_aspect_ratio_crop_set_caps (aspect_ratio_crop, caps);
      break;
    }
    default:
      break;
  }

  return gst_pad_event_default (pad, parent, evt);
}

static void
gst_aspect_ratio_crop_class_init (GstAspectRatioCropClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_aspect_ratio_crop_set_property;
  gobject_class->get_property = gst_aspect_ratio_crop_get_property;
  gobject_class->finalize = gst_aspect_ratio_crop_finalize;

  g_object_class_install_property (gobject_class, PROP_ASPECT_RATIO_CROP,
      gst_param_spec_fraction ("aspect-ratio", "aspect-ratio",
          "Target aspect-ratio of video", 0, 1, G_MAXINT, 1, 0, 1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class, "aspectratiocrop",
      "Filter/Effect/Video",
      "Crops video into a user-defined aspect-ratio",
      "Thijs Vermeir <thijsvermeir@gmail.com>");

  gst_element_class_add_static_pad_template (element_class, &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);
}

static void
gst_aspect_ratio_crop_finalize (GObject * object)
{
  GstAspectRatioCrop *aspect_ratio_crop;

  aspect_ratio_crop = GST_ASPECT_RATIO_CROP (object);

  g_mutex_clear (&aspect_ratio_crop->crop_lock);
  gst_clear_caps (&aspect_ratio_crop->renegotiation_caps);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstFlowReturn
gst_aspect_ratio_crop_sink_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{
  GstCaps *caps = NULL;
  GstAspectRatioCrop *aspect_ratio_crop = GST_ASPECT_RATIO_CROP (parent);

  GST_OBJECT_LOCK (parent);
  caps = aspect_ratio_crop->renegotiation_caps;
  aspect_ratio_crop->renegotiation_caps = NULL;
  GST_OBJECT_UNLOCK (parent);

  if (caps) {
    gst_aspect_ratio_crop_set_caps (GST_ASPECT_RATIO_CROP (parent), caps);
    gst_caps_unref (caps);
  }

  return gst_proxy_pad_chain_default (pad, parent, buffer);

}

static void
gst_aspect_ratio_crop_init (GstAspectRatioCrop * aspect_ratio_crop)
{
  GstPad *link_pad;
  GstPad *src_pad;

  GST_DEBUG_CATEGORY_INIT (aspect_ratio_crop_debug, "aspectratiocrop", 0,
      "aspectratiocrop");

  aspect_ratio_crop->ar_num = 0;
  aspect_ratio_crop->ar_denom = 1;

  g_mutex_init (&aspect_ratio_crop->crop_lock);

  /* add the transform element */
  aspect_ratio_crop->videocrop = gst_element_factory_make ("videocrop", NULL);
  gst_bin_add (GST_BIN (aspect_ratio_crop), aspect_ratio_crop->videocrop);

  /* create ghost pad src */
  link_pad =
      gst_element_get_static_pad (GST_ELEMENT (aspect_ratio_crop->videocrop),
      "src");
  src_pad = gst_ghost_pad_new ("src", link_pad);
  gst_pad_set_query_function (src_pad,
      GST_DEBUG_FUNCPTR (gst_aspect_ratio_crop_src_query));
  gst_element_add_pad (GST_ELEMENT (aspect_ratio_crop), src_pad);
  gst_object_unref (link_pad);
  /* create ghost pad sink */
  link_pad =
      gst_element_get_static_pad (GST_ELEMENT (aspect_ratio_crop->videocrop),
      "sink");
  aspect_ratio_crop->sink = gst_ghost_pad_new ("sink", link_pad);
  gst_element_add_pad (GST_ELEMENT (aspect_ratio_crop),
      aspect_ratio_crop->sink);
  gst_object_unref (link_pad);

  gst_pad_set_event_function (aspect_ratio_crop->sink,
      GST_DEBUG_FUNCPTR (gst_aspect_ratio_crop_sink_event));
  gst_pad_set_chain_function (aspect_ratio_crop->sink,
      GST_DEBUG_FUNCPTR (gst_aspect_ratio_crop_sink_chain));
}

static void
gst_aspect_ratio_transform_structure (GstAspectRatioCrop * aspect_ratio_crop,
    GstStructure * structure, GstStructure ** new_structure,
    gboolean set_videocrop)
{
  gdouble incoming_ar;
  gdouble requested_ar;
  gint width, height;
  gint cropvalue;
  gint par_d, par_n;

  /* Check if we need to change the aspect ratio */
  if (aspect_ratio_crop->ar_num < 1) {
    GST_DEBUG_OBJECT (aspect_ratio_crop, "No cropping requested");
    goto beach;
  }

  /* get the information from the caps */
  if (!gst_structure_get_int (structure, "width", &width) ||
      !gst_structure_get_int (structure, "height", &height))
    goto beach;

  if (!gst_structure_get_fraction (structure, "pixel-aspect-ratio",
          &par_n, &par_d)) {
    par_d = par_n = 1;
  }

  incoming_ar = ((gdouble) (width * par_n)) / (height * par_d);
  GST_LOG_OBJECT (aspect_ratio_crop,
      "incoming caps width(%d), height(%d), par (%d/%d) : ar = %f", width,
      height, par_n, par_d, incoming_ar);

  requested_ar =
      (gdouble) aspect_ratio_crop->ar_num / aspect_ratio_crop->ar_denom;

  /* check if the original aspect-ratio is the aspect-ratio that we want */
  if (requested_ar == incoming_ar) {
    GST_DEBUG_OBJECT (aspect_ratio_crop,
        "Input video already has the correct aspect ratio (%.3f == %.3f)",
        incoming_ar, requested_ar);
    goto beach;
  } else if (requested_ar > incoming_ar) {
    /* fix aspect ratio with cropping on top and bottom */
    cropvalue =
        ((((double) aspect_ratio_crop->ar_denom /
                (double) (aspect_ratio_crop->ar_num)) * ((double) par_n /
                (double) par_d) * width) - height) / 2;
    if (cropvalue < 0) {
      cropvalue *= -1;
    }
    if (cropvalue >= (height / 2))
      goto crop_failed;
    if (set_videocrop) {
      gst_aspect_ratio_crop_set_cropping (aspect_ratio_crop, cropvalue, 0,
          cropvalue, 0);
    }
    if (new_structure) {
      *new_structure = gst_structure_copy (structure);
      gst_structure_set (*new_structure,
          "height", G_TYPE_INT, (int) (height - (cropvalue * 2)), NULL);
    }
  } else {
    /* fix aspect ratio with cropping on left and right */
    cropvalue =
        ((((double) aspect_ratio_crop->ar_num /
                (double) (aspect_ratio_crop->ar_denom)) * ((double) par_d /
                (double) par_n) * height) - width) / 2;
    if (cropvalue < 0) {
      cropvalue *= -1;
    }
    if (cropvalue >= (width / 2))
      goto crop_failed;
    if (set_videocrop) {
      gst_aspect_ratio_crop_set_cropping (aspect_ratio_crop, 0, cropvalue,
          0, cropvalue);
    }
    if (new_structure) {
      *new_structure = gst_structure_copy (structure);
      gst_structure_set (*new_structure,
          "width", G_TYPE_INT, (int) (width - (cropvalue * 2)), NULL);
    }
  }

  return;

crop_failed:
  GST_WARNING_OBJECT (aspect_ratio_crop,
      "can't crop to aspect ratio requested");
  goto beach;
beach:
  if (set_videocrop) {
    gst_aspect_ratio_crop_set_cropping (aspect_ratio_crop, 0, 0, 0, 0);
  }

  if (new_structure) {
    *new_structure = gst_structure_copy (structure);
  }
}

static GstCaps *
gst_aspect_ratio_crop_transform_caps (GstAspectRatioCrop * aspect_ratio_crop,
    GstCaps * caps)
{
  GstCaps *transform;
  gint size, i;

  transform = gst_caps_new_empty ();

  size = gst_caps_get_size (caps);

  for (i = 0; i < size; i++) {
    GstStructure *s;
    GstStructure *trans_s;

    s = gst_caps_get_structure (caps, i);

    gst_aspect_ratio_transform_structure (aspect_ratio_crop, s, &trans_s,
        FALSE);
    gst_caps_append_structure (transform, trans_s);
  }

  return transform;
}

static GstCaps *
gst_aspect_ratio_crop_get_caps (GstPad * pad, GstCaps * filter)
{
  GstPad *peer;
  GstAspectRatioCrop *aspect_ratio_crop;
  GstCaps *return_caps;

  aspect_ratio_crop = GST_ASPECT_RATIO_CROP (gst_pad_get_parent (pad));

  g_mutex_lock (&aspect_ratio_crop->crop_lock);

  peer = gst_pad_get_peer (aspect_ratio_crop->sink);
  if (peer == NULL) {
    return_caps = gst_static_pad_template_get_caps (&src_template);
  } else {
    GstCaps *peer_caps;

    peer_caps = gst_pad_query_caps (peer, filter);
    return_caps =
        gst_aspect_ratio_crop_transform_caps (aspect_ratio_crop, peer_caps);
    gst_caps_unref (peer_caps);
    gst_object_unref (peer);
  }

  g_mutex_unlock (&aspect_ratio_crop->crop_lock);
  gst_object_unref (aspect_ratio_crop);

  if (return_caps && filter) {
    GstCaps *tmp =
        gst_caps_intersect_full (filter, return_caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_replace (&return_caps, tmp);
    gst_caps_unref (tmp);
  }

  return return_caps;
}

static gboolean
gst_aspect_ratio_crop_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = gst_aspect_ratio_crop_get_caps (pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }
  return res;
}

static void
gst_aspect_ratio_crop_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAspectRatioCrop *aspect_ratio_crop;
  gboolean recheck = FALSE;

  aspect_ratio_crop = GST_ASPECT_RATIO_CROP (object);

  GST_OBJECT_LOCK (aspect_ratio_crop);
  switch (prop_id) {
    case PROP_ASPECT_RATIO_CROP:
      if (GST_VALUE_HOLDS_FRACTION (value)) {
        aspect_ratio_crop->ar_num = gst_value_get_fraction_numerator (value);
        aspect_ratio_crop->ar_denom =
            gst_value_get_fraction_denominator (value);
        recheck = gst_pad_has_current_caps (aspect_ratio_crop->sink);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (aspect_ratio_crop);

  if (recheck) {
    GST_OBJECT_LOCK (aspect_ratio_crop);
    gst_clear_caps (&aspect_ratio_crop->renegotiation_caps);
    aspect_ratio_crop->renegotiation_caps =
        gst_pad_get_current_caps (aspect_ratio_crop->sink);
    GST_OBJECT_UNLOCK (aspect_ratio_crop);
  }
}

static void
gst_aspect_ratio_crop_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAspectRatioCrop *aspect_ratio_crop;

  aspect_ratio_crop = GST_ASPECT_RATIO_CROP (object);

  GST_OBJECT_LOCK (aspect_ratio_crop);
  switch (prop_id) {
    case PROP_ASPECT_RATIO_CROP:
      gst_value_set_fraction (value, aspect_ratio_crop->ar_num,
          aspect_ratio_crop->ar_denom);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (aspect_ratio_crop);
}
