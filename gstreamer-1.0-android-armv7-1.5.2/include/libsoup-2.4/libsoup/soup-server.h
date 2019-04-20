/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2000-2003, Ximian, Inc.
 */

#ifndef SOUP_SERVER_H
#define SOUP_SERVER_H 1

#include <libsoup/soup-types.h>
#include <libsoup/soup-uri.h>
#include <libsoup/soup-websocket-connection.h>

G_BEGIN_DECLS

#define SOUP_TYPE_SERVER            (soup_server_get_type ())
#define SOUP_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOUP_TYPE_SERVER, SoupServer))
#define SOUP_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SOUP_TYPE_SERVER, SoupServerClass))
#define SOUP_IS_SERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOUP_TYPE_SERVER))
#define SOUP_IS_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), SOUP_TYPE_SERVER))
#define SOUP_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SOUP_TYPE_SERVER, SoupServerClass))

typedef struct SoupClientContext SoupClientContext;
GType soup_client_context_get_type (void);
#define SOUP_TYPE_CLIENT_CONTEXT (soup_client_context_get_type ())

typedef enum {
	SOUP_SERVER_LISTEN_HTTPS     = (1 << 0),
	SOUP_SERVER_LISTEN_IPV4_ONLY = (1 << 1),
	SOUP_SERVER_LISTEN_IPV6_ONLY = (1 << 2)
} SoupServerListenOptions;

struct _SoupServer {
	GObject parent;

};

typedef struct {
	GObjectClass parent_class;

	/* signals */
	void (*request_started)  (SoupServer *server, SoupMessage *msg,
				  SoupClientContext *client);
	void (*request_read)     (SoupServer *server, SoupMessage *msg,
				  SoupClientContext *client);
	void (*request_finished) (SoupServer *server, SoupMessage *msg,
				  SoupClientContext *client);
	void (*request_aborted)  (SoupServer *server, SoupMessage *msg,
				  SoupClientContext *client);

	/* Padding for future expansion */
	void (*_libsoup_reserved1) (void);
	void (*_libsoup_reserved2) (void);
	void (*_libsoup_reserved3) (void);
	void (*_libsoup_reserved4) (void);
} SoupServerClass;

GType soup_server_get_type (void);

#define SOUP_SERVER_TLS_CERTIFICATE "tls-certificate"
#define SOUP_SERVER_RAW_PATHS       "raw-paths"
#define SOUP_SERVER_SERVER_HEADER   "server-header"
#define SOUP_SERVER_HTTP_ALIASES    "http-aliases"
#define SOUP_SERVER_HTTPS_ALIASES   "https-aliases"

SoupServer     *soup_server_new                (const char               *optname1,
					        ...) G_GNUC_NULL_TERMINATED;

SOUP_AVAILABLE_IN_2_48
gboolean        soup_server_set_ssl_cert_file  (SoupServer               *server,
					        const char               *ssl_cert_file,
					        const char               *ssl_key_file,
					        GError                  **error);
gboolean        soup_server_is_https           (SoupServer               *server);

SOUP_AVAILABLE_IN_2_48
gboolean        soup_server_listen             (SoupServer               *server,
					        GSocketAddress           *address,
					        SoupServerListenOptions   options,
					        GError                  **error);
SOUP_AVAILABLE_IN_2_48
gboolean        soup_server_listen_all         (SoupServer               *server,
					        guint                     port,
					        SoupServerListenOptions   options,
					        GError                  **error);
SOUP_AVAILABLE_IN_2_48
gboolean        soup_server_listen_local       (SoupServer               *server,
					        guint                     port,
					        SoupServerListenOptions   options,
					        GError                  **error);
SOUP_AVAILABLE_IN_2_48
gboolean        soup_server_listen_socket      (SoupServer               *server,
					        GSocket                  *socket,
					        SoupServerListenOptions   options,
					        GError                  **error);
SOUP_AVAILABLE_IN_2_48
gboolean        soup_server_listen_fd          (SoupServer               *server,
					        int                       fd,
					        SoupServerListenOptions   options,
					        GError                  **error);
SOUP_AVAILABLE_IN_2_48
GSList         *soup_server_get_uris           (SoupServer               *server);
SOUP_AVAILABLE_IN_2_48
GSList         *soup_server_get_listeners      (SoupServer               *server);

