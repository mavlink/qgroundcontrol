/* GStreamer GdkPixbuf sink
 * Copyright (C) 2006-2008 Tim-Philipp Müller <tim centricular net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is free software; you can redistribute it and/or
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
 * SECTION:element-gdkpixbufsink
 * @title: gdkpixbufsink
 *
 * This sink element takes RGB or RGBA images as input and wraps them into
 * #GdkPixbuf objects, for easy saving to file via the
 * GdkPixbuf library API or displaying in Gtk+ applications (e.g. using
 * the #GtkImage widget).
 *
 * There are two ways to use this element and obtain the #GdkPixbuf objects
 * created:
 *
 * * Watching for element messages named `preroll-pixbuf` or `pixbuf` on the bus, which
 * will be posted whenever an image would usually be rendered. See below for
 * more details on these messages and how to extract the pixbuf object
 * contained in them.
 *
 * * Retrieving the current pixbuf via the #GstGdkPixbufSink:last-pixbuf property
 * when needed. This is the easiest way to get at pixbufs for snapshotting
 * purposes - just wait until the pipeline is prerolled (ASYNC_DONE message
 * on the bus), then read the property. If you use this method, you may want
 * to disable message posting by setting the #GstGdkPixbufSink:post-messages
 * property to %FALSE. This avoids unnecessary memory overhead.
 *
 * The primary purpose of this element is to abstract away the #GstBuffer to
 * #GdkPixbuf conversion. Other than that it's very similar to the fakesink
 * element.
 *
 * This element is meant for easy no-hassle video snapshotting. It is not
 * suitable for video playback or video display at high framerates. Use
 * ximagesink, xvimagesink or some other suitable video sink in connection
 * with the #GstVideoOverlay interface instead if you want to do video playback.
 *
 * ## Message details
 *
 * As mentioned above, this element will by default post element messages
 * containing structures named `preroll-pixbuf`
 * ` or `pixbuf` on the bus (this
 * can be disabled by setting the #GstGdkPixbufSink:post-messages property
 * to %FALSE though). The element message structure has the following fields:
 *
 * * `pixbuf`: the #GdkPixbuf object
 * * `pixel-aspect-ratio`: the pixel aspect ratio (PAR) of the input image
 *   (this field contains a value of type #GST_TYPE_FRACTION); the
 *   PAR is usually 1:1 for images, but is often something non-1:1 in the case
 *   of video input. In this case the image may be distorted and you may need
 *   to rescale it accordingly before saving it to file or displaying it. This
 *   can easily be done using gdk_pixbuf_scale() (the reason this is not done
 *   automatically is that the application will often scale the image anyway
 *   according to the size of the output window, in which case it is much more
 *   efficient to only scale once rather than twice). You can put a videoscale
 *   element and a capsfilter element with
 *   `video/x-raw-rgb,pixel-aspect-ratio=(fraction)1/1` caps
 *   in front of this element to make sure the pixbufs always have a 1:1 PAR.
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 -m -v videotestsrc num-buffers=1 ! gdkpixbufsink
 * ]| Process one single test image as pixbuf (note that the output you see will
 * be slightly misleading. The message structure does contain a valid pixbuf
 * object even if the structure string says &apos;(NULL)&apos;).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstgdkpixbufsink.h"

#include <gst/video/video.h>

#define DEFAULT_SEND_MESSAGES TRUE
#define DEFAULT_POST_MESSAGES TRUE

enum
{
  PROP_0,
  PROP_POST_MESSAGES,
  PROP_LAST_PIXBUF,
  PROP_LAST
};


G_DEFINE_TYPE (GstGdkPixbufSink, gst_gdk_pixbuf_sink, GST_TYPE_VIDEO_SINK);

static void gst_gdk_pixbuf_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_gdk_pixbuf_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_gdk_pixbuf_sink_start (GstBaseSink * basesink);
static gboolean gst_gdk_pixbuf_sink_stop (GstBaseSink * basesink);
static gboolean gst_gdk_pixbuf_sink_set_caps (GstBaseSink * basesink,
    GstCaps * caps);
static GstFlowReturn gst_gdk_pixbuf_sink_render (GstBaseSink * bsink,
    GstBuffer * buf);
static GstFlowReturn gst_gdk_pixbuf_sink_preroll (GstBaseSink * bsink,
    GstBuffer * buf);
static GdkPixbuf *gst_gdk_pixbuf_sink_get_pixbuf_from_buffer (GstGdkPixbufSink *
    sink, GstBuffer * buf);

static GstStaticPadTemplate pixbufsink_sink_factory =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("RGB") ";"
        GST_VIDEO_CAPS_MAKE ("RGBA"))
    );

static void
gst_gdk_pixbuf_sink_class_init (GstGdkPixbufSinkClass * klass)
{
  GstBaseSinkClass *basesink_class = GST_BASE_SINK_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gst_element_class_set_static_metadata (element_class, "GdkPixbuf sink",
      "Sink/Video", "Output images as GdkPixbuf objects in bus messages",
      "Tim-Philipp Müller <tim centricular net>");

  gst_element_class_add_static_pad_template (element_class,
      &pixbufsink_sink_factory);

  gobject_class->set_property = gst_gdk_pixbuf_sink_set_property;
  gobject_class->get_property = gst_gdk_pixbuf_sink_get_property;

  /**
   * GstGdkPixbuf:post-messages:
   *
   * Post messages on the bus containing pixbufs.
   */
  g_object_class_install_property (gobject_class, PROP_POST_MESSAGES,
      g_param_spec_boolean ("post-messages", "Post Messages",
          "Whether to post messages containing pixbufs on the bus",
          DEFAULT_POST_MESSAGES, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LAST_PIXBUF,
      g_param_spec_object ("last-pixbuf", "Last Pixbuf",
          "Last GdkPixbuf object rendered", GDK_TYPE_PIXBUF,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  basesink_class->start = GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_sink_start);
  basesink_class->stop = GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_sink_stop);
  basesink_class->render = GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_sink_render);
  basesink_class->preroll = GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_sink_preroll);
  basesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_sink_set_caps);
}

