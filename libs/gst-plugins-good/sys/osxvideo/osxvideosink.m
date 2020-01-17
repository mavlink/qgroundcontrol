/* GStreamer
 * OSX video sink
 * Copyright (C) 2004-6 Zaheer Abbas Merali <zaheerabbas at merali dot org>
 * Copyright (C) 2007,2008,2009 Pioneers of the Inevitable <songbird@songbirdnest.com>
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
 * The development of this code was made possible due to the involvement of
 * Pioneers of the Inevitable, the creators of the Songbird Music player.
 *
 */

/**
 * SECTION:element-osxvideosink
 *
 * The OSXVideoSink renders video frames to a MacOSX window. The video output
 * must be directed to a window embedded in an existing NSApp.
 *
 */

#include "config.h"
#include <gst/video/videooverlay.h>
#include <gst/video/navigation.h>
#include <gst/video/video.h>

#include "osxvideosink.h"
#include <unistd.h>
#import "cocoawindow.h"

GST_DEBUG_CATEGORY (gst_debug_osx_video_sink);
#define GST_CAT_DEFAULT gst_debug_osx_video_sink

#ifndef GSTREAMER_GLIB_COCOA_NSAPPLICATION
#include <pthread.h>
extern void _CFRunLoopSetCurrent (CFRunLoopRef rl);
extern pthread_t _CFMainPThread;
#endif

static GstStaticPadTemplate gst_osx_video_sink_sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], "
        "height = (int) [ 1, MAX ], "
#if G_BYTE_ORDER == G_BIG_ENDIAN
       "format = (string) YUY2")
#else
        "format = (string) UYVY")
#endif
    );

enum
{
  ARG_0,
  ARG_EMBED,
  ARG_FORCE_PAR,
};

static void gst_osx_video_sink_osxwindow_destroy (GstOSXVideoSink * osxvideosink);

#ifndef GSTREAMER_GLIB_COCOA_NSAPPLICATION
static GMutex _run_loop_check_mutex;
static GMutex _run_loop_mutex;
static GCond _run_loop_cond;
#endif

static GstOSXVideoSinkClass *sink_class = NULL;
static GstVideoSinkClass *parent_class = NULL;

#if MAC_OS_X_VERSION_MAX_ALLOWED < 101200
#define NSEventMaskAny                       NSAnyEventMask
#define NSWindowStyleMaskTitled              NSTitledWindowMask
#define NSWindowStyleMaskClosable            NSClosableWindowMask
#define NSWindowStyleMaskResizable           NSResizableWindowMask
#define NSWindowStyleMaskTexturedBackground  NSTexturedBackgroundWindowMask
#define NSWindowStyleMaskMiniaturizable      NSMiniaturizableWindowMask
#endif

/* Helper to trigger calls from the main thread */
static void
gst_osx_video_sink_call_from_main_thread(GstOSXVideoSink *osxvideosink,
    NSObject * object, SEL function, NSObject *data, BOOL waitUntilDone)
{

  NSThread *thread;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  if (sink_class->ns_app_thread == NULL){
    thread = [NSThread mainThread];
  } else {
    thread = sink_class->ns_app_thread;
  }

  [object performSelector:function onThread:thread
          withObject:data waitUntilDone:waitUntilDone];
  [pool release];
}

#ifndef GSTREAMER_GLIB_COCOA_NSAPPLICATION
/* Poll for cocoa events */
static void
run_ns_app_loop (void) {
  NSEvent *event;
  NSAutoreleasePool *pool =[[NSAutoreleasePool alloc] init];
  NSDate *pollTime = nil;

  /* when running the loop in a thread we want to sleep as long as possible */
  pollTime = [NSDate distantFuture];

  do {
      event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:pollTime
          inMode:NSDefaultRunLoopMode dequeue:YES];
      [NSApp sendEvent:event];
    }
  while (event != nil);
  [pool release];
}

