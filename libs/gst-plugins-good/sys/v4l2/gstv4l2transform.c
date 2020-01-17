/*
 * Copyright (C) 2014 Collabora Ltd.
 *     Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
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
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "gstv4l2object.h"
#include "gstv4l2transform.h"

#include <string.h>
#include <gst/gst-i18n-plugin.h>

#define DEFAULT_PROP_DEVICE "/dev/video10"

#define V4L2_TRANSFORM_QUARK \
	g_quark_from_static_string("gst-v4l2-transform-info")

GST_DEBUG_CATEGORY_STATIC (gst_v4l2_transform_debug);
#define GST_CAT_DEFAULT gst_v4l2_transform_debug


enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS,
  PROP_DISABLE_PASSTHROUGH
};

typedef struct
{
  gchar *device;
  GstCaps *sink_caps;
  GstCaps *src_caps;
} GstV4l2TransformCData;

#define gst_v4l2_transform_parent_class parent_class
G_DEFINE_ABSTRACT_TYPE (GstV4l2Transform, gst_v4l2_transform,
    GST_TYPE_BASE_TRANSFORM);

static void
gst_v4l2_transform_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (object);

  switch (prop_id) {
    case PROP_CAPTURE_IO_MODE:
      gst_v4l2_object_set_property_helper (self->v4l2capture, prop_id, value,
          pspec);
      break;
    case PROP_DISABLE_PASSTHROUGH:
      self->disable_passthrough = g_value_get_boolean (value);
      break;

      /* By default, only set on output */
    default:
      if (!gst_v4l2_object_set_property_helper (self->v4l2output,
              prop_id, value, pspec)) {
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
      break;
  }
}

static void
gst_v4l2_transform_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (object);

  switch (prop_id) {
    case PROP_CAPTURE_IO_MODE:
      gst_v4l2_object_get_property_helper (self->v4l2capture, prop_id, value,
          pspec);
      break;
    case PROP_DISABLE_PASSTHROUGH:
      g_value_set_boolean (value, self->disable_passthrough);
      break;

      /* By default read from output */
    default:
      if (!gst_v4l2_object_get_property_helper (self->v4l2output,
              prop_id, value, pspec)) {
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
      break;
  }
}

static gboolean
gst_v4l2_transform_open (GstV4l2Transform * self)
{
  GST_DEBUG_OBJECT (self, "Opening");

  if (!gst_v4l2_object_open (self->v4l2output))
    goto failure;

  if (!gst_v4l2_object_open_shared (self->v4l2capture, self->v4l2output))
    goto failure;

  self->probed_sinkcaps = gst_v4l2_object_get_caps (self->v4l2output,
      gst_v4l2_object_get_raw_caps ());

  if (gst_caps_is_empty (self->probed_sinkcaps))
    goto no_input_format;

  self->probed_srccaps = gst_v4l2_object_get_caps (self->v4l2capture,
      gst_v4l2_object_get_raw_caps ());

  if (gst_caps_is_empty (self->probed_srccaps))
    goto no_output_format;

  return TRUE;

no_input_format:
  GST_ELEMENT_ERROR (self, RESOURCE, SETTINGS,
      (_("Converter on device %s has no supported input format"),
          self->v4l2output->videodev), (NULL));
  goto failure;


no_output_format:
  GST_ELEMENT_ERROR (self, RESOURCE, SETTINGS,
      (_("Converter on device %s has no supported output format"),
          self->v4l2output->videodev), (NULL));
  goto failure;

failure:
  if (GST_V4L2_IS_OPEN (self->v4l2output))
    gst_v4l2_object_close (self->v4l2output);

  if (GST_V4L2_IS_OPEN (self->v4l2capture))
    gst_v4l2_object_close (self->v4l2capture);

  gst_caps_replace (&self->probed_srccaps, NULL);
  gst_caps_replace (&self->probed_sinkcaps, NULL);

  return FALSE;
}

static void
gst_v4l2_transform_close (GstV4l2Transform * self)
{
  GST_DEBUG_OBJECT (self, "Closing");

  gst_v4l2_object_close (self->v4l2output);
  gst_v4l2_object_close (self->v4l2capture);

  gst_caps_replace (&self->probed_srccaps, NULL);
  gst_caps_replace (&self->probed_sinkcaps, NULL);
}

static gboolean
gst_v4l2_transform_stop (GstBaseTransform * trans)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (trans);

  GST_DEBUG_OBJECT (self, "Stop");

  gst_v4l2_object_stop (self->v4l2output);
  gst_v4l2_object_stop (self->v4l2capture);
  gst_caps_replace (&self->incaps, NULL);
  gst_caps_replace (&self->outcaps, NULL);

  return TRUE;
}

