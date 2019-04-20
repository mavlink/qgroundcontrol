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

#include <gst/rtsp/gstrtsptransport.h>

#ifndef __GST_RTSP_SESSION_MEDIA_H__
#define __GST_RTSP_SESSION_MEDIA_H__

G_BEGIN_DECLS

#define GST_TYPE_RTSP_SESSION_MEDIA              (gst_rtsp_session_media_get_type ())
#define GST_IS_RTSP_SESSION_MEDIA(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_SESSION_MEDIA))
#define GST_IS_RTSP_SESSION_MEDIA_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_SESSION_MEDIA))
#define GST_RTSP_SESSION_MEDIA_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_SESSION_MEDIA, GstRTSPSessionMediaClass))
#define GST_RTSP_SESSION_MEDIA(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_SESSION_MEDIA, GstRTSPSessionMedia))
#define GST_RTSP_SESSION_MEDIA_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_SESSION_MEDIA, GstRTSPSessionMediaClass))
#define GST_RTSP_SESSION_MEDIA_CAST(obj)         ((GstRTSPSessionMedia*)(obj))
#define GST_RTSP_SESSION_MEDIA_CLASS_CAST(klass) ((GstRTSPSessionMediaClass*)(klass))

typedef struct _GstRTSPSessionMedia GstRTSPSessionMedia;
typedef struct _GstRTSPSessionMediaClass GstRTSPSessionMediaClass;
typedef struct _GstRTSPSessionMediaPrivate GstRTSPSessionMediaPrivate;

/**
 * GstRTSPSessionMedia:
 *
 * State of a client session regarding a specific media identified by path.
 */
struct _GstRTSPSessionMedia
{
  GObject  parent;

  /*< private >*/
  GstRTSPSessionMediaPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

struct _GstRTSPSessionMediaClass
{
  GObjectClass  parent_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType                    gst_rtsp_session_media_get_type       (void);

GstRTSPSessionMedia *    gst_rtsp_session_media_new            (const gchar *path,
                                                                GstRTSPMedia *media);

gboolean                 gst_rtsp_session_media_matches        (GstRTSPSessionMedia *media,
                                                                const gchar *path,
                                                                gint * matched);
GstRTSPMedia *           gst_rtsp_session_media_get_media      (GstRTSPSessionMedia *media);

GstClockTime             gst_rtsp_session_media_get_base_time  (GstRTSPSessionMedia *media);
/* control media */
gboolean                 gst_rtsp_session_media_set_state      (GstRTSPSessionMedia *media,
                                                                GstState state);

void                     gst_rtsp_session_media_set_rtsp_state (GstRTSPSessionMedia *media,
                                                                GstRTSPState state);
GstRTSPState             gst_rtsp_session_media_get_rtsp_state (GstRTSPSessionMedia *media);

/* get stream transport config */
GstRTSPStreamTransport * gst_rtsp_session_media_set_transport  (GstRTSPSessionMedia *media,
                                                                GstRTSPStream *stream,
                                                                GstRTSPTransport *tr);
GstRTSPStreamTransport * gst_rtsp_session_media_get_transport  (GstRTSPSessionMedia *media,
                                                                guint idx);

gboolean                 gst_rtsp_session_media_alloc_channels (GstRTSPSessionMedia *media,
                                                                GstRTSPRange *range);

gchar *                  gst_rtsp_session_media_get_rtpinfo    (GstRTSPSessionMedia * media);

G_END_DECLS

#endif /* __GST_RTSP_SESSION_MEDIA_H__ */
