/* GStreamer unit tests for the souphttpsrc element
 * Copyright (C) 2006-2007 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2008 Wouter Cloetens <wouter@mind.be>
 * Copyright (C) 2001-2003, Ximian, Inc.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>

#include <glib.h>
#include <glib/gprintf.h>

#define SOUP_VERSION_MIN_REQUIRED (SOUP_VERSION_2_40)
#include <libsoup/soup.h>
#include <gst/check/gstcheck.h>

#if !defined(SOUP_MINOR_VERSION) || SOUP_MINOR_VERSION < 44
#define SoupStatus SoupKnownStatusCode
#endif


gboolean redirect = TRUE;

static const char **cookies = NULL;

/* Variables for authentication tests */
static const char *user_id = NULL;
static const char *user_pw = NULL;
static const char *good_user = "good_user";
static const char *bad_user = "bad_user";
static const char *good_pw = "good_pw";
static const char *bad_pw = "bad_pw";
static const char *realm = "SOUPHTTPSRC_REALM";
static const char *basic_auth_path = "/basic_auth";
static const char *digest_auth_path = "/digest_auth";

static const char *ssl_cert_file = GST_TEST_FILES_PATH "/test-cert.pem";
static const char *ssl_key_file = GST_TEST_FILES_PATH "/test-key.pem";

static guint get_port_from_server (SoupServer * server);
static SoupServer *run_server (gboolean use_https);

static void
handoff_cb (GstElement * fakesink, GstBuffer * buf, GstPad * pad,
    GstBuffer ** p_outbuf)
{
  GST_LOG ("handoff, buf = %p", buf);
  if (*p_outbuf == NULL)
    *p_outbuf = gst_buffer_ref (buf);
}

static gboolean
basic_auth_cb (SoupAuthDomain * domain, SoupMessage * msg,
    const char *username, const char *password, gpointer user_data)
{
  /* There is only one good login for testing */
  return (strcmp (username, good_user) == 0)
      && (strcmp (password, good_pw) == 0);
}


static char *
digest_auth_cb (SoupAuthDomain * domain, SoupMessage * msg,
    const char *username, gpointer user_data)
{
  /* There is only one good login for testing */
  if (strcmp (username, good_user) == 0)
    return soup_auth_domain_digest_encode_password (good_user, realm, good_pw);
  return NULL;
}