static gboolean
gst_v4l2_transform_set_caps (GstBaseTransform * trans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstV4l2Error error = GST_V4L2_ERROR_INIT;
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (trans);

  if (self->disable_passthrough)
    gst_base_transform_set_passthrough (trans, FALSE);

  if (self->incaps && self->outcaps) {
    if (gst_caps_is_equal (incaps, self->incaps) &&
        gst_caps_is_equal (outcaps, self->outcaps)) {
      GST_DEBUG_OBJECT (trans, "Caps did not changed");
      return TRUE;
    }
  }

  /* TODO Add renegotiation support */
  g_return_val_if_fail (!GST_V4L2_IS_ACTIVE (self->v4l2output), FALSE);
  g_return_val_if_fail (!GST_V4L2_IS_ACTIVE (self->v4l2capture), FALSE);

  gst_caps_replace (&self->incaps, incaps);
  gst_caps_replace (&self->outcaps, outcaps);

  if (!gst_v4l2_object_set_format (self->v4l2output, incaps, &error))
    goto incaps_failed;

  if (!gst_v4l2_object_set_format (self->v4l2capture, outcaps, &error))
    goto outcaps_failed;

  /* FIXME implement fallback if crop not supported */
  if (!gst_v4l2_object_set_crop (self->v4l2output))
    goto failed;

  if (!gst_v4l2_object_set_crop (self->v4l2capture))
    goto failed;

  return TRUE;

incaps_failed:
  {
    GST_ERROR_OBJECT (self, "failed to set input caps: %" GST_PTR_FORMAT,
        incaps);
    gst_v4l2_error (self, &error);
    goto failed;
  }
outcaps_failed:
  {
    gst_v4l2_object_stop (self->v4l2output);
    GST_ERROR_OBJECT (self, "failed to set output caps: %" GST_PTR_FORMAT,
        outcaps);
    gst_v4l2_error (self, &error);
    goto failed;
  }
failed:
  return FALSE;
}

static gboolean
gst_v4l2_transform_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (trans);
  gboolean ret = TRUE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:{
      GstCaps *filter, *caps = NULL, *result = NULL;
      GstPad *pad, *otherpad;

      gst_query_parse_caps (query, &filter);

      if (direction == GST_PAD_SRC) {
        pad = GST_BASE_TRANSFORM_SRC_PAD (trans);
        otherpad = GST_BASE_TRANSFORM_SINK_PAD (trans);
        if (self->probed_srccaps)
          caps = gst_caps_ref (self->probed_srccaps);
      } else {
        pad = GST_BASE_TRANSFORM_SINK_PAD (trans);
        otherpad = GST_BASE_TRANSFORM_SRC_PAD (trans);
        if (self->probed_sinkcaps)
          caps = gst_caps_ref (self->probed_sinkcaps);
      }

      if (!caps)
        caps = gst_pad_get_pad_template_caps (pad);

      if (filter) {
        GstCaps *tmp = caps;
        caps = gst_caps_intersect_full (filter, tmp, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref (tmp);
      }

      result = gst_pad_peer_query_caps (otherpad, caps);
      result = gst_caps_make_writable (result);
      gst_caps_append (result, caps);

      GST_DEBUG_OBJECT (self, "Returning %s caps %" GST_PTR_FORMAT,
          GST_PAD_NAME (pad), result);

      gst_query_set_caps_result (query, result);
      gst_caps_unref (result);
      break;
    }

    default:
      ret = GST_BASE_TRANSFORM_CLASS (parent_class)->query (trans, direction,
          query);
      break;
  }

  return ret;
}

static gboolean
gst_v4l2_transform_decide_allocation (GstBaseTransform * trans,
    GstQuery * query)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (trans);
  gboolean ret = FALSE;

  GST_DEBUG_OBJECT (self, "called");

  if (gst_v4l2_object_decide_allocation (self->v4l2capture, query)) {
    GstBufferPool *pool = GST_BUFFER_POOL (self->v4l2capture->pool);

    ret = GST_BASE_TRANSFORM_CLASS (parent_class)->decide_allocation (trans,
        query);

    if (!gst_buffer_pool_set_active (pool, TRUE))
      goto activate_failed;
  }

  return ret;

activate_failed:
  GST_ELEMENT_ERROR (self, RESOURCE, SETTINGS,
      ("failed to activate bufferpool"), ("failed to activate bufferpool"));
  return TRUE;
}

static gboolean
gst_v4l2_transform_propose_allocation (GstBaseTransform * trans,
    GstQuery * decide_query, GstQuery * query)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (trans);
  gboolean ret = FALSE;

  GST_DEBUG_OBJECT (self, "called");

  if (decide_query == NULL)
    ret = TRUE;
  else
    ret = gst_v4l2_object_propose_allocation (self->v4l2output, query);

  if (ret)
    ret = GST_BASE_TRANSFORM_CLASS (parent_class)->propose_allocation (trans,
        decide_query, query);

  return ret;
}

