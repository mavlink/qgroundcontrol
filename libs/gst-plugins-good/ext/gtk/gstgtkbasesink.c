/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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
 * SECTION:gtkgstsink
 * @title: GstGtkBaseSink
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstgtkbasesink.h"
#include "gstgtkutils.h"

GST_DEBUG_CATEGORY (gst_debug_gtk_base_sink);
#define GST_CAT_DEFAULT gst_debug_gtk_base_sink

#define DEFAULT_FORCE_ASPECT_RATIO  TRUE
#define DEFAULT_PAR_N               0
#define DEFAULT_PAR_D               1
#define DEFAULT_IGNORE_ALPHA        TRUE

static void gst_gtk_base_sink_finalize (GObject * object);
static void gst_gtk_base_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * param_spec);
static void gst_gtk_base_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * param_spec);

static gboolean gst_gtk_base_sink_start (GstBaseSink * bsink);
static gboolean gst_gtk_base_sink_stop (GstBaseSink * bsink);

static GstStateChangeReturn
gst_gtk_base_sink_change_state (GstElement * element,
    GstStateChange transition);

static void gst_gtk_base_sink_get_times (GstBaseSink * bsink, GstBuffer * buf,
    GstClockTime * start, GstClockTime * end);
static gboolean gst_gtk_base_sink_set_caps (GstBaseSink * bsink,
    GstCaps * caps);
static GstFlowReturn gst_gtk_base_sink_show_frame (GstVideoSink * bsink,
    GstBuffer * buf);

static void
gst_gtk_base_sink_navigation_interface_init (GstNavigationInterface * iface);

enum
{
  PROP_0,
  PROP_WIDGET,
  PROP_FORCE_ASPECT_RATIO,
  PROP_PIXEL_ASPECT_RATIO,
  PROP_IGNORE_ALPHA,
};

#define gst_gtk_base_sink_parent_class parent_class
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GstGtkBaseSink, gst_gtk_base_sink,
    GST_TYPE_VIDEO_SINK,
    G_IMPLEMENT_INTERFACE (GST_TYPE_NAVIGATION,
        gst_gtk_base_sink_navigation_interface_init);
    GST_DEBUG_CATEGORY_INIT (gst_debug_gtk_base_sink,
        "gtkbasesink", 0, "Gtk Video Sink base class"));


