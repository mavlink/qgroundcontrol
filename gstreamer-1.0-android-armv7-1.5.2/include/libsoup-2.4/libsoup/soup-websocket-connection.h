/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * soup-websocket-connection.h: This file was originally part of Cockpit.
 *
 * Copyright 2013, 2014 Red Hat, Inc.
 *
 * Cockpit is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Cockpit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SOUP_WEBSOCKET_CONNECTION_H__
#define __SOUP_WEBSOCKET_CONNECTION_H__

#include <libsoup/soup-types.h>
#include <libsoup/soup-websocket.h>

G_BEGIN_DECLS

#define SOUP_TYPE_WEBSOCKET_CONNECTION         (soup_websocket_connection_get_type ())
#define SOUP_WEBSOCKET_CONNECTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SOUP_TYPE_WEBSOCKET_CONNECTION, SoupWebsocketConnection))
#define SOUP_IS_WEBSOCKET_CONNECTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SOUP_TYPE_WEBSOCKET_CONNECTION))
#define SOUP_WEBSOCKET_CONNECTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), SOUP_TYPE_WEBSOCKET_CONNECTION, SoupWebsocketConnectionClass))
#define SOUP_WEBSOCKET_CONNECTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SOUP_TYPE_WEBSOCKET_CONNECTION, SoupWebsocketConnectionClass))
#define SOUP_IS_WEBSOCKET_CONNECTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SOUP_TYPE_WEBSOCKET_CONNECTION))

typedef struct _SoupWebsocketConnectionPrivate  SoupWebsocketConnectionPrivate;

struct _SoupWebsocketConnection {
	GObject parent;

	/*< private >*/
	SoupWebsocketConnectionPrivate *pv;
};

typedef struct {
	GObjectClass parent;

	/* signals */
	void      (* message)     (SoupWebsocketConnection *self,
				   SoupWebsocketDataType type,
				   GBytes *message);

	void      (* error)       (SoupWebsocketConnection *self,
				   GError *error);

	void      (* closing)     (SoupWebsocketConnection *self);

	void      (* closed)      (SoupWebsocketConnection *self);
} SoupWebsocketConnectionClass;

SOUP_AVAILABLE_IN_2_50
GType soup_websocket_connection_get_type (void) G_GNUC_CONST;

SoupWebsocketConnection *soup_websocket_connection_new (GIOStream                    *stream,
							SoupURI                      *uri,
							SoupWebsocketConnectionType   type,
							const char                   *origin,
							const char                   *protocol);

SOUP_AVAILABLE_IN_2_50
GIOStream *         soup_websocket_connection_get_io_stream  (SoupWebsocketConnection *self);

SOUP_AVAILABLE_IN_2_50
SoupWebsocketConnectionType soup_websocket_connection_get_connection_type (SoupWebsocketConnection *self);

SOUP_AVAILABLE_IN_2_50
SoupURI *           soup_websocket_connection_get_uri        (SoupWebsocketConnection *self);

SOUP_AVAILABLE_IN_2_50
const char *        soup_websocket_connection_get_origin     (SoupWebsocketConnection *self);

SOUP_AVAILABLE_IN_2_50
const char *        soup_websocket_connection_get_protocol   (SoupWebsocketConnection *self);

SOUP_AVAILABLE_IN_2_50
SoupWebsocketState  soup_websocket_connection_get_state      (SoupWebsocketConnection *self);

SOUP_AVAILABLE_IN_2_50
gushort             soup_websocket_connection_get_close_code (SoupWebsocketConnection *self);

SOUP_AVAILABLE_IN_2_50
const char *        soup_websocket_connection_get_close_data (SoupWebsocketConnection *self);

SOUP_AVAILABLE_IN_2_50
void                soup_websocket_connection_send_text      (SoupWebsocketConnection *self,
							      const char *text);
SOUP_AVAILABLE_IN_2_50
void                soup_websocket_connection_send_binary    (SoupWebsocketConnection *self,
							      gconstpointer data,
							      gsize length);

SOUP_AVAILABLE_IN_2_50
void                soup_websocket_connection_close          (SoupWebsocketConnection *self,
							      gushort code,
							      const char *data);

G_END_DECLS

#endif /* __SOUP_WEBSOCKET_CONNECTION_H__ */
