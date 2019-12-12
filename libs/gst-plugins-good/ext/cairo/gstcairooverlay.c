/* GStreamer
 * Copyright (C) <2011> Jon Nordby <jononor@gmail.com>
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
 * SECTION:element-cairooverlay
 * @title: cairooverlay
 *
 * cairooverlay renders an overlay using a application provided render function.
 *
 * The full example can be found in tests/examples/cairo/cairo_overlay.c
 *
 * ## Example code
 * |[
 *
 * #include &lt;gst/gst.h&gt;
 * #include &lt;gst/video/video.h&gt;
 *
 * ...
 *
 * typedef struct {
 *   gboolean valid;
 *   int width;
 *   int height;
 * } CairoOverlayState;
 *
 * ...
 *
 * static void
 * prepare_overlay (GstElement * overlay, GstCaps * caps, gpointer user_data)
 * {
 *   CairoOverlayState *state = (CairoOverlayState *)user_data;
 *
 *   gst_video_format_parse_caps (caps, NULL, &amp;state-&gt;width, &amp;state-&gt;height);
 *   state-&gt;valid = TRUE;
 * }
 *
 * static void
 * draw_overlay (GstElement * overlay, cairo_t * cr, guint64 timestamp,
 *   guint64 duration, gpointer user_data)
 * {
 *   CairoOverlayState *s = (CairoOverlayState *)user_data;
 *   double scale;
 *
 *   if (!s-&gt;valid)
 *     return;
 *
 *   scale = 2*(((timestamp/(int)1e7) % 70)+30)/100.0;
 *   cairo_translate(cr, s-&gt;width/2, (s-&gt;height/2)-30);
 *   cairo_scale (cr, scale, scale);
 *
 *   cairo_move_to (cr, 0, 0);
 *   cairo_curve_to (cr, 0,-30, -50,-30, -50,0);
 *   cairo_curve_to (cr, -50,30, 0,35, 0,60 );
 *   cairo_curve_to (cr, 0,35, 50,30, 50,0 ); *
 *   cairo_curve_to (cr, 50,-30, 0,-30, 0,0 );
 *   cairo_set_source_rgba (cr, 0.9, 0.0, 0.1, 0.7);
 *   cairo_fill (cr);
 * }
 *
 * ...
 *
 * cairo_overlay = gst_element_factory_make (&quot;cairooverlay&quot;, &quot;overlay&quot;);
 *
 * g_signal_connect (cairo_overlay, &quot;draw&quot;, G_CALLBACK (draw_overlay),
 *   overlay_state);
 * g_signal_connect (cairo_overlay, &quot;caps-changed&quot;,
 *   G_CALLBACK (prepare_overlay), overlay_state);
 * ...
 *
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstcairooverlay.h"

#include <gst/video/video.h>

#include <cairo.h>

/* RGB16 is native-endianness in GStreamer */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define TEMPLATE_CAPS GST_VIDEO_CAPS_MAKE("{ BGRx, BGRA, RGB16 }")
#else
#define TEMPLATE_CAPS GST_VIDEO_CAPS_MAKE("{ xRGB, ARGB, RGB16 }")
#endif

static GstStaticPadTemplate gst_cairo_overlay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (TEMPLATE_CAPS)
    );

static GstStaticPadTemplate gst_cairo_overlay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (TEMPLATE_CAPS)
    );

G_DEFINE_TYPE (GstCairoOverlay, gst_cairo_overlay, GST_TYPE_BASE_TRANSFORM);

enum
{
  PROP_0,
  PROP_DRAW_ON_TRANSPARENT_SURFACE,
};

#define DEFAULT_DRAW_ON_TRANSPARENT_SURFACE (FALSE)

enum
{
  SIGNAL_DRAW,
  SIGNAL_CAPS_CHANGED,
  N_SIGNALS
};

static guint gst_cairo_overlay_signals[N_SIGNALS];

static void
gst_cairo_overlay_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCairoOverlay *overlay = GST_CAIRO_OVERLAY (object);

  GST_OBJECT_LOCK (overlay);

  switch (property_id) {
    case PROP_DRAW_ON_TRANSPARENT_SURFACE:
      overlay->draw_on_transparent_surface = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (overlay);
}

static void
gst_cairo_overlay_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstCairoOverlay *overlay = GST_CAIRO_OVERLAY (object);

  GST_OBJECT_LOCK (overlay);

  switch (property_id) {
    case PROP_DRAW_ON_TRANSPARENT_SURFACE:
      g_value_set_boolean (value, overlay->draw_on_transparent_surface);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (overlay);
}

