/* GStreamer
 * Copyright (C) 2007-2008 Wouter Cloetens <wouter@mind.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more
 */

/**
 * SECTION:element-souphttpsrc
 * @title: souphttpsrc
 *
 * This plugin reads data from a remote location specified by a URI.
 * Supported protocols are 'http', 'https'.
 *
 * An HTTP proxy must be specified by its URL.
 * If the "http_proxy" environment variable is set, its value is used.
 * If built with libsoup's GNOME integration features, the GNOME proxy
 * configuration will be used, or failing that, proxy autodetection.
 * The #GstSoupHTTPSrc:proxy property can be used to override the default.
 *
 * In case the #GstSoupHTTPSrc:iradio-mode property is set and the location is
 * an HTTP resource, souphttpsrc will send special Icecast HTTP headers to the
 * server to request additional Icecast meta-information.
 * If the server is not an Icecast server, it will behave as if the
 * #GstSoupHTTPSrc:iradio-mode property were not set. If it is, souphttpsrc will
 * output data with a media type of application/x-icy, in which case you will
 * need to use the #GstICYDemux element as follow-up element to extract the Icecast
 * metadata and to determine the underlying media type.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v souphttpsrc location=https://some.server.org/index.html
 *     ! filesink location=/home/joe/server.html
 * ]| The above pipeline reads a web page from a server using the HTTPS protocol
 * and writes it to a local file.
 * |[
 * gst-launch-1.0 -v souphttpsrc user-agent="FooPlayer 0.99 beta"
 *     automatic-redirect=false proxy=http://proxy.intranet.local:8080
 *     location=http://music.foobar.com/demo.mp3 ! mpgaudioparse
 *     ! mpg123audiodec ! audioconvert ! audioresample ! autoaudiosink
 * ]| The above pipeline will read and decode and play an mp3 file from a
 * web server using the HTTP protocol. If the server sends redirects,
 * the request fails instead of following the redirect. The specified
 * HTTP proxy server is used. The User-Agent HTTP request header
 * is set to a custom string instead of "GStreamer souphttpsrc."
 * |[
 * gst-launch-1.0 -v souphttpsrc location=http://10.11.12.13/mjpeg
 *     do-timestamp=true ! multipartdemux
 *     ! image/jpeg,width=640,height=480 ! matroskamux
 *     ! filesink location=mjpeg.mkv
 * ]| The above pipeline reads a motion JPEG stream from an IP camera
 * using the HTTP protocol, encoded as mime/multipart image/jpeg
 * parts, and writes a Matroska motion JPEG file. The width and
 * height properties are set in the caps to provide the Matroska
 * multiplexer with the information to set this in the header.
 * Timestamps are set on the buffers as they arrive from the camera.
 * These are used by the mime/multipart demultiplexer to emit timestamps
 * on the JPEG-encoded video frame buffers. This allows the Matroska
 * multiplexer to timestamp the frames in the resulting file.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>             /* atoi() */
#endif
#include <gst/gstelement.h>
#include <gst/gst-i18n-plugin.h>
#include <libsoup/soup.h>
#include "gstsouphttpsrc.h"
#include "gstsouputils.h"

#include <gst/tag/tag.h>

GST_DEBUG_CATEGORY_STATIC (souphttpsrc_debug);
#define GST_CAT_DEFAULT souphttpsrc_debug

#define GST_SOUP_SESSION_CONTEXT "gst.soup.session"

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_IS_LIVE,
  PROP_USER_AGENT,
  PROP_AUTOMATIC_REDIRECT,
  PROP_PROXY,
  PROP_USER_ID,
  PROP_USER_PW,
  PROP_PROXY_ID,
  PROP_PROXY_PW,
  PROP_COOKIES,
  PROP_IRADIO_MODE,
  PROP_TIMEOUT,
  PROP_EXTRA_HEADERS,
  PROP_SOUP_LOG_LEVEL,
  PROP_COMPRESS,
  PROP_KEEP_ALIVE,
  PROP_SSL_STRICT,
  PROP_SSL_CA_FILE,
  PROP_SSL_USE_SYSTEM_CA_FILE,
  PROP_TLS_DATABASE,
  PROP_RETRIES,
  PROP_METHOD,
  PROP_TLS_INTERACTION,
};

#define DEFAULT_USER_AGENT           "GStreamer souphttpsrc " PACKAGE_VERSION " "
#define DEFAULT_IRADIO_MODE          TRUE
#define DEFAULT_SOUP_LOG_LEVEL       SOUP_LOGGER_LOG_HEADERS
#define DEFAULT_COMPRESS             FALSE
#define DEFAULT_KEEP_ALIVE           TRUE
#define DEFAULT_SSL_STRICT           TRUE
#define DEFAULT_SSL_CA_FILE          NULL
#define DEFAULT_SSL_USE_SYSTEM_CA_FILE TRUE
#define DEFAULT_TLS_DATABASE         NULL
#define DEFAULT_TLS_INTERACTION      NULL
#define DEFAULT_TIMEOUT              15
#define DEFAULT_RETRIES              3
#define DEFAULT_SOUP_METHOD          NULL

#define GROW_BLOCKSIZE_LIMIT 1
#define GROW_BLOCKSIZE_COUNT 1
#define GROW_BLOCKSIZE_FACTOR 2
#define REDUCE_BLOCKSIZE_LIMIT 0.20
#define REDUCE_BLOCKSIZE_COUNT 2
#define REDUCE_BLOCKSIZE_FACTOR 0.5
#define GROW_TIME_LIMIT (1 * GST_SECOND)

static void gst_soup_http_src_uri_handler_init (gpointer g_iface,
    gpointer iface_data);
static void gst_soup_http_src_finalize (GObject * gobject);
static void gst_soup_http_src_dispose (GObject * gobject);

static void gst_soup_http_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_soup_http_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn gst_soup_http_src_change_state (GstElement *
    element, GstStateChange transition);
static void gst_soup_http_src_set_context (GstElement * element,
    GstContext * context);
static GstFlowReturn gst_soup_http_src_create (GstPushSrc * psrc,
    GstBuffer ** outbuf);
