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
#include <gst/rtsp/rtsp.h>
#include <gio/gio.h>

#ifndef __GST_RTSP_STREAM_H__
#define __GST_RTSP_STREAM_H__

G_BEGIN_DECLS

/* types for the media stream */
#define GST_TYPE_RTSP_STREAM              (gst_rtsp_stream_get_type ())
#define GST_IS_RTSP_STREAM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_STREAM))
#define GST_IS_RTSP_STREAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_STREAM))
#define GST_RTSP_STREAM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_STREAM, GstRTSPStreamClass))
#define GST_RTSP_STREAM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_STREAM, GstRTSPStream))
#define GST_RTSP_STREAM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_STREAM, GstRTSPStreamClass))
#define GST_RTSP_STREAM_CAST(obj)         ((GstRTSPStream*)(obj))
#define GST_RTSP_STREAM_CLASS_CAST(klass) ((GstRTSPStreamClass*)(klass))

typedef struct _GstRTSPStream GstRTSPStream;
typedef struct _GstRTSPStreamClass GstRTSPStreamClass;
typedef struct _GstRTSPStreamPrivate GstRTSPStreamPrivate;

#include "rtsp-stream-transport.h"
#include "rtsp-address-pool.h"
#include "rtsp-session.h"

/**
 * GstRTSPStream:
 *
 * The definition of a media stream.
 */
struct _GstRTSPStream {
  GObject       parent;