static gboolean
gst_cairo_overlay_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  GstCairoOverlay *overlay = GST_CAIRO_OVERLAY (trans);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ALLOCATION:
    {
      /* We're always running in passthrough mode, which means that
       * basetransform just passes through ALLOCATION queries and
       * never ever calls BaseTransform::decide_allocation().
       *
       * We hook into the query handling for that reason
       */
      overlay->attach_compo_to_buffer = FALSE;

      if (!GST_BASE_TRANSFORM_CLASS (gst_cairo_overlay_parent_class)->query
          (trans, direction, query)) {
        return FALSE;
      }

      overlay->attach_compo_to_buffer = gst_query_find_allocation_meta (query,
          GST_VIDEO_OVERLAY_COMPOSITION_META_API_TYPE, NULL);

      return TRUE;
    }
    default:
      return
          GST_BASE_TRANSFORM_CLASS (gst_cairo_overlay_parent_class)->query
          (trans, direction, query);
  }
}

static gboolean
gst_cairo_overlay_set_caps (GstBaseTransform * trans, GstCaps * in_caps,
    GstCaps * out_caps)
{
  GstCairoOverlay *overlay = GST_CAIRO_OVERLAY (trans);

  if (!gst_video_info_from_caps (&overlay->info, in_caps))
    return FALSE;

  g_signal_emit (overlay, gst_cairo_overlay_signals[SIGNAL_CAPS_CHANGED], 0,
      in_caps, NULL);

  return TRUE;
}

/* Copy from video-overlay-composition.c */
static void
gst_video_overlay_rectangle_premultiply_0 (GstVideoFrame * frame)
{
  int i, j;
  int width = GST_VIDEO_FRAME_WIDTH (frame);
  int height = GST_VIDEO_FRAME_HEIGHT (frame);
  int stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 0);
  guint8 *data = GST_VIDEO_FRAME_PLANE_DATA (frame, 0);

  for (j = 0; j < height; ++j) {
    guint8 *line;

    line = data;
    line += stride * j;
    for (i = 0; i < width; ++i) {
      int a = line[0];
      line[1] = line[1] * a / 255;
      line[2] = line[2] * a / 255;
      line[3] = line[3] * a / 255;
      line += 4;
    }
  }
}

/* Copy from video-overlay-composition.c */
static void
gst_video_overlay_rectangle_premultiply_3 (GstVideoFrame * frame)
{
  int i, j;
  int width = GST_VIDEO_FRAME_WIDTH (frame);
  int height = GST_VIDEO_FRAME_HEIGHT (frame);
  int stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 0);
  guint8 *data = GST_VIDEO_FRAME_PLANE_DATA (frame, 0);

  for (j = 0; j < height; ++j) {
    guint8 *line;

    line = data;
    line += stride * j;
    for (i = 0; i < width; ++i) {
      int a = line[3];
      line[0] = line[0] * a / 255;
      line[1] = line[1] * a / 255;
      line[2] = line[2] * a / 255;
      line += 4;
    }
  }
}

