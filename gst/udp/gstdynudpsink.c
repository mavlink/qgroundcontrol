/* GStreamer
 * Copyright (C) <2005> Philippe Khalaf <burger@speedy.org>
 * Copyright (C) <2005> Nokia Corporation <kai.vehmanen@nokia.com>
 * Copyright (C) <2006> Joni Valtanen <joni.valtanen@movial.fi>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gstdynudpsink.h"

#include <gst/net/gstnetaddressmeta.h>

GST_DEBUG_CATEGORY_STATIC (dynudpsink_debug);
#define GST_CAT_DEFAULT (dynudpsink_debug)

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* DynUDPSink signals and args */
enum
{
  /* methods */
  SIGNAL_GET_STATS,

  /* signals */

  /* FILL ME */
  LAST_SIGNAL
};

#define UDP_DEFAULT_SOCKET		NULL
#define UDP_DEFAULT_CLOSE_SOCKET	TRUE
#define UDP_DEFAULT_BIND_ADDRESS	NULL
#define UDP_DEFAULT_BIND_PORT   	0

enum
{
  PROP_0,
  PROP_SOCKET,
  PROP_SOCKET_V6,
  PROP_CLOSE_SOCKET,
  PROP_BIND_ADDRESS,
  PROP_BIND_PORT
};

static void gst_dynudpsink_finalize (GObject * object);

static GstFlowReturn gst_dynudpsink_render (GstBaseSink * sink,
    GstBuffer * buffer);
static gboolean gst_dynudpsink_stop (GstBaseSink * bsink);
static gboolean gst_dynudpsink_start (GstBaseSink * bsink);
static gboolean gst_dynudpsink_unlock (GstBaseSink * bsink);
static gboolean gst_dynudpsink_unlock_stop (GstBaseSink * bsink);

static void gst_dynudpsink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dynudpsink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static GstStructure *gst_dynudpsink_get_stats (GstDynUDPSink * sink,
    const gchar * host, gint port);

static guint gst_dynudpsink_signals[LAST_SIGNAL] = { 0 };

#define gst_dynudpsink_parent_class parent_class
G_DEFINE_TYPE (GstDynUDPSink, gst_dynudpsink, GST_TYPE_BASE_SINK);