/* copies the given caps */
static GstCaps *
gst_v4l2_transform_caps_remove_format_info (GstCaps * caps)
{
  GstStructure *st;
  GstCapsFeatures *f;
  gint i, n;
  GstCaps *res;

  res = gst_caps_new_empty ();

  n = gst_caps_get_size (caps);
  for (i = 0; i < n; i++) {
    st = gst_caps_get_structure (caps, i);
    f = gst_caps_get_features (caps, i);

    /* If this is already expressed by the existing caps
     * skip this structure */
    if (i > 0 && gst_caps_is_subset_structure_full (res, st, f))
      continue;

    st = gst_structure_copy (st);
    /* Only remove format info for the cases when we can actually convert */
    if (!gst_caps_features_is_any (f)
        && gst_caps_features_is_equal (f,
            GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY))
      gst_structure_remove_fields (st, "format", "colorimetry", "chroma-site",
          "width", "height", "pixel-aspect-ratio", NULL);

    gst_caps_append_structure_full (res, st, gst_caps_features_copy (f));
  }

  return res;
}

/* The caps can be transformed into any other caps with format info removed.
 * However, we should prefer passthrough, so if passthrough is possible,
 * put it first in the list. */
static GstCaps *
gst_v4l2_transform_transform_caps (GstBaseTransform * btrans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstCaps *tmp, *tmp2;
  GstCaps *result;

  /* Get all possible caps that we can transform to */
  tmp = gst_v4l2_transform_caps_remove_format_info (caps);

  if (filter) {
    tmp2 = gst_caps_intersect_full (filter, tmp, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tmp);
    tmp = tmp2;
  }

  result = tmp;

  GST_DEBUG_OBJECT (btrans, "transformed %" GST_PTR_FORMAT " into %"
      GST_PTR_FORMAT, caps, result);

  return result;
}

