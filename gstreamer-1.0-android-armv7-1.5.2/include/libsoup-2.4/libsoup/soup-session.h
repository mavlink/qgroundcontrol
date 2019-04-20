/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2000-2003, Ximian, Inc.
 */

#ifndef SOUP_SESSION_H
#define SOUP_SESSION_H 1

#include <libsoup/soup-types.h>
#include <libsoup/soup-address.h>
#include <libsoup/soup-message.h>
#include <libsoup/soup-websocket-connection.h>

G_BEGIN_DECLS

#define SOUP_TYPE_SESSION            (soup_session_get_type ())
#define SOUP_SESSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOUP_TYPE_SESSION, SoupSession))
#define SOUP_SESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SOUP_TYPE_SESSION, SoupSessionClass))
#define SOUP_IS_SESSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOUP_TYPE_SESSION))
#define SOUP_IS_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), SOUP_TYPE_SESSION))
#define SOUP_SESSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SOUP_TYPE_SESSION, SoupSessionClass))

typedef void (*SoupSessionCallback) (SoupSession           *session,
				     SoupMessage           *msg,
				     gpointer               user_data);

struct _SoupSession {
	GObject parent;

};

typedef struct {
	GObjectClass parent_class;

	/* signals */
	void (*request_started) (SoupSession *session, SoupMessage *msg,
				 SoupSocket *socket);
	void (*authenticate)    (SoupSession *session, SoupMessage *msg,
				 SoupAuth *auth, gboolean retrying);

	/* methods */
	void  (*queue_message)   (SoupSession *session, SoupMessage *msg,
				  SoupSessionCallback callback,
				  gpointer user_data);
	void  (*requeue_message) (SoupSession *session, SoupMessage *msg);
	guint (*send_message)    (SoupSession *session, SoupMessage *msg);

	void  (*cancel_message)  (SoupSession *session, SoupMessage *msg,
				  guint status_code);

	void  (*auth_required)   (SoupSession *session, SoupMessage *msg,
				  SoupAuth *auth, gboolean retrying);

	void  (*flush_queue)     (SoupSession *session);

	void  (*kick)            (SoupSession *session);

	/* Padding for future expansion */
	void (*_libsoup_reserved4) (void);
} SoupSessionClass;

GType soup_session_get_type (void);

#define SOUP_SESSION_LOCAL_ADDRESS          "local-address"
#define SOUP_SESSION_PROXY_URI              "proxy-uri"
#define SOUP_SESSION_PROXY_RESOLVER         "proxy-resolver"
#define SOUP_SESSION_MAX_CONNS              "max-conns"
#define SOUP_SESSION_MAX_CONNS_PER_HOST     "max-conns-per-host"
#define SOUP_SESSION_USE_NTLM               "use-ntlm"
#define SOUP_SESSION_SSL_CA_FILE            "ssl-ca-file"
#define SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE "ssl-use-system-ca-file"
#define SOUP_SESSION_TLS_DATABASE           "tls-database"
#define SOUP_SESSION_SSL_STRICT             "ssl-strict"
#define SOUP_SESSION_TLS_INTERACTION        "tls-interaction"
#define SOUP_SESSION_ASYNC_CONTEXT          "async-context"
#define SOUP_SESSION_USE_THREAD_CONTEXT     "use-thread-context"
#define SOUP_SESSION_TIMEOUT                "timeout"
#define SOUP_SESSION_USER_AGENT             "user-agent"
#define SOUP_SESSION_ACCEPT_LANGUAGE        "accept-language"
#define SOUP_SESSION_ACCEPT_LANGUAGE_AUTO   "accept-language-auto"
#define SOUP_SESSION_IDLE_TIMEOUT           "idle-timeout"
#define SOUP_SESSION_ADD_FEATURE            "add-feature"
#define SOUP_SESSION_ADD_FEATURE_BY_TYPE    "add-feature-by-type"
#define SOUP_SESSION_REMOVE_FEATURE_BY_TYPE "remove-feature-by-type"
#define SOUP_SESSION_HTTP_ALIASES       "http-aliases"
#define SOUP_SESSION_HTTPS_ALIASES      "https-aliases"

SOUP_AVAILABLE_IN_2_42
SoupSession    *soup_session_new              (void);

SOUP_AVAILABLE_IN_2_42
SoupSession    *soup_session_new_with_options (const char *optname1,
					       ...) G_GNUC_NULL_TERMINATED;

void            soup_session_queue_message    (SoupSession           *session,
					       SoupMessage           *msg,
					       SoupSessionCallback    callback,
					       gpointer               user_data);
void            soup_session_requeue_message  (SoupSession           *session,
					       SoupMessage           *msg);

guint           soup_session_send_message     (SoupSession           *session,
					       SoupMessage           *msg);

void            soup_session_pause_message    (SoupSession           *session,
					       SoupMessage           *msg);
void            soup_session_unpause_message  (SoupSession           *session,
					       SoupMessage           *msg);

