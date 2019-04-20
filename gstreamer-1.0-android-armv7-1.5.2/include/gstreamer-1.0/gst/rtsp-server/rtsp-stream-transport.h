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
#include <gst/rtsp/gstrtsprange.h>
#include <gst/rtsp/gstrtspurl.h>

#ifndef __GST_RTSP_STREAM_TRANSPORT_H__
#define __GST_RTSP_STREAM_TRANSPORT_H__

G_BEGIN_DECLS

/* types for the media */
#define GST_TYPE_RTSP_STREAM_TRANSPORT              (gst_rtsp_stream_transport_get_type ())
#define GST_IS_RTSP_STREAM_TRANSPORT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_STREAM_TRANSPORT))
#define GST_IS_RTSP_STREAM_TRANSPORT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_STREAM_TRANSPORT))
#define GST_RTSP_STREAM_TRANSPORT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_STREAM_TRANSPORT, GstRTSPStreamTransportClass))
#define GST_RTSP_STREAM_TRANSPORT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_STREAM_TRANSPORT, GstRTSPStreamTransport))
#define GST_RTSP_STREAM_TRANSPORT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_STREAM_TRANSPORT, GstRTSPStreamTransportClass))
#define GST_RTSP_STREAM_TRANSPORT_CAST(obj)         ((GstRTSPStreamTransport*)(obj))
#define GST_RTSP_STREAM_TRANSPORT_CLASS_CAST(klass) ((GstRTSPStreamTransportClass*)(klass))

typedef struct _GstRTSPStreamTransport GstRTSPStreamTransport;
typedef struct _GstRTSPStreamTransportClass GstRTSPStreamTransportClass;
typedef struct _GstRTSPStreamTransportPrivate GstRTSPStreamTransportPrivate;

#include "rtsp-stream.h"

/**
 * GstRTSPSendFunc:
 * @buffer: a #GstBuffer
 * @channel: a channel
 * @user_data: user data
 *
 * Function registered with gst_rtsp_stream_transport_set_callbacks() and
 * called when @buffer must be sent on @channel.
 *
 * Returns: %TRUE on success
 */
typedef gboolean (*GstRTSPSendFunc)      (GstBuffer *buffer, guint8 channel, gpointer user_data);
/**
 * GstRTSPKeepAliveFunc:
 * @user_data: user data
 *
 * Function registered with gst_rtsp_stream_transport_set_keepalive() and called
 * when the stream is active.
 */
typedef void     (*GstRTSPKeepAliveFunc) (gpointer user_data);

/**
 * GstRTSPStreamTransport:
 * @parent: parent instance
 *
 * A Transport description for a stream
 */
struct _GstRTSPStreamTransport {
  GObject              parent;

  /*< private >*/
  GstRTSPStreamTransportPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

struct _GstRTSPStreamTransportClass {
  GObjectClass parent_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType                    gst_rtsp_stream_transport_get_type (void);

GstRTSPStreamTransport * gst_rtsp_stream_transport_new           (GstRTSPStream *stream,
                                                                  GstRTSPTransport *tr);

GstRTSPStream *          gst_rtsp_stream_transport_get_stream    (GstRTSPStreamTransport *trans);

void                     gst_rtsp_stream_transport_set_transport (GstRTSPStreamTransport *trans,
                                                                  GstRTSPTransport * tr);
const GstRTSPTransport * gst_rtsp_stream_transport_get_transport (GstRTSPStreamTransport *trans);

void                     gst_rtsp_stream_transport_set_url       (GstRTSPStreamTransport *trans,
                                                                  const GstRTSPUrl * url);
const GstRTSPUrl *       gst_rtsp_stream_transport_get_url       (GstRTSPStreamTransport *trans);


gchar *                  gst_rtsp_stream_transport_get_rtpinfo   (GstRTSPStreamTransport *trans,
                                                                  GstClockTime start_time);

void                     gst_rtsp_stream_transport_set_callbacks (GstRTSPStreamTransport *trans,
                                                                  GstRTSPSendFunc send_rtp,
                                                                  GstRTSPSendFunc send_rtcp,
                                                                  gpointer user_data,
                                                                  GDestroyNotify  notify);
void                     gst_rtsp_stream_transport_set_keepalive (GstRTSPStreamTransport *trans,
                                                                  GstRTSPKeepAliveFunc keep_alive,
                                                                  gpointer user_data,
                                                                  GDestroyNotify  notify);
void                     gst_rtsp_stream_transport_keep_alive    (GstRTSPStreamTransport *trans);

gboolean                 gst_rtsp_stream_transport_set_active    (GstRTSPStreamTransport *trans,
                                                                  gboolean active);

void                     gst_rtsp_stream_transport_set_timed_out (GstRTSPStreamTransport *trans,
                                                                  gboolean timedout);
gboolean                 gst_rtsp_stream_transport_is_timed_out  (GstRTSPStreamTransport *trans);



gboolean                 gst_rtsp_stream_transport_send_rtp      (GstRTSPStreamTransport *trans,
                                                                  GstBuffer *buffer);
gboolean                 gst_rtsp_stream_transport_send_rtcp     (GstRTSPStreamTransport *trans,
                                                                  GstBuffer *buffer);

GstFlowReturn            gst_rtsp_stream_transport_recv_data     (GstRTSPStreamTransport *trans,
                                                                  guint channel, GstBuffer *buffer);

G_END_DECLS

#endif /* __GST_RTSP_STREAM_TRANSPORT_H__ */