static GstCaps *
gst_v4l2_transform_fixate_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps)
{
  GstStructure *ins, *outs;
  const GValue *from_par, *to_par;
  GValue fpar = { 0, }, tpar = {
  0,};

  othercaps = gst_caps_truncate (othercaps);
  othercaps = gst_caps_make_writable (othercaps);

  GST_DEBUG_OBJECT (trans, "trying to fixate othercaps %" GST_PTR_FORMAT
      " based on caps %" GST_PTR_FORMAT, othercaps, caps);

  ins = gst_caps_get_structure (caps, 0);
  outs = gst_caps_get_structure (othercaps, 0);

  {
    const gchar *in_format;

    in_format = gst_structure_get_string (ins, "format");
    if (in_format) {
      /* Try to set output format for pass through */
      gst_structure_fixate_field_string (outs, "format", in_format);
    }

  }

  from_par = gst_structure_get_value (ins, "pixel-aspect-ratio");
  to_par = gst_structure_get_value (outs, "pixel-aspect-ratio");

  /* If we're fixating from the sinkpad we always set the PAR and
   * assume that missing PAR on the sinkpad means 1/1 and
   * missing PAR on the srcpad means undefined
   */
  if (direction == GST_PAD_SINK) {
    if (!from_par) {
      g_value_init (&fpar, GST_TYPE_FRACTION);
      gst_value_set_fraction (&fpar, 1, 1);
      from_par = &fpar;
    }
    if (!to_par) {
      g_value_init (&tpar, GST_TYPE_FRACTION_RANGE);
      gst_value_set_fraction_range_full (&tpar, 1, G_MAXINT, G_MAXINT, 1);
      to_par = &tpar;
    }
  } else {
    if (!to_par) {
      g_value_init (&tpar, GST_TYPE_FRACTION);
      gst_value_set_fraction (&tpar, 1, 1);
      to_par = &tpar;

      gst_structure_set (outs, "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
          NULL);
    }
    if (!from_par) {
      g_value_init (&fpar, GST_TYPE_FRACTION);
      gst_value_set_fraction (&fpar, 1, 1);
      from_par = &fpar;
    }
  }

  /* we have both PAR but they might not be fixated */
  {
    gint from_w, from_h, from_par_n, from_par_d, to_par_n, to_par_d;
    gint w = 0, h = 0;
    gint from_dar_n, from_dar_d;
    gint num, den;

    /* from_par should be fixed */
    g_return_val_if_fail (gst_value_is_fixed (from_par), othercaps);

    from_par_n = gst_value_get_fraction_numerator (from_par);
    from_par_d = gst_value_get_fraction_denominator (from_par);

    gst_structure_get_int (ins, "width", &from_w);
    gst_structure_get_int (ins, "height", &from_h);

    gst_structure_get_int (outs, "width", &w);
    gst_structure_get_int (outs, "height", &h);

    /* if both width and height are already fixed, we can't do anything
     * about it anymore */
    if (w && h) {
      guint n, d;

      GST_DEBUG_OBJECT (trans, "dimensions already set to %dx%d, not fixating",
          w, h);
      if (!gst_value_is_fixed (to_par)) {
        if (gst_video_calculate_display_ratio (&n, &d, from_w, from_h,
                from_par_n, from_par_d, w, h)) {
          GST_DEBUG_OBJECT (trans, "fixating to_par to %dx%d", n, d);
          if (gst_structure_has_field (outs, "pixel-aspect-ratio"))
            gst_structure_fixate_field_nearest_fraction (outs,
                "pixel-aspect-ratio", n, d);
          else if (n != d)
            gst_structure_set (outs, "pixel-aspect-ratio", GST_TYPE_FRACTION,
                n, d, NULL);
        }
      }
      goto done;
    }

    /* Calculate input DAR */
    if (!gst_util_fraction_multiply (from_w, from_h, from_par_n, from_par_d,
            &from_dar_n, &from_dar_d)) {
      GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
          ("Error calculating the output scaled size - integer overflow"));
      goto done;
    }

    GST_DEBUG_OBJECT (trans, "Input DAR is %d/%d", from_dar_n, from_dar_d);

    /* If either width or height are fixed there's not much we
     * can do either except choosing a height or width and PAR
     * that matches the DAR as good as possible
     */
    if (h) {
      GstStructure *tmp;
      gint set_w, set_par_n, set_par_d;

      GST_DEBUG_OBJECT (trans, "height is fixed (%d)", h);

      /* If the PAR is fixed too, there's not much to do
       * except choosing the width that is nearest to the
       * width with the same DAR */
      if (gst_value_is_fixed (to_par)) {
        to_par_n = gst_value_get_fraction_numerator (to_par);
        to_par_d = gst_value_get_fraction_denominator (to_par);

        GST_DEBUG_OBJECT (trans, "PAR is fixed %d/%d", to_par_n, to_par_d);

        if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, to_par_d,
                to_par_n, &num, &den)) {
          GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
              ("Error calculating the output scaled size - integer overflow"));
          goto done;
        }

        w = (guint) gst_util_uint64_scale_int (h, num, den);
        gst_structure_fixate_field_nearest_int (outs, "width", w);

        goto done;
      }

      /* The PAR is not fixed and it's quite likely that we can set
       * an arbitrary PAR. */

      /* Check if we can keep the input width */
      tmp = gst_structure_copy (outs);
      gst_structure_fixate_field_nearest_int (tmp, "width", from_w);
      gst_structure_get_int (tmp, "width", &set_w);

      /* Might have failed but try to keep the DAR nonetheless by
       * adjusting the PAR */
      if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, h, set_w,
              &to_par_n, &to_par_d)) {
        GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
            ("Error calculating the output scaled size - integer overflow"));
        gst_structure_free (tmp);
        goto done;
      }

      if (!gst_structure_has_field (tmp, "pixel-aspect-ratio"))
        gst_structure_set_value (tmp, "pixel-aspect-ratio", to_par);
      gst_structure_fixate_field_nearest_fraction (tmp, "pixel-aspect-ratio",
          to_par_n, to_par_d);
      gst_structure_get_fraction (tmp, "pixel-aspect-ratio", &set_par_n,
          &set_par_d);
      gst_structure_free (tmp);

      /* Check if the adjusted PAR is accepted */
      if (set_par_n == to_par_n && set_par_d == to_par_d) {
        if (gst_structure_has_field (outs, "pixel-aspect-ratio") ||
            set_par_n != set_par_d)
          gst_structure_set (outs, "width", G_TYPE_INT, set_w,
              "pixel-aspect-ratio", GST_TYPE_FRACTION, set_par_n, set_par_d,
              NULL);
        goto done;
      }

      /* Otherwise scale the width to the new PAR and check if the
       * adjusted with is accepted. If all that fails we can't keep
       * the DAR */
      if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, set_par_d,
              set_par_n, &num, &den)) {
        GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
            ("Error calculating the output scaled size - integer overflow"));
        goto done;
      }

      w = (guint) gst_util_uint64_scale_int (h, num, den);
      gst_structure_fixate_field_nearest_int (outs, "width", w);
      if (gst_structure_has_field (outs, "pixel-aspect-ratio") ||
          set_par_n != set_par_d)
        gst_structure_set (outs, "pixel-aspect-ratio", GST_TYPE_FRACTION,
            set_par_n, set_par_d, NULL);

      goto done;
    } else if (w) {
      GstStructure *tmp;
      gint set_h, set_par_n, set_par_d;

      GST_DEBUG_OBJECT (trans, "width is fixed (%d)", w);

      /* If the PAR is fixed too, there's not much to do
       * except choosing the height that is nearest to the
       * height with the same DAR */
      if (gst_value_is_fixed (to_par)) {
        to_par_n = gst_value_get_fraction_numerator (to_par);
        to_par_d = gst_value_get_fraction_denominator (to_par);

        GST_DEBUG_OBJECT (trans, "PAR is fixed %d/%d", to_par_n, to_par_d);

        if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, to_par_d,
                to_par_n, &num, &den)) {
          GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
              ("Error calculating the output scaled size - integer overflow"));
          goto done;
        }

        h = (guint) gst_util_uint64_scale_int (w, den, num);
        gst_structure_fixate_field_nearest_int (outs, "height", h);

        goto done;
      }

      /* The PAR is not fixed and it's quite likely that we can set
       * an arbitrary PAR. */

      /* Check if we can keep the input height */
      tmp = gst_structure_copy (outs);
      gst_structure_fixate_field_nearest_int (tmp, "height", from_h);
      gst_structure_get_int (tmp, "height", &set_h);

      /* Might have failed but try to keep the DAR nonetheless by
       * adjusting the PAR */
      if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, set_h, w,
              &to_par_n, &to_par_d)) {
        GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
            ("Error calculating the output scaled size - integer overflow"));
        gst_structure_free (tmp);
        goto done;
      }
      if (!gst_structure_has_field (tmp, "pixel-aspect-ratio"))
        gst_structure_set_value (tmp, "pixel-aspect-ratio", to_par);
      gst_structure_fixate_field_nearest_fraction (tmp, "pixel-aspect-ratio",
          to_par_n, to_par_d);
      gst_structure_get_fraction (tmp, "pixel-aspect-ratio", &set_par_n,
          &set_par_d);
      gst_structure_free (tmp);

      /* Check if the adjusted PAR is accepted */
      if (set_par_n == to_par_n && set_par_d == to_par_d) {
        if (gst_structure_has_field (outs, "pixel-aspect-ratio") ||
            set_par_n != set_par_d)
          gst_structure_set (outs, "height", G_TYPE_INT, set_h,
              "pixel-aspect-ratio", GST_TYPE_FRACTION, set_par_n, set_par_d,
              NULL);
        goto done;
      }

      /* Otherwise scale the height to the new PAR and check if the
       * adjusted with is accepted. If all that fails we can't keep
       * the DAR */
      if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, set_par_d,
              set_par_n, &num, &den)) {
        GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
            ("Error calculating the output scaled size - integer overflow"));
        goto done;
      }

      h = (guint) gst_util_uint64_scale_int (w, den, num);
      gst_structure_fixate_field_nearest_int (outs, "height", h);
      if (gst_structure_has_field (outs, "pixel-aspect-ratio") ||
          set_par_n != set_par_d)
        gst_structure_set (outs, "pixel-aspect-ratio", GST_TYPE_FRACTION,
            set_par_n, set_par_d, NULL);

      goto done;
    } else if (gst_value_is_fixed (to_par)) {
      GstStructure *tmp;
      gint set_h, set_w, f_h, f_w;

      to_par_n = gst_value_get_fraction_numerator (to_par);
      to_par_d = gst_value_get_fraction_denominator (to_par);

      GST_DEBUG_OBJECT (trans, "PAR is fixed %d/%d", to_par_n, to_par_d);

      /* Calculate scale factor for the PAR change */
      if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, to_par_d,
              to_par_n, &num, &den)) {
        GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
            ("Error calculating the output scaled size - integer overflow"));
        goto done;
      }

      /* Try to keep the input height (because of interlacing) */
      tmp = gst_structure_copy (outs);
      gst_structure_fixate_field_nearest_int (tmp, "height", from_h);
      gst_structure_get_int (tmp, "height", &set_h);

      /* This might have failed but try to scale the width
       * to keep the DAR nonetheless */
      w = (guint) gst_util_uint64_scale_int (set_h, num, den);
      gst_structure_fixate_field_nearest_int (tmp, "width", w);
      gst_structure_get_int (tmp, "width", &set_w);
      gst_structure_free (tmp);

      /* We kept the DAR and the height is nearest to the original height */
      if (set_w == w) {
        gst_structure_set (outs, "width", G_TYPE_INT, set_w, "height",
            G_TYPE_INT, set_h, NULL);
        goto done;
      }

      f_h = set_h;
      f_w = set_w;

      /* If the former failed, try to keep the input width at least */
      tmp = gst_structure_copy (outs);
      gst_structure_fixate_field_nearest_int (tmp, "width", from_w);
      gst_structure_get_int (tmp, "width", &set_w);

      /* This might have failed but try to scale the width
       * to keep the DAR nonetheless */
      h = (guint) gst_util_uint64_scale_int (set_w, den, num);
      gst_structure_fixate_field_nearest_int (tmp, "height", h);
      gst_structure_get_int (tmp, "height", &set_h);
      gst_structure_free (tmp);

      /* We kept the DAR and the width is nearest to the original width */
      if (set_h == h) {
        gst_structure_set (outs, "width", G_TYPE_INT, set_w, "height",
            G_TYPE_INT, set_h, NULL);
        goto done;
      }

      /* If all this failed, keep the height that was nearest to the original
       * height and the nearest possible width. This changes the DAR but
       * there's not much else to do here.
       */
      gst_structure_set (outs, "width", G_TYPE_INT, f_w, "height", G_TYPE_INT,
          f_h, NULL);
      goto done;
    } else {
      GstStructure *tmp;
      gint set_h, set_w, set_par_n, set_par_d, tmp2;

      /* width, height and PAR are not fixed but passthrough is not possible */

      /* First try to keep the height and width as good as possible
       * and scale PAR */
      tmp = gst_structure_copy (outs);
      gst_structure_fixate_field_nearest_int (tmp, "height", from_h);
      gst_structure_get_int (tmp, "height", &set_h);
      gst_structure_fixate_field_nearest_int (tmp, "width", from_w);
      gst_structure_get_int (tmp, "width", &set_w);

      if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, set_h, set_w,
              &to_par_n, &to_par_d)) {
        GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
            ("Error calculating the output scaled size - integer overflow"));
        gst_structure_free (tmp);
        goto done;
      }

      if (!gst_structure_has_field (tmp, "pixel-aspect-ratio"))
        gst_structure_set_value (tmp, "pixel-aspect-ratio", to_par);
      gst_structure_fixate_field_nearest_fraction (tmp, "pixel-aspect-ratio",
          to_par_n, to_par_d);
      gst_structure_get_fraction (tmp, "pixel-aspect-ratio", &set_par_n,
          &set_par_d);
      gst_structure_free (tmp);

      if (set_par_n == to_par_n && set_par_d == to_par_d) {
        gst_structure_set (outs, "width", G_TYPE_INT, set_w, "height",
            G_TYPE_INT, set_h, NULL);

        if (gst_structure_has_field (outs, "pixel-aspect-ratio") ||
            set_par_n != set_par_d)
          gst_structure_set (outs, "pixel-aspect-ratio", GST_TYPE_FRACTION,
              set_par_n, set_par_d, NULL);
        goto done;
      }

      /* Otherwise try to scale width to keep the DAR with the set
       * PAR and height */
      if (!gst_util_fraction_multiply (from_dar_n, from_dar_d, set_par_d,
              set_par_n, &num, &den)) {
        GST_ELEMENT_ERROR (trans, CORE, NEGOTIATION, (NULL),
            ("Error calculating the output scaled size - integer overflow"));
        goto done;
      }

      w = (guint) gst_util_uint64_scale_int (set_h, num, den);
      tmp = gst_structure_copy (outs);
      gst_structure_fixate_field_nearest_int (tmp, "width", w);
      gst_structure_get_int (tmp, "width", &tmp2);
      gst_structure_free (tmp);

      if (tmp2 == w) {
        gst_structure_set (outs, "width", G_TYPE_INT, tmp2, "height",
            G_TYPE_INT, set_h, NULL);
        if (gst_structure_has_field (outs, "pixel-aspect-ratio") ||
            set_par_n != set_par_d)
          gst_structure_set (outs, "pixel-aspect-ratio", GST_TYPE_FRACTION,
              set_par_n, set_par_d, NULL);
        goto done;
      }

      /* ... or try the same with the height */
      h = (guint) gst_util_uint64_scale_int (set_w, den, num);
      tmp = gst_structure_copy (outs);
      gst_structure_fixate_field_nearest_int (tmp, "height", h);
      gst_structure_get_int (tmp, "height", &tmp2);
      gst_structure_free (tmp);

      if (tmp2 == h) {
        gst_structure_set (outs, "width", G_TYPE_INT, set_w, "height",
            G_TYPE_INT, tmp2, NULL);
        if (gst_structure_has_field (outs, "pixel-aspect-ratio") ||
            set_par_n != set_par_d)
          gst_structure_set (outs, "pixel-aspect-ratio", GST_TYPE_FRACTION,
              set_par_n, set_par_d, NULL);
        goto done;
      }

      /* If all fails we can't keep the DAR and take the nearest values
       * for everything from the first try */
      gst_structure_set (outs, "width", G_TYPE_INT, set_w, "height",
          G_TYPE_INT, set_h, NULL);
      if (gst_structure_has_field (outs, "pixel-aspect-ratio") ||
          set_par_n != set_par_d)
        gst_structure_set (outs, "pixel-aspect-ratio", GST_TYPE_FRACTION,
            set_par_n, set_par_d, NULL);
    }
  }