void            soup_session_cancel_message   (SoupSession           *session,
					       SoupMessage           *msg,
					       guint                  status_code);
void            soup_session_abort            (SoupSession           *session);

GMainContext   *soup_session_get_async_context(SoupSession           *session);

SOUP_AVAILABLE_IN_2_42
void            soup_session_send_async       (SoupSession           *session,
					       SoupMessage           *msg,
					       GCancellable          *cancellable,
					       GAsyncReadyCallback    callback,
					       gpointer               user_data);
SOUP_AVAILABLE_IN_2_42
GInputStream   *soup_session_send_finish      (SoupSession           *session,
					       GAsyncResult          *result,
					       GError               **error);
SOUP_AVAILABLE_IN_2_42
GInputStream   *soup_session_send             (SoupSession           *session,
					       SoupMessage           *msg,
					       GCancellable          *cancellable,
					       GError               **error);

#ifndef SOUP_DISABLE_DEPRECATED
/* SOUP_AVAILABLE_IN_2_30 -- this trips up gtkdoc-scan */
SOUP_DEPRECATED_IN_2_38_FOR (soup_session_prefetch_dns)
void            soup_session_prepare_for_uri  (SoupSession           *session,
					       SoupURI               *uri);
#endif

SOUP_AVAILABLE_IN_2_38
void            soup_session_prefetch_dns     (SoupSession           *session,
					       const char            *hostname,
					       GCancellable          *cancellable,
					       SoupAddressCallback    callback,
					       gpointer               user_data);

SOUP_AVAILABLE_IN_2_38
gboolean        soup_session_would_redirect   (SoupSession           *session,
					       SoupMessage           *msg);
SOUP_AVAILABLE_IN_2_38
gboolean        soup_session_redirect_message (SoupSession           *session,
					       SoupMessage           *msg);

SOUP_AVAILABLE_IN_2_24
void                soup_session_add_feature            (SoupSession        *session,
							 SoupSessionFeature *feature);
SOUP_AVAILABLE_IN_2_24
void                soup_session_add_feature_by_type    (SoupSession        *session,
							 GType               feature_type);
SOUP_AVAILABLE_IN_2_24
void                soup_session_remove_feature         (SoupSession        *session,
							 SoupSessionFeature *feature);
SOUP_AVAILABLE_IN_2_24
void                soup_session_remove_feature_by_type (SoupSession        *session,
							 GType               feature_type);
SOUP_AVAILABLE_IN_2_42
gboolean            soup_session_has_feature            (SoupSession        *session,
							 GType               feature_type);
SOUP_AVAILABLE_IN_2_26
GSList             *soup_session_get_features           (SoupSession        *session,
							 GType               feature_type);
SOUP_AVAILABLE_IN_2_26
SoupSessionFeature *soup_session_get_feature            (SoupSession        *session,
							 GType               feature_type);
SOUP_AVAILABLE_IN_2_28
SoupSessionFeature *soup_session_get_feature_for_message(SoupSession        *session,
							 GType               feature_type,
							 SoupMessage        *msg);

SOUP_AVAILABLE_IN_2_42
SoupRequest     *soup_session_request          (SoupSession  *session,
						const char   *uri_string,
						GError      **error);
SOUP_AVAILABLE_IN_2_42
SoupRequest     *soup_session_request_uri      (SoupSession  *session,
						SoupURI      *uri,
						GError      **error);
SOUP_AVAILABLE_IN_2_42
SoupRequestHTTP *soup_session_request_http     (SoupSession  *session,
						const char   *method,
						const char   *uri_string,
						GError      **error);
SOUP_AVAILABLE_IN_2_42
SoupRequestHTTP *soup_session_request_http_uri (SoupSession  *session,
						const char   *method,
						SoupURI      *uri,
						GError      **error);

SOUP_AVAILABLE_IN_2_42
GQuark soup_request_error_quark (void);
#define SOUP_REQUEST_ERROR soup_request_error_quark ()

typedef enum {
	SOUP_REQUEST_ERROR_BAD_URI,
	SOUP_REQUEST_ERROR_UNSUPPORTED_URI_SCHEME,
	SOUP_REQUEST_ERROR_PARSING,
	SOUP_REQUEST_ERROR_ENCODING
} SoupRequestError;

SOUP_AVAILABLE_IN_2_50
GIOStream *soup_session_steal_connection (SoupSession *session,
					  SoupMessage *msg);

SOUP_AVAILABLE_IN_2_50
void                     soup_session_websocket_connect_async  (SoupSession          *session,
								SoupMessage          *msg,
								const char           *origin,
								char                **protocols,
								GCancellable         *cancellable,
								GAsyncReadyCallback   callback,
								gpointer              user_data);

SOUP_AVAILABLE_IN_2_50
SoupWebsocketConnection *soup_session_websocket_connect_finish (SoupSession          *session,
								GAsyncResult         *result,
								GError              **error);

G_END_DECLS

#endif /* SOUP_SESSION_H */
