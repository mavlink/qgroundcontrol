/* GStreamer GdkPixbuf-based image decoder
 * Copyright (C) 1999-2001 Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2003 David A. Schleef <ds@schleef.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>

#include "gstgdkpixbufdec.h"

GST_DEBUG_CATEGORY_STATIC (gdkpixbufdec_debug);
#define GST_CAT_DEFAULT gdkpixbufdec_debug

static GstStaticPadTemplate gst_gdk_pixbuf_dec_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/png; "
        /* "image/jpeg; " disabled because we can't handle MJPEG */
        /*"image/gif; " disabled because we can't handle animated gifs */
        "image/x-icon; "
        "application/x-navi-animation; "
        "image/x-cmu-raster; "
        "image/x-sun-raster; "
        "image/x-pixmap; "
        "image/tiff; "
        "image/x-portable-anymap; "
        "image/x-portable-bitmap; "
        "image/x-portable-graymap; "
        "image/x-portable-pixmap; "
        "image/bmp; "
        "image/x-bmp; "
        "image/x-MS-bmp; "
        "image/vnd.wap.wbmp; " "image/x-bitmap; " "image/x-tga; "
        "image/x-pcx; image/svg; image/svg+xml")
    );

static GstStaticPadTemplate gst_gdk_pixbuf_dec_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("RGB") "; "
        GST_VIDEO_CAPS_MAKE ("RGBA"))
    );

static GstStateChangeReturn
gst_gdk_pixbuf_dec_change_state (GstElement * element,
    GstStateChange transition);
static GstFlowReturn gst_gdk_pixbuf_dec_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);
static gboolean gst_gdk_pixbuf_dec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

#define gst_gdk_pixbuf_dec_parent_class parent_class
G_DEFINE_TYPE (GstGdkPixbufDec, gst_gdk_pixbuf_dec, GST_TYPE_ELEMENT);

static gboolean
gst_gdk_pixbuf_dec_sink_setcaps (GstGdkPixbufDec * filter, GstCaps * caps)
{
  const GValue *framerate;
  GstStructure *s;

  s = gst_caps_get_structure (caps, 0);

  if ((framerate = gst_structure_get_value (s, "framerate")) != NULL) {
    filter->in_fps_n = gst_value_get_fraction_numerator (framerate);
    filter->in_fps_d = gst_value_get_fraction_denominator (framerate);
    GST_DEBUG_OBJECT (filter, "got framerate of %d/%d fps => packetized mode",
        filter->in_fps_n, filter->in_fps_d);
  } else {
    filter->in_fps_n = 0;
    filter->in_fps_d = 1;
    GST_DEBUG_OBJECT (filter, "no framerate, assuming single image");
  }

  return TRUE;
}

static GstCaps *
gst_gdk_pixbuf_dec_get_capslist (GstCaps * filter)
{
  GSList *slist;
  GSList *slist0;
  GstCaps *capslist = NULL;
  GstCaps *return_caps = NULL;
  GstCaps *tmpl_caps;

  capslist = gst_caps_new_empty ();
  slist0 = gdk_pixbuf_get_formats ();

  for (slist = slist0; slist; slist = g_slist_next (slist)) {
    GdkPixbufFormat *pixbuf_format;
    char **mimetypes;
    char **mimetype;

    pixbuf_format = slist->data;
    mimetypes = gdk_pixbuf_format_get_mime_types (pixbuf_format);

    for (mimetype = mimetypes; *mimetype; mimetype++) {
      gst_caps_append_structure (capslist, gst_structure_new_empty (*mimetype));
    }
    g_strfreev (mimetypes);
  }
  g_slist_free (slist0);

  tmpl_caps =
      gst_static_caps_get (&gst_gdk_pixbuf_dec_sink_template.static_caps);
  return_caps = gst_caps_intersect (capslist, tmpl_caps);

  gst_caps_unref (tmpl_caps);
  gst_caps_unref (capslist);

  if (filter && return_caps) {
    GstCaps *temp;

    temp = gst_caps_intersect (return_caps, filter);
    gst_caps_unref (return_caps);
    return_caps = temp;
  }

  return return_caps;
}

static gboolean
gst_gdk_pixbuf_dec_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean res;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = gst_gdk_pixbuf_dec_get_capslist (filter);
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