done:
  GST_DEBUG_OBJECT (trans, "fixated othercaps to %" GST_PTR_FORMAT, othercaps);

  if (from_par == &fpar)
    g_value_unset (&fpar);
  if (to_par == &tpar)
    g_value_unset (&tpar);

  /* fixate remaining fields */
  othercaps = gst_caps_fixate (othercaps);

  if (direction == GST_PAD_SINK) {
    if (gst_caps_is_subset (caps, othercaps)) {
      gst_caps_replace (&othercaps, caps);
    }
  }

  return othercaps;
}

static GstFlowReturn
gst_v4l2_transform_prepare_output_buffer (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer ** outbuf)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (trans);
  GstBufferPool *pool = GST_BUFFER_POOL (self->v4l2output->pool);
  GstFlowReturn ret = GST_FLOW_OK;
  GstBaseTransformClass *bclass = GST_BASE_TRANSFORM_CLASS (parent_class);

  if (gst_base_transform_is_passthrough (trans)) {
    GST_DEBUG_OBJECT (self, "Passthrough, no need to do anything");
    *outbuf = inbuf;
    goto beach;
  }

  /* Ensure input internal pool is active */
  if (!gst_buffer_pool_is_active (pool)) {
    GstStructure *config = gst_buffer_pool_get_config (pool);
    gint min = MAX (GST_V4L2_MIN_BUFFERS, self->v4l2output->min_buffers);

    if (self->v4l2output->mode == GST_V4L2_IO_USERPTR ||
        self->v4l2output->mode == GST_V4L2_IO_DMABUF_IMPORT) {
      if (!gst_v4l2_object_try_import (self->v4l2output, inbuf)) {
        GST_ERROR_OBJECT (self, "cannot import buffers from upstream");
        return GST_FLOW_ERROR;
      }

      if (self->v4l2output->need_video_meta) {
        /* We may need video meta if imported buffer is using non-standard
         * stride/padding */
        gst_buffer_pool_config_add_option (config,
            GST_BUFFER_POOL_OPTION_VIDEO_META);
      }
    }

    gst_buffer_pool_config_set_params (config, self->incaps,
        self->v4l2output->info.size, min, min);

    /* There is no reason to refuse this config */
    if (!gst_buffer_pool_set_config (pool, config))
      goto activate_failed;

    if (!gst_buffer_pool_set_active (pool, TRUE))
      goto activate_failed;
  }

  GST_DEBUG_OBJECT (self, "Queue input buffer");
  ret = gst_v4l2_buffer_pool_process (GST_V4L2_BUFFER_POOL (pool), &inbuf);
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto beach;

  do {
    pool = gst_base_transform_get_buffer_pool (trans);

    if (!gst_buffer_pool_set_active (pool, TRUE))
      goto activate_failed;

    GST_DEBUG_OBJECT (self, "Dequeue output buffer");
    ret = gst_buffer_pool_acquire_buffer (pool, outbuf, NULL);
    g_object_unref (pool);

    if (ret != GST_FLOW_OK)
      goto alloc_failed;

    pool = self->v4l2capture->pool;
    ret = gst_v4l2_buffer_pool_process (GST_V4L2_BUFFER_POOL (pool), outbuf);

  } while (ret == GST_V4L2_FLOW_CORRUPTED_BUFFER);

  if (ret != GST_FLOW_OK) {
    gst_buffer_unref (*outbuf);
    *outbuf = NULL;
  }

  if (bclass->copy_metadata)
    if (!bclass->copy_metadata (trans, inbuf, *outbuf)) {
      /* something failed, post a warning */
      GST_ELEMENT_WARNING (self, STREAM, NOT_IMPLEMENTED,
          ("could not copy metadata"), (NULL));
    }