static gboolean
run_test (gboolean use_https, const gchar * path, gint expected)
{
  GstStateChangeReturn ret;
  GstElement *pipe, *src, *sink;
  GstBuffer *buf = NULL;
  GstMessage *msg;
  gchar *url;
  gboolean res = FALSE;
  SoupServer *server;
  guint port;

  server = run_server (use_https);
  if (server == NULL) {
    g_print ("Failed to start up %s server", use_https ? "HTTPS" : "HTTP");
    /* skip this test */
    return TRUE;
  }

  pipe = gst_pipeline_new (NULL);

  src = gst_element_factory_make ("souphttpsrc", NULL);
  fail_unless (src != NULL);

  sink = gst_element_factory_make ("fakesink", NULL);
  fail_unless (sink != NULL);

  gst_bin_add (GST_BIN (pipe), src);
  gst_bin_add (GST_BIN (pipe), sink);
  fail_unless (gst_element_link (src, sink));

  port = get_port_from_server (server);
  url = g_strdup_printf ("%s://127.0.0.1:%u%s",
      use_https ? "https" : "http", port, path);
  fail_unless (url != NULL);
  g_object_set (src, "location", url, NULL);
  g_free (url);

  if (use_https) {
    GTlsDatabase *tlsdb;
    GError *error = NULL;
    gchar *path;

    /* GTlsFileDatabase needs an absolute path. Using a relative one
     * causes a warning from GLib-Net followed by a segfault in GnuTLS */
    if (g_path_is_absolute (ssl_cert_file)) {
      path = g_strdup (ssl_cert_file);
    } else {
      gchar *cwd = g_get_current_dir ();
      path = g_build_filename (cwd, ssl_cert_file, NULL);
      g_free (cwd);
    }

    tlsdb = g_tls_file_database_new (path, &error);
    fail_unless (tlsdb, "Failed to load certificate: %s", error->message);

    g_object_set (src, "tls-database", tlsdb, NULL);

    g_object_unref (tlsdb);
    g_free (path);
  }

  g_object_set (src, "automatic-redirect", redirect, NULL);
  if (cookies != NULL)
    g_object_set (src, "cookies", cookies, NULL);
  g_object_set (sink, "signal-handoffs", TRUE, NULL);
  g_signal_connect (sink, "preroll-handoff", G_CALLBACK (handoff_cb), &buf);

  if (user_id != NULL)
    g_object_set (src, "user-id", user_id, NULL);
  if (user_pw != NULL)
    g_object_set (src, "user-pw", user_pw, NULL);

  ret = gst_element_set_state (pipe, GST_STATE_PAUSED);
  if (ret != GST_STATE_CHANGE_ASYNC) {
    GST_DEBUG ("failed to start up soup http src, ret = %d", ret);
    goto done;
  }

  gst_element_set_state (pipe, GST_STATE_PLAYING);
  msg = gst_bus_poll (GST_ELEMENT_BUS (pipe),
      GST_MESSAGE_EOS | GST_MESSAGE_ERROR, -1);
  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
    gchar *debug = NULL;
    GError *err = NULL;
    gint rc = -1;

    gst_message_parse_error (msg, &err, &debug);
    GST_INFO ("error: %s", err->message);
    if (g_str_has_suffix (err->message, "Not Found"))
      rc = 404;
    else if (g_str_has_suffix (err->message, "Forbidden"))
      rc = 403;
    else if (g_str_has_suffix (err->message, "Unauthorized"))
      rc = 401;
    else if (g_str_has_suffix (err->message, "Found"))
      rc = 302;
    GST_INFO ("debug: %s", debug);
    /* should not've gotten any output in case of a 40x error. Wait a bit
     * to give the streaming thread a chance to push out a buffer and trigger
     * our callback before shutting down the pipeline */
    g_usleep (G_USEC_PER_SEC / 2);
    fail_unless (buf == NULL);
    g_error_free (err);
    g_free (debug);
    gst_message_unref (msg);
    GST_DEBUG ("Got HTTP error %u, expected %u", rc, expected);
    res = (rc == expected);
    goto done;
  }
  gst_message_unref (msg);

  /* don't wait for more than 10 seconds */
  ret = gst_element_get_state (pipe, NULL, NULL, 10 * GST_SECOND);
  GST_LOG ("ret = %u", ret);

  if (buf == NULL) {
    /* we want to test the buffer offset, nothing else; if there's a failure
     * it might be for lots of reasons (no network connection, whatever), we're
     * not interested in those */
    GST_DEBUG ("didn't manage to get data within 10 seconds, skipping test");
    res = TRUE;
    goto done;
  }

  GST_DEBUG ("buffer offset = %" G_GUINT64_FORMAT, GST_BUFFER_OFFSET (buf));

  /* first buffer should have a 0 offset */
  fail_unless (GST_BUFFER_OFFSET (buf) == 0);
  gst_buffer_unref (buf);
  res = (expected == 0);

done:

  gst_element_set_state (pipe, GST_STATE_NULL);
  gst_object_unref (pipe);
  gst_object_unref (server);
  return res;
}

GST_START_TEST (test_first_buffer_has_offset)
{
  fail_unless (run_test (FALSE, "/", 0));
}

GST_END_TEST;

GST_START_TEST (test_not_found)
{
  fail_unless (run_test (FALSE, "/404", 404));
  fail_unless (run_test (FALSE, "/404-with-data", 404));
}

GST_END_TEST;

GST_START_TEST (test_forbidden)
{
  fail_unless (run_test (FALSE, "/403", 403));
}

GST_END_TEST;

GST_START_TEST (test_redirect_no)
{
  redirect = FALSE;
  fail_unless (run_test (FALSE, "/302", 302));
}

GST_END_TEST;

GST_START_TEST (test_redirect_yes)
{
  redirect = TRUE;
  fail_unless (run_test (FALSE, "/302", 0));
}

GST_END_TEST;

GST_START_TEST (test_https)
{
  fail_unless (run_test (TRUE, "/", 0));
}

GST_END_TEST;