/* initialize the plugin's class */
static void
gst_gdk_pixbuf_dec_class_init (GstGdkPixbufDecClass * klass)
{
  GstElementClass *gstelement_class;

  gstelement_class = (GstElementClass *) klass;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_dec_change_state);

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_gdk_pixbuf_dec_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_gdk_pixbuf_dec_sink_template);
  gst_element_class_set_static_metadata (gstelement_class,
      "GdkPixbuf image decoder", "Codec/Decoder/Image",
      "Decodes images in a video stream using GdkPixbuf",
      "David A. Schleef <ds@schleef.org>, Renato Filho <renato.filho@indt.org.br>");

  GST_DEBUG_CATEGORY_INIT (gdkpixbufdec_debug, "gdkpixbuf", 0,
      "GdkPixbuf image decoder");
}

static void
gst_gdk_pixbuf_dec_init (GstGdkPixbufDec * filter)
{
  filter->sinkpad =
      gst_pad_new_from_static_template (&gst_gdk_pixbuf_dec_sink_template,
      "sink");
  gst_pad_set_query_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_dec_sink_query));
  gst_pad_set_chain_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_dec_chain));
  gst_pad_set_event_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_gdk_pixbuf_dec_sink_event));
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad =
      gst_pad_new_from_static_template (&gst_gdk_pixbuf_dec_src_template,
      "src");
  gst_pad_use_fixed_caps (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->last_timestamp = GST_CLOCK_TIME_NONE;
  filter->pixbuf_loader = NULL;
  filter->packetized = FALSE;
}

static gboolean
gst_gdk_pixbuf_dec_setup_pool (GstGdkPixbufDec * filter, GstVideoInfo * info)
{
  GstCaps *target;
  GstQuery *query;
  GstBufferPool *pool;
  GstStructure *config;
  guint size, min, max;

  target = gst_pad_get_current_caps (filter->srcpad);
  if (!target)
    return FALSE;

  /* try to get a bufferpool now */
  /* find a pool for the negotiated caps now */
  query = gst_query_new_allocation (target, TRUE);

  if (!gst_pad_peer_query (filter->srcpad, query)) {
    /* not a problem, we use the query defaults */
    GST_DEBUG_OBJECT (filter, "ALLOCATION query failed");
  }

  if (gst_query_get_n_allocation_pools (query) > 0) {
    /* we got configuration from our peer, parse them */
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
  } else {
    pool = NULL;
    size = info->size;
    min = max = 0;
  }

  gst_query_unref (query);

  if (pool == NULL) {
    /* we did not get a pool, make one ourselves then */
    pool = gst_buffer_pool_new ();
  }

  /* and configure */
  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_set_params (config, target, size, min, max);
  gst_buffer_pool_set_config (pool, config);

  if (filter->pool) {
    gst_buffer_pool_set_active (filter->pool, FALSE);
    gst_object_unref (filter->pool);
  }
  filter->pool = pool;

  /* and activate */
  gst_buffer_pool_set_active (filter->pool, TRUE);

  gst_caps_unref (target);

  return TRUE;
}