beach:
  return ret;

activate_failed:
  GST_ELEMENT_ERROR (self, RESOURCE, SETTINGS,
      ("failed to activate bufferpool"), ("failed to activate bufferpool"));
  g_object_unref (pool);
  return GST_FLOW_ERROR;

alloc_failed:
  GST_DEBUG_OBJECT (self, "could not allocate buffer from pool");
  return ret;
}

static GstFlowReturn
gst_v4l2_transform_transform (GstBaseTransform * trans, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  /* Nothing to do */
  return GST_FLOW_OK;
}

static gboolean
gst_v4l2_transform_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (trans);
  gboolean ret;
  GstEventType type = GST_EVENT_TYPE (event);

  /* Nothing to flush in passthrough */
  if (gst_base_transform_is_passthrough (trans))
    return GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);

  switch (type) {
    case GST_EVENT_FLUSH_START:
      GST_DEBUG_OBJECT (self, "flush start");
      gst_v4l2_object_unlock (self->v4l2output);
      gst_v4l2_object_unlock (self->v4l2capture);
      break;
    default:
      break;
  }

  ret = GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);

  switch (type) {
    case GST_EVENT_FLUSH_STOP:
      /* Buffer should be back now */
      GST_DEBUG_OBJECT (self, "flush stop");
      gst_v4l2_object_unlock_stop (self->v4l2capture);
      gst_v4l2_object_unlock_stop (self->v4l2output);
      if (self->v4l2output->pool)
        gst_v4l2_buffer_pool_flush (self->v4l2output->pool);
      if (self->v4l2capture->pool)
        gst_v4l2_buffer_pool_flush (self->v4l2capture->pool);
      break;
    default:
      break;
  }

  return ret;
}

