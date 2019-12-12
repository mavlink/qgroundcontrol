/* GStreamer
 * Copyright (C) 2011 David Schleef <ds@entropywave.com>
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

#ifndef _GST_SOUP_HTTP_CLIENT_SINK_H_
#define _GST_SOUP_HTTP_CLIENT_SINK_H_

#include <gst/base/gstbasesink.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS

#define GST_TYPE_SOUP_HTTP_CLIENT_SINK           (gst_soup_http_client_sink_get_type())
#define GST_SOUP_HTTP_CLIENT_SINK(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SOUP_HTTP_CLIENT_SINK,GstSoupHttpClientSink))
#define GST_SOUP_HTTP_CLIENT_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SOUP_HTTP_CLIENT_SINK,GstSoupHttpClientSinkClass))
#define GST_IS_SOUP_HTTP_CLIENT_SINK(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SOUP_HTTP_CLIENT_SINK))
#define GST_IS_SOUP_HTTP_CLIENT_SINK_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SOUP_HTTP_CLIENT_SINK))

typedef struct _GstSoupHttpClientSink GstSoupHttpClientSink;
typedef struct _GstSoupHttpClientSinkClass GstSoupHttpClientSinkClass;

struct _GstSoupHttpClientSink
{
  GstBaseSink base_souphttpsink;

  GMutex mutex;
  GCond cond;
  GMainContext *context;
  GMainLoop *loop;
  GThread *thread;
  GSource *timer;
  SoupMessage *message;
  SoupSession *session;
  GList *queued_buffers;
  GList *sent_buffers;
  GList *streamheader_buffers;

  int status_code;
  char *reason_phrase;

  guint64 offset;
  int timeout;
  gint failures;

  /* properties */
  SoupSession *prop_session;
  char *location;
  char *user_id;
  char *user_pw;
  SoupURI *proxy;
  char *proxy_id;
  char *proxy_pw;
  char *user_agent;
  gboolean automatic_redirect;
  gchar **cookies;
  SoupLoggerLogLevel log_level;
  gint retry_delay;
  gint retries;
};

struct _GstSoupHttpClientSinkClass
{
  GstBaseSinkClass base_souphttpsink_class;
};

GType gst_soup_http_client_sink_get_type (void);

G_END_DECLS

#endif