static GstFlowReturn
gst_gdk_pixbuf_dec_flush (GstGdkPixbufDec * filter)
{
  GstBuffer *outbuf;
  GdkPixbuf *pixbuf;
  int y;
  guint8 *out_pix;
  guint8 *in_pix;
  int in_rowstride, out_rowstride;
  GstFlowReturn ret;
  GstCaps *caps = NULL;
  gint width, height;
  gint n_channels;
  GstVideoFrame frame;

  pixbuf = gdk_pixbuf_loader_get_pixbuf (filter->pixbuf_loader);
  if (pixbuf == NULL)
    goto no_pixbuf;

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (GST_VIDEO_INFO_FORMAT (&filter->info) == GST_VIDEO_FORMAT_UNKNOWN) {
    GstVideoInfo info;
    GstVideoFormat fmt;
    GList *l;

    GST_DEBUG ("Set size to %dx%d", width, height);

    n_channels = gdk_pixbuf_get_n_channels (pixbuf);
    switch (n_channels) {
      case 3:
        fmt = GST_VIDEO_FORMAT_RGB;
        break;
      case 4:
        fmt = GST_VIDEO_FORMAT_RGBA;
        break;
      default:
        goto channels_not_supported;
    }


    gst_video_info_init (&info);
    gst_video_info_set_format (&info, fmt, width, height);
    info.fps_n = filter->in_fps_n;
    info.fps_d = filter->in_fps_d;
    caps = gst_video_info_to_caps (&info);

    filter->info = info;

    gst_pad_set_caps (filter->srcpad, caps);
    gst_caps_unref (caps);

    gst_gdk_pixbuf_dec_setup_pool (filter, &info);

    for (l = filter->pending_events; l; l = l->next)
      gst_pad_push_event (filter->srcpad, l->data);
    g_list_free (filter->pending_events);
    filter->pending_events = NULL;
  }

  ret = gst_buffer_pool_acquire_buffer (filter->pool, &outbuf, NULL);
  if (ret != GST_FLOW_OK)
    goto no_buffer;

  GST_BUFFER_TIMESTAMP (outbuf) = filter->last_timestamp;
  GST_BUFFER_DURATION (outbuf) = GST_CLOCK_TIME_NONE;

  in_pix = gdk_pixbuf_get_pixels (pixbuf);
  in_rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  gst_video_frame_map (&frame, &filter->info, outbuf, GST_MAP_WRITE);
  out_pix = GST_VIDEO_FRAME_PLANE_DATA (&frame, 0);
  out_rowstride = GST_VIDEO_FRAME_PLANE_STRIDE (&frame, 0);

  for (y = 0; y < height; y++) {
    memcpy (out_pix, in_pix, width * GST_VIDEO_FRAME_COMP_PSTRIDE (&frame, 0));
    in_pix += in_rowstride;
    out_pix += out_rowstride;
  }

  gst_video_frame_unmap (&frame);

  GST_DEBUG ("pushing... %" G_GSIZE_FORMAT " bytes",
      gst_buffer_get_size (outbuf));
  ret = gst_pad_push (filter->srcpad, outbuf);

  if (ret != GST_FLOW_OK)
    GST_DEBUG_OBJECT (filter, "flow: %s", gst_flow_get_name (ret));

  return ret;

  /* ERRORS */
no_pixbuf:
  {
    GST_ELEMENT_ERROR (filter, STREAM, DECODE, (NULL),
        ("error getting pixbuf"));
    return GST_FLOW_ERROR;
  }
channels_not_supported:
  {
    GST_ELEMENT_ERROR (filter, STREAM, DECODE, (NULL),
        ("%d channels not supported", n_channels));
    return GST_FLOW_ERROR;
  }
no_buffer:
  {
    GST_DEBUG ("Failed to create outbuffer - %s", gst_flow_get_name (ret));
    return ret;
  }
}