static GstStateChangeReturn
gst_v4l2_transform_change_state (GstElement * element,
    GstStateChange transition)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_v4l2_transform_open (self))
        return GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_v4l2_object_unlock (self->v4l2output);
      gst_v4l2_object_unlock (self->v4l2capture);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_v4l2_transform_close (self);
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_v4l2_transform_dispose (GObject * object)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (object);

  gst_caps_replace (&self->probed_sinkcaps, NULL);
  gst_caps_replace (&self->probed_srccaps, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_v4l2_transform_finalize (GObject * object)
{
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (object);

  gst_v4l2_object_destroy (self->v4l2capture);
  gst_v4l2_object_destroy (self->v4l2output);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_v4l2_transform_init (GstV4l2Transform * self)
{
  /* V4L2 object are created in subinstance_init */
  /* enable QoS */
  gst_base_transform_set_qos_enabled (GST_BASE_TRANSFORM (self), TRUE);
}

static void
gst_v4l2_transform_subinstance_init (GTypeInstance * instance, gpointer g_class)
{
  GstV4l2TransformClass *klass = GST_V4L2_TRANSFORM_CLASS (g_class);
  GstV4l2Transform *self = GST_V4L2_TRANSFORM (instance);

  self->v4l2output = gst_v4l2_object_new (GST_ELEMENT (self),
      GST_OBJECT (GST_BASE_TRANSFORM_SINK_PAD (self)),
      V4L2_BUF_TYPE_VIDEO_OUTPUT, klass->default_device,
      gst_v4l2_get_output, gst_v4l2_set_output, NULL);
  self->v4l2output->no_initial_format = TRUE;
  self->v4l2output->keep_aspect = FALSE;

  self->v4l2capture = gst_v4l2_object_new (GST_ELEMENT (self),
      GST_OBJECT (GST_BASE_TRANSFORM_SRC_PAD (self)),
      V4L2_BUF_TYPE_VIDEO_CAPTURE, klass->default_device,
      gst_v4l2_get_input, gst_v4l2_set_input, NULL);
}

static void
gst_v4l2_transform_class_init (GstV4l2TransformClass * klass)
{
  GstElementClass *element_class;
  GObjectClass *gobject_class;
  GstBaseTransformClass *base_transform_class;

  element_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;
  base_transform_class = (GstBaseTransformClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_v4l2_transform_debug, "v4l2transform", 0,
      "V4L2 Converter");

  gst_element_class_set_static_metadata (element_class,
      "V4L2 Video Converter",
      "Filter/Converter/Video/Scaler",
      "Transform streams via V4L2 API",
      "Nicolas Dufresne <nicolas.dufresne@collabora.com>");

  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_v4l2_transform_dispose);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_v4l2_transform_finalize);
  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_get_property);

  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_v4l2_transform_stop);
  base_transform_class->set_caps =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_set_caps);
  base_transform_class->query = GST_DEBUG_FUNCPTR (gst_v4l2_transform_query);
  base_transform_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_sink_event);
  base_transform_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_decide_allocation);
  base_transform_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_propose_allocation);
  base_transform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_transform_caps);
  base_transform_class->fixate_caps =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_fixate_caps);
  base_transform_class->prepare_output_buffer =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_prepare_output_buffer);
  base_transform_class->transform =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_transform);

  base_transform_class->passthrough_on_same_caps = TRUE;

  element_class->change_state =
      GST_DEBUG_FUNCPTR (gst_v4l2_transform_change_state);

  gst_v4l2_object_install_m2m_properties_helper (gobject_class);

  g_object_class_install_property (gobject_class, PROP_DISABLE_PASSTHROUGH,
      g_param_spec_boolean ("disable-passthrough", "Disable Passthrough",
          "Forces passing buffers through the converter", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_v4l2_transform_subclass_init (gpointer g_class, gpointer data)
{
  GstV4l2TransformClass *klass = GST_V4L2_TRANSFORM_CLASS (g_class);
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  GstV4l2TransformCData *cdata = data;

  klass->default_device = cdata->device;

  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          cdata->sink_caps));
  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          cdata->src_caps));

  gst_caps_unref (cdata->sink_caps);
  gst_caps_unref (cdata->src_caps);
  g_free (cdata);
}