static gboolean gst_soup_http_src_start (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_stop (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_get_size (GstBaseSrc * bsrc, guint64 * size);
static gboolean gst_soup_http_src_is_seekable (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_do_seek (GstBaseSrc * bsrc,
    GstSegment * segment);
static gboolean gst_soup_http_src_query (GstBaseSrc * bsrc, GstQuery * query);
static gboolean gst_soup_http_src_unlock (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_unlock_stop (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_set_location (GstSoupHTTPSrc * src,
    const gchar * uri, GError ** error);
static gboolean gst_soup_http_src_set_proxy (GstSoupHTTPSrc * src,
    const gchar * uri);
static char *gst_soup_http_src_unicodify (const char *str);
static gboolean gst_soup_http_src_build_message (GstSoupHTTPSrc * src,
    const gchar * method);
static void gst_soup_http_src_cancel_message (GstSoupHTTPSrc * src);
static gboolean gst_soup_http_src_add_range_header (GstSoupHTTPSrc * src,
    guint64 offset, guint64 stop_offset);
static gboolean gst_soup_http_src_session_open (GstSoupHTTPSrc * src);
static void gst_soup_http_src_session_close (GstSoupHTTPSrc * src);
static GstFlowReturn gst_soup_http_src_parse_status (SoupMessage * msg,
    GstSoupHTTPSrc * src);
static GstFlowReturn gst_soup_http_src_got_headers (GstSoupHTTPSrc * src,
    SoupMessage * msg);
static void gst_soup_http_src_authenticate_cb (SoupSession * session,
    SoupMessage * msg, SoupAuth * auth, gboolean retrying,
    GstSoupHTTPSrc * src);

#define gst_soup_http_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstSoupHTTPSrc, gst_soup_http_src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_soup_http_src_uri_handler_init));

static void
gst_soup_http_src_class_init (GstSoupHTTPSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_soup_http_src_set_property;
  gobject_class->get_property = gst_soup_http_src_get_property;
  gobject_class->finalize = gst_soup_http_src_finalize;
  gobject_class->dispose = gst_soup_http_src_dispose;

  g_object_class_install_property (gobject_class,
      PROP_LOCATION,
      g_param_spec_string ("location", "Location",
          "Location to read from", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_USER_AGENT,
      g_param_spec_string ("user-agent", "User-Agent",
          "Value of the User-Agent HTTP request header field",
          DEFAULT_USER_AGENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_AUTOMATIC_REDIRECT,
      g_param_spec_boolean ("automatic-redirect", "automatic-redirect",
          "Automatically follow HTTP redirects (HTTP Status Code 3xx)",
          TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_PROXY,
      g_param_spec_string ("proxy", "Proxy",
          "HTTP proxy server URI", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_USER_ID,
      g_param_spec_string ("user-id", "user-id",
          "HTTP location URI user id for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_USER_PW,
      g_param_spec_string ("user-pw", "user-pw",
          "HTTP location URI user password for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROXY_ID,
      g_param_spec_string ("proxy-id", "proxy-id",
          "HTTP proxy URI user id for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROXY_PW,
      g_param_spec_string ("proxy-pw", "proxy-pw",
          "HTTP proxy URI user password for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_COOKIES,
      g_param_spec_boxed ("cookies", "Cookies", "HTTP request cookies",
          G_TYPE_STRV, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", "is-live", "Act like a live source",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TIMEOUT,
      g_param_spec_uint ("timeout", "timeout",
          "Value in seconds to timeout a blocking I/O (0 = No timeout).", 0,
          3600, DEFAULT_TIMEOUT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_EXTRA_HEADERS,
      g_param_spec_boxed ("extra-headers", "Extra Headers",
          "Extra headers to append to the HTTP request",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_IRADIO_MODE,
      g_param_spec_boolean ("iradio-mode", "iradio-mode",
          "Enable internet radio mode (ask server to send shoutcast/icecast "
          "metadata interleaved with the actual stream data)",
          DEFAULT_IRADIO_MODE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
   * GstSoupHTTPSrc::http-log-level:
   *
   * If set and > 0, captures and dumps HTTP session data as
   * log messages if log level >= GST_LEVEL_TRACE
   *
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_SOUP_LOG_LEVEL,
      g_param_spec_enum ("http-log-level", "HTTP log level",
          "Set log level for soup's HTTP session log",
          SOUP_TYPE_LOGGER_LOG_LEVEL, DEFAULT_SOUP_LOG_LEVEL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
   * GstSoupHTTPSrc::compress:
   *
   * If set to %TRUE, souphttpsrc will automatically handle gzip
   * and deflate Content-Encodings. This does not make much difference
   * and causes more load for normal media files, but makes a real
   * difference in size for plaintext files.
   *
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_COMPRESS,
      g_param_spec_boolean ("compress", "Compress",
          "Allow compressed content encodings",
          DEFAULT_COMPRESS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
   * GstSoupHTTPSrc::keep-alive:
   *
   * If set to %TRUE, souphttpsrc will keep alive connections when being
   * set to READY state and only will close connections when connecting
   * to a different server or when going to NULL state..
   *
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_KEEP_ALIVE,
      g_param_spec_boolean ("keep-alive", "keep-alive",
          "Use HTTP persistent connections", DEFAULT_KEEP_ALIVE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
   * GstSoupHTTPSrc::ssl-strict:
   *
   * If set to %TRUE, souphttpsrc will reject all SSL certificates that
   * are considered invalid.
   *
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_SSL_STRICT,
      g_param_spec_boolean ("ssl-strict", "SSL Strict",
          "Strict SSL certificate checking", DEFAULT_SSL_STRICT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
   * GstSoupHTTPSrc::ssl-ca-file:
   *
   * A SSL anchor CA file that should be used for checking certificates
   * instead of the system CA file.
   *
   * If this property is non-%NULL, #GstSoupHTTPSrc::ssl-use-system-ca-file
   * value will be ignored.
   *
   * Deprecated: Use #GstSoupHTTPSrc::tls-database property instead.
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_SSL_CA_FILE,
      g_param_spec_string ("ssl-ca-file", "SSL CA File",
          "Location of a SSL anchor CA file to use", DEFAULT_SSL_CA_FILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
   * GstSoupHTTPSrc::ssl-use-system-ca-file:
   *
   * If set to %TRUE, souphttpsrc will use the system's CA file for
   * checking certificates, unless #GstSoupHTTPSrc::ssl-ca-file or
   * #GstSoupHTTPSrc::tls-database are non-%NULL.
   *
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_SSL_USE_SYSTEM_CA_FILE,
      g_param_spec_boolean ("ssl-use-system-ca-file", "Use System CA File",
          "Use system CA file", DEFAULT_SSL_USE_SYSTEM_CA_FILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstSoupHTTPSrc::tls-database:
   *
   * TLS database with anchor certificate authorities used to validate
   * the server certificate.
   *
   * If this property is non-%NULL, #GstSoupHTTPSrc::ssl-use-system-ca-file
   * and #GstSoupHTTPSrc::ssl-ca-file values will be ignored.
   *
   * Since: 1.6
   */
  g_object_class_install_property (gobject_class, PROP_TLS_DATABASE,
      g_param_spec_object ("tls-database", "TLS database",
          "TLS database with anchor certificate authorities used to validate the server certificate",
          G_TYPE_TLS_DATABASE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstSoupHTTPSrc::tls-interaction:
   *
   * A #GTlsInteraction object to be used when the connection or certificate
   * database need to interact with the user. This will be used to prompt the
   * user for passwords or certificate where necessary.
   *
   * Since: 1.8
   */
  g_object_class_install_property (gobject_class, PROP_TLS_INTERACTION,
      g_param_spec_object ("tls-interaction", "TLS interaction",
          "A GTlsInteraction object to be used when the connection or certificate database need to interact with the user.",
          G_TYPE_TLS_INTERACTION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
   * GstSoupHTTPSrc::retries:
   *
   * Maximum number of retries until giving up.
   *
   * Since: 1.4
   */
  g_object_class_install_property (gobject_class, PROP_RETRIES,
      g_param_spec_int ("retries", "Retries",
          "Maximum number of retries until giving up (-1=infinite)", -1,
          G_MAXINT, DEFAULT_RETRIES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
   * GstSoupHTTPSrc::method
   *
   * The HTTP method to use when making a request
   *
   * Since: 1.6
   */
  g_object_class_install_property (gobject_class, PROP_METHOD,
      g_param_spec_string ("method", "HTTP method",
          "The HTTP method to use (GET, HEAD, OPTIONS, etc)",
          DEFAULT_SOUP_METHOD, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &srctemplate);

  gst_element_class_set_static_metadata (gstelement_class, "HTTP client source",
      "Source/Network",
      "Receive data as a client over the network via HTTP using SOUP",
      "Wouter Cloetens <wouter@mind.be>");
  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_soup_http_src_change_state);
  gstelement_class->set_context =
      GST_DEBUG_FUNCPTR (gst_soup_http_src_set_context);

  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_soup_http_src_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_soup_http_src_stop);
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_soup_http_src_unlock);
  gstbasesrc_class->unlock_stop =
      GST_DEBUG_FUNCPTR (gst_soup_http_src_unlock_stop);
  gstbasesrc_class->get_size = GST_DEBUG_FUNCPTR (gst_soup_http_src_get_size);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gst_soup_http_src_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_soup_http_src_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_soup_http_src_query);

  gstpushsrc_class->create = GST_DEBUG_FUNCPTR (gst_soup_http_src_create);

  GST_DEBUG_CATEGORY_INIT (souphttpsrc_debug, "souphttpsrc", 0,
      "SOUP HTTP src");
}

static void
gst_soup_http_src_reset (GstSoupHTTPSrc * src)
{
  src->retry_count = 0;
  src->have_size = FALSE;
  src->got_headers = FALSE;
  src->seekable = FALSE;
  src->read_position = 0;
  src->request_position = 0;
  src->stop_position = -1;
  src->content_size = 0;
  src->have_body = FALSE;

  src->reduce_blocksize_count = 0;
  src->increase_blocksize_count = 0;
  src->last_socket_read_time = 0;

  g_cancellable_reset (src->cancellable);
  g_mutex_lock (&src->mutex);
  if (src->input_stream) {
    g_object_unref (src->input_stream);
    src->input_stream = NULL;
  }
  g_mutex_unlock (&src->mutex);

  gst_caps_replace (&src->src_caps, NULL);
  g_free (src->iradio_name);
  src->iradio_name = NULL;
  g_free (src->iradio_genre);
  src->iradio_genre = NULL;
  g_free (src->iradio_url);
  src->iradio_url = NULL;
}

static void
gst_soup_http_src_init (GstSoupHTTPSrc * src)
{
  const gchar *proxy;

  g_mutex_init (&src->mutex);
  g_cond_init (&src->have_headers_cond);
  src->cancellable = g_cancellable_new ();
  src->location = NULL;
  src->redirection_uri = NULL;
  src->automatic_redirect = TRUE;
  src->user_agent = g_strdup (DEFAULT_USER_AGENT);
  src->user_id = NULL;
  src->user_pw = NULL;
  src->proxy_id = NULL;
  src->proxy_pw = NULL;
  src->cookies = NULL;
  src->iradio_mode = DEFAULT_IRADIO_MODE;
  src->session = NULL;
  src->external_session = NULL;
  src->forced_external_session = FALSE;
  src->msg = NULL;
  src->timeout = DEFAULT_TIMEOUT;
  src->log_level = DEFAULT_SOUP_LOG_LEVEL;
  src->compress = DEFAULT_COMPRESS;
  src->keep_alive = DEFAULT_KEEP_ALIVE;
  src->ssl_strict = DEFAULT_SSL_STRICT;
  src->ssl_use_system_ca_file = DEFAULT_SSL_USE_SYSTEM_CA_FILE;
  src->tls_database = DEFAULT_TLS_DATABASE;
  src->tls_interaction = DEFAULT_TLS_INTERACTION;
  src->max_retries = DEFAULT_RETRIES;
  src->method = DEFAULT_SOUP_METHOD;
  src->minimum_blocksize = gst_base_src_get_blocksize (GST_BASE_SRC_CAST (src));
  proxy = g_getenv ("http_proxy");
  if (!gst_soup_http_src_set_proxy (src, proxy)) {
    GST_WARNING_OBJECT (src,
        "The proxy in the http_proxy env var (\"%s\") cannot be parsed.",
        proxy);
  }

  gst_base_src_set_automatic_eos (GST_BASE_SRC (src), FALSE);

  gst_soup_http_src_reset (src);
}

static void
gst_soup_http_src_dispose (GObject * gobject)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (gobject);

  GST_DEBUG_OBJECT (src, "dispose");

  gst_soup_http_src_session_close (src);

  if (src->external_session) {
    g_object_unref (src->external_session);
    src->external_session = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (gobject);
}

static void
gst_soup_http_src_finalize (GObject * gobject)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (gobject);

  GST_DEBUG_OBJECT (src, "finalize");

  g_mutex_clear (&src->mutex);
  g_cond_clear (&src->have_headers_cond);
  g_object_unref (src->cancellable);
  g_free (src->location);
  g_free (src->redirection_uri);
  g_free (src->user_agent);
  if (src->proxy != NULL) {
    soup_uri_free (src->proxy);
  }
  g_free (src->user_id);
  g_free (src->user_pw);
  g_free (src->proxy_id);
  g_free (src->proxy_pw);
  g_strfreev (src->cookies);

  if (src->extra_headers) {
    gst_structure_free (src->extra_headers);
    src->extra_headers = NULL;
  }

  g_free (src->ssl_ca_file);

  if (src->tls_database)
    g_object_unref (src->tls_database);
  g_free (src->method);

  if (src->tls_interaction)
    g_object_unref (src->tls_interaction);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
gst_soup_http_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
    {
      const gchar *location;

      location = g_value_get_string (value);

      if (location == NULL) {
        GST_WARNING ("location property cannot be NULL");
        goto done;
      }
      if (!gst_soup_http_src_set_location (src, location, NULL)) {
        GST_WARNING ("badly formatted location");
        goto done;
      }
      break;
    }
    case PROP_USER_AGENT:
      g_free (src->user_agent);
      src->user_agent = g_value_dup_string (value);
      break;
    case PROP_IRADIO_MODE:
      src->iradio_mode = g_value_get_boolean (value);
      break;
    case PROP_AUTOMATIC_REDIRECT:
      src->automatic_redirect = g_value_get_boolean (value);
      break;
    case PROP_PROXY:
    {
      const gchar *proxy;

      proxy = g_value_get_string (value);
      if (!gst_soup_http_src_set_proxy (src, proxy)) {
        GST_WARNING ("badly formatted proxy URI");
        goto done;
      }
      break;
    }
    case PROP_COOKIES:
      g_strfreev (src->cookies);
      src->cookies = g_strdupv (g_value_get_boxed (value));
      break;
    case PROP_IS_LIVE:
      gst_base_src_set_live (GST_BASE_SRC (src), g_value_get_boolean (value));
      break;
    case PROP_USER_ID:
      g_free (src->user_id);
      src->user_id = g_value_dup_string (value);
      break;
    case PROP_USER_PW:
      g_free (src->user_pw);
      src->user_pw = g_value_dup_string (value);
      break;
    case PROP_PROXY_ID:
      g_free (src->proxy_id);
      src->proxy_id = g_value_dup_string (value);
      break;
    case PROP_PROXY_PW:
      g_free (src->proxy_pw);
      src->proxy_pw = g_value_dup_string (value);
      break;
    case PROP_TIMEOUT:
      src->timeout = g_value_get_uint (value);
      break;
    case PROP_EXTRA_HEADERS:{
      const GstStructure *s = gst_value_get_structure (value);

      if (src->extra_headers)
        gst_structure_free (src->extra_headers);

      src->extra_headers = s ? gst_structure_copy (s) : NULL;
      break;
    }
    case PROP_SOUP_LOG_LEVEL:
      src->log_level = g_value_get_enum (value);
      break;
    case PROP_COMPRESS:
      src->compress = g_value_get_boolean (value);
      break;
    case PROP_KEEP_ALIVE:
      src->keep_alive = g_value_get_boolean (value);
      break;
    case PROP_SSL_STRICT:
      src->ssl_strict = g_value_get_boolean (value);
      break;
    case PROP_SSL_CA_FILE:
      g_free (src->ssl_ca_file);
      src->ssl_ca_file = g_value_dup_string (value);
      break;
    case PROP_SSL_USE_SYSTEM_CA_FILE:
      src->ssl_use_system_ca_file = g_value_get_boolean (value);
      break;
    case PROP_TLS_DATABASE:
      g_clear_object (&src->tls_database);
      src->tls_database = g_value_dup_object (value);
      break;
    case PROP_TLS_INTERACTION:
      g_clear_object (&src->tls_interaction);
      src->tls_interaction = g_value_dup_object (value);
      break;
    case PROP_RETRIES:
      src->max_retries = g_value_get_int (value);
      break;
    case PROP_METHOD:
      g_free (src->method);
      src->method = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
done:
  return;
}

static void
gst_soup_http_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_set_string (value, src->location);
      break;
    case PROP_USER_AGENT:
      g_value_set_string (value, src->user_agent);
      break;
    case PROP_AUTOMATIC_REDIRECT:
      g_value_set_boolean (value, src->automatic_redirect);
      break;
    case PROP_PROXY:
      if (src->proxy == NULL)
        g_value_set_static_string (value, "");
      else {
        char *proxy = soup_uri_to_string (src->proxy, FALSE);

        g_value_set_string (value, proxy);
        g_free (proxy);
      }
      break;
    case PROP_COOKIES:
      g_value_set_boxed (value, g_strdupv (src->cookies));
      break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (src)));
      break;
    case PROP_IRADIO_MODE:
      g_value_set_boolean (value, src->iradio_mode);
      break;
    case PROP_USER_ID:
      g_value_set_string (value, src->user_id);
      break;
    case PROP_USER_PW:
      g_value_set_string (value, src->user_pw);
      break;
    case PROP_PROXY_ID:
      g_value_set_string (value, src->proxy_id);
      break;
    case PROP_PROXY_PW:
      g_value_set_string (value, src->proxy_pw);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint (value, src->timeout);
      break;
    case PROP_EXTRA_HEADERS:
      gst_value_set_structure (value, src->extra_headers);
      break;
    case PROP_SOUP_LOG_LEVEL:
      g_value_set_enum (value, src->log_level);
      break;
    case PROP_COMPRESS:
      g_value_set_boolean (value, src->compress);
      break;
    case PROP_KEEP_ALIVE:
      g_value_set_boolean (value, src->keep_alive);
      break;
    case PROP_SSL_STRICT:
      g_value_set_boolean (value, src->ssl_strict);
      break;
    case PROP_SSL_CA_FILE:
      g_value_set_string (value, src->ssl_ca_file);
      break;
    case PROP_SSL_USE_SYSTEM_CA_FILE:
      g_value_set_boolean (value, src->ssl_use_system_ca_file);
      break;
    case PROP_TLS_DATABASE:
      g_value_set_object (value, src->tls_database);
      break;
    case PROP_TLS_INTERACTION:
      g_value_set_object (value, src->tls_interaction);
      break;
    case PROP_RETRIES:
      g_value_set_int (value, src->max_retries);
      break;
    case PROP_METHOD:
      g_value_set_string (value, src->method);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gchar *
gst_soup_http_src_unicodify (const gchar * str)
{
  const gchar *env_vars[] = { "GST_ICY_TAG_ENCODING",
    "GST_TAG_ENCODING", NULL
  };

  return gst_tag_freeform_string_to_utf8 (str, -1, env_vars);
}

static void
gst_soup_http_src_cancel_message (GstSoupHTTPSrc * src)
{
  g_cancellable_cancel (src->cancellable);
  g_cond_signal (&src->have_headers_cond);
}

static gboolean
gst_soup_http_src_add_range_header (GstSoupHTTPSrc * src, guint64 offset,
    guint64 stop_offset)
{
  gchar buf[64];
  gint rc;

  soup_message_headers_remove (src->msg->request_headers, "Range");
  if (offset || stop_offset != -1) {
    if (stop_offset != -1) {
      g_assert (offset != stop_offset);

      rc = g_snprintf (buf, sizeof (buf), "bytes=%" G_GUINT64_FORMAT "-%"
          G_GUINT64_FORMAT, offset, (stop_offset > 0) ? stop_offset - 1 :
          stop_offset);
    } else {
      rc = g_snprintf (buf, sizeof (buf), "bytes=%" G_GUINT64_FORMAT "-",
          offset);
    }
    if (rc > sizeof (buf) || rc < 0)
      return FALSE;
    soup_message_headers_append (src->msg->request_headers, "Range", buf);
  }
  src->read_position = offset;
  return TRUE;
}

static gboolean
_append_extra_header (GQuark field_id, const GValue * value, gpointer user_data)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (user_data);
  const gchar *field_name = g_quark_to_string (field_id);
  gchar *field_content = NULL;

  if (G_VALUE_TYPE (value) == G_TYPE_STRING) {
    field_content = g_value_dup_string (value);
  } else {
    GValue dest = { 0, };

    g_value_init (&dest, G_TYPE_STRING);
    if (g_value_transform (value, &dest)) {
      field_content = g_value_dup_string (&dest);
    }
  }

  if (field_content == NULL) {
    GST_ERROR_OBJECT (src, "extra-headers field '%s' contains no value "
        "or can't be converted to a string", field_name);
    return FALSE;
  }

  GST_DEBUG_OBJECT (src, "Appending extra header: \"%s: %s\"", field_name,
      field_content);
  soup_message_headers_append (src->msg->request_headers, field_name,
      field_content);

  g_free (field_content);

  return TRUE;
}

static gboolean
_append_extra_headers (GQuark field_id, const GValue * value,
    gpointer user_data)
{
  if (G_VALUE_TYPE (value) == GST_TYPE_ARRAY) {
    guint n = gst_value_array_get_size (value);
    guint i;

    for (i = 0; i < n; i++) {
      const GValue *v = gst_value_array_get_value (value, i);

      if (!_append_extra_header (field_id, v, user_data))
        return FALSE;
    }
  } else if (G_VALUE_TYPE (value) == GST_TYPE_LIST) {
    guint n = gst_value_list_get_size (value);
    guint i;

    for (i = 0; i < n; i++) {
      const GValue *v = gst_value_list_get_value (value, i);

      if (!_append_extra_header (field_id, v, user_data))
        return FALSE;
    }
  } else {
    return _append_extra_header (field_id, value, user_data);
  }

  return TRUE;
}


static gboolean
gst_soup_http_src_add_extra_headers (GstSoupHTTPSrc * src)
{
  if (!src->extra_headers)
    return TRUE;

  return gst_structure_foreach (src->extra_headers, _append_extra_headers, src);
}

static gboolean
gst_soup_http_src_session_open (GstSoupHTTPSrc * src)
{
  if (src->session) {
    GST_DEBUG_OBJECT (src, "Session is already open");
    return TRUE;
  }

  if (!src->location) {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (_("No URL set.")),
        ("Missing location property"));
    return FALSE;
  }

  if (!src->session) {
    GstQuery *query;
    gboolean can_share = (src->timeout == DEFAULT_TIMEOUT)
        && (src->ssl_strict == DEFAULT_SSL_STRICT)
        && (src->tls_interaction == NULL) && (src->proxy == NULL)
        && (src->tls_database == DEFAULT_TLS_DATABASE)
        && (src->ssl_ca_file == DEFAULT_SSL_CA_FILE)
        && (src->ssl_use_system_ca_file == DEFAULT_SSL_USE_SYSTEM_CA_FILE);

    query = gst_query_new_context (GST_SOUP_SESSION_CONTEXT);
    if (gst_pad_peer_query (GST_BASE_SRC_PAD (src), query)) {
      GstContext *context;

      gst_query_parse_context (query, &context);
      gst_element_set_context (GST_ELEMENT_CAST (src), context);
    } else {
      GstMessage *message;

      message =
          gst_message_new_need_context (GST_OBJECT_CAST (src),
          GST_SOUP_SESSION_CONTEXT);
      gst_element_post_message (GST_ELEMENT_CAST (src), message);
    }
    gst_query_unref (query);

    GST_OBJECT_LOCK (src);
    if (src->external_session && (can_share || src->forced_external_session)) {
      GST_DEBUG_OBJECT (src, "Using external session %p",
          src->external_session);
      src->session = g_object_ref (src->external_session);
      src->session_is_shared = TRUE;
    } else {
      GST_DEBUG_OBJECT (src, "Creating session (can share %d)", can_share);

      /* We explicitly set User-Agent to NULL here and overwrite it per message
       * to be able to have the same session with different User-Agents per
       * source */
      if (src->proxy == NULL) {
        src->session =
            soup_session_new_with_options (SOUP_SESSION_USER_AGENT,
            NULL, SOUP_SESSION_TIMEOUT, src->timeout,
            SOUP_SESSION_SSL_STRICT, src->ssl_strict,
            SOUP_SESSION_TLS_INTERACTION, src->tls_interaction, NULL);
      } else {
        src->session =
            soup_session_new_with_options (SOUP_SESSION_PROXY_URI, src->proxy,
            SOUP_SESSION_TIMEOUT, src->timeout,
            SOUP_SESSION_SSL_STRICT, src->ssl_strict,
            SOUP_SESSION_USER_AGENT, NULL,
            SOUP_SESSION_TLS_INTERACTION, src->tls_interaction, NULL);
      }

      if (src->session) {
        gst_soup_util_log_setup (src->session, src->log_level,
            GST_ELEMENT (src));
        soup_session_add_feature_by_type (src->session,
            SOUP_TYPE_CONTENT_DECODER);
        soup_session_add_feature_by_type (src->session, SOUP_TYPE_COOKIE_JAR);

        if (can_share) {
          GstContext *context;
          GstMessage *message;
          GstStructure *s;

          GST_DEBUG_OBJECT (src, "Sharing session %p", src->session);
          src->session_is_shared = TRUE;

          /* Unset the limit the number of maximum allowed connection */
          g_object_set (src->session, SOUP_SESSION_MAX_CONNS, G_MAXINT,
              SOUP_SESSION_MAX_CONNS_PER_HOST, G_MAXINT, NULL);

          context = gst_context_new (GST_SOUP_SESSION_CONTEXT, TRUE);
          s = gst_context_writable_structure (context);
          gst_structure_set (s, "session", SOUP_TYPE_SESSION, src->session,
              "force", G_TYPE_BOOLEAN, FALSE, NULL);

          gst_object_ref (src->session);
          GST_OBJECT_UNLOCK (src);
          gst_element_set_context (GST_ELEMENT_CAST (src), context);
          message =
              gst_message_new_have_context (GST_OBJECT_CAST (src), context);
          gst_element_post_message (GST_ELEMENT_CAST (src), message);
          GST_OBJECT_LOCK (src);
          gst_object_unref (src->session);
        } else {
          src->session_is_shared = FALSE;
        }
      }
    }

    if (!src->session) {
      GST_ELEMENT_ERROR (src, LIBRARY, INIT,
          (NULL), ("Failed to create session"));
      GST_OBJECT_UNLOCK (src);
      return FALSE;
    }

    g_signal_connect (src->session, "authenticate",
        G_CALLBACK (gst_soup_http_src_authenticate_cb), src);

    if (!src->session_is_shared) {
      if (src->tls_database)
        g_object_set (src->session, "tls-database", src->tls_database, NULL);
      else if (src->ssl_ca_file)
        g_object_set (src->session, "ssl-ca-file", src->ssl_ca_file, NULL);
      else
        g_object_set (src->session, "ssl-use-system-ca-file",
            src->ssl_use_system_ca_file, NULL);
    }
    GST_OBJECT_UNLOCK (src);
  } else {
    GST_DEBUG_OBJECT (src, "Re-using session");
  }

  return TRUE;
}

static void
gst_soup_http_src_session_close (GstSoupHTTPSrc * src)
{
  GST_DEBUG_OBJECT (src, "Closing session");

  g_mutex_lock (&src->mutex);
  if (src->msg) {
    soup_session_cancel_message (src->session, src->msg, SOUP_STATUS_CANCELLED);
    g_object_unref (src->msg);
    src->msg = NULL;
  }

  if (src->session) {
    if (!src->session_is_shared)
      soup_session_abort (src->session);
    g_signal_handlers_disconnect_by_func (src->session,
        G_CALLBACK (gst_soup_http_src_authenticate_cb), src);
    g_object_unref (src->session);
    src->session = NULL;
  }

  g_mutex_unlock (&src->mutex);
}

static void
gst_soup_http_src_authenticate_cb (SoupSession * session, SoupMessage * msg,
    SoupAuth * auth, gboolean retrying, GstSoupHTTPSrc * src)
{
  /* Might be from another user of the shared session */
  if (!GST_IS_SOUP_HTTP_SRC (src) || msg != src->msg)
    return;

  if (!retrying) {
    /* First time authentication only, if we fail and are called again with retry true fall through */
    if (msg->status_code == SOUP_STATUS_UNAUTHORIZED) {
      if (src->user_id && src->user_pw)
        soup_auth_authenticate (auth, src->user_id, src->user_pw);
    } else if (msg->status_code == SOUP_STATUS_PROXY_AUTHENTICATION_REQUIRED) {
      if (src->proxy_id && src->proxy_pw)
        soup_auth_authenticate (auth, src->proxy_id, src->proxy_pw);
    }
  }
}

static void
insert_http_header (const gchar * name, const gchar * value, gpointer user_data)
{
  GstStructure *headers = user_data;
  const GValue *gv;

  if (!g_utf8_validate (name, -1, NULL) || !g_utf8_validate (value, -1, NULL))
    return;

  gv = gst_structure_get_value (headers, name);
  if (gv && GST_VALUE_HOLDS_ARRAY (gv)) {
    GValue v = G_VALUE_INIT;

    g_value_init (&v, G_TYPE_STRING);
    g_value_set_string (&v, value);
    gst_value_array_append_value ((GValue *) gv, &v);
    g_value_unset (&v);
  } else if (gv && G_VALUE_HOLDS_STRING (gv)) {
    GValue arr = G_VALUE_INIT;
    GValue v = G_VALUE_INIT;
    const gchar *old_value = g_value_get_string (gv);

    g_value_init (&arr, GST_TYPE_ARRAY);
    g_value_init (&v, G_TYPE_STRING);
    g_value_set_string (&v, old_value);
    gst_value_array_append_value (&arr, &v);
    g_value_set_string (&v, value);
    gst_value_array_append_value (&arr, &v);

    gst_structure_set_value (headers, name, &arr);
    g_value_unset (&v);
    g_value_unset (&arr);
  } else {
    gst_structure_set (headers, name, G_TYPE_STRING, value, NULL);
  }
}

static GstFlowReturn
gst_soup_http_src_got_headers (GstSoupHTTPSrc * src, SoupMessage * msg)
{
  const char *value;
  GstTagList *tag_list;
  GstBaseSrc *basesrc;
  guint64 newsize;
  GHashTable *params = NULL;
  GstEvent *http_headers_event;
  GstStructure *http_headers, *headers;
  const gchar *accept_ranges;

  GST_INFO_OBJECT (src, "got headers");

  if (msg->status_code == SOUP_STATUS_PROXY_AUTHENTICATION_REQUIRED &&
      src->proxy_id && src->proxy_pw) {
    /* wait for authenticate callback */
    return GST_FLOW_OK;
  }

  http_headers = gst_structure_new_empty ("http-headers");
  gst_structure_set (http_headers, "uri", G_TYPE_STRING, src->location,
      "http-status-code", G_TYPE_UINT, msg->status_code, NULL);
  if (src->redirection_uri)
    gst_structure_set (http_headers, "redirection-uri", G_TYPE_STRING,
        src->redirection_uri, NULL);
  headers = gst_structure_new_empty ("request-headers");
  soup_message_headers_foreach (msg->request_headers, insert_http_header,
      headers);
  gst_structure_set (http_headers, "request-headers", GST_TYPE_STRUCTURE,
      headers, NULL);
  gst_structure_free (headers);
  headers = gst_structure_new_empty ("response-headers");
  soup_message_headers_foreach (msg->response_headers, insert_http_header,
      headers);
  gst_structure_set (http_headers, "response-headers", GST_TYPE_STRUCTURE,
      headers, NULL);
  gst_structure_free (headers);

  gst_element_post_message (GST_ELEMENT_CAST (src),
      gst_message_new_element (GST_OBJECT_CAST (src),
          gst_structure_copy (http_headers)));

  if (msg->status_code == SOUP_STATUS_UNAUTHORIZED) {
    /* force an error */
    gst_structure_free (http_headers);
    return gst_soup_http_src_parse_status (msg, src);
  }

  src->got_headers = TRUE;
  g_cond_broadcast (&src->have_headers_cond);

  http_headers_event =
      gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM_STICKY, http_headers);
  gst_event_replace (&src->http_headers_event, http_headers_event);
  gst_event_unref (http_headers_event);

  /* Parse Content-Length. */
  if (soup_message_headers_get_encoding (msg->response_headers) ==
      SOUP_ENCODING_CONTENT_LENGTH) {
    newsize = src->request_position +
        soup_message_headers_get_content_length (msg->response_headers);
    if (!src->have_size || (src->content_size != newsize)) {
      src->content_size = newsize;
      src->have_size = TRUE;
      src->seekable = TRUE;
      GST_DEBUG_OBJECT (src, "size = %" G_GUINT64_FORMAT, src->content_size);

      basesrc = GST_BASE_SRC_CAST (src);
      basesrc->segment.duration = src->content_size;
      gst_element_post_message (GST_ELEMENT (src),
          gst_message_new_duration_changed (GST_OBJECT (src)));
    }
  }

  /* If the server reports Accept-Ranges: none we don't have to try
   * doing range requests at all
   */
  if ((accept_ranges =
          soup_message_headers_get_one (msg->response_headers,
              "Accept-Ranges"))) {
    if (g_ascii_strcasecmp (accept_ranges, "none") == 0)
      src->seekable = FALSE;
  }

  /* Icecast stuff */
  tag_list = gst_tag_list_new_empty ();

  if ((value =
          soup_message_headers_get_one (msg->response_headers,
              "icy-metaint")) != NULL) {
    gint icy_metaint;

    if (g_utf8_validate (value, -1, NULL)) {
      icy_metaint = atoi (value);

      GST_DEBUG_OBJECT (src, "icy-metaint: %s (parsed: %d)", value,
          icy_metaint);
      if (icy_metaint > 0) {
        if (src->src_caps)
          gst_caps_unref (src->src_caps);

        src->src_caps = gst_caps_new_simple ("application/x-icy",
            "metadata-interval", G_TYPE_INT, icy_metaint, NULL);

        gst_base_src_set_caps (GST_BASE_SRC (src), src->src_caps);
      }
    }
  }
  if ((value =
          soup_message_headers_get_content_type (msg->response_headers,
              &params)) != NULL) {
    if (!g_utf8_validate (value, -1, NULL)) {
      GST_WARNING_OBJECT (src, "Content-Type is invalid UTF-8");
    } else if (g_ascii_strcasecmp (value, "audio/L16") == 0) {
      gint channels = 2;
      gint rate = 44100;
      char *param;

      GST_DEBUG_OBJECT (src, "Content-Type: %s", value);

      if (src->src_caps) {
        gst_caps_unref (src->src_caps);
        src->src_caps = NULL;
      }

      param = g_hash_table_lookup (params, "channels");
      if (param != NULL) {
        guint64 val = g_ascii_strtoull (param, NULL, 10);
        if (val < 64)
          channels = val;
        else
          channels = 0;
      }

      param = g_hash_table_lookup (params, "rate");
      if (param != NULL) {
        guint64 val = g_ascii_strtoull (param, NULL, 10);
        if (val < G_MAXINT)
          rate = val;
        else
          rate = 0;
      }

      if (rate > 0 && channels > 0) {
        src->src_caps = gst_caps_new_simple ("audio/x-unaligned-raw",
            "format", G_TYPE_STRING, "S16BE",
            "layout", G_TYPE_STRING, "interleaved",
            "channels", G_TYPE_INT, channels, "rate", G_TYPE_INT, rate, NULL);

        gst_base_src_set_caps (GST_BASE_SRC (src), src->src_caps);
      }
    } else {
      GST_DEBUG_OBJECT (src, "Content-Type: %s", value);

      /* Set the Content-Type field on the caps */
      if (src->src_caps) {
        src->src_caps = gst_caps_make_writable (src->src_caps);
        gst_caps_set_simple (src->src_caps, "content-type", G_TYPE_STRING,
            value, NULL);
        gst_base_src_set_caps (GST_BASE_SRC (src), src->src_caps);
      }
    }
  }

  if (params != NULL)
    g_hash_table_destroy (params);

  if ((value =
          soup_message_headers_get_one (msg->response_headers,
              "icy-name")) != NULL) {
    if (g_utf8_validate (value, -1, NULL)) {
      g_free (src->iradio_name);
      src->iradio_name = gst_soup_http_src_unicodify (value);
      if (src->iradio_name) {
        gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE, GST_TAG_ORGANIZATION,
            src->iradio_name, NULL);
      }
    }
  }
  if ((value =
          soup_message_headers_get_one (msg->response_headers,
              "icy-genre")) != NULL) {
    if (g_utf8_validate (value, -1, NULL)) {
      g_free (src->iradio_genre);
      src->iradio_genre = gst_soup_http_src_unicodify (value);
      if (src->iradio_genre) {
        gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE, GST_TAG_GENRE,
            src->iradio_genre, NULL);
      }
    }
  }
  if ((value = soup_message_headers_get_one (msg->response_headers, "icy-url"))
      != NULL) {
    if (g_utf8_validate (value, -1, NULL)) {
      g_free (src->iradio_url);
      src->iradio_url = gst_soup_http_src_unicodify (value);
      if (src->iradio_url) {
        gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE, GST_TAG_LOCATION,
            src->iradio_url, NULL);
      }
    }
  }
  if (!gst_tag_list_is_empty (tag_list)) {
    GST_DEBUG_OBJECT (src,
        "calling gst_element_found_tags with %" GST_PTR_FORMAT, tag_list);
    gst_pad_push_event (GST_BASE_SRC_PAD (src), gst_event_new_tag (tag_list));
  } else {
    gst_tag_list_unref (tag_list);
  }

  /* Handle HTTP errors. */
  return gst_soup_http_src_parse_status (msg, src);
}

static GstBuffer *
gst_soup_http_src_alloc_buffer (GstSoupHTTPSrc * src)
{
  GstBaseSrc *basesrc = GST_BASE_SRC_CAST (src);
  GstFlowReturn rc;
  GstBuffer *gstbuf;

  rc = GST_BASE_SRC_CLASS (parent_class)->alloc (basesrc, -1,
      basesrc->blocksize, &gstbuf);
  if (G_UNLIKELY (rc != GST_FLOW_OK)) {
    return NULL;
  }

  return gstbuf;
}

#define SOUP_HTTP_SRC_ERROR(src,soup_msg,cat,code,error_message)     \
  do { \
    GST_ELEMENT_ERROR_WITH_DETAILS ((src), cat, code, ("%s", error_message), \
        ("%s (%d), URL: %s, Redirect to: %s", (soup_msg)->reason_phrase, \
            (soup_msg)->status_code, (src)->location, GST_STR_NULL ((src)->redirection_uri)), \
            ("http-status-code", G_TYPE_UINT, (soup_msg)->status_code, \
             "http-redirect-uri", G_TYPE_STRING, GST_STR_NULL ((src)->redirection_uri), NULL)); \
  } while(0)

static GstFlowReturn
gst_soup_http_src_parse_status (SoupMessage * msg, GstSoupHTTPSrc * src)
{
  if (msg->method == SOUP_METHOD_HEAD) {
    if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
      GST_DEBUG_OBJECT (src, "Ignoring error %d during HEAD request",
          msg->status_code);
    return GST_FLOW_OK;
  }

  if (SOUP_STATUS_IS_TRANSPORT_ERROR (msg->status_code)) {
    switch (msg->status_code) {
      case SOUP_STATUS_CANT_RESOLVE:
      case SOUP_STATUS_CANT_RESOLVE_PROXY:
        SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, NOT_FOUND,
            _("Could not resolve server name."));
        return GST_FLOW_ERROR;
      case SOUP_STATUS_CANT_CONNECT:
      case SOUP_STATUS_CANT_CONNECT_PROXY:
        SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, OPEN_READ,
            _("Could not establish connection to server."));
        return GST_FLOW_ERROR;
      case SOUP_STATUS_SSL_FAILED:
        SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, OPEN_READ,
            _("Secure connection setup failed."));
        return GST_FLOW_ERROR;
      case SOUP_STATUS_IO_ERROR:
        if (src->max_retries == -1 || src->retry_count < src->max_retries)
          return GST_FLOW_CUSTOM_ERROR;
        SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, READ,
            _("A network error occurred, or the server closed the connection "
                "unexpectedly."));
        return GST_FLOW_ERROR;
      case SOUP_STATUS_MALFORMED:
        SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, READ,
            _("Server sent bad data."));
        return GST_FLOW_ERROR;
      case SOUP_STATUS_CANCELLED:
        /* No error message when interrupted by program. */
        break;
    }
    return GST_FLOW_OK;
  }

  if (SOUP_STATUS_IS_CLIENT_ERROR (msg->status_code) ||
      SOUP_STATUS_IS_REDIRECTION (msg->status_code) ||
      SOUP_STATUS_IS_SERVER_ERROR (msg->status_code)) {
    const gchar *reason_phrase;

    reason_phrase = msg->reason_phrase;
    if (reason_phrase && !g_utf8_validate (reason_phrase, -1, NULL)) {
      GST_ERROR_OBJECT (src, "Invalid UTF-8 in reason");
      reason_phrase = "(invalid)";
    }

    /* Report HTTP error. */

    /* when content_size is unknown and we have just finished receiving
     * a body message, requests that go beyond the content limits will result
     * in an error. Here we convert those to EOS */
    if (msg->status_code == SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE &&
        src->have_body && !src->have_size) {
      GST_DEBUG_OBJECT (src, "Requested range out of limits and received full "
          "body, returning EOS");
      return GST_FLOW_EOS;
    }

    /* FIXME: reason_phrase is not translated and not suitable for user
     * error dialog according to libsoup documentation.
     */
    if (msg->status_code == SOUP_STATUS_NOT_FOUND) {
      SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, NOT_FOUND, (reason_phrase));
    } else if (msg->status_code == SOUP_STATUS_UNAUTHORIZED
        || msg->status_code == SOUP_STATUS_PAYMENT_REQUIRED
        || msg->status_code == SOUP_STATUS_FORBIDDEN
        || msg->status_code == SOUP_STATUS_PROXY_AUTHENTICATION_REQUIRED) {
      SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, NOT_AUTHORIZED, (reason_phrase));
    } else {
      SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, OPEN_READ, (reason_phrase));
    }
    return GST_FLOW_ERROR;
  }

  return GST_FLOW_OK;
}

static void
gst_soup_http_src_restarted_cb (SoupMessage * msg, GstSoupHTTPSrc * src)
{
  if (soup_session_would_redirect (src->session, msg)) {
    src->redirection_uri =
        soup_uri_to_string (soup_message_get_uri (msg), FALSE);
    src->redirection_permanent =
        (msg->status_code == SOUP_STATUS_MOVED_PERMANENTLY);
    GST_DEBUG_OBJECT (src, "%u redirect to \"%s\" (permanent %d)",
        msg->status_code, src->redirection_uri, src->redirection_permanent);
  }
}

static gboolean
gst_soup_http_src_build_message (GstSoupHTTPSrc * src, const gchar * method)
{
  g_return_val_if_fail (src->msg == NULL, FALSE);

  src->msg = soup_message_new (method, src->location);
  if (!src->msg) {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ,
        ("Error parsing URL."), ("URL: %s", src->location));
    return FALSE;
  }

  /* Duplicating the defaults of libsoup here. We don't want to set a
   * User-Agent in the session as each source might have its own User-Agent
   * set */
  if (!src->user_agent || !*src->user_agent) {
    gchar *user_agent =
        g_strdup_printf ("libsoup/%u.%u.%u", soup_get_major_version (),
        soup_get_minor_version (), soup_get_micro_version ());
    soup_message_headers_append (src->msg->request_headers, "User-Agent",
        user_agent);
    g_free (user_agent);
  } else if (g_str_has_suffix (src->user_agent, " ")) {
    gchar *user_agent = g_strdup_printf ("%slibsoup/%u.%u.%u", src->user_agent,
        soup_get_major_version (),
        soup_get_minor_version (), soup_get_micro_version ());
    soup_message_headers_append (src->msg->request_headers, "User-Agent",
        user_agent);
    g_free (user_agent);
  } else {
    soup_message_headers_append (src->msg->request_headers, "User-Agent",
        src->user_agent);
  }

  if (!src->keep_alive) {
    soup_message_headers_append (src->msg->request_headers, "Connection",
        "close");
  }
  if (src->iradio_mode) {
    soup_message_headers_append (src->msg->request_headers, "icy-metadata",
        "1");
  }
  if (src->cookies) {
    gchar **cookie;

    for (cookie = src->cookies; *cookie != NULL; cookie++) {
      soup_message_headers_append (src->msg->request_headers, "Cookie",
          *cookie);
    }
  }

  if (!src->compress)
    soup_message_disable_feature (src->msg, SOUP_TYPE_CONTENT_DECODER);

  soup_message_set_flags (src->msg, SOUP_MESSAGE_OVERWRITE_CHUNKS |
      (src->automatic_redirect ? 0 : SOUP_MESSAGE_NO_REDIRECT));

  if (src->automatic_redirect) {
    g_signal_connect (src->msg, "restarted",
        G_CALLBACK (gst_soup_http_src_restarted_cb), src);
  }

  gst_soup_http_src_add_range_header (src, src->request_position,
      src->stop_position);

  gst_soup_http_src_add_extra_headers (src);

  return TRUE;
}

/* Lock taken */
static GstFlowReturn
gst_soup_http_src_send_message (GstSoupHTTPSrc * src)
{
  GstFlowReturn ret;
  GError *error = NULL;

  g_return_val_if_fail (src->msg != NULL, GST_FLOW_ERROR);
  g_assert (src->input_stream == NULL);

  src->input_stream =
      soup_session_send (src->session, src->msg, src->cancellable, &error);

  if (error)
    GST_DEBUG_OBJECT (src, "Sending message failed: %s", error->message);

  if (g_cancellable_is_cancelled (src->cancellable)) {
    ret = GST_FLOW_FLUSHING;
    goto done;
  }

  ret = gst_soup_http_src_got_headers (src, src->msg);
  if (ret != GST_FLOW_OK) {
    goto done;
  }

  if (!src->input_stream) {
    GST_DEBUG_OBJECT (src, "Didn't get an input stream: %s", error->message);
    ret = GST_FLOW_ERROR;
    goto done;
  }

  if (SOUP_STATUS_IS_SUCCESSFUL (src->msg->status_code)) {
    GST_DEBUG_OBJECT (src, "Successfully got a reply");
  } else {
    /* FIXME - be more helpful to people debugging */
    ret = GST_FLOW_ERROR;
  }

done:
  if (error)
    g_error_free (error);
  return ret;
}

static GstFlowReturn
gst_soup_http_src_do_request (GstSoupHTTPSrc * src, const gchar * method)
{
  GstFlowReturn ret;

  if (src->max_retries != -1 && src->retry_count > src->max_retries) {
    GST_DEBUG_OBJECT (src, "Max retries reached");
    return GST_FLOW_ERROR;
  }

  src->retry_count++;
  /* EOS immediately if we have an empty segment */
  if (src->request_position == src->stop_position)
    return GST_FLOW_EOS;

  GST_LOG_OBJECT (src, "Running request for method: %s", method);

  /* Update the position if we are retrying */
  if (src->msg && src->request_position > 0) {
    gst_soup_http_src_add_range_header (src, src->request_position,
        src->stop_position);
  } else if (src->msg && src->request_position == 0)
    soup_message_headers_remove (src->msg->request_headers, "Range");

  /* add_range_header() has the side effect of setting read_position to
   * the requested position. This *needs* to be set regardless of having
   * a message or not. Failure to do so would result in calculation being
   * done with stale/wrong read position */
  src->read_position = src->request_position;

  if (!src->msg) {
    if (!gst_soup_http_src_build_message (src, method)) {
      return GST_FLOW_ERROR;
    }
  }

  if (g_cancellable_is_cancelled (src->cancellable)) {
    GST_INFO_OBJECT (src, "interrupted");
    return GST_FLOW_FLUSHING;
  }

  ret = gst_soup_http_src_send_message (src);

  /* Check if Range header was respected. */
  if (ret == GST_FLOW_OK && src->request_position > 0 &&
      src->msg->status_code != SOUP_STATUS_PARTIAL_CONTENT) {
    src->seekable = FALSE;
    GST_ELEMENT_ERROR_WITH_DETAILS (src, RESOURCE, SEEK,
        (_("Server does not support seeking.")),
        ("Server does not accept Range HTTP header, URL: %s, Redirect to: %s",
            src->location, GST_STR_NULL (src->redirection_uri)),
        ("http-status-code", G_TYPE_UINT, src->msg->status_code,
            "http-redirection-uri", G_TYPE_STRING,
            GST_STR_NULL (src->redirection_uri), NULL));
    ret = GST_FLOW_ERROR;
  }

  return ret;
}

/*
 * Check if the bytes_read is above a certain threshold of the blocksize, if
 * that happens a few times in a row, increase the blocksize; Do the same in
 * the opposite direction to reduce the blocksize.
 */
static void
gst_soup_http_src_check_update_blocksize (GstSoupHTTPSrc * src,
    gint64 bytes_read)
{
  guint blocksize = gst_base_src_get_blocksize (GST_BASE_SRC_CAST (src));

  gint64 time_since_last_read =
      g_get_monotonic_time () * GST_USECOND - src->last_socket_read_time;

  GST_LOG_OBJECT (src, "Checking to update blocksize. Read: %" G_GINT64_FORMAT
      " bytes, blocksize: %u bytes, time since last read: %" GST_TIME_FORMAT,
      bytes_read, blocksize, GST_TIME_ARGS (time_since_last_read));

  if (bytes_read >= blocksize * GROW_BLOCKSIZE_LIMIT
      && time_since_last_read <= GROW_TIME_LIMIT) {
    src->reduce_blocksize_count = 0;
    src->increase_blocksize_count++;

    if (src->increase_blocksize_count >= GROW_BLOCKSIZE_COUNT) {
      blocksize *= GROW_BLOCKSIZE_FACTOR;
      GST_DEBUG_OBJECT (src, "Increased blocksize to %u", blocksize);
      gst_base_src_set_blocksize (GST_BASE_SRC_CAST (src), blocksize);
      src->increase_blocksize_count = 0;
    }
  } else if (bytes_read < blocksize * REDUCE_BLOCKSIZE_LIMIT
      || time_since_last_read > GROW_TIME_LIMIT) {
    src->reduce_blocksize_count++;
    src->increase_blocksize_count = 0;

    if (src->reduce_blocksize_count >= REDUCE_BLOCKSIZE_COUNT) {
      blocksize *= REDUCE_BLOCKSIZE_FACTOR;
      blocksize = MAX (blocksize, src->minimum_blocksize);
      GST_DEBUG_OBJECT (src, "Decreased blocksize to %u", blocksize);
      gst_base_src_set_blocksize (GST_BASE_SRC_CAST (src), blocksize);
      src->reduce_blocksize_count = 0;
    }
  } else {
    src->reduce_blocksize_count = src->increase_blocksize_count = 0;
  }
}

static void
gst_soup_http_src_update_position (GstSoupHTTPSrc * src, gint64 bytes_read)
{
  GstBaseSrc *basesrc = GST_BASE_SRC_CAST (src);
  guint64 new_position;

  new_position = src->read_position + bytes_read;
  if (G_LIKELY (src->request_position == src->read_position))
    src->request_position = new_position;
  src->read_position = new_position;

  if (src->have_size) {
    if (new_position > src->content_size) {
      GST_DEBUG_OBJECT (src, "Got position previous estimated content size "
          "(%" G_GINT64_FORMAT " > %" G_GINT64_FORMAT ")", new_position,
          src->content_size);
      src->content_size = new_position;
      basesrc->segment.duration = src->content_size;
      gst_element_post_message (GST_ELEMENT (src),
          gst_message_new_duration_changed (GST_OBJECT (src)));
    } else if (new_position == src->content_size) {
      GST_DEBUG_OBJECT (src, "We're EOS now");
    }
  }
}

static GstFlowReturn
gst_soup_http_src_read_buffer (GstSoupHTTPSrc * src, GstBuffer ** outbuf)
{
  gssize read_bytes;
  GstMapInfo mapinfo;
  GstBaseSrc *bsrc;
  GstFlowReturn ret;

  bsrc = GST_BASE_SRC_CAST (src);

  *outbuf = gst_soup_http_src_alloc_buffer (src);
  if (!*outbuf) {
    GST_WARNING_OBJECT (src, "Failed to allocate buffer");
    return GST_FLOW_ERROR;
  }

  if (!gst_buffer_map (*outbuf, &mapinfo, GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (src, "Failed to map buffer");
    return GST_FLOW_ERROR;
  }

  read_bytes =
      g_input_stream_read (src->input_stream, mapinfo.data, mapinfo.size,
      src->cancellable, NULL);
  GST_DEBUG_OBJECT (src, "Read %" G_GSSIZE_FORMAT " bytes from http input",
      read_bytes);

  g_mutex_lock (&src->mutex);
  if (g_cancellable_is_cancelled (src->cancellable)) {
    gst_buffer_unmap (*outbuf, &mapinfo);
    gst_buffer_unref (*outbuf);
    g_mutex_unlock (&src->mutex);
    return GST_FLOW_FLUSHING;
  }

  gst_buffer_unmap (*outbuf, &mapinfo);
  if (read_bytes > 0) {
    gst_buffer_set_size (*outbuf, read_bytes);
    GST_BUFFER_OFFSET (*outbuf) = bsrc->segment.position;
    ret = GST_FLOW_OK;
    gst_soup_http_src_update_position (src, read_bytes);

    /* Got some data, reset retry counter */
    src->retry_count = 0;

    gst_soup_http_src_check_update_blocksize (src, read_bytes);

    src->last_socket_read_time = g_get_monotonic_time () * GST_USECOND;

    /* If we're at the end of a range request, read again to let libsoup
     * finalize the request. This allows to reuse the connection again later,
     * otherwise we would have to cancel the message and close the connection
     */
    if (bsrc->segment.stop != -1
        && bsrc->segment.position + read_bytes >= bsrc->segment.stop) {
      guint8 tmp[128];

      g_object_unref (src->msg);
      src->msg = NULL;
      src->have_body = TRUE;

      /* This should return immediately as we're at the end of the range */
      read_bytes =
          g_input_stream_read (src->input_stream, tmp, sizeof (tmp),
          src->cancellable, NULL);
      if (read_bytes > 0)
        GST_ERROR_OBJECT (src,
            "Read %" G_GSIZE_FORMAT " bytes after end of range", read_bytes);
    }
  } else {
    gst_buffer_unref (*outbuf);
    if (read_bytes < 0 ||
        (src->have_size && src->read_position < src->content_size)) {
      /* Maybe the server disconnected, retry */
      ret = GST_FLOW_CUSTOM_ERROR;
    } else {
      g_object_unref (src->msg);
      src->msg = NULL;
      ret = GST_FLOW_EOS;
      src->have_body = TRUE;
    }
  }
  g_mutex_unlock (&src->mutex);

  return ret;
}

static GstFlowReturn
gst_soup_http_src_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  GstSoupHTTPSrc *src;
  GstFlowReturn ret = GST_FLOW_OK;
  GstEvent *http_headers_event = NULL;

  src = GST_SOUP_HTTP_SRC (psrc);

retry:
  g_mutex_lock (&src->mutex);

  /* Check for pending position change */
  if (src->request_position != src->read_position) {
    if (src->input_stream) {
      g_input_stream_close (src->input_stream, src->cancellable, NULL);
      g_object_unref (src->input_stream);
      src->input_stream = NULL;
    }
  }

  if (g_cancellable_is_cancelled (src->cancellable)) {
    ret = GST_FLOW_FLUSHING;
    g_mutex_unlock (&src->mutex);
    goto done;
  }

  /* If we have no open connection to the server, start one */
  if (!src->input_stream) {
    *outbuf = NULL;
    ret =
        gst_soup_http_src_do_request (src,
        src->method ? src->method : SOUP_METHOD_GET);
    http_headers_event = src->http_headers_event;
    src->http_headers_event = NULL;
  }
  g_mutex_unlock (&src->mutex);

  if (ret == GST_FLOW_OK || ret == GST_FLOW_CUSTOM_ERROR) {
    if (http_headers_event) {
      gst_pad_push_event (GST_BASE_SRC_PAD (src), http_headers_event);
      http_headers_event = NULL;
    }
  }

  if (ret == GST_FLOW_OK)
    ret = gst_soup_http_src_read_buffer (src, outbuf);

done:
  GST_DEBUG_OBJECT (src, "Returning %d %s", ret, gst_flow_get_name (ret));
  if (ret != GST_FLOW_OK) {
    if (http_headers_event)
      gst_event_unref (http_headers_event);

    g_mutex_lock (&src->mutex);
    if (src->input_stream) {
      g_object_unref (src->input_stream);
      src->input_stream = NULL;
    }
    g_mutex_unlock (&src->mutex);
    if (ret == GST_FLOW_CUSTOM_ERROR) {
      ret = GST_FLOW_OK;
      goto retry;
    }
  }

  if (ret == GST_FLOW_FLUSHING) {
    g_mutex_lock (&src->mutex);
    src->retry_count = 0;
    g_mutex_unlock (&src->mutex);
  }

  return ret;
}

static gboolean
gst_soup_http_src_start (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);

  GST_DEBUG_OBJECT (src, "start(\"%s\")", src->location);

  return gst_soup_http_src_session_open (src);
}

static gboolean
gst_soup_http_src_stop (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (bsrc);
  GST_DEBUG_OBJECT (src, "stop()");
  if (src->keep_alive && !src->msg && !src->session_is_shared)
    gst_soup_http_src_cancel_message (src);
  else
    gst_soup_http_src_session_close (src);

  gst_soup_http_src_reset (src);
  return TRUE;
}

static GstStateChangeReturn
gst_soup_http_src_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_soup_http_src_session_close (src);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  return ret;
}

static void
gst_soup_http_src_set_context (GstElement * element, GstContext * context)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (element);

  if (g_strcmp0 (gst_context_get_context_type (context),
          GST_SOUP_SESSION_CONTEXT) == 0) {
    const GstStructure *s = gst_context_get_structure (context);

    GST_OBJECT_LOCK (src);
    if (src->external_session)
      g_object_unref (src->external_session);
    src->external_session = NULL;
    gst_structure_get (s, "session", SOUP_TYPE_SESSION, &src->external_session,
        NULL);
    src->forced_external_session = FALSE;
    gst_structure_get (s, "force", G_TYPE_BOOLEAN,
        &src->forced_external_session, NULL);

    GST_DEBUG_OBJECT (src, "Setting external session %p (force: %d)",
        src->external_session, src->forced_external_session);
    GST_OBJECT_UNLOCK (src);
  }

  GST_ELEMENT_CLASS (parent_class)->set_context (element, context);
}

/* Interrupt a blocking request. */
static gboolean
gst_soup_http_src_unlock (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (bsrc);
  GST_DEBUG_OBJECT (src, "unlock()");

  gst_soup_http_src_cancel_message (src);
  return TRUE;
}

/* Interrupt interrupt. */
static gboolean
gst_soup_http_src_unlock_stop (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (bsrc);
  GST_DEBUG_OBJECT (src, "unlock_stop()");

  g_cancellable_reset (src->cancellable);
  return TRUE;
}

static gboolean
gst_soup_http_src_get_size (GstBaseSrc * bsrc, guint64 * size)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (bsrc);

  if (src->have_size) {
    GST_DEBUG_OBJECT (src, "get_size() = %" G_GUINT64_FORMAT,
        src->content_size);
    *size = src->content_size;
    return TRUE;
  }
  GST_DEBUG_OBJECT (src, "get_size() = FALSE");
  return FALSE;
}

static void
gst_soup_http_src_check_seekable (GstSoupHTTPSrc * src)
{
  GstFlowReturn ret = GST_FLOW_OK;

  /* Special case to check if the server allows range requests
   * before really starting to get data in the buffer creation
   * loops.
   */
  if (!src->got_headers && GST_STATE (src) >= GST_STATE_PAUSED) {
    g_mutex_lock (&src->mutex);
    while (!src->got_headers && !g_cancellable_is_cancelled (src->cancellable)
        && ret == GST_FLOW_OK) {
      if ((src->msg && src->msg->method != SOUP_METHOD_HEAD)) {
        /* wait for the current request to finish */
        g_cond_wait (&src->have_headers_cond, &src->mutex);
      } else {
        if (gst_soup_http_src_session_open (src)) {
          ret = gst_soup_http_src_do_request (src, SOUP_METHOD_HEAD);
        }
      }
    }
    g_mutex_unlock (&src->mutex);
  }
}

static gboolean
gst_soup_http_src_is_seekable (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);

  gst_soup_http_src_check_seekable (src);

  return src->seekable;
}

static gboolean
gst_soup_http_src_do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);

  GST_DEBUG_OBJECT (src, "do_seek(%" G_GUINT64_FORMAT "-%" G_GUINT64_FORMAT
      ")", segment->start, segment->stop);
  if (src->read_position == segment->start &&
      src->request_position == src->read_position &&
      src->stop_position == segment->stop) {
    GST_DEBUG_OBJECT (src,
        "Seek to current read/end position and no seek pending");
    return TRUE;
  }

  gst_soup_http_src_check_seekable (src);

  /* If we have no headers we don't know yet if it is seekable or not.
   * Store the start position and error out later if it isn't */
  if (src->got_headers && !src->seekable) {
    GST_WARNING_OBJECT (src, "Not seekable");
    return FALSE;
  }

  if (segment->rate < 0.0 || segment->format != GST_FORMAT_BYTES) {
    GST_WARNING_OBJECT (src, "Invalid seek segment");
    return FALSE;
  }

  if (src->have_size && segment->start >= src->content_size) {
    GST_WARNING_OBJECT (src,
        "Potentially seeking behind end of file, might EOS immediately");
  }

  /* Wait for create() to handle the jump in offset. */
  src->request_position = segment->start;
  src->stop_position = segment->stop;

  return TRUE;
}

static gboolean
gst_soup_http_src_query (GstBaseSrc * bsrc, GstQuery * query)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);
  gboolean ret;
  GstSchedulingFlags flags;
  gint minsize, maxsize, align;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_URI:
      gst_query_set_uri (query, src->location);
      if (src->redirection_uri != NULL) {
        gst_query_set_uri_redirection (query, src->redirection_uri);
        gst_query_set_uri_redirection_permanent (query,
            src->redirection_permanent);
      }
      ret = TRUE;
      break;
    default:
      ret = FALSE;
      break;
  }

  if (!ret)
    ret = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_SCHEDULING:
      gst_query_parse_scheduling (query, &flags, &minsize, &maxsize, &align);
      flags |= GST_SCHEDULING_FLAG_BANDWIDTH_LIMITED;
      gst_query_set_scheduling (query, flags, minsize, maxsize, align);
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
gst_soup_http_src_set_location (GstSoupHTTPSrc * src, const gchar * uri,
    GError ** error)
{
  const char *alt_schemes[] = { "icy://", "icyx://" };
  guint i;

  if (src->location) {
    g_free (src->location);
    src->location = NULL;
  }

  if (uri == NULL)
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (alt_schemes); i++) {
    if (g_str_has_prefix (uri, alt_schemes[i])) {
      src->location =
          g_strdup_printf ("http://%s", uri + strlen (alt_schemes[i]));
      return TRUE;
    }
  }

  if (src->redirection_uri) {
    g_free (src->redirection_uri);
    src->redirection_uri = NULL;
  }

  src->location = g_strdup (uri);

  return TRUE;
}