void            soup_server_disconnect         (SoupServer               *server);

SOUP_AVAILABLE_IN_2_50
gboolean        soup_server_accept_iostream    (SoupServer               *server,
						GIOStream                *stream,
						GSocketAddress           *local_addr,
						GSocketAddress           *remote_addr,
						GError                  **error);

/* Handlers and auth */

typedef void  (*SoupServerCallback)            (SoupServer         *server,
						SoupMessage        *msg,
						const char         *path,
						GHashTable         *query,
						SoupClientContext  *client,
						gpointer            user_data);

void            soup_server_add_handler        (SoupServer         *server,
					        const char         *path,
					        SoupServerCallback  callback,
					        gpointer            user_data,
					        GDestroyNotify      destroy);
SOUP_AVAILABLE_IN_2_50
void            soup_server_add_early_handler  (SoupServer         *server,
						const char         *path,
						SoupServerCallback  callback,
						gpointer            user_data,
						GDestroyNotify      destroy);

typedef void (*SoupServerWebsocketCallback) (SoupServer              *server,
					     SoupWebsocketConnection *connection,
					     const char              *path,
					     SoupClientContext       *client,
					     gpointer                 user_data);
SOUP_AVAILABLE_IN_2_50
void            soup_server_add_websocket_handler (SoupServer                   *server,
						   const char                   *path,
						   const char                   *origin,
						   char                        **protocols,
						   SoupServerWebsocketCallback   callback,
						   gpointer                      user_data,
						   GDestroyNotify                destroy);

void            soup_server_remove_handler     (SoupServer         *server,
					        const char         *path);

void            soup_server_add_auth_domain    (SoupServer         *server,
					        SoupAuthDomain     *auth_domain);
void            soup_server_remove_auth_domain (SoupServer         *server,
					        SoupAuthDomain     *auth_domain);

/* I/O */

void            soup_server_pause_message   (SoupServer  *server,
					     SoupMessage *msg);
void            soup_server_unpause_message (SoupServer  *server,
					     SoupMessage *msg);

/* Client context */

SOUP_AVAILABLE_IN_2_48
GSocket        *soup_client_context_get_gsocket        (SoupClientContext *client);
SOUP_AVAILABLE_IN_2_48
GSocketAddress *soup_client_context_get_local_address  (SoupClientContext *client);
SOUP_AVAILABLE_IN_2_48
GSocketAddress *soup_client_context_get_remote_address (SoupClientContext *client);
const char     *soup_client_context_get_host           (SoupClientContext *client);
SoupAuthDomain *soup_client_context_get_auth_domain    (SoupClientContext *client);
const char     *soup_client_context_get_auth_user      (SoupClientContext *client);

SOUP_AVAILABLE_IN_2_50
GIOStream      *soup_client_context_steal_connection   (SoupClientContext *client);

/* Legacy API */

#define SOUP_SERVER_PORT          "port"
#define SOUP_SERVER_INTERFACE     "interface"
#define SOUP_SERVER_ASYNC_CONTEXT "async-context"
#define SOUP_SERVER_SSL_CERT_FILE "ssl-cert-file"
#define SOUP_SERVER_SSL_KEY_FILE  "ssl-key-file"

SOUP_DEPRECATED_IN_2_48
guint         soup_server_get_port            (SoupServer        *server);

SOUP_DEPRECATED_IN_2_48
SoupSocket   *soup_server_get_listener        (SoupServer        *server);

SOUP_DEPRECATED_IN_2_48
GMainContext *soup_server_get_async_context   (SoupServer        *server);

SOUP_DEPRECATED_IN_2_48
void          soup_server_run                 (SoupServer        *server);
SOUP_DEPRECATED_IN_2_48
void          soup_server_run_async           (SoupServer        *server);
SOUP_DEPRECATED_IN_2_48
void          soup_server_quit                (SoupServer        *server);

SOUP_DEPRECATED_IN_2_48
SoupAddress  *soup_client_context_get_address (SoupClientContext *client);
SOUP_DEPRECATED_IN_2_48
SoupSocket   *soup_client_context_get_socket  (SoupClientContext *client);

G_END_DECLS

#endif /* SOUP_SERVER_H */