static void
gst_gdk_pixbuf_sink_init (GstGdkPixbufSink * sink)
{
  sink->par_n = 0;
  sink->par_d = 0;
  sink->has_alpha = FALSE;
  sink->last_pixbuf = NULL;
  sink->post_messages = DEFAULT_POST_MESSAGES;

  /* we're not a real video sink, we just derive from GstVideoSink in case
   * anything interesting is added to it in future */
  gst_base_sink_set_max_lateness (GST_BASE_SINK (sink), -1);
  gst_base_sink_set_qos_enabled (GST_BASE_SINK (sink), FALSE);
}


static gboolean
gst_gdk_pixbuf_sink_start (GstBaseSink * basesink)
{
  GST_LOG_OBJECT (basesink, "start");

  return TRUE;
}

static gboolean
gst_gdk_pixbuf_sink_stop (GstBaseSink * basesink)
{
  GstGdkPixbufSink *sink = GST_GDK_PIXBUF_SINK (basesink);

  GST_VIDEO_SINK_WIDTH (sink) = 0;
  GST_VIDEO_SINK_HEIGHT (sink) = 0;

  sink->par_n = 0;
  sink->par_d = 0;
  sink->has_alpha = FALSE;

  if (sink->last_pixbuf) {
    g_object_unref (sink->last_pixbuf);
    sink->last_pixbuf = NULL;
  }

  GST_LOG_OBJECT (sink, "stop");

  return TRUE;
}

static gboolean
gst_gdk_pixbuf_sink_set_caps (GstBaseSink * basesink, GstCaps * caps)
{
  GstGdkPixbufSink *sink = GST_GDK_PIXBUF_SINK (basesink);
  GstVideoInfo info;
  GstVideoFormat fmt;
  gint w, h, par_n, par_d;

  GST_LOG_OBJECT (sink, "caps: %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&info, caps)) {
    GST_WARNING_OBJECT (sink, "parse_caps failed");
    return FALSE;
  }

  fmt = GST_VIDEO_INFO_FORMAT (&info);
  w = GST_VIDEO_INFO_WIDTH (&info);
  h = GST_VIDEO_INFO_HEIGHT (&info);
  par_n = GST_VIDEO_INFO_PAR_N (&info);
  par_d = GST_VIDEO_INFO_PAR_N (&info);

#ifndef G_DISABLE_ASSERT
  {
    gint s;
    s = GST_VIDEO_INFO_COMP_PSTRIDE (&info, 0);
    g_assert ((fmt == GST_VIDEO_FORMAT_RGB && s == 3) ||
        (fmt == GST_VIDEO_FORMAT_RGBA && s == 4));
  }
#endif

  GST_VIDEO_SINK_WIDTH (sink) = w;
  GST_VIDEO_SINK_HEIGHT (sink) = h;

  sink->par_n = par_n;
  sink->par_d = par_d;

  sink->has_alpha = GST_VIDEO_INFO_HAS_ALPHA (&info);

  GST_INFO_OBJECT (sink, "format             : %d", fmt);
  GST_INFO_OBJECT (sink, "width x height     : %d x %d", w, h);
  GST_INFO_OBJECT (sink, "pixel-aspect-ratio : %d/%d", par_n, par_d);

  sink->info = info;

  return TRUE;
}

static void
gst_gdk_pixbuf_sink_pixbuf_destroy_notify (guchar * pixels,
    GstVideoFrame * frame)
{
  gst_video_frame_unmap (frame);
  gst_buffer_unref (frame->buffer);
  g_slice_free (GstVideoFrame, frame);
}