  /*< private >*/
  GstRTSPStreamPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

struct _GstRTSPStreamClass {
  GObjectClass parent_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType             gst_rtsp_stream_get_type         (void);

GstRTSPStream *   gst_rtsp_stream_new              (guint idx, GstElement *payloader,
                                                    GstPad *pad);
guint             gst_rtsp_stream_get_index        (GstRTSPStream *stream);
guint             gst_rtsp_stream_get_pt           (GstRTSPStream *stream);
GstPad *          gst_rtsp_stream_get_srcpad       (GstRTSPStream *stream);
GstPad *          gst_rtsp_stream_get_sinkpad      (GstRTSPStream *stream);

void              gst_rtsp_stream_set_control      (GstRTSPStream *stream, const gchar *control);
gchar *           gst_rtsp_stream_get_control      (GstRTSPStream *stream);
gboolean          gst_rtsp_stream_has_control      (GstRTSPStream *stream, const gchar *control);

void              gst_rtsp_stream_set_mtu          (GstRTSPStream *stream, guint mtu);
guint             gst_rtsp_stream_get_mtu          (GstRTSPStream *stream);

void              gst_rtsp_stream_set_dscp_qos     (GstRTSPStream *stream, gint dscp_qos);
gint              gst_rtsp_stream_get_dscp_qos     (GstRTSPStream *stream);

gboolean          gst_rtsp_stream_is_transport_supported  (GstRTSPStream *stream,
                                                           GstRTSPTransport *transport);

void              gst_rtsp_stream_set_profiles     (GstRTSPStream *stream, GstRTSPProfile profiles);
GstRTSPProfile    gst_rtsp_stream_get_profiles     (GstRTSPStream *stream);

void              gst_rtsp_stream_set_protocols    (GstRTSPStream *stream, GstRTSPLowerTrans protocols);
GstRTSPLowerTrans gst_rtsp_stream_get_protocols    (GstRTSPStream *stream);

void              gst_rtsp_stream_set_address_pool (GstRTSPStream *stream, GstRTSPAddressPool *pool);
GstRTSPAddressPool *
                  gst_rtsp_stream_get_address_pool (GstRTSPStream *stream);

GstRTSPAddress *  gst_rtsp_stream_reserve_address  (GstRTSPStream *stream,
                                                    const gchar * address,
                                                    guint port,
                                                    guint n_ports,
                                                    guint ttl);

gboolean          gst_rtsp_stream_join_bin         (GstRTSPStream *stream,
                                                    GstBin *bin, GstElement *rtpbin,
                                                    GstState state);
gboolean          gst_rtsp_stream_leave_bin        (GstRTSPStream *stream,
                                                    GstBin *bin, GstElement *rtpbin);

gboolean          gst_rtsp_stream_set_blocked      (GstRTSPStream * stream,
                                                    gboolean blocked);
gboolean          gst_rtsp_stream_is_blocking      (GstRTSPStream * stream);

void              gst_rtsp_stream_get_server_port  (GstRTSPStream *stream,
                                                    GstRTSPRange *server_port,
                                                    GSocketFamily family);
GstRTSPAddress *  gst_rtsp_stream_get_multicast_address (GstRTSPStream *stream,
                                                         GSocketFamily family);


GObject *         gst_rtsp_stream_get_rtpsession   (GstRTSPStream *stream);

void              gst_rtsp_stream_get_ssrc         (GstRTSPStream *stream,
                                                    guint *ssrc);

gboolean          gst_rtsp_stream_get_rtpinfo      (GstRTSPStream *stream,
                                                    guint *rtptime, guint *seq,
                                                    guint *clock_rate,
                                                    GstClockTime *running_time);
GstCaps *         gst_rtsp_stream_get_caps         (GstRTSPStream *stream);

GstFlowReturn     gst_rtsp_stream_recv_rtp         (GstRTSPStream *stream,
                                                    GstBuffer *buffer);
GstFlowReturn     gst_rtsp_stream_recv_rtcp        (GstRTSPStream *stream,
                                                    GstBuffer *buffer);

gboolean          gst_rtsp_stream_add_transport    (GstRTSPStream *stream,
                                                    GstRTSPStreamTransport *trans);
gboolean          gst_rtsp_stream_remove_transport (GstRTSPStream *stream,
                                                    GstRTSPStreamTransport *trans);

GSocket *         gst_rtsp_stream_get_rtp_socket   (GstRTSPStream *stream,
                                                    GSocketFamily family);
GSocket *         gst_rtsp_stream_get_rtcp_socket  (GstRTSPStream *stream,
                                                    GSocketFamily family);

gboolean          gst_rtsp_stream_update_crypto    (GstRTSPStream * stream,
                                                    guint ssrc, GstCaps * crypto);

gboolean          gst_rtsp_stream_query_position   (GstRTSPStream * stream,
                                                    gint64 * position);
gboolean          gst_rtsp_stream_query_stop       (GstRTSPStream * stream,
                                                    gint64 * stop);

void              gst_rtsp_stream_set_seqnum_offset          (GstRTSPStream *stream, guint16 seqnum);
guint16           gst_rtsp_stream_get_current_seqnum          (GstRTSPStream *stream);
void              gst_rtsp_stream_set_retransmission_time     (GstRTSPStream *stream, GstClockTime time);
GstClockTime      gst_rtsp_stream_get_retransmission_time     (GstRTSPStream *stream);
guint             gst_rtsp_stream_get_retransmission_pt       (GstRTSPStream * stream);
void              gst_rtsp_stream_set_retransmission_pt       (GstRTSPStream * stream,
                                                               guint rtx_pt);

void              gst_rtsp_stream_set_pt_map                 (GstRTSPStream * stream, guint pt, GstCaps * caps);
GstElement *      gst_rtsp_stream_request_aux_sender         (GstRTSPStream * stream, guint sessid);
/**
 * GstRTSPStreamTransportFilterFunc:
 * @stream: a #GstRTSPStream object
 * @trans: a #GstRTSPStreamTransport in @stream
 * @user_data: user data that has been given to gst_rtsp_stream_transport_filter()
 *
 * This function will be called by the gst_rtsp_stream_transport_filter(). An
 * implementation should return a value of #GstRTSPFilterResult.
 *
 * When this function returns #GST_RTSP_FILTER_REMOVE, @trans will be removed
 * from @stream.
 *
 * A return value of #GST_RTSP_FILTER_KEEP will leave @trans untouched in
 * @stream.
 *
 * A value of #GST_RTSP_FILTER_REF will add @trans to the result #GList of
 * gst_rtsp_stream_transport_filter().
 *
 * Returns: a #GstRTSPFilterResult.
 */
typedef GstRTSPFilterResult (*GstRTSPStreamTransportFilterFunc) (GstRTSPStream *stream,
                                                                 GstRTSPStreamTransport *trans,
                                                                 gpointer user_data);

GList *                gst_rtsp_stream_transport_filter  (GstRTSPStream *stream,
                                                          GstRTSPStreamTransportFilterFunc func,
                                                          gpointer user_data);

G_END_DECLS

#endif /* __GST_RTSP_STREAM_H__ */
