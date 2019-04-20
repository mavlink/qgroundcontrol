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

#ifndef __GST_RTSP_SERVER_H__
#define __GST_RTSP_SERVER_H__

#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _GstRTSPServer GstRTSPServer;
typedef struct _GstRTSPServerClass GstRTSPServerClass;
typedef struct _GstRTSPServerPrivate GstRTSPServerPrivate;

#include "rtsp-session-pool.h"
#include "rtsp-session.h"
#include "rtsp-media.h"
#include "rtsp-stream.h"
#include "rtsp-stream-transport.h"
#include "rtsp-address-pool.h"
#include "rtsp-thread-pool.h"
#include "rtsp-client.h"
#include "rtsp-context.h"
#include "rtsp-server.h"
#include "rtsp-mount-points.h"
#include "rtsp-media-factory.h"
#include "rtsp-permissions.h"
#include "rtsp-auth.h"
#include "rtsp-token.h"
#include "rtsp-session-media.h"
#include "rtsp-sdp.h"
#include "rtsp-media-factory-uri.h"
#include "rtsp-params.h"

#define GST_TYPE_RTSP_SERVER              (gst_rtsp_server_get_type ())
#define GST_IS_RTSP_SERVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_SERVER))
#define GST_IS_RTSP_SERVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_SERVER))
#define GST_RTSP_SERVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_SERVER, GstRTSPServerClass))
#define GST_RTSP_SERVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_SERVER, GstRTSPServer))
#define GST_RTSP_SERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_SERVER, GstRTSPServerClass))
#define GST_RTSP_SERVER_CAST(obj)         ((GstRTSPServer*)(obj))
#define GST_RTSP_SERVER_CLASS_CAST(klass) ((GstRTSPServerClass*)(klass))

/**
 * GstRTSPServer:
 *
 * This object listens on a port, creates and manages the clients connected to
 * it.
 */
struct _GstRTSPServer {
  GObject      parent;

  /*< private >*/
  GstRTSPServerPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

/**
 * GstRTSPServerClass:
 * @create_client: Create, configure a new GstRTSPClient
 *          object that handles the new connection on @socket. The default
 *          implementation will create a GstRTSPClient and will configure the
 *          mount-points, auth, session-pool and thread-pool on the client.
 * @client_connected: emited when a new client connected.
 *
 * The RTSP server class structure
 */
struct _GstRTSPServerClass {
  GObjectClass  parent_class;

  GstRTSPClient * (*create_client)      (GstRTSPServer *server);

  /* signals */
  void            (*client_connected)   (GstRTSPServer *server, GstRTSPClient *client);

  /*< private >*/
  gpointer         _gst_reserved[GST_PADDING_LARGE];
};

GType                 gst_rtsp_server_get_type             (void);

GstRTSPServer *       gst_rtsp_server_new                  (void);

void                  gst_rtsp_server_set_address          (GstRTSPServer *server, const gchar *address);
gchar *               gst_rtsp_server_get_address          (GstRTSPServer *server);

void                  gst_rtsp_server_set_service          (GstRTSPServer *server, const gchar *service);
gchar *               gst_rtsp_server_get_service          (GstRTSPServer *server);

void                  gst_rtsp_server_set_backlog          (GstRTSPServer *server, gint backlog);
gint                  gst_rtsp_server_get_backlog          (GstRTSPServer *server);

int                   gst_rtsp_server_get_bound_port       (GstRTSPServer *server);

void                  gst_rtsp_server_set_session_pool     (GstRTSPServer *server, GstRTSPSessionPool *pool);
GstRTSPSessionPool *  gst_rtsp_server_get_session_pool     (GstRTSPServer *server);

void                  gst_rtsp_server_set_mount_points     (GstRTSPServer *server, GstRTSPMountPoints *mounts);
GstRTSPMountPoints *  gst_rtsp_server_get_mount_points     (GstRTSPServer *server);

void                  gst_rtsp_server_set_auth             (GstRTSPServer *server, GstRTSPAuth *auth);
GstRTSPAuth *         gst_rtsp_server_get_auth             (GstRTSPServer *server);

void                  gst_rtsp_server_set_thread_pool      (GstRTSPServer *server, GstRTSPThreadPool *pool);
GstRTSPThreadPool *   gst_rtsp_server_get_thread_pool      (GstRTSPServer *server);

gboolean              gst_rtsp_server_transfer_connection  (GstRTSPServer * server, GSocket *socket,
                                                            const gchar * ip, gint port,
                                                            const gchar *initial_buffer);

gboolean              gst_rtsp_server_io_func              (GSocket *socket, GIOCondition condition,
                                                            GstRTSPServer *server);

GSocket *             gst_rtsp_server_create_socket        (GstRTSPServer *server,
                                                            GCancellable  *cancellable,
                                                            GError **error);
GSource *             gst_rtsp_server_create_source        (GstRTSPServer *server,
                                                            GCancellable * cancellable,
                                                            GError **error);
guint                 gst_rtsp_server_attach               (GstRTSPServer *server,
                                                            GMainContext *context);

/**
 * GstRTSPServerClientFilterFunc:
 * @server: a #GstRTSPServer object
 * @client: a #GstRTSPClient in @server
 * @user_data: user data that has been given to gst_rtsp_server_client_filter()
 *
 * This function will be called by the gst_rtsp_server_client_filter(). An
 * implementation should return a value of #GstRTSPFilterResult.
 *
 * When this function returns #GST_RTSP_FILTER_REMOVE, @client will be removed
 * from @server.
 *
 * A return value of #GST_RTSP_FILTER_KEEP will leave @client untouched in
 * @server.
 *
 * A value of #GST_RTSP_FILTER_REF will add @client to the result #GList of
 * gst_rtsp_server_client_filter().
 *
 * Returns: a #GstRTSPFilterResult.
 */
typedef GstRTSPFilterResult (*GstRTSPServerClientFilterFunc)  (GstRTSPServer *server,
                                                               GstRTSPClient *client,
                                                               gpointer user_data);

GList *                gst_rtsp_server_client_filter    (GstRTSPServer *server,
                                                         GstRTSPServerClientFilterFunc func,
                                                         gpointer user_data);

G_END_DECLS

#endif /* __GST_RTSP_SERVER_H__ */