static void
gst_osx_videosink_check_main_run_loop (GstOSXVideoSink *sink)
{
  /* check if the main run loop is running */
  gboolean is_running;

  /* the easy way */
  is_running = [[NSRunLoop mainRunLoop] currentMode] != nil;
  if (is_running) {
    goto exit;
  } else {
    /* the previous check doesn't always work with main loops that run
     * cocoa's main run loop manually, like the gdk one, giving false
     * negatives. This check defers a call to the main thread and waits to
     * be awaken by this function. */
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    GstOSXVideoSinkObject * object = (GstOSXVideoSinkObject *) sink->osxvideosinkobject;
    gint64 abstime;

    g_mutex_lock (&_run_loop_mutex);
    [object performSelectorOnMainThread:
          @selector(checkMainRunLoop)
          withObject:nil waitUntilDone:NO];
    /* Wait 100 ms */
    abstime = g_get_monotonic_time () + 100 * 1000;
    is_running = g_cond_wait_until (&_run_loop_cond,
        &_run_loop_mutex, abstime);
    g_mutex_unlock (&_run_loop_mutex);

    [pool release];
  }

exit:
  {
  GST_DEBUG_OBJECT(sink, "The main runloop %s is running",
      is_running ? "" : " not ");
  if (is_running) {
    sink_class->run_loop_state = GST_OSX_VIDEO_SINK_RUN_LOOP_STATE_RUNNING;
    sink_class->ns_app_thread = [NSThread mainThread];
  } else {
    sink_class->run_loop_state = GST_OSX_VIDEO_SINK_RUN_LOOP_STATE_NOT_RUNNING;
  }
  }
}

static void
gst_osx_video_sink_run_cocoa_loop (GstOSXVideoSink * sink )
{
  /* Cocoa applications require a main runloop running to dispatch UI
   * events and process deferred calls to the main thread through
   * perfermSelectorOnMainThread.
   * Since the sink needs to create it's own Cocoa window when no
   * external NSView is passed to the sink through the GstVideoOverlay API,
   * we need to run the cocoa mainloop somehow.
   * This run loop can only be started once, by the first sink needing it
   */

  g_mutex_lock (&_run_loop_check_mutex);

  if (sink_class->run_loop_state == GST_OSX_VIDEO_SINK_RUN_LOOP_STATE_UNKNOWN) {
    gst_osx_videosink_check_main_run_loop (sink);
  }

  if (sink_class->run_loop_state == GST_OSX_VIDEO_SINK_RUN_LOOP_STATE_RUNNING) {
    g_mutex_unlock (&_run_loop_check_mutex);
    return;
  }

  if (sink_class->ns_app_thread == NULL) {
    /* run the main runloop in a separate thread */

    /* override [NSThread isMainThread] with our own implementation so that we can
     * make it believe our dedicated thread is the main thread
     */
    Method origIsMainThread = class_getClassMethod([NSThread class],
        NSSelectorFromString(@"isMainThread"));
    Method ourIsMainThread = class_getClassMethod([GstOSXVideoSinkObject class],
        NSSelectorFromString(@"isMainThread"));

    method_exchangeImplementations(origIsMainThread, ourIsMainThread);

    sink_class->ns_app_thread = [[NSThread alloc]
        initWithTarget:sink->osxvideosinkobject
        selector:@selector(nsAppThread) object:nil];
    [sink_class->ns_app_thread start];

    g_mutex_lock (&_run_loop_mutex);
    g_cond_wait (&_run_loop_cond, &_run_loop_mutex);
    g_mutex_unlock (&_run_loop_mutex);
  }

  g_mutex_unlock (&_run_loop_check_mutex);
}

static void
gst_osx_video_sink_stop_cocoa_loop (GstOSXVideoSink * osxvideosink)
{
}
#endif