static void
gst_dynudpsink_class_init (GstDynUDPSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gst_dynudpsink_set_property;
  gobject_class->get_property = gst_dynudpsink_get_property;
  gobject_class->finalize = gst_dynudpsink_finalize;

  gst_dynudpsink_signals[SIGNAL_GET_STATS] =
      g_signal_new ("get-stats", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstDynUDPSinkClass, get_stats),
      NULL, NULL, NULL, GST_TYPE_STRUCTURE, 2, G_TYPE_STRING, G_TYPE_INT);

  g_object_class_install_property (gobject_class, PROP_SOCKET,
      g_param_spec_object ("socket", "Socket",
          "Socket to use for UDP sending. (NULL == allocate)",
          G_TYPE_SOCKET, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SOCKET_V6,
      g_param_spec_object ("socket-v6", "Socket IPv6",
          "Socket to use for UDPv6 sending. (NULL == allocate)",
          G_TYPE_SOCKET, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CLOSE_SOCKET,
      g_param_spec_boolean ("close-socket", "Close socket",
          "Close socket if passed as property on state change",
          UDP_DEFAULT_CLOSE_SOCKET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_BIND_ADDRESS,
      g_param_spec_string ("bind-address", "Bind Address",
          "Address to bind the socket to", UDP_DEFAULT_BIND_ADDRESS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_BIND_PORT,
      g_param_spec_int ("bind-port", "Bind Port",
          "Port to bind the socket to", 0, G_MAXUINT16,
          UDP_DEFAULT_BIND_PORT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);

  gst_element_class_set_static_metadata (gstelement_class, "UDP packet sender",
      "Sink/Network",
      "Send data over the network via UDP with packet destinations picked up "
      "dynamically from meta on the buffers passed",
      "Philippe Khalaf <burger@speedy.org>");

  gstbasesink_class->render = gst_dynudpsink_render;
  gstbasesink_class->start = gst_dynudpsink_start;
  gstbasesink_class->stop = gst_dynudpsink_stop;
  gstbasesink_class->unlock = gst_dynudpsink_unlock;
  gstbasesink_class->unlock_stop = gst_dynudpsink_unlock_stop;

  klass->get_stats = gst_dynudpsink_get_stats;

  GST_DEBUG_CATEGORY_INIT (dynudpsink_debug, "dynudpsink", 0, "UDP sink");
}

static void
gst_dynudpsink_init (GstDynUDPSink * sink)
{
  sink->socket = UDP_DEFAULT_SOCKET;
  sink->socket_v6 = UDP_DEFAULT_SOCKET;
  sink->close_socket = UDP_DEFAULT_CLOSE_SOCKET;
  sink->external_socket = FALSE;
  sink->bind_address = UDP_DEFAULT_BIND_ADDRESS;
  sink->bind_port = UDP_DEFAULT_BIND_PORT;

  sink->used_socket = NULL;
  sink->used_socket_v6 = NULL;
}

static void
gst_dynudpsink_finalize (GObject * object)
{
  GstDynUDPSink *sink;

  sink = GST_DYNUDPSINK (object);

  if (sink->socket)
    g_object_unref (sink->socket);
  sink->socket = NULL;

  if (sink->socket_v6)
    g_object_unref (sink->socket_v6);
  sink->socket_v6 = NULL;

  if (sink->used_socket)
    g_object_unref (sink->used_socket);
  sink->used_socket = NULL;

  if (sink->used_socket_v6)
    g_object_unref (sink->used_socket_v6);
  sink->used_socket_v6 = NULL;

  g_free (sink->bind_address);
  sink->bind_address = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstFlowReturn
gst_dynudpsink_render (GstBaseSink * bsink, GstBuffer * buffer)
{
  GstDynUDPSink *sink;
  gssize ret;
  GstMapInfo map;
  GstNetAddressMeta *meta;
  GSocketAddress *addr;
  GError *err = NULL;
  GSocketFamily family;
  GSocket *socket;

  meta = gst_buffer_get_net_address_meta (buffer);

  if (meta == NULL) {
    GST_DEBUG ("Received buffer without GstNetAddressMeta, skipping");
    return GST_FLOW_OK;
  }

  sink = GST_DYNUDPSINK (bsink);

  /* let's get the address from the metadata */
  addr = meta->addr;

  family = g_socket_address_get_family (addr);
  if (family == G_SOCKET_FAMILY_IPV6 && !sink->used_socket_v6)
    goto invalid_family;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  GST_DEBUG ("about to send %" G_GSIZE_FORMAT " bytes", map.size);

#ifndef GST_DISABLE_GST_DEBUG
  {
    gchar *host;

    host =
        g_inet_address_to_string (g_inet_socket_address_get_address
        (G_INET_SOCKET_ADDRESS (addr)));
    GST_DEBUG ("sending %" G_GSIZE_FORMAT " bytes to client %s port %d",
        map.size, host,
        g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr)));
    g_free (host);
  }
#endif

  /* Select socket to send from for this address */
  if (family == G_SOCKET_FAMILY_IPV6 || !sink->used_socket)
    socket = sink->used_socket_v6;
  else
    socket = sink->used_socket;

  ret =
      g_socket_send_to (socket, addr, (gchar *) map.data, map.size,
      sink->cancellable, &err);
  gst_buffer_unmap (buffer, &map);

  if (ret < 0)
    goto send_error;

  GST_DEBUG ("sent %" G_GSSIZE_FORMAT " bytes", ret);

  return GST_FLOW_OK;

send_error:
  {
    GstFlowReturn flow_ret;

    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      GST_DEBUG_OBJECT (sink, "send cancelled");
      flow_ret = GST_FLOW_FLUSHING;
    } else {
      GST_ELEMENT_ERROR (sink, RESOURCE, WRITE, (NULL),
          ("send error: %s", err->message));
      flow_ret = GST_FLOW_ERROR;
    }
    g_clear_error (&err);
    return flow_ret;
  }
invalid_family:
  {
    GST_DEBUG ("invalid address family (got %d)", family);
    return GST_FLOW_ERROR;
  }
}

static void
gst_dynudpsink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDynUDPSink *udpsink;

  udpsink = GST_DYNUDPSINK (object);

  switch (prop_id) {
    case PROP_SOCKET:
      if (udpsink->socket != NULL && udpsink->socket != udpsink->used_socket &&
          udpsink->close_socket) {
        GError *err = NULL;

        if (!g_socket_close (udpsink->socket, &err)) {
          GST_ERROR ("failed to close socket %p: %s", udpsink->socket,
              err->message);
          g_clear_error (&err);
        }
      }
      if (udpsink->socket)
        g_object_unref (udpsink->socket);
      udpsink->socket = g_value_dup_object (value);
      GST_DEBUG ("setting socket to %p", udpsink->socket);
      break;
    case PROP_SOCKET_V6:
      if (udpsink->socket_v6 != NULL
          && udpsink->socket_v6 != udpsink->used_socket_v6
          && udpsink->close_socket) {
        GError *err = NULL;

        if (!g_socket_close (udpsink->socket_v6, &err)) {
          GST_ERROR ("failed to close socket %p: %s", udpsink->socket_v6,
              err->message);
          g_clear_error (&err);
        }
      }
      if (udpsink->socket_v6)
        g_object_unref (udpsink->socket_v6);
      udpsink->socket_v6 = g_value_dup_object (value);
      GST_DEBUG ("setting socket v6 to %p", udpsink->socket_v6);
      break;
    case PROP_CLOSE_SOCKET:
      udpsink->close_socket = g_value_get_boolean (value);
      break;
    case PROP_BIND_ADDRESS:
      g_free (udpsink->bind_address);
      udpsink->bind_address = g_value_dup_string (value);
      break;
    case PROP_BIND_PORT:
      udpsink->bind_port = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dynudpsink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstDynUDPSink *udpsink;

  udpsink = GST_DYNUDPSINK (object);

  switch (prop_id) {
    case PROP_SOCKET:
      g_value_set_object (value, udpsink->socket);
      break;
    case PROP_SOCKET_V6:
      g_value_set_object (value, udpsink->socket_v6);
      break;
    case PROP_CLOSE_SOCKET:
      g_value_set_boolean (value, udpsink->close_socket);
      break;
    case PROP_BIND_ADDRESS:
      g_value_set_string (value, udpsink->bind_address);
      break;
    case PROP_BIND_PORT:
      g_value_set_int (value, udpsink->bind_port);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dynudpsink_create_cancellable (GstDynUDPSink * sink)
{
  GPollFD pollfd;

  sink->cancellable = g_cancellable_new ();
  sink->made_cancel_fd = g_cancellable_make_pollfd (sink->cancellable, &pollfd);
}

static void
gst_dynudpsink_free_cancellable (GstDynUDPSink * sink)
{
  if (sink->made_cancel_fd) {
    g_cancellable_release_fd (sink->cancellable);
    sink->made_cancel_fd = FALSE;
  }
  g_object_unref (sink->cancellable);
  sink->cancellable = NULL;
}

/* create a socket for sending to remote machine */
static gboolean
gst_dynudpsink_start (GstBaseSink * bsink)
{
  GstDynUDPSink *udpsink;
  GError *err = NULL;

  udpsink = GST_DYNUDPSINK (bsink);

  gst_dynudpsink_create_cancellable (udpsink);

  udpsink->external_socket = FALSE;

  if (udpsink->socket) {
    if (g_socket_get_family (udpsink->socket) == G_SOCKET_FAMILY_IPV6) {
      udpsink->used_socket_v6 = G_SOCKET (g_object_ref (udpsink->socket));
      udpsink->external_socket = TRUE;
    } else {
      udpsink->used_socket = G_SOCKET (g_object_ref (udpsink->socket));
      udpsink->external_socket = TRUE;
    }
  }

  if (udpsink->socket_v6) {
    g_return_val_if_fail (g_socket_get_family (udpsink->socket) !=
        G_SOCKET_FAMILY_IPV6, FALSE);

    if (udpsink->used_socket_v6
        && udpsink->used_socket_v6 != udpsink->socket_v6) {
      GST_ERROR_OBJECT (udpsink,
          "Provided different IPv6 sockets in socket and socket-v6 properties");
      return FALSE;
    }

    udpsink->used_socket_v6 = G_SOCKET (g_object_ref (udpsink->socket_v6));
    udpsink->external_socket = TRUE;
  }

  if (!udpsink->used_socket && !udpsink->used_socket_v6) {
    GSocketAddress *bind_addr;
    GInetAddress *bind_iaddr;

    if (udpsink->bind_address) {
      GSocketFamily family;

      bind_iaddr = g_inet_address_new_from_string (udpsink->bind_address);
      if (!bind_iaddr) {
        GList *results;
        GResolver *resolver;

        resolver = g_resolver_get_default ();
        results =
            g_resolver_lookup_by_name (resolver, udpsink->bind_address,
            udpsink->cancellable, &err);
        if (!results) {
          g_object_unref (resolver);
          goto name_resolve;
        }
        bind_iaddr = G_INET_ADDRESS (g_object_ref (results->data));
        g_resolver_free_addresses (results);
        g_object_unref (resolver);
      }

      bind_addr = g_inet_socket_address_new (bind_iaddr, udpsink->bind_port);
      g_object_unref (bind_iaddr);
      family = g_socket_address_get_family (G_SOCKET_ADDRESS (bind_addr));

      if ((udpsink->used_socket =
              g_socket_new (family, G_SOCKET_TYPE_DATAGRAM,
                  G_SOCKET_PROTOCOL_UDP, &err)) == NULL) {
        g_object_unref (bind_addr);
        goto no_socket;
      }

      g_socket_bind (udpsink->used_socket, bind_addr, TRUE, &err);
      if (err != NULL)
        goto bind_error;
    } else {
      /* create sender sockets if none available */
      if ((udpsink->used_socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
                  G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &err)) == NULL)
        goto no_socket;

      bind_iaddr = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
      bind_addr = g_inet_socket_address_new (bind_iaddr, 0);
      g_socket_bind (udpsink->used_socket, bind_addr, TRUE, &err);
      g_object_unref (bind_addr);
      g_object_unref (bind_iaddr);
      if (err != NULL)
        goto bind_error;

      if ((udpsink->used_socket_v6 = g_socket_new (G_SOCKET_FAMILY_IPV6,
                  G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP,
                  &err)) == NULL) {
        GST_INFO_OBJECT (udpsink, "Failed to create IPv6 socket: %s",
            err->message);
        g_clear_error (&err);
      } else {
        bind_iaddr = g_inet_address_new_any (G_SOCKET_FAMILY_IPV6);
        bind_addr = g_inet_socket_address_new (bind_iaddr, 0);
        g_socket_bind (udpsink->used_socket_v6, bind_addr, TRUE, &err);
        g_object_unref (bind_addr);
        g_object_unref (bind_iaddr);
        if (err != NULL)
          goto bind_error;
      }
    }
  }

  if (udpsink->used_socket)
    g_socket_set_broadcast (udpsink->used_socket, TRUE);
  if (udpsink->used_socket_v6)
    g_socket_set_broadcast (udpsink->used_socket_v6, TRUE);

  return TRUE;

  /* ERRORS */
no_socket:
  {
    GST_ERROR_OBJECT (udpsink, "Failed to create IPv4 socket: %s",
        err->message);
    g_clear_error (&err);
    return FALSE;
  }
bind_error:
  {
    GST_ELEMENT_ERROR (udpsink, RESOURCE, FAILED, (NULL),
        ("Failed to bind socket: %s", err->message));
    g_clear_error (&err);
    return FALSE;
  }
name_resolve:
  {
    GST_ELEMENT_ERROR (udpsink, RESOURCE, FAILED, (NULL),
        ("Failed to resolve bind address %s: %s", udpsink->bind_address,
            err->message));
    g_clear_error (&err);
    return FALSE;
  }
}

static GstStructure *
gst_dynudpsink_get_stats (GstDynUDPSink * sink, const gchar * host, gint port)
{
  return NULL;
}

static gboolean
gst_dynudpsink_stop (GstBaseSink * bsink)
{
  GstDynUDPSink *udpsink;

  udpsink = GST_DYNUDPSINK (bsink);

  if (udpsink->used_socket) {
    if (udpsink->close_socket || !udpsink->external_socket) {
      GError *err = NULL;

      if (!g_socket_close (udpsink->used_socket, &err)) {
        GST_ERROR_OBJECT (udpsink, "Failed to close socket: %s", err->message);
        g_clear_error (&err);
      }
    }

    g_object_unref (udpsink->used_socket);
    udpsink->used_socket = NULL;
  }

  if (udpsink->used_socket_v6) {
    if (udpsink->close_socket || !udpsink->external_socket) {
      GError *err = NULL;

      if (!g_socket_close (udpsink->used_socket_v6, &err)) {
        GST_ERROR_OBJECT (udpsink, "Failed to close socket: %s", err->message);
        g_clear_error (&err);
      }
    }

    g_object_unref (udpsink->used_socket_v6);
    udpsink->used_socket_v6 = NULL;
  }

  gst_dynudpsink_free_cancellable (udpsink);

  return TRUE;
}

static gboolean
gst_dynudpsink_unlock (GstBaseSink * bsink)
{
  GstDynUDPSink *udpsink;

  udpsink = GST_DYNUDPSINK (bsink);

  g_cancellable_cancel (udpsink->cancellable);

  return TRUE;
}

static gboolean
gst_dynudpsink_unlock_stop (GstBaseSink * bsink)
{
  GstDynUDPSink *udpsink;

  udpsink = GST_DYNUDPSINK (bsink);

  gst_dynudpsink_free_cancellable (udpsink);
  gst_dynudpsink_create_cancellable (udpsink);

  return TRUE;
}