/* Probing functions */
gboolean
gst_v4l2_is_transform (GstCaps * sink_caps, GstCaps * src_caps)
{
  gboolean ret = FALSE;

  if (gst_caps_is_subset (sink_caps, gst_v4l2_object_get_raw_caps ())
      && gst_caps_is_subset (src_caps, gst_v4l2_object_get_raw_caps ()))
    ret = TRUE;

  return ret;
}

void
gst_v4l2_transform_register (GstPlugin * plugin, const gchar * basename,
    const gchar * device_path, GstCaps * sink_caps, GstCaps * src_caps)
{
  GTypeQuery type_query;
  GTypeInfo type_info = { 0, };
  GType type, subtype;
  gchar *type_name;
  GstV4l2TransformCData *cdata;

  cdata = g_new0 (GstV4l2TransformCData, 1);
  cdata->device = g_strdup (device_path);
  cdata->sink_caps = gst_caps_ref (sink_caps);
  cdata->src_caps = gst_caps_ref (src_caps);

  type = gst_v4l2_transform_get_type ();
  g_type_query (type, &type_query);
  memset (&type_info, 0, sizeof (type_info));
  type_info.class_size = type_query.class_size;
  type_info.instance_size = type_query.instance_size;
  type_info.class_init = gst_v4l2_transform_subclass_init;
  type_info.class_data = cdata;
  type_info.instance_init = gst_v4l2_transform_subinstance_init;

  if (g_type_from_name ("v4l2convert") != 0)
    type_name = g_strdup_printf ("v4l2%sconvert", basename);
  else
    type_name = g_strdup ("v4l2convert");
  subtype = g_type_register_static (type, type_name, &type_info, 0);

  if (!gst_element_register (plugin, type_name, GST_RANK_NONE, subtype))
    GST_WARNING ("Failed to register plugin '%s'", type_name);

  g_free (type_name);
}