/* This function handles osx window creation */
static gboolean
gst_osx_video_sink_osxwindow_create (GstOSXVideoSink * osxvideosink, gint width,
    gint height)
{
  NSRect rect;
  GstOSXWindow *osxwindow = NULL;
  gboolean res = TRUE;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  g_return_val_if_fail (GST_IS_OSX_VIDEO_SINK (osxvideosink), FALSE);

  GST_DEBUG_OBJECT (osxvideosink, "Creating new OSX window");

  osxvideosink->osxwindow = osxwindow = g_new0 (GstOSXWindow, 1);

  osxwindow->width = width;
  osxwindow->height = height;
  osxwindow->closed = FALSE;
  osxwindow->internal = FALSE;

  /* Allocate our GstGLView for the window, and then tell the application
   * about it (hopefully it's listening...) */
  rect.origin.x = 0.0;
  rect.origin.y = 0.0;
  rect.size.width = (float) osxwindow->width;
  rect.size.height = (float) osxwindow->height;
  osxwindow->gstview =[[GstGLView alloc] initWithFrame:rect];

#ifndef GSTREAMER_GLIB_COCOA_NSAPPLICATION
  gst_osx_video_sink_run_cocoa_loop (osxvideosink);
  [osxwindow->gstview setMainThread:sink_class->ns_app_thread];
#endif

  if (osxvideosink->superview == NULL) {
    GST_INFO_OBJECT (osxvideosink, "emitting prepare-xwindow-id");
    gst_video_overlay_prepare_window_handle (GST_VIDEO_OVERLAY (osxvideosink));
  }

  if (osxvideosink->superview != NULL) {
    /* prepare-xwindow-id was handled, we have the superview in
     * osxvideosink->superview. We now add osxwindow->gstview to the superview
     * from the main thread
     */
    GST_INFO_OBJECT (osxvideosink, "we have a superview, adding our view to it");
    gst_osx_video_sink_call_from_main_thread(osxvideosink, osxwindow->gstview,
        @selector(addToSuperview:), osxvideosink->superview, NO);

  } else {
    gst_osx_video_sink_call_from_main_thread(osxvideosink,
      osxvideosink->osxvideosinkobject,
      @selector(createInternalWindow), nil, YES);
    GST_INFO_OBJECT (osxvideosink, "No superview, creating an internal window.");
  }
  [osxwindow->gstview setNavigation: GST_NAVIGATION(osxvideosink)];
  [osxvideosink->osxwindow->gstview setKeepAspectRatio: osxvideosink->keep_par];

  [pool release];

  return res;
}

static void
gst_osx_video_sink_osxwindow_destroy (GstOSXVideoSink * osxvideosink)
{
  NSAutoreleasePool *pool;

  g_return_if_fail (GST_IS_OSX_VIDEO_SINK (osxvideosink));
  pool = [[NSAutoreleasePool alloc] init];

  GST_OBJECT_LOCK (osxvideosink);
  gst_osx_video_sink_call_from_main_thread(osxvideosink,
      osxvideosink->osxvideosinkobject,
      @selector(destroy), (id) nil, YES);
  GST_OBJECT_UNLOCK (osxvideosink);
#ifndef GSTREAMER_GLIB_COCOA_NSAPPLICATION
  gst_osx_video_sink_stop_cocoa_loop (osxvideosink);
#endif
  [pool release];
}

/* This function resizes a GstXWindow */
static void
gst_osx_video_sink_osxwindow_resize (GstOSXVideoSink * osxvideosink,
    GstOSXWindow * osxwindow, guint width, guint height)
{
  GstOSXVideoSinkObject *object = osxvideosink->osxvideosinkobject;

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  g_return_if_fail (osxwindow != NULL);
  g_return_if_fail (GST_IS_OSX_VIDEO_SINK (osxvideosink));

  osxwindow->width = width;
  osxwindow->height = height;

  GST_DEBUG_OBJECT (osxvideosink, "Resizing window to (%d,%d)", width, height);

  /* Directly resize the underlying view */
  GST_DEBUG_OBJECT (osxvideosink, "Calling setVideoSize on %p", osxwindow->gstview);
  gst_osx_video_sink_call_from_main_thread (osxvideosink, object,
      @selector(resize), (id)nil, YES);

  [pool release];
}