GST_START_TEST (test_cookies)
{
  static const char *biscotti[] = { "delacre=yummie", "koekje=lu", NULL };
  gboolean res;

  cookies = biscotti;
  res = run_test (FALSE, "/", 0);
  cookies = NULL;
  fail_unless (res);
}

GST_END_TEST;

GST_START_TEST (test_good_user_basic_auth)
{
  gboolean res;

  user_id = good_user;
  user_pw = good_pw;
  res = run_test (FALSE, basic_auth_path, 0);
  GST_DEBUG ("Basic Auth user %s password %s res = %d", user_id, user_pw, res);
  user_id = user_pw = NULL;
  fail_unless (res);
}

GST_END_TEST;

GST_START_TEST (test_bad_user_basic_auth)
{
  gboolean res;

  user_id = bad_user;
  user_pw = good_pw;
  res = run_test (FALSE, basic_auth_path, 401);
  GST_DEBUG ("Basic Auth user %s password %s res = %d", user_id, user_pw, res);
  user_id = user_pw = NULL;
  fail_unless (res);
}

GST_END_TEST;

GST_START_TEST (test_bad_password_basic_auth)
{
  gboolean res;

  user_id = good_user;
  user_pw = bad_pw;
  res = run_test (FALSE, basic_auth_path, 401);
  GST_DEBUG ("Basic Auth user %s password %s res = %d", user_id, user_pw, res);
  user_id = user_pw = NULL;
  fail_unless (res);
}

GST_END_TEST;

GST_START_TEST (test_good_user_digest_auth)
{
  gboolean res;

  user_id = good_user;
  user_pw = good_pw;
  res = run_test (FALSE, digest_auth_path, 0);
  GST_DEBUG ("Digest Auth user %s password %s res = %d", user_id, user_pw, res);
  user_id = user_pw = NULL;
  fail_unless (res);
}

GST_END_TEST;

GST_START_TEST (test_bad_user_digest_auth)
{
  gboolean res;

  user_id = bad_user;
  user_pw = good_pw;
  res = run_test (FALSE, digest_auth_path, 401);
  GST_DEBUG ("Digest Auth user %s password %s res = %d", user_id, user_pw, res);
  user_id = user_pw = NULL;
  fail_unless (res);
}

GST_END_TEST;

GST_START_TEST (test_bad_password_digest_auth)
{
  gboolean res;

  user_id = good_user;
  user_pw = bad_pw;
  res = run_test (FALSE, digest_auth_path, 401);
  GST_DEBUG ("Digest Auth user %s password %s res = %d", user_id, user_pw, res);
  user_id = user_pw = NULL;
  fail_unless (res);
}

GST_END_TEST;

static gboolean icy_caps = FALSE;

static void
got_buffer (GstElement * fakesink, GstBuffer * buf, GstPad * pad,
    gpointer user_data)
{
  GstStructure *s;
  GstCaps *caps;

  /* Caps can be anything if we don't except icy caps */
  if (!icy_caps)
    return;

  /* Otherwise they _must_ be "application/x-icy" */
  caps = gst_pad_get_current_caps (pad);
  fail_unless (caps != NULL);
  s = gst_caps_get_structure (caps, 0);
  fail_unless_equals_string (gst_structure_get_name (s), "application/x-icy");
  gst_caps_unref (caps);
}

GST_START_TEST (test_icy_stream)
{
  GstElement *pipe, *src, *sink;

  GstMessage *msg;

  pipe = gst_pipeline_new (NULL);

  src = gst_element_factory_make ("souphttpsrc", NULL);
  fail_unless (src != NULL);

  sink = gst_element_factory_make ("fakesink", NULL);
  fail_unless (sink != NULL);
  g_object_set (sink, "signal-handoffs", TRUE, NULL);
  g_signal_connect (sink, "handoff", G_CALLBACK (got_buffer), NULL);

  gst_bin_add (GST_BIN (pipe), src);
  gst_bin_add (GST_BIN (pipe), sink);
  fail_unless (gst_element_link (src, sink));

  /* Radionomy Hot40Music shoutcast stream */
  g_object_set (src, "location",
      "http://streaming.radionomy.com:80/Hot40Music", NULL);

  /* EOS after the first buffer */
  g_object_set (src, "num-buffers", 1, NULL);
  icy_caps = TRUE;
  gst_element_set_state (pipe, GST_STATE_PLAYING);
  msg = gst_bus_poll (GST_ELEMENT_BUS (pipe),
      GST_MESSAGE_EOS | GST_MESSAGE_ERROR, -1);

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      GST_DEBUG ("success, we're done here");
      gst_message_unref (msg);
      break;
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;

      gst_message_parse_error (msg, &err, NULL);
      GST_INFO ("Error with ICY mp3 shoutcast stream: %s", err->message);
      gst_message_unref (msg);
      g_clear_error (&err);
      break;
    }
    default:
      break;
  }

  icy_caps = FALSE;

  gst_element_set_state (pipe, GST_STATE_NULL);
  gst_object_unref (pipe);
}