static GdkPixbuf *
gst_gdk_pixbuf_sink_get_pixbuf_from_buffer (GstGdkPixbufSink * sink,
    GstBuffer * buf)
{
  GdkPixbuf *pix = NULL;
  GstVideoFrame *frame;
  gint minsize, bytes_per_pixel;

  g_return_val_if_fail (GST_VIDEO_SINK_WIDTH (sink) > 0, NULL);
  g_return_val_if_fail (GST_VIDEO_SINK_HEIGHT (sink) > 0, NULL);

  frame = g_slice_new0 (GstVideoFrame);
  gst_video_frame_map (frame, &sink->info, buf, GST_MAP_READ);

  bytes_per_pixel = (sink->has_alpha) ? 4 : 3;

  /* last row needn't have row padding */
  minsize = (GST_VIDEO_FRAME_COMP_STRIDE (frame, 0) *
      (GST_VIDEO_SINK_HEIGHT (sink) - 1)) +
      (bytes_per_pixel * GST_VIDEO_SINK_WIDTH (sink));

  g_return_val_if_fail (gst_buffer_get_size (buf) >= minsize, NULL);

  gst_buffer_ref (buf);
  pix = gdk_pixbuf_new_from_data (GST_VIDEO_FRAME_COMP_DATA (frame, 0),
      GDK_COLORSPACE_RGB, sink->has_alpha, 8, GST_VIDEO_SINK_WIDTH (sink),
      GST_VIDEO_SINK_HEIGHT (sink), GST_VIDEO_FRAME_COMP_STRIDE (frame, 0),
      (GdkPixbufDestroyNotify) gst_gdk_pixbuf_sink_pixbuf_destroy_notify,
      frame);

  return pix;
}

static GstFlowReturn
gst_gdk_pixbuf_sink_handle_buffer (GstBaseSink * basesink, GstBuffer * buf,
    const gchar * msg_name)
{
  GstGdkPixbufSink *sink;
  GdkPixbuf *pixbuf;
  gboolean do_post;

  sink = GST_GDK_PIXBUF_SINK (basesink);

  pixbuf = gst_gdk_pixbuf_sink_get_pixbuf_from_buffer (sink, buf);

  GST_OBJECT_LOCK (sink);

  do_post = sink->post_messages;

  if (sink->last_pixbuf)
    g_object_unref (sink->last_pixbuf);

  sink->last_pixbuf = pixbuf;   /* take ownership */

  GST_OBJECT_UNLOCK (sink);

  if (G_UNLIKELY (pixbuf == NULL))
    goto error;

  if (do_post) {
    GstStructure *s;
    GstMessage *msg;
    GstFormat format;
    GstClockTime timestamp;
    GstClockTime running_time, stream_time;

    GstSegment *segment = &basesink->segment;
    format = segment->format;

    timestamp = GST_BUFFER_PTS (buf);
    running_time = gst_segment_to_running_time (segment, format, timestamp);
    stream_time = gst_segment_to_stream_time (segment, format, timestamp);

    /* it's okay to keep using pixbuf here, we can be sure no one is going to
     * unref or change sink->last_pixbuf before we return from this function.
     * The structure will take its own ref to the pixbuf. */
    s = gst_structure_new (msg_name,
        "pixbuf", GDK_TYPE_PIXBUF, pixbuf,
        "pixel-aspect-ratio", GST_TYPE_FRACTION, sink->par_n, sink->par_d,
        "timestamp", G_TYPE_UINT64, timestamp,
        "stream-time", G_TYPE_UINT64, stream_time,
        "running-time", G_TYPE_UINT64, running_time, NULL);

    msg = gst_message_new_element (GST_OBJECT_CAST (sink), s);
    gst_element_post_message (GST_ELEMENT_CAST (sink), msg);
  }

  g_object_notify (G_OBJECT (sink), "last-pixbuf");

  return GST_FLOW_OK;

/* ERRORS */
error:
  {
    /* This shouldn't really happen */
    GST_ELEMENT_ERROR (sink, LIBRARY, FAILED,
        ("Couldn't create pixbuf from RGB image."),
        ("Probably not enough free memory"));
    return GST_FLOW_ERROR;
  }
}

static GstFlowReturn
gst_gdk_pixbuf_sink_preroll (GstBaseSink * basesink, GstBuffer * buf)
{
  return gst_gdk_pixbuf_sink_handle_buffer (basesink, buf, "preroll-pixbuf");
}

static GstFlowReturn
gst_gdk_pixbuf_sink_render (GstBaseSink * basesink, GstBuffer * buf)
{
  return gst_gdk_pixbuf_sink_handle_buffer (basesink, buf, "pixbuf");
}

static void
gst_gdk_pixbuf_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGdkPixbufSink *sink;

  sink = GST_GDK_PIXBUF_SINK (object);

  switch (prop_id) {
    case PROP_POST_MESSAGES:
      GST_OBJECT_LOCK (sink);
      sink->post_messages = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (sink);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gdk_pixbuf_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstGdkPixbufSink *sink;

  sink = GST_GDK_PIXBUF_SINK (object);

  switch (prop_id) {
    case PROP_POST_MESSAGES:
      GST_OBJECT_LOCK (sink);
      g_value_set_boolean (value, sink->post_messages);
      GST_OBJECT_UNLOCK (sink);
      break;
    case PROP_LAST_PIXBUF:
      GST_OBJECT_LOCK (sink);
      g_value_set_object (value, sink->last_pixbuf);
      GST_OBJECT_UNLOCK (sink);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