static gboolean
gst_gdk_pixbuf_dec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstFlowReturn res = GST_FLOW_OK;
  gboolean ret = TRUE, forward = TRUE;
  GstGdkPixbufDec *pixbuf;

  pixbuf = GST_GDK_PIXBUF_DEC (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      ret = gst_gdk_pixbuf_dec_sink_setcaps (pixbuf, caps);
      forward = FALSE;
      break;
    }
    case GST_EVENT_EOS:
      if (pixbuf->pixbuf_loader != NULL) {
        gdk_pixbuf_loader_close (pixbuf->pixbuf_loader, NULL);
        res = gst_gdk_pixbuf_dec_flush (pixbuf);
        g_object_unref (G_OBJECT (pixbuf->pixbuf_loader));
        pixbuf->pixbuf_loader = NULL;
        /* as long as we don't have flow returns for event functions we need
         * to post an error here, or the application might never know that
         * things failed */
        if (res != GST_FLOW_OK && res != GST_FLOW_FLUSHING
            && res != GST_FLOW_EOS && res != GST_FLOW_NOT_LINKED) {
          GST_ELEMENT_FLOW_ERROR (pixbuf, res);
          forward = FALSE;
          ret = FALSE;
        }
      }
      break;
    case GST_EVENT_FLUSH_STOP:
      g_list_free_full (pixbuf->pending_events,
          (GDestroyNotify) gst_event_unref);
      pixbuf->pending_events = NULL;
      /* Fall through */
    case GST_EVENT_SEGMENT:
    {
      const GstSegment *segment;
      GstSegment output_segment;
      guint32 seqnum;

      gst_event_parse_segment (event, &segment);
      if (segment->format == GST_FORMAT_BYTES)
        pixbuf->packetized = FALSE;
      else
        pixbuf->packetized = TRUE;

      if (segment->format != GST_FORMAT_TIME) {
        seqnum = gst_event_get_seqnum (event);
        gst_event_unref (event);
        gst_segment_init (&output_segment, GST_FORMAT_TIME);
        event = gst_event_new_segment (&output_segment);
        gst_event_set_seqnum (event, seqnum);
      }

      if (pixbuf->pixbuf_loader != NULL) {
        gdk_pixbuf_loader_close (pixbuf->pixbuf_loader, NULL);
        g_object_unref (G_OBJECT (pixbuf->pixbuf_loader));
        pixbuf->pixbuf_loader = NULL;
      }
      break;
    }
    default:
      break;
  }
  if (forward) {
    if (!gst_pad_has_current_caps (pixbuf->srcpad) &&
        GST_EVENT_IS_SERIALIZED (event)
        && GST_EVENT_TYPE (event) > GST_EVENT_CAPS
        && GST_EVENT_TYPE (event) != GST_EVENT_FLUSH_STOP
        && GST_EVENT_TYPE (event) != GST_EVENT_EOS) {
      ret = TRUE;
      pixbuf->pending_events = g_list_prepend (pixbuf->pending_events, event);
    } else {
      ret = gst_pad_event_default (pad, parent, event);
    }
  } else {
    gst_event_unref (event);
  }
  return ret;
}

static GstFlowReturn
gst_gdk_pixbuf_dec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstGdkPixbufDec *filter;
  GstFlowReturn ret = GST_FLOW_OK;
  GError *error = NULL;
  GstClockTime timestamp;
  GstMapInfo map;

  filter = GST_GDK_PIXBUF_DEC (parent);

  timestamp = GST_BUFFER_TIMESTAMP (buf);

  if (GST_CLOCK_TIME_IS_VALID (timestamp))
    filter->last_timestamp = timestamp;

  GST_LOG_OBJECT (filter, "buffer with ts: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (filter->pixbuf_loader == NULL)
    filter->pixbuf_loader = gdk_pixbuf_loader_new ();

  gst_buffer_map (buf, &map, GST_MAP_READ);

  GST_LOG_OBJECT (filter, "Writing buffer size %d", (gint) map.size);
  if (!gdk_pixbuf_loader_write (filter->pixbuf_loader, map.data, map.size,
          &error))
    goto error;

  if (filter->packetized == TRUE) {
    gdk_pixbuf_loader_close (filter->pixbuf_loader, NULL);
    ret = gst_gdk_pixbuf_dec_flush (filter);
    g_object_unref (filter->pixbuf_loader);
    filter->pixbuf_loader = NULL;
  }

  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  return ret;

  /* ERRORS */
error:
  {
    GST_ELEMENT_ERROR (filter, STREAM, DECODE, (NULL),
        ("gdk_pixbuf_loader_write error: %s", error->message));
    g_error_free (error);
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
}

static GstStateChangeReturn
gst_gdk_pixbuf_dec_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstGdkPixbufDec *dec = GST_GDK_PIXBUF_DEC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      /* default to single image mode, setcaps function might not be called */
      dec->in_fps_n = 0;
      dec->in_fps_d = 1;
      gst_video_info_init (&dec->info);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      dec->in_fps_n = 0;
      dec->in_fps_d = 0;
      if (dec->pool) {
        gst_buffer_pool_set_active (dec->pool, FALSE);
        gst_object_replace ((GstObject **) & dec->pool, NULL);
      }
      g_list_free_full (dec->pending_events, (GDestroyNotify) gst_event_unref);
      dec->pending_events = NULL;
      if (dec->pixbuf_loader != NULL) {
        gdk_pixbuf_loader_close (dec->pixbuf_loader, NULL);
        g_object_unref (G_OBJECT (dec->pixbuf_loader));
        dec->pixbuf_loader = NULL;
      }
      break;
    default:
      break;
  }

  return ret;
}
