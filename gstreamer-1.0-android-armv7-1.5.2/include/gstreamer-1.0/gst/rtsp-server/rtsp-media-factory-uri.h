/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
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

#include <gst/gst.h>

#include "rtsp-media-factory.h"

#ifndef __GST_RTSP_MEDIA_FACTORY_URI_H__
#define __GST_RTSP_MEDIA_FACTORY_URI_H__

G_BEGIN_DECLS

/* types for the media factory */
#define GST_TYPE_RTSP_MEDIA_FACTORY_URI              (gst_rtsp_media_factory_uri_get_type ())
#define GST_IS_RTSP_MEDIA_FACTORY_URI(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_MEDIA_FACTORY_URI))
#define GST_IS_RTSP_MEDIA_FACTORY_URI_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_MEDIA_FACTORY_URI))
#define GST_RTSP_MEDIA_FACTORY_URI_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_MEDIA_FACTORY_URI, GstRTSPMediaFactoryURIClass))
#define GST_RTSP_MEDIA_FACTORY_URI(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_MEDIA_FACTORY_URI, GstRTSPMediaFactoryURI))
#define GST_RTSP_MEDIA_FACTORY_URI_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_MEDIA_FACTORY_URI, GstRTSPMediaFactoryURIClass))
#define GST_RTSP_MEDIA_FACTORY_URI_CAST(obj)         ((GstRTSPMediaFactoryURI*)(obj))
#define GST_RTSP_MEDIA_FACTORY_URI_CLASS_CAST(klass) ((GstRTSPMediaFactoryURIClass*)(klass))

typedef struct _GstRTSPMediaFactoryURI GstRTSPMediaFactoryURI;
typedef struct _GstRTSPMediaFactoryURIClass GstRTSPMediaFactoryURIClass;
typedef struct _GstRTSPMediaFactoryURIPrivate GstRTSPMediaFactoryURIPrivate;

/**
 * GstRTSPMediaFactoryURI:
 *
 * A media factory that creates a pipeline to play and uri.
 */
struct _GstRTSPMediaFactoryURI {
  GstRTSPMediaFactory   parent;

  /*< private >*/
  GstRTSPMediaFactoryURIPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

/**
 * GstRTSPMediaFactoryURIClass:
 *
 * The #GstRTSPMediaFactoryURI class structure.
 */
struct _GstRTSPMediaFactoryURIClass {
  GstRTSPMediaFactoryClass  parent_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType                 gst_rtsp_media_factory_uri_get_type   (void);

/* creating the factory */
GstRTSPMediaFactoryURI * gst_rtsp_media_factory_uri_new     (void);

/* configuring the factory */
void                  gst_rtsp_media_factory_uri_set_uri  (GstRTSPMediaFactoryURI *factory,
                                                           const gchar *uri);
gchar *               gst_rtsp_media_factory_uri_get_uri  (GstRTSPMediaFactoryURI *factory);

G_END_DECLS

#endif /* __GST_RTSP_MEDIA_FACTORY_URI_H__ */