static gboolean
gst_soup_http_src_set_proxy (GstSoupHTTPSrc * src, const gchar * uri)
{
  if (src->proxy) {
    soup_uri_free (src->proxy);
    src->proxy = NULL;
  }

  if (uri == NULL || *uri == '\0')
    return TRUE;

  if (g_strstr_len (uri, -1, "://")) {
    src->proxy = soup_uri_new (uri);
  } else {
    gchar *new_uri = g_strconcat ("http://", uri, NULL);

    src->proxy = soup_uri_new (new_uri);
    g_free (new_uri);
  }

  return (src->proxy != NULL);
}

static GstURIType
gst_soup_http_src_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_soup_http_src_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "http", "https", "icy", "icyx", NULL };

  return protocols;
}

static gchar *
gst_soup_http_src_uri_get_uri (GstURIHandler * handler)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (handler);

  /* FIXME: make thread-safe */
  return g_strdup (src->location);
}

static gboolean
gst_soup_http_src_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (handler);

  return gst_soup_http_src_set_location (src, uri, error);
}

static void
gst_soup_http_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_soup_http_src_uri_get_type;
  iface->get_protocols = gst_soup_http_src_uri_get_protocols;
  iface->get_uri = gst_soup_http_src_uri_get_uri;
  iface->set_uri = gst_soup_http_src_uri_set_uri;
}
