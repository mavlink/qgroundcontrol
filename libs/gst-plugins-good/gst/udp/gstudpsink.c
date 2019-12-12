/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim@fluendo.com>
 * Copyright (C) <2012> Collabora Ltd.
 *   Author: Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
/**
 * SECTION:element-udpsink
 * @title: udpsink
 * @see_also: udpsrc, multifdsink
 *
 * udpsink is a network sink that sends UDP packets to the network.
 * It can be combined with RTP payloaders to implement RTP streaming.
 *
 * ## Examples
 * |[
 * gst-launch-1.0 -v audiotestsrc ! udpsink
 * ]|
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gstudpsink.h"

#define UDP_DEFAULT_HOST        "localhost"
#define UDP_DEFAULT_PORT        5004

/* UDPSink signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_HOST,
  PROP_PORT,
  PROP_URI,
  /* FILL ME */
};

static void gst_udpsink_finalize (GstUDPSink * udpsink);

static void gst_udpsink_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

static void gst_udpsink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_udpsink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/*static guint gst_udpsink_signals[LAST_SIGNAL] = { 0 }; */
#define gst_udpsink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstUDPSink, gst_udpsink, GST_TYPE_MULTIUDPSINK,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER, gst_udpsink_uri_handler_init));

static void
gst_udpsink_class_init (GstUDPSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_udpsink_set_property;
  gobject_class->get_property = gst_udpsink_get_property;

  gobject_class->finalize = (GObjectFinalizeFunc) gst_udpsink_finalize;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_HOST,
      g_param_spec_string ("host", "host",
          "The host/IP/Multicast group to send the packets to",
          UDP_DEFAULT_HOST, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PORT,
      g_param_spec_int ("port", "port", "The port to send the packets to",
          0, 65535, UDP_DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class, "UDP packet sender",
      "Sink/Network",
      "Send data over the network via UDP", "Wim Taymans <wim@fluendo.com>");
}

static void
gst_udpsink_init (GstUDPSink * udpsink)
{
  udpsink->host = g_strdup (UDP_DEFAULT_HOST);
  udpsink->port = UDP_DEFAULT_PORT;
  udpsink->uri = g_strdup_printf ("udp://%s:%d", udpsink->host, udpsink->port);

  gst_multiudpsink_add (GST_MULTIUDPSINK (udpsink), udpsink->host,
      udpsink->port);
}

static void
gst_udpsink_finalize (GstUDPSink * udpsink)
{
  g_free (udpsink->host);
  udpsink->host = NULL;

  g_free (udpsink->uri);
  udpsink->uri = NULL;

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) udpsink);
}

static gboolean
gst_udpsink_set_uri (GstUDPSink * sink, const gchar * uri, GError ** error)
{
  gchar *host;
  guint16 port;

  gst_multiudpsink_remove (GST_MULTIUDPSINK (sink), sink->host, sink->port);

  if (!gst_udp_parse_uri (uri, &host, &port))
    goto wrong_uri;

  g_free (sink->host);
  sink->host = host;
  sink->port = port;

  g_free (sink->uri);
  sink->uri = g_strdup (uri);

  gst_multiudpsink_add (GST_MULTIUDPSINK (sink), sink->host, sink->port);

  return TRUE;

  /* ERRORS */
wrong_uri:
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, READ, (NULL),
        ("error parsing uri %s", uri));
    g_set_error_literal (error, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
        "Could not parse UDP URI");
    return FALSE;
  }
}

static void
gst_udpsink_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstUDPSink *udpsink;

  udpsink = GST_UDPSINK (object);

  /* remove old host */
  gst_multiudpsink_remove (GST_MULTIUDPSINK (udpsink),
      udpsink->host, udpsink->port);

  switch (prop_id) {
    case PROP_HOST:
    {
      const gchar *host;

      host = g_value_get_string (value);
      g_free (udpsink->host);
      udpsink->host = g_strdup (host);
      g_free (udpsink->uri);
      udpsink->uri =
          g_strdup_printf ("udp://%s:%d", udpsink->host, udpsink->port);
      break;
    }
    case PROP_PORT:
      udpsink->port = g_value_get_int (value);
      g_free (udpsink->uri);
      udpsink->uri =
          g_strdup_printf ("udp://%s:%d", udpsink->host, udpsink->port);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  /* add new host */
  gst_multiudpsink_add (GST_MULTIUDPSINK (udpsink),
      udpsink->host, udpsink->port);
}

static void
gst_udpsink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstUDPSink *udpsink;

  udpsink = GST_UDPSINK (object);

  switch (prop_id) {
    case PROP_HOST:
      g_value_set_string (value, udpsink->host);
      break;
    case PROP_PORT:
      g_value_set_int (value, udpsink->port);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_udpsink_uri_get_type (GType type)
{
  return GST_URI_SINK;
}

static const gchar *const *
gst_udpsink_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "udp", NULL };

  return protocols;
}

static gchar *
gst_udpsink_uri_get_uri (GstURIHandler * handler)
{
  GstUDPSink *sink = GST_UDPSINK (handler);

  return g_strdup (sink->uri);
}

static gboolean
gst_udpsink_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  return gst_udpsink_set_uri (GST_UDPSINK (handler), uri, error);
}

static void
gst_udpsink_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_udpsink_uri_get_type;
  iface->get_protocols = gst_udpsink_uri_get_protocols;
  iface->get_uri = gst_udpsink_uri_get_uri;
  iface->set_uri = gst_udpsink_uri_set_uri;
}