static void
gst_gtk_base_sink_class_init (GstGtkBaseSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstVideoSinkClass *gstvideosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstvideosink_class = (GstVideoSinkClass *) klass;

  gobject_class->set_property = gst_gtk_base_sink_set_property;
  gobject_class->get_property = gst_gtk_base_sink_get_property;

  g_object_class_install_property (gobject_class, PROP_WIDGET,
      g_param_spec_object ("widget", "Gtk Widget",
          "The GtkWidget to place in the widget hierarchy "
          "(must only be get from the GTK main thread)",
          GTK_TYPE_WIDGET, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FORCE_ASPECT_RATIO,
      g_param_spec_boolean ("force-aspect-ratio",
          "Force aspect ratio",
          "When enabled, scaling will respect original aspect ratio",
          DEFAULT_FORCE_ASPECT_RATIO,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PIXEL_ASPECT_RATIO,
      gst_param_spec_fraction ("pixel-aspect-ratio", "Pixel Aspect Ratio",
          "The pixel aspect ratio of the device", DEFAULT_PAR_N, DEFAULT_PAR_D,
          G_MAXINT, 1, 1, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_IGNORE_ALPHA,
      g_param_spec_boolean ("ignore-alpha", "Ignore Alpha",
          "When enabled, alpha will be ignored and converted to black",
          DEFAULT_IGNORE_ALPHA, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gobject_class->finalize = gst_gtk_base_sink_finalize;

  gstelement_class->change_state = gst_gtk_base_sink_change_state;
  gstbasesink_class->set_caps = gst_gtk_base_sink_set_caps;
  gstbasesink_class->get_times = gst_gtk_base_sink_get_times;
  gstbasesink_class->start = gst_gtk_base_sink_start;
  gstbasesink_class->stop = gst_gtk_base_sink_stop;

  gstvideosink_class->show_frame = gst_gtk_base_sink_show_frame;
}

static void
gst_gtk_base_sink_init (GstGtkBaseSink * gtk_sink)
{
  gtk_sink->force_aspect_ratio = DEFAULT_FORCE_ASPECT_RATIO;
  gtk_sink->par_n = DEFAULT_PAR_N;
  gtk_sink->par_d = DEFAULT_PAR_D;
  gtk_sink->ignore_alpha = DEFAULT_IGNORE_ALPHA;
}

static void
gst_gtk_base_sink_finalize (GObject * object)
{
  GstGtkBaseSink *gtk_sink = GST_GTK_BASE_SINK (object);

  GST_OBJECT_LOCK (gtk_sink);
  if (gtk_sink->window && gtk_sink->window_destroy_id)
    g_signal_handler_disconnect (gtk_sink->window, gtk_sink->window_destroy_id);
  if (gtk_sink->widget && gtk_sink->widget_destroy_id)
    g_signal_handler_disconnect (gtk_sink->widget, gtk_sink->widget_destroy_id);

  g_clear_object (&gtk_sink->widget);
  GST_OBJECT_UNLOCK (gtk_sink);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
widget_destroy_cb (GtkWidget * widget, GstGtkBaseSink * gtk_sink)
{
  GST_OBJECT_LOCK (gtk_sink);
  g_clear_object (&gtk_sink->widget);
  GST_OBJECT_UNLOCK (gtk_sink);
}

static void
window_destroy_cb (GtkWidget * widget, GstGtkBaseSink * gtk_sink)
{
  GST_OBJECT_LOCK (gtk_sink);
  gtk_sink->window = NULL;
  GST_OBJECT_UNLOCK (gtk_sink);
}

static GtkGstBaseWidget *
gst_gtk_base_sink_get_widget (GstGtkBaseSink * gtk_sink)
{
  if (gtk_sink->widget != NULL)
    return gtk_sink->widget;

  /* Ensure GTK is initialized, this has no side effect if it was already
   * initialized. Also, we do that lazily, so the application can be first */
  if (!gtk_init_check (NULL, NULL)) {
    GST_ERROR_OBJECT (gtk_sink, "Could not ensure GTK initialization.");
    return NULL;
  }

  g_assert (GST_GTK_BASE_SINK_GET_CLASS (gtk_sink)->create_widget);
  gtk_sink->widget = (GtkGstBaseWidget *)
      GST_GTK_BASE_SINK_GET_CLASS (gtk_sink)->create_widget ();

  gtk_sink->bind_aspect_ratio =
      g_object_bind_property (gtk_sink, "force-aspect-ratio", gtk_sink->widget,
      "force-aspect-ratio", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  gtk_sink->bind_pixel_aspect_ratio =
      g_object_bind_property (gtk_sink, "pixel-aspect-ratio", gtk_sink->widget,
      "pixel-aspect-ratio", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  gtk_sink->bind_ignore_alpha =
      g_object_bind_property (gtk_sink, "ignore-alpha", gtk_sink->widget,
      "ignore-alpha", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  /* Take the floating ref, other wise the destruction of the container will
   * make this widget disappear possibly before we are done. */
  gst_object_ref_sink (gtk_sink->widget);
  gtk_sink->widget_destroy_id = g_signal_connect (gtk_sink->widget, "destroy",
      G_CALLBACK (widget_destroy_cb), gtk_sink);

  /* back pointer */
  gtk_gst_base_widget_set_element (GTK_GST_BASE_WIDGET (gtk_sink->widget),
      GST_ELEMENT (gtk_sink));

  return gtk_sink->widget;
}

static void
gst_gtk_base_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstGtkBaseSink *gtk_sink = GST_GTK_BASE_SINK (object);

  switch (prop_id) {
    case PROP_WIDGET:
    {
      GObject *widget = NULL;

      GST_OBJECT_LOCK (gtk_sink);
      if (gtk_sink->widget != NULL)
        widget = G_OBJECT (gtk_sink->widget);
      GST_OBJECT_UNLOCK (gtk_sink);

      if (!widget)
        widget =
            gst_gtk_invoke_on_main ((GThreadFunc) gst_gtk_base_sink_get_widget,
            gtk_sink);

      g_value_set_object (value, widget);
      break;
    }
    case PROP_FORCE_ASPECT_RATIO:
      g_value_set_boolean (value, gtk_sink->force_aspect_ratio);
      break;
    case PROP_PIXEL_ASPECT_RATIO:
      gst_value_set_fraction (value, gtk_sink->par_n, gtk_sink->par_d);
      break;
    case PROP_IGNORE_ALPHA:
      g_value_set_boolean (value, gtk_sink->ignore_alpha);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gtk_base_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGtkBaseSink *gtk_sink = GST_GTK_BASE_SINK (object);

  switch (prop_id) {
    case PROP_FORCE_ASPECT_RATIO:
      gtk_sink->force_aspect_ratio = g_value_get_boolean (value);
      break;
    case PROP_PIXEL_ASPECT_RATIO:
      gtk_sink->par_n = gst_value_get_fraction_numerator (value);
      gtk_sink->par_d = gst_value_get_fraction_denominator (value);
      break;
    case PROP_IGNORE_ALPHA:
      gtk_sink->ignore_alpha = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gtk_base_sink_navigation_send_event (GstNavigation * navigation,
    GstStructure * structure)
{
  GstGtkBaseSink *sink = GST_GTK_BASE_SINK (navigation);
  GstEvent *event;
  GstPad *pad;

  event = gst_event_new_navigation (structure);
  pad = gst_pad_get_peer (GST_VIDEO_SINK_PAD (sink));

  GST_TRACE_OBJECT (sink, "navigation event %" GST_PTR_FORMAT, structure);

  if (GST_IS_PAD (pad) && GST_IS_EVENT (event)) {
    if (!gst_pad_send_event (pad, gst_event_ref (event))) {
      /* If upstream didn't handle the event we'll post a message with it
       * for the application in case it wants to do something with it */
      gst_element_post_message (GST_ELEMENT_CAST (sink),
          gst_navigation_message_new_event (GST_OBJECT_CAST (sink), event));
    }
    gst_event_unref (event);
    gst_object_unref (pad);
  }
}

static void
gst_gtk_base_sink_navigation_interface_init (GstNavigationInterface * iface)
{
  iface->send_event = gst_gtk_base_sink_navigation_send_event;
}

static gboolean
gst_gtk_base_sink_start_on_main (GstBaseSink * bsink)
{
  GstGtkBaseSink *gst_sink = GST_GTK_BASE_SINK (bsink);
  GstGtkBaseSinkClass *klass = GST_GTK_BASE_SINK_GET_CLASS (bsink);
  GtkWidget *toplevel;

  if (gst_gtk_base_sink_get_widget (gst_sink) == NULL)
    return FALSE;

  /* After this point, gtk_sink->widget will always be set */

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (gst_sink->widget));
  if (!gtk_widget_is_toplevel (toplevel)) {
    /* sanity check */
    g_assert (klass->window_title);

    /* User did not add widget its own UI, let's popup a new GtkWindow to
     * make gst-launch-1.0 work. */
    gst_sink->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (gst_sink->window), 640, 480);
    gtk_window_set_title (GTK_WINDOW (gst_sink->window), klass->window_title);
    gtk_container_add (GTK_CONTAINER (gst_sink->window), toplevel);
    gst_sink->window_destroy_id = g_signal_connect (gst_sink->window, "destroy",
        G_CALLBACK (window_destroy_cb), gst_sink);
  }

  return TRUE;
}

static gboolean
gst_gtk_base_sink_start (GstBaseSink * bsink)
{
  return ! !gst_gtk_invoke_on_main ((GThreadFunc)
      gst_gtk_base_sink_start_on_main, bsink);
}

static gboolean
gst_gtk_base_sink_stop_on_main (GstBaseSink * bsink)
{
  GstGtkBaseSink *gst_sink = GST_GTK_BASE_SINK (bsink);

  if (gst_sink->window) {
    gtk_widget_destroy (gst_sink->window);
    gst_sink->window = NULL;
    gst_sink->widget = NULL;
  }

  return TRUE;
}

static gboolean
gst_gtk_base_sink_stop (GstBaseSink * bsink)
{
  GstGtkBaseSink *gst_sink = GST_GTK_BASE_SINK (bsink);

  if (gst_sink->window)
    return ! !gst_gtk_invoke_on_main ((GThreadFunc)
        gst_gtk_base_sink_stop_on_main, bsink);

  return TRUE;
}

static void
gst_gtk_widget_show_all_and_unref (GtkWidget * widget)
{
  gtk_widget_show_all (widget);
  g_object_unref (widget);
}

static GstStateChangeReturn
gst_gtk_base_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstGtkBaseSink *gtk_sink = GST_GTK_BASE_SINK (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  GST_DEBUG_OBJECT (element, "changing state: %s => %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    {
      GtkWindow *window = NULL;

      GST_OBJECT_LOCK (gtk_sink);
      if (gtk_sink->window)
        window = g_object_ref (GTK_WINDOW (gtk_sink->window));
      GST_OBJECT_UNLOCK (gtk_sink);

      if (window)
        gst_gtk_invoke_on_main ((GThreadFunc) gst_gtk_widget_show_all_and_unref,
            window);

      break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_OBJECT_LOCK (gtk_sink);
      if (gtk_sink->widget)
        gtk_gst_base_widget_set_buffer (gtk_sink->widget, NULL);
      GST_OBJECT_UNLOCK (gtk_sink);
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_gtk_base_sink_get_times (GstBaseSink * bsink, GstBuffer * buf,
    GstClockTime * start, GstClockTime * end)
{
  GstGtkBaseSink *gtk_sink;

  gtk_sink = GST_GTK_BASE_SINK (bsink);

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
    *start = GST_BUFFER_TIMESTAMP (buf);
    if (GST_BUFFER_DURATION_IS_VALID (buf))
      *end = *start + GST_BUFFER_DURATION (buf);
    else {
      if (GST_VIDEO_INFO_FPS_N (&gtk_sink->v_info) > 0) {
        *end = *start +
            gst_util_uint64_scale_int (GST_SECOND,
            GST_VIDEO_INFO_FPS_D (&gtk_sink->v_info),
            GST_VIDEO_INFO_FPS_N (&gtk_sink->v_info));
      }
    }
  }
}

gboolean
gst_gtk_base_sink_set_caps (GstBaseSink * bsink, GstCaps * caps)
{
  GstGtkBaseSink *gtk_sink = GST_GTK_BASE_SINK (bsink);

  GST_DEBUG ("set caps with %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&gtk_sink->v_info, caps))
    return FALSE;

  GST_OBJECT_LOCK (gtk_sink);

  if (gtk_sink->widget == NULL) {
    GST_OBJECT_UNLOCK (gtk_sink);
    GST_ELEMENT_ERROR (gtk_sink, RESOURCE, NOT_FOUND,
        ("%s", "Output widget was destroyed"), (NULL));
    return FALSE;
  }

  if (!gtk_gst_base_widget_set_format (gtk_sink->widget, &gtk_sink->v_info)) {
    GST_OBJECT_UNLOCK (gtk_sink);
    return FALSE;
  }
  GST_OBJECT_UNLOCK (gtk_sink);

  return TRUE;
}

static GstFlowReturn
gst_gtk_base_sink_show_frame (GstVideoSink * vsink, GstBuffer * buf)
{
  GstGtkBaseSink *gtk_sink;

  GST_TRACE ("rendering buffer:%p", buf);

  gtk_sink = GST_GTK_BASE_SINK (vsink);

  GST_OBJECT_LOCK (vsink);

  if (gtk_sink->widget == NULL) {
    GST_OBJECT_UNLOCK (gtk_sink);
    GST_ELEMENT_ERROR (gtk_sink, RESOURCE, NOT_FOUND,
        ("%s", "Output widget was destroyed"), (NULL));
    return GST_FLOW_ERROR;
  }

  gtk_gst_base_widget_set_buffer (gtk_sink->widget, buf);

  GST_OBJECT_UNLOCK (gtk_sink);

  return GST_FLOW_OK;
}