GST_END_TEST;

static Suite *
souphttpsrc_suite (void)
{
  TCase *tc_chain, *tc_internet;
  Suite *s;

  /* we don't support exceptions from the proxy, so just unset the environment
   * variable - in case it's set in the test environment it would otherwise
   * prevent us from connecting to localhost (like jenkins.qa.ubuntu.com) */
  g_unsetenv ("http_proxy");

  s = suite_create ("souphttpsrc");
  tc_chain = tcase_create ("general");
  tc_internet = tcase_create ("internet");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_first_buffer_has_offset);
  tcase_add_test (tc_chain, test_redirect_yes);
  tcase_add_test (tc_chain, test_redirect_no);
  tcase_add_test (tc_chain, test_not_found);
  tcase_add_test (tc_chain, test_forbidden);
  tcase_add_test (tc_chain, test_cookies);
  tcase_add_test (tc_chain, test_good_user_basic_auth);
  tcase_add_test (tc_chain, test_bad_user_basic_auth);
  tcase_add_test (tc_chain, test_bad_password_basic_auth);
  tcase_add_test (tc_chain, test_good_user_digest_auth);
  tcase_add_test (tc_chain, test_bad_user_digest_auth);
  tcase_add_test (tc_chain, test_bad_password_digest_auth);
  tcase_add_test (tc_chain, test_https);

  suite_add_tcase (s, tc_internet);
  tcase_set_timeout (tc_internet, 250);
  tcase_add_test (tc_internet, test_icy_stream);

  return s;
}

GST_CHECK_MAIN (souphttpsrc);

static void
do_get (SoupMessage * msg, const char *path)
{
  gboolean send_error_doc = FALSE;
  char *uri;

  int buflen = 4096;

  SoupStatus status = SOUP_STATUS_OK;

  uri = soup_uri_to_string (soup_message_get_uri (msg), FALSE);
  GST_DEBUG ("request: \"%s\"", uri);

  if (!strcmp (path, "/301"))
    status = SOUP_STATUS_MOVED_PERMANENTLY;
  else if (!strcmp (path, "/302"))
    status = SOUP_STATUS_MOVED_TEMPORARILY;
  else if (!strcmp (path, "/307"))
    status = SOUP_STATUS_TEMPORARY_REDIRECT;
  else if (!strcmp (path, "/403"))
    status = SOUP_STATUS_FORBIDDEN;
  else if (!strcmp (path, "/404"))
    status = SOUP_STATUS_NOT_FOUND;
  else if (!strcmp (path, "/404-with-data")) {
    status = SOUP_STATUS_NOT_FOUND;
    send_error_doc = TRUE;
  }

  if (SOUP_STATUS_IS_REDIRECTION (status)) {
    char *redir_uri;

    redir_uri = g_strdup_printf ("%s-redirected", uri);
    soup_message_headers_append (msg->response_headers, "Location", redir_uri);
    g_free (redir_uri);
  }
  if (status != (SoupStatus) SOUP_STATUS_OK && !send_error_doc)
    goto leave;

  if (msg->method == SOUP_METHOD_GET) {
    char *buf;

    buf = g_malloc (buflen);
    memset (buf, 0, buflen);
    soup_message_body_append (msg->response_body, SOUP_MEMORY_TAKE,
        buf, buflen);
  } else {                      /* msg->method == SOUP_METHOD_HEAD */

    char *length;

    /* We could just use the same code for both GET and
     * HEAD. But we'll optimize and avoid the extra
     * malloc.
     */
    length = g_strdup_printf ("%lu", (gulong) buflen);
    soup_message_headers_append (msg->response_headers,
        "Content-Length", length);
    g_free (length);
  }

leave:
  soup_message_set_status (msg, status);
  g_free (uri);
}