static gboolean
gst_osx_video_sink_setcaps (GstBaseSink * bsink, GstCaps * caps)
{
  GstOSXVideoSink *osxvideosink;
  GstStructure *structure;
  gboolean res, result = FALSE;
  gint video_width, video_height;

  osxvideosink = GST_OSX_VIDEO_SINK (bsink);

  GST_DEBUG_OBJECT (osxvideosink, "caps: %" GST_PTR_FORMAT, caps);

  structure = gst_caps_get_structure (caps, 0);
  res = gst_structure_get_int (structure, "width", &video_width);
  res &= gst_structure_get_int (structure, "height", &video_height);

  if (!res) {
    goto beach;
  }

  GST_DEBUG_OBJECT (osxvideosink, "our format is: %dx%d video",
      video_width, video_height);

  GST_VIDEO_SINK_WIDTH (osxvideosink) = video_width;
  GST_VIDEO_SINK_HEIGHT (osxvideosink) = video_height;

  gst_osx_video_sink_osxwindow_resize (osxvideosink, osxvideosink->osxwindow,
      video_width, video_height);

  gst_video_info_from_caps (&osxvideosink->info, caps);

  result = TRUE;

beach:
  return result;

}

static GstStateChangeReturn
gst_osx_video_sink_change_state (GstElement * element,
    GstStateChange transition)
{
  GstOSXVideoSink *osxvideosink;
  GstStateChangeReturn ret;

  osxvideosink = GST_OSX_VIDEO_SINK (element);

  GST_DEBUG_OBJECT (osxvideosink, "%s => %s",
        gst_element_state_get_name(GST_STATE_TRANSITION_CURRENT (transition)),
        gst_element_state_get_name(GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      /* Creating our window and our image */
      GST_VIDEO_SINK_WIDTH (osxvideosink) = 320;
      GST_VIDEO_SINK_HEIGHT (osxvideosink) = 240;
      if (!gst_osx_video_sink_osxwindow_create (osxvideosink,
          GST_VIDEO_SINK_WIDTH (osxvideosink),
          GST_VIDEO_SINK_HEIGHT (osxvideosink))) {
        ret = GST_STATE_CHANGE_FAILURE;
        goto done;
      }
      break;
    default:
      break;
  }

  ret = (GST_ELEMENT_CLASS (parent_class))->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_VIDEO_SINK_WIDTH (osxvideosink) = 0;
      GST_VIDEO_SINK_HEIGHT (osxvideosink) = 0;
      gst_osx_video_sink_osxwindow_destroy (osxvideosink);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

done:
  return ret;
}

static GstFlowReturn
gst_osx_video_sink_show_frame (GstBaseSink * bsink, GstBuffer * buf)
{
  GstOSXVideoSink *osxvideosink;
  GstBufferObject* bufferobject;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  osxvideosink = GST_OSX_VIDEO_SINK (bsink);

  GST_DEBUG ("show_frame");
  bufferobject = [[GstBufferObject alloc] initWithBuffer:buf];
  gst_osx_video_sink_call_from_main_thread(osxvideosink,
      osxvideosink->osxvideosinkobject,
      @selector(showFrame:), bufferobject, NO);
  [pool release];
  return GST_FLOW_OK;
}

/* Buffer management */



/* =========================================== */
/*                                             */
/*              Init & Class init              */
/*                                             */
/* =========================================== */

static void
gst_osx_video_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOSXVideoSink *osxvideosink;

  g_return_if_fail (GST_IS_OSX_VIDEO_SINK (object));

  osxvideosink = GST_OSX_VIDEO_SINK (object);

  switch (prop_id) {
    case ARG_EMBED:
      g_warning ("The \"embed\" property of osxvideosink is deprecated and "
          "has no effect anymore. Use the GstVideoOverlay "
          "instead.");
      break;
    case ARG_FORCE_PAR:
      osxvideosink->keep_par = g_value_get_boolean(value);
      if (osxvideosink->osxwindow)
        [osxvideosink->osxwindow->gstview
            setKeepAspectRatio: osxvideosink->keep_par];
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_osx_video_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstOSXVideoSink *osxvideosink;

  g_return_if_fail (GST_IS_OSX_VIDEO_SINK (object));

  osxvideosink = GST_OSX_VIDEO_SINK (object);

  switch (prop_id) {
    case ARG_EMBED:
      g_value_set_boolean (value, FALSE);
      break;
    case ARG_FORCE_PAR:
      g_value_set_boolean (value, osxvideosink->keep_par);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_osx_video_sink_propose_allocation (GstBaseSink * base_sink, GstQuery * query)
{
    gst_query_add_allocation_meta (query,
        GST_VIDEO_META_API_TYPE, NULL);

    return TRUE;
}

static void
gst_osx_video_sink_init (GstOSXVideoSink * sink)
{
  sink->osxwindow = NULL;
  sink->superview = NULL;
  sink->osxvideosinkobject = [[GstOSXVideoSinkObject alloc] initWithSink:sink];
  sink->keep_par = FALSE;
}

static void
gst_osx_video_sink_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_static_metadata (element_class, "OSX Video sink",
      "Sink/Video", "OSX native videosink",
      "Zaheer Abbas Merali <zaheerabbas at merali dot org>");

  gst_element_class_add_static_pad_template (element_class, &gst_osx_video_sink_sink_template_factory);
}

static void
gst_osx_video_sink_finalize (GObject *object)
{
  GstOSXVideoSink *osxvideosink = GST_OSX_VIDEO_SINK (object);

  if (osxvideosink->superview)
    [osxvideosink->superview release];

  if (osxvideosink->osxvideosinkobject)
    [(GstOSXVideoSinkObject*)(osxvideosink->osxvideosinkobject) release];

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_osx_video_sink_class_init (GstOSXVideoSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;

  parent_class = g_type_class_ref (GST_TYPE_VIDEO_SINK);
  sink_class = klass;

  klass->run_loop_state = GST_OSX_VIDEO_SINK_RUN_LOOP_STATE_UNKNOWN;
  klass->ns_app_thread = NULL;

  gobject_class->set_property = gst_osx_video_sink_set_property;
  gobject_class->get_property = gst_osx_video_sink_get_property;
  gobject_class->finalize = gst_osx_video_sink_finalize;

  gstbasesink_class->set_caps = gst_osx_video_sink_setcaps;
  gstbasesink_class->preroll = gst_osx_video_sink_show_frame;
  gstbasesink_class->render = gst_osx_video_sink_show_frame;
  gstbasesink_class->propose_allocation = gst_osx_video_sink_propose_allocation;
  gstelement_class->change_state = gst_osx_video_sink_change_state;

  /**
   * GstOSXVideoSink:embed
   *
   * For ABI comatibility only, do not use
   *
   **/

  g_object_class_install_property (gobject_class, ARG_EMBED,
      g_param_spec_boolean ("embed", "embed", "For ABI compatibility only, do not use",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstOSXVideoSink:force-aspect-ratio
   *
   * When enabled, scaling will respect original aspect ratio.
   *
   **/

  g_object_class_install_property (gobject_class, ARG_FORCE_PAR,
      g_param_spec_boolean ("force-aspect-ratio", "force aspect ration",
          "When enabled, scaling will respect original aspect ration",
          TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_osx_video_sink_navigation_send_event (GstNavigation * navigation,
    GstStructure * structure)
{
  GstOSXVideoSink *osxvideosink = GST_OSX_VIDEO_SINK (navigation);
  GstPad *peer;
  GstEvent *event;
  GstVideoRectangle src = { 0, };
  GstVideoRectangle dst = { 0, };
  GstVideoRectangle result;
  NSRect bounds;
  gdouble x, y, xscale = 1.0, yscale = 1.0;

  peer = gst_pad_get_peer (GST_VIDEO_SINK_PAD (osxvideosink));

  if (!peer || !osxvideosink->osxwindow)
    return;

  event = gst_event_new_navigation (structure);

  bounds = [osxvideosink->osxwindow->gstview getDrawingBounds];

  if (osxvideosink->keep_par) {
    /* We get the frame position using the calculated geometry from _setcaps
       that respect pixel aspect ratios */
    src.w = GST_VIDEO_SINK_WIDTH (osxvideosink);
    src.h = GST_VIDEO_SINK_HEIGHT (osxvideosink);
    dst.w = bounds.size.width;
    dst.h = bounds.size.height;

    gst_video_sink_center_rect (src, dst, &result, TRUE);
    result.x += bounds.origin.x;
    result.y += bounds.origin.y;
  } else {
    result.x = bounds.origin.x;
    result.y = bounds.origin.y;
    result.w = bounds.size.width;
    result.h = bounds.size.height;
  }

  /* We calculate scaling using the original video frames geometry to include
     pixel aspect ratio scaling. */
  xscale = (gdouble) osxvideosink->osxwindow->width / result.w;
  yscale = (gdouble) osxvideosink->osxwindow->height / result.h;

  /* Converting pointer coordinates to the non scaled geometry */
  if (gst_structure_get_double (structure, "pointer_x", &x)) {
    x = MIN (x, result.x + result.w);
    x = MAX (x - result.x, 0);
    gst_structure_set (structure, "pointer_x", G_TYPE_DOUBLE,
        (gdouble) x * xscale, NULL);
  }
  if (gst_structure_get_double (structure, "pointer_y", &y)) {
    y = MIN (y, result.y + result.h);
    y = MAX (y - result.y, 0);
    gst_structure_set (structure, "pointer_y", G_TYPE_DOUBLE,
        (gdouble) y * yscale, NULL);
  }

  gst_pad_send_event (peer, event);
  gst_object_unref (peer);
}

static void
gst_osx_video_sink_navigation_init (GstNavigationInterface * iface)
{
  iface->send_event = gst_osx_video_sink_navigation_send_event;
}

static void
gst_osx_video_sink_set_window_handle (GstVideoOverlay * overlay, guintptr handle_id)
{
  GstOSXVideoSink *osxvideosink = GST_OSX_VIDEO_SINK (overlay);
  NSView *view = (NSView *) handle_id;

  gst_osx_video_sink_call_from_main_thread(osxvideosink,
      osxvideosink->osxvideosinkobject,
      @selector(setView:), view, YES);
}

static void
gst_osx_video_sink_xoverlay_init (GstVideoOverlayInterface * iface)
{
  iface->set_window_handle = gst_osx_video_sink_set_window_handle;
  iface->expose = NULL;
  iface->handle_events = NULL;
}

/* ============================================================= */
/*                                                               */
/*                       Public Methods                          */
/*                                                               */
/* ============================================================= */

/* =========================================== */
/*                                             */
/*          Object typing & Creation           */
/*                                             */
/* =========================================== */

GType
gst_osx_video_sink_get_type (void)
{
  static GType osxvideosink_type = 0;

  if (!osxvideosink_type) {
    static const GTypeInfo osxvideosink_info = {
      sizeof (GstOSXVideoSinkClass),
      gst_osx_video_sink_base_init,
      NULL,
      (GClassInitFunc) gst_osx_video_sink_class_init,
      NULL,
      NULL,
      sizeof (GstOSXVideoSink),
      0,
      (GInstanceInitFunc) gst_osx_video_sink_init,
    };

    static const GInterfaceInfo overlay_info = {
      (GInterfaceInitFunc) gst_osx_video_sink_xoverlay_init,
      NULL,
      NULL,
    };

    static const GInterfaceInfo navigation_info = {
      (GInterfaceInitFunc) gst_osx_video_sink_navigation_init,
      NULL,
      NULL,
    };
    osxvideosink_type = g_type_register_static (GST_TYPE_VIDEO_SINK,
        "GstOSXVideoSink", &osxvideosink_info, 0);

    g_type_add_interface_static (osxvideosink_type, GST_TYPE_VIDEO_OVERLAY,
        &overlay_info);
    g_type_add_interface_static (osxvideosink_type, GST_TYPE_NAVIGATION,
        &navigation_info);
  }

  return osxvideosink_type;
}

@implementation GstWindowDelegate
- (id) initWithSink: (GstOSXVideoSink *) sink
{
  self = [super init];
  self->osxvideosink = sink;
  return self;
}

- (void)windowWillClose:(NSNotification *)notification {
  /* Only handle close events if the window was closed manually by the user
   * and not because of a state change state to READY */
  if (osxvideosink->osxwindow == NULL) {
    return;
  }
  if (!osxvideosink->osxwindow->closed) {
    osxvideosink->osxwindow->closed = TRUE;
    GST_ELEMENT_ERROR (osxvideosink, RESOURCE, NOT_FOUND, ("Output window was closed"), (NULL));
    gst_osx_video_sink_osxwindow_destroy(osxvideosink);
  }
}

@end

@ implementation GstOSXVideoSinkObject

-(id) initWithSink: (GstOSXVideoSink*) sink
{
  self = [super init];
  self->osxvideosink = gst_object_ref (sink);
  return self;
}

-(void) dealloc {
  gst_object_unref (osxvideosink);
  [super dealloc];
}

-(void) createInternalWindow
{
  GstOSXWindow *osxwindow = osxvideosink->osxwindow;
  NSRect rect;
  unsigned int mask;

  [NSApplication sharedApplication];

  osxwindow->internal = TRUE;

  mask =  NSWindowStyleMaskTitled             |
          NSWindowStyleMaskClosable           |
          NSWindowStyleMaskResizable          |
          NSWindowStyleMaskTexturedBackground |
          NSWindowStyleMaskMiniaturizable;

  rect.origin.x = 100.0;
  rect.origin.y = 100.0;
  rect.size.width = (float) osxwindow->width;
  rect.size.height = (float) osxwindow->height;

  osxwindow->win =[[[GstOSXVideoSinkWindow alloc]
                       initWithContentNSRect: rect
                       styleMask: mask
                       backing: NSBackingStoreBuffered
                       defer: NO
                       screen: nil] retain];
  GST_DEBUG("VideoSinkWindow created, %p", osxwindow->win);
  [osxwindow->win orderFrontRegardless];
  osxwindow->gstview =[osxwindow->win gstView];
  [osxwindow->win setDelegate:[[GstWindowDelegate alloc]
      initWithSink:osxvideosink]];

}

+ (BOOL) isMainThread
{
  /* FIXME: ideally we should return YES only for ->ns_app_thread here */
  return YES;
}

- (void) setView: (NSView*)view
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  if (osxvideosink->superview) {
    GST_INFO_OBJECT (osxvideosink, "old xwindow id %p", osxvideosink->superview);
    if (osxvideosink->osxwindow) {
      [osxvideosink->osxwindow->gstview removeFromSuperview];
    }
    [osxvideosink->superview release];
  }
  if (osxvideosink->osxwindow != NULL && view != NULL) {
    if (osxvideosink->osxwindow->internal) {
      GST_INFO_OBJECT (osxvideosink, "closing internal window");
      osxvideosink->osxwindow->closed = TRUE;
      [osxvideosink->osxwindow->win close];
      [osxvideosink->osxwindow->win release];
    }
  }

  GST_INFO_OBJECT (osxvideosink, "set xwindow id %p", view);
  osxvideosink->superview = [view retain];
  if (osxvideosink->osxwindow) {
    [osxvideosink->osxwindow->gstview addToSuperview: osxvideosink->superview];
    if (view) {
      osxvideosink->osxwindow->internal = FALSE;
    }
  }

  [pool release];
}

- (void) resize
{
  GstOSXWindow *osxwindow = osxvideosink->osxwindow;

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  GST_INFO_OBJECT (osxvideosink, "resizing");
  NSSize size = {osxwindow->width, osxwindow->height};
  if (osxwindow->internal) {
    [osxwindow->win setContentSize:size];
  }
  if (osxwindow->gstview) {
      [osxwindow->gstview setVideoSize :(int)osxwindow->width :(int)osxwindow->height];
  }
  GST_INFO_OBJECT (osxvideosink, "done");

  [pool release];
}

- (void) showFrame: (GstBufferObject *) object
{
  GstVideoFrame frame;
  guint8 *data, *readp, *writep;
  gint i, active_width, stride;
  guint8 *texture_buffer;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  GstBuffer *buf = object->buf;

  GST_OBJECT_LOCK (osxvideosink);
  if (osxvideosink->osxwindow == NULL)
      goto no_window;

  texture_buffer = (guint8 *) [osxvideosink->osxwindow->gstview getTextureBuffer];
  if (G_UNLIKELY (texture_buffer == NULL))
      goto no_texture_buffer;

  if (!gst_video_frame_map (&frame, &osxvideosink->info, buf, GST_MAP_READ))
      goto no_map;

  data = readp = GST_VIDEO_FRAME_PLANE_DATA (&frame, 0);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE (&frame, 0);
  writep = texture_buffer;
  active_width = GST_VIDEO_SINK_WIDTH (osxvideosink) * sizeof (short);
  for (i = 0; i < GST_VIDEO_SINK_HEIGHT (osxvideosink); i++) {
      memcpy (writep, readp, active_width);
      writep += active_width;
      readp += stride;
  }
  [osxvideosink->osxwindow->gstview displayTexture];

  gst_video_frame_unmap (&frame);

out:
  GST_OBJECT_UNLOCK (osxvideosink);
  [object release];

  [pool release];
  return;

no_map:
  GST_WARNING_OBJECT (osxvideosink, "couldn't map frame");
  goto out;

no_window:
  GST_WARNING_OBJECT (osxvideosink, "not showing frame since we have no window (!?)");
  goto out;

no_texture_buffer:
  GST_ELEMENT_ERROR (osxvideosink, RESOURCE, WRITE, (NULL),
          ("the texture buffer is NULL"));
  goto out;
}

-(void) destroy
{
  NSAutoreleasePool *pool;
  GstOSXWindow *osxwindow;

  pool = [[NSAutoreleasePool alloc] init];

  osxwindow = osxvideosink->osxwindow;
  osxvideosink->osxwindow = NULL;

  if (osxwindow) {
    if (osxvideosink->superview) {
      [osxwindow->gstview removeFromSuperview];
    }
    [osxwindow->gstview release];
    if (osxwindow->internal) {
      if (!osxwindow->closed) {
        osxwindow->closed = TRUE;
        [osxwindow->win close];
        [osxwindow->win release];
      }
    }
    g_free (osxwindow);
  }
  [pool release];
}

#ifndef GSTREAMER_GLIB_COCOA_NSAPPLICATION
-(void) nsAppThread
{
  NSAutoreleasePool *pool;

  /* set the main runloop as the runloop for the current thread. This has the
   * effect that calling NSApp nextEventMatchingMask:untilDate:inMode:dequeue
   * runs the main runloop.
   */
  _CFRunLoopSetCurrent(CFRunLoopGetMain());

  /* this is needed to make IsMainThread checks in core foundation work from the
   * current thread
   */
  _CFMainPThread = pthread_self();

  pool = [[NSAutoreleasePool alloc] init];

  [NSApplication sharedApplication];
  [NSApp finishLaunching];

  g_mutex_lock (&_run_loop_mutex);
  g_cond_signal (&_run_loop_cond);
  g_mutex_unlock (&_run_loop_mutex);

  /* run the loop */
  run_ns_app_loop ();

  [pool release];
}

-(void) checkMainRunLoop
{
  g_mutex_lock (&_run_loop_mutex);
  g_cond_signal (&_run_loop_cond);
  g_mutex_unlock (&_run_loop_mutex);
}
#endif

@end

@ implementation GstBufferObject
-(id) initWithBuffer: (GstBuffer*) buffer
{
  self = [super init];
  gst_buffer_ref(buffer);
  self->buf = buffer;
  return self;
}

-(void) dealloc{
  gst_buffer_unref(buf);
  [super dealloc];
}
@end

static gboolean
plugin_init (GstPlugin * plugin)
{

  if (!gst_element_register (plugin, "osxvideosink",
          GST_RANK_MARGINAL, GST_TYPE_OSX_VIDEO_SINK))
    return FALSE;

  GST_DEBUG_CATEGORY_INIT (gst_debug_osx_video_sink, "osxvideosink", 0,
      "osxvideosink element");

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    osxvideo,
    "OSX native video output plugin",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