/* Copy from video-overlay-composition.c */
static void
gst_video_overlay_rectangle_premultiply (GstVideoFrame * frame)
{
  gint alpha_offset;

  alpha_offset = GST_VIDEO_FRAME_COMP_POFFSET (frame, 3);
  switch (alpha_offset) {
    case 0:
      gst_video_overlay_rectangle_premultiply_0 (frame);
      break;
    case 3:
      gst_video_overlay_rectangle_premultiply_3 (frame);
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

/* Copy from video-overlay-composition.c */
static void
gst_video_overlay_rectangle_unpremultiply_0 (GstVideoFrame * frame)
{
  int i, j;
  int width = GST_VIDEO_FRAME_WIDTH (frame);
  int height = GST_VIDEO_FRAME_HEIGHT (frame);
  int stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 0);
  guint8 *data = GST_VIDEO_FRAME_PLANE_DATA (frame, 0);

  for (j = 0; j < height; ++j) {
    guint8 *line;

    line = data;
    line += stride * j;
    for (i = 0; i < width; ++i) {
      int a = line[0];
      if (a) {
        line[1] = MIN ((line[1] * 255 + a / 2) / a, 255);
        line[2] = MIN ((line[2] * 255 + a / 2) / a, 255);
        line[3] = MIN ((line[3] * 255 + a / 2) / a, 255);
      }
      line += 4;
    }
  }
}

/* Copy from video-overlay-composition.c */
static void
gst_video_overlay_rectangle_unpremultiply_3 (GstVideoFrame * frame)
{
  int i, j;
  int width = GST_VIDEO_FRAME_WIDTH (frame);
  int height = GST_VIDEO_FRAME_HEIGHT (frame);
  int stride = GST_VIDEO_FRAME_PLANE_STRIDE (frame, 0);
  guint8 *data = GST_VIDEO_FRAME_PLANE_DATA (frame, 0);

  for (j = 0; j < height; ++j) {
    guint8 *line;

    line = data;
    line += stride * j;
    for (i = 0; i < width; ++i) {
      int a = line[3];
      if (a) {
        line[0] = MIN ((line[0] * 255 + a / 2) / a, 255);
        line[1] = MIN ((line[1] * 255 + a / 2) / a, 255);
        line[2] = MIN ((line[2] * 255 + a / 2) / a, 255);
      }
      line += 4;
    }
  }
}

/* Copy from video-overlay-composition.c */
static void
gst_video_overlay_rectangle_unpremultiply (GstVideoFrame * frame)
{
  gint alpha_offset;

  alpha_offset = GST_VIDEO_FRAME_COMP_POFFSET (frame, 3);
  switch (alpha_offset) {
    case 0:
      gst_video_overlay_rectangle_unpremultiply_0 (frame);
      break;
    case 3:
      gst_video_overlay_rectangle_unpremultiply_3 (frame);
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

static GstFlowReturn
gst_cairo_overlay_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstCairoOverlay *overlay = GST_CAIRO_OVERLAY (trans);
  GstVideoFrame frame;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_format_t format;
  gboolean draw_on_transparent_surface = overlay->draw_on_transparent_surface;

  switch (GST_VIDEO_INFO_FORMAT (&overlay->info)) {
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_BGRA:
      format = CAIRO_FORMAT_ARGB32;
      break;
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_BGRx:
      format = CAIRO_FORMAT_RGB24;
      break;
    case GST_VIDEO_FORMAT_RGB16:
      format = CAIRO_FORMAT_RGB16_565;
      break;
    default:
    {
      GST_WARNING ("No matching cairo format for %s",
          gst_video_format_to_string (GST_VIDEO_INFO_FORMAT (&overlay->info)));
      return GST_FLOW_ERROR;
    }
  }

  /* If we need to map the buffer writable, do so */
  if (!draw_on_transparent_surface || !overlay->attach_compo_to_buffer) {
    if (!gst_video_frame_map (&frame, &overlay->info, buf, GST_MAP_READWRITE)) {
      return GST_FLOW_ERROR;
    }
  } else {
    frame.buffer = NULL;
  }

  if (draw_on_transparent_surface) {
    surface =
        cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
        GST_VIDEO_INFO_WIDTH (&overlay->info),
        GST_VIDEO_INFO_HEIGHT (&overlay->info));
  } else {
    if (format == CAIRO_FORMAT_ARGB32)
      gst_video_overlay_rectangle_premultiply (&frame);

    surface =
        cairo_image_surface_create_for_data (GST_VIDEO_FRAME_PLANE_DATA (&frame,
            0), format, GST_VIDEO_FRAME_WIDTH (&frame),
        GST_VIDEO_FRAME_HEIGHT (&frame), GST_VIDEO_FRAME_PLANE_STRIDE (&frame,
            0));
  }

  if (G_UNLIKELY (!surface))
    return GST_FLOW_ERROR;

  cr = cairo_create (surface);
  if (G_UNLIKELY (!cr)) {
    cairo_surface_destroy (surface);
    return GST_FLOW_ERROR;
  }

  g_signal_emit (overlay, gst_cairo_overlay_signals[SIGNAL_DRAW], 0,
      cr, GST_BUFFER_PTS (buf), GST_BUFFER_DURATION (buf), NULL);

  cairo_destroy (cr);

  if (draw_on_transparent_surface) {
    guint size;
    GstBuffer *surface_buffer;
    GstVideoOverlayRectangle *rect;
    GstVideoOverlayComposition *composition;
    gsize offset[GST_VIDEO_MAX_PLANES] = { 0, };
    gint stride[GST_VIDEO_MAX_PLANES] = { 0, };

    size =
        cairo_image_surface_get_height (surface) *
        cairo_image_surface_get_stride (surface);
    stride[0] = cairo_image_surface_get_stride (surface);

    /* Create a GstVideoOverlayComposition for blending, this handles
     * pre-multiplied alpha correctly */
    surface_buffer =
        gst_buffer_new_wrapped_full (0, cairo_image_surface_get_data (surface),
        size, 0, size, surface, (GDestroyNotify) cairo_surface_destroy);
    gst_buffer_add_video_meta_full (surface_buffer, GST_VIDEO_FRAME_FLAG_NONE,
        (G_BYTE_ORDER ==
            G_LITTLE_ENDIAN ? GST_VIDEO_FORMAT_BGRA : GST_VIDEO_FORMAT_ARGB),
        GST_VIDEO_INFO_WIDTH (&overlay->info),
        GST_VIDEO_INFO_HEIGHT (&overlay->info), 1, offset, stride);
    rect =
        gst_video_overlay_rectangle_new_raw (surface_buffer, 0, 0,
        GST_VIDEO_INFO_WIDTH (&overlay->info),
        GST_VIDEO_INFO_HEIGHT (&overlay->info),
        GST_VIDEO_OVERLAY_FORMAT_FLAG_PREMULTIPLIED_ALPHA);
    gst_buffer_unref (surface_buffer);

    if (overlay->attach_compo_to_buffer) {
      GstVideoOverlayCompositionMeta *composition_meta;

      composition_meta = gst_buffer_get_video_overlay_composition_meta (buf);
      if (composition_meta) {
        GstVideoOverlayComposition *merged_composition =
            gst_video_overlay_composition_copy (composition_meta->overlay);
        gst_video_overlay_composition_add_rectangle (merged_composition, rect);
        gst_video_overlay_composition_unref (composition_meta->overlay);
        composition_meta->overlay = merged_composition;
        gst_video_overlay_rectangle_unref (rect);
      } else {
        composition = gst_video_overlay_composition_new (rect);
        gst_video_overlay_rectangle_unref (rect);
        gst_buffer_add_video_overlay_composition_meta (buf, composition);
        gst_video_overlay_composition_unref (composition);
      }
    } else {
      composition = gst_video_overlay_composition_new (rect);
      gst_video_overlay_rectangle_unref (rect);
      gst_video_overlay_composition_blend (composition, &frame);
      gst_video_overlay_composition_unref (composition);
    }
  } else {
    cairo_surface_destroy (surface);
    if (format == CAIRO_FORMAT_ARGB32)
      gst_video_overlay_rectangle_unpremultiply (&frame);
  }

  if (frame.buffer) {
    gst_video_frame_unmap (&frame);
  }

  return GST_FLOW_OK;
}

static void
gst_cairo_overlay_class_init (GstCairoOverlayClass * klass)
{
  GstBaseTransformClass *btrans_class;
  GstElementClass *element_class;
  GObjectClass *gobject_class;

  btrans_class = (GstBaseTransformClass *) klass;
  element_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;

  btrans_class->set_caps = gst_cairo_overlay_set_caps;
  btrans_class->transform_ip = gst_cairo_overlay_transform_ip;
  btrans_class->query = gst_cairo_overlay_query;

  gobject_class->set_property = gst_cairo_overlay_set_property;
  gobject_class->get_property = gst_cairo_overlay_get_property;

  g_object_class_install_property (gobject_class,
      PROP_DRAW_ON_TRANSPARENT_SURFACE,
      g_param_spec_boolean ("draw-on-transparent-surface",
          "Draw on transparent surface",
          "Let the draw signal work on a transparent surface "
          "and blend the results with the video at a later time",
          DEFAULT_DRAW_ON_TRANSPARENT_SURFACE,
          GST_PARAM_CONTROLLABLE | GST_PARAM_MUTABLE_PLAYING | G_PARAM_READWRITE
          | G_PARAM_STATIC_STRINGS));

  /**
   * GstCairoOverlay::draw:
   * @overlay: Overlay element emitting the signal.
   * @cr: Cairo context to draw to.
   * @timestamp: Timestamp (see #GstClockTime) of the current buffer.
   * @duration: Duration (see #GstClockTime) of the current buffer.
   *
   * This signal is emitted when the overlay should be drawn.
   */
  gst_cairo_overlay_signals[SIGNAL_DRAW] =
      g_signal_new ("draw",
      G_TYPE_FROM_CLASS (klass),
      0, 0, NULL, NULL, NULL,
      G_TYPE_NONE, 3, CAIRO_GOBJECT_TYPE_CONTEXT, G_TYPE_UINT64, G_TYPE_UINT64);

  /**
   * GstCairoOverlay::caps-changed:
   * @overlay: Overlay element emitting the signal.
   * @caps: The #GstCaps of the element.
   *
   * This signal is emitted when the caps of the element has changed.
   */
  gst_cairo_overlay_signals[SIGNAL_CAPS_CHANGED] =
      g_signal_new ("caps-changed",
      G_TYPE_FROM_CLASS (klass),
      0, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_CAPS);

  gst_element_class_set_static_metadata (element_class, "Cairo overlay",
      "Filter/Editor/Video",
      "Render overlay on a video stream using Cairo",
      "Jon Nordby <jononor@gmail.com>");

  gst_element_class_add_static_pad_template (element_class,
      &gst_cairo_overlay_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_cairo_overlay_src_template);
}

static void
gst_cairo_overlay_init (GstCairoOverlay * overlay)
{
  /* nothing to do */
}