static void
print_header (const char *name, const char *value, gpointer data)
{
  GST_DEBUG ("header: %s: %s", name, value);
}

static void
server_callback (SoupServer * server, SoupMessage * msg,
    const char *path, GHashTable * query,
    SoupClientContext * context, gpointer data)
{
  GST_DEBUG ("%s %s HTTP/1.%d", msg->method, path,
      soup_message_get_http_version (msg));
  soup_message_headers_foreach (msg->request_headers, print_header, NULL);
  if (msg->request_body->length)
    GST_DEBUG ("%s", msg->request_body->data);

  if (msg->method == SOUP_METHOD_GET || msg->method == SOUP_METHOD_HEAD)
    do_get (msg, path);
  else
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);

  GST_DEBUG ("  -> %d %s", msg->status_code, msg->reason_phrase);
}

static guint
get_port_from_server (SoupServer * server)
{
  GSList *uris;
  guint port;

  uris = soup_server_get_uris (server);
  g_assert (g_slist_length (uris) == 1);
  port = soup_uri_get_port (uris->data);
  g_slist_free_full (uris, (GDestroyNotify) soup_uri_free);

  return port;
}

static SoupServer *
run_server (gboolean use_https)
{
  SoupServer *server = soup_server_new (NULL, NULL);
  SoupServerListenOptions listen_flags = 0;
  guint port;


  if (use_https) {
    GTlsBackend *backend = g_tls_backend_get_default ();
    GError *err = NULL;

    if (backend == NULL || !g_tls_backend_supports_tls (backend)) {
      GST_INFO ("No TLS support");
      g_object_unref (server);
      return NULL;
    }

    if (!soup_server_set_ssl_cert_file (server, ssl_cert_file, ssl_key_file,
            &err)) {
      GST_INFO ("Failed to load certificate: %s", err->message);
      g_object_unref (server);
      g_error_free (err);
      return NULL;
    }

    listen_flags |= SOUP_SERVER_LISTEN_HTTPS;
  }

  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);

  {
    SoupAuthDomain *domain;

    domain = soup_auth_domain_basic_new (SOUP_AUTH_DOMAIN_REALM, realm,
        SOUP_AUTH_DOMAIN_BASIC_AUTH_CALLBACK, basic_auth_cb,
        SOUP_AUTH_DOMAIN_ADD_PATH, basic_auth_path, NULL);
    soup_server_add_auth_domain (server, domain);
    g_object_unref (domain);

    domain = soup_auth_domain_digest_new (SOUP_AUTH_DOMAIN_REALM, realm,
        SOUP_AUTH_DOMAIN_DIGEST_AUTH_CALLBACK, digest_auth_cb,
        SOUP_AUTH_DOMAIN_ADD_PATH, digest_auth_path, NULL);
    soup_server_add_auth_domain (server, domain);
    g_object_unref (domain);
  }

  {
    GSocketAddress *address;
    GError *err = NULL;

    address =
        g_inet_socket_address_new_from_string ("0.0.0.0",
        SOUP_ADDRESS_ANY_PORT);
    soup_server_listen (server, address, listen_flags, &err);
    g_object_unref (address);

    if (err) {
      GST_ERROR ("Failed to start %s server: %s",
          use_https ? "HTTPS" : "HTTP", err->message);
      g_object_unref (server);
      g_error_free (err);
      return NULL;
    }
  }

  port = get_port_from_server (server);
  GST_DEBUG ("%s server listening on port %u", use_https ? "HTTPS" : "HTTP",
      port);

  /* check if we can connect to our local http server */
  {
    GSocketConnection *conn;
    GSocketClient *client;

    client = g_socket_client_new ();
    g_socket_client_set_timeout (client, 2);
    conn =
        g_socket_client_connect_to_host (client, "127.0.0.1", port, NULL, NULL);
    if (conn == NULL) {
      GST_INFO ("Couldn't connect to 127.0.0.1:%u", port);
      g_object_unref (client);
      g_object_unref (server);
      return NULL;
    }

    g_object_unref (conn);
    g_object_unref (client);
  }

  return server;
}
