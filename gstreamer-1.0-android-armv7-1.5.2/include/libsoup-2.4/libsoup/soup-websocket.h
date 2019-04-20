/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * soup-websocket.h: This file was originally part of Cockpit.
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

#ifndef __SOUP_WEBSOCKET_H__
#define __SOUP_WEBSOCKET_H__

#include <libsoup/soup-types.h>

G_BEGIN_DECLS

#define SOUP_WEBSOCKET_ERROR (soup_websocket_error_get_quark ())
SOUP_AVAILABLE_IN_2_50
GQuark soup_websocket_error_get_quark (void) G_GNUC_CONST;

typedef enum {
	SOUP_WEBSOCKET_ERROR_FAILED,
	SOUP_WEBSOCKET_ERROR_NOT_WEBSOCKET,
	SOUP_WEBSOCKET_ERROR_BAD_HANDSHAKE,
	SOUP_WEBSOCKET_ERROR_BAD_ORIGIN,
} SoupWebsocketError;

typedef enum {
	SOUP_WEBSOCKET_CONNECTION_UNKNOWN,
	SOUP_WEBSOCKET_CONNECTION_CLIENT,
	SOUP_WEBSOCKET_CONNECTION_SERVER
} SoupWebsocketConnectionType;

typedef enum {
	SOUP_WEBSOCKET_DATA_TEXT = 0x01,
	SOUP_WEBSOCKET_DATA_BINARY = 0x02,
} SoupWebsocketDataType;

typedef enum {
	SOUP_WEBSOCKET_CLOSE_NORMAL = 1000,
	SOUP_WEBSOCKET_CLOSE_GOING_AWAY = 1001,
	SOUP_WEBSOCKET_CLOSE_PROTOCOL_ERROR = 1002,
	SOUP_WEBSOCKET_CLOSE_UNSUPPORTED_DATA = 1003,
	SOUP_WEBSOCKET_CLOSE_NO_STATUS = 1005,
	SOUP_WEBSOCKET_CLOSE_ABNORMAL = 1006,
	SOUP_WEBSOCKET_CLOSE_BAD_DATA = 1007,
	SOUP_WEBSOCKET_CLOSE_POLICY_VIOLATION = 1008,
	SOUP_WEBSOCKET_CLOSE_TOO_BIG = 1009,
	SOUP_WEBSOCKET_CLOSE_NO_EXTENSION = 1010,
	SOUP_WEBSOCKET_CLOSE_SERVER_ERROR = 1011,
	SOUP_WEBSOCKET_CLOSE_TLS_HANDSHAKE = 1015,
} SoupWebsocketCloseCode;

typedef enum {
	SOUP_WEBSOCKET_STATE_OPEN = 1,
	SOUP_WEBSOCKET_STATE_CLOSING = 2,
	SOUP_WEBSOCKET_STATE_CLOSED = 3,
} SoupWebsocketState;

SOUP_AVAILABLE_IN_2_50
void     soup_websocket_client_prepare_handshake (SoupMessage  *msg,
						  const char   *origin,
						  char        **protocols);

SOUP_AVAILABLE_IN_2_50
gboolean soup_websocket_client_verify_handshake  (SoupMessage  *msg,
						  GError      **error);

SOUP_AVAILABLE_IN_2_50
gboolean soup_websocket_server_check_handshake   (SoupMessage  *msg,
						  const char   *origin,
						  char        **protocols,
						  GError      **error);

SOUP_AVAILABLE_IN_2_50
gboolean soup_websocket_server_process_handshake (SoupMessage  *msg,
						  const char   *origin,
						  char        **protocols);

G_END_DECLS

#endif /* __SOUP_WEBSOCKET_H__ */
