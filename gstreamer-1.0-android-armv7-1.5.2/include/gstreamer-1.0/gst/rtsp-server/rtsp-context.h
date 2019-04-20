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
#include <gst/rtsp/gstrtspconnection.h>

#ifndef __GST_RTSP_CONTEXT_H__
#define __GST_RTSP_CONTEXT_H__

G_BEGIN_DECLS

#define GST_TYPE_RTSP_CONTEXT              (gst_rtsp_context_get_type ())

typedef struct _GstRTSPContext GstRTSPContext;

#include "rtsp-server.h"
#include "rtsp-media.h"
#include "rtsp-media-factory.h"
#include "rtsp-session-media.h"
#include "rtsp-auth.h"
#include "rtsp-thread-pool.h"
#include "rtsp-token.h"

/**
 * GstRTSPContext:
 * @server: the server
 * @conn: the connection
 * @client: the client
 * @request: the complete request
 * @uri: the complete url parsed from @request
 * @method: the parsed method of @uri
 * @auth: the current auth object or %NULL
 * @token: authorisation token
 * @session: the session, can be %NULL
 * @sessmedia: the session media for the url can be %NULL
 * @factory: the media factory for the url, can be %NULL
 * @media: the media for the url can be %NULL
 * @stream: the stream for the url can be %NULL
 * @response: the response
 * @trans: the stream transport, can be %NULL
 *
 * Information passed around containing the context of a request.
 */
struct _GstRTSPContext {
  GstRTSPServer          *server;
  GstRTSPConnection      *conn;
  GstRTSPClient          *client;
  GstRTSPMessage         *request;
  GstRTSPUrl             *uri;
  GstRTSPMethod           method;
  GstRTSPAuth            *auth;
  GstRTSPToken           *token;
  GstRTSPSession         *session;
  GstRTSPSessionMedia    *sessmedia;
  GstRTSPMediaFactory    *factory;
  GstRTSPMedia           *media;
  GstRTSPStream          *stream;
  GstRTSPMessage         *response;
  GstRTSPStreamTransport *trans;

  /*< private >*/
  gpointer            _gst_reserved[GST_PADDING - 1];
};

GType gst_rtsp_context_get_type (void);

GstRTSPContext *     gst_rtsp_context_get_current   (void);
void                 gst_rtsp_context_push_current  (GstRTSPContext * ctx);
void                 gst_rtsp_context_pop_current   (GstRTSPContext * ctx);


G_END_DECLS

#endif /* __GST_RTSP_CONTEXT_H__ */
