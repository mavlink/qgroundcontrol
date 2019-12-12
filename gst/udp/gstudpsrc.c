/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim@fluendo.com>
 * Copyright (C) <2005> Nokia Corporation <kai.vehmanen@nokia.com>
 * Copyright (C) <2012> Collabora Ltd.
 *   Author: Sebastian Dröge <sebastian.droege@collabora.co.uk>
 * Copyright (C) 2014 Tim-Philipp Müller <tim@centricular.com>
 * Copyright (C) 2014 Centricular Ltd
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
 * SECTION:element-udpsrc
 * @title: udpsrc
 * @see_also: udpsink, multifdsink
 *
 * udpsrc is a network source that reads UDP packets from the network.
 * It can be combined with RTP depayloaders to implement RTP streaming.
 *
 * The udpsrc element supports automatic port allocation by setting the
 * #GstUDPSrc:port property to 0. After setting the udpsrc to PAUSED, the
 * allocated port can be obtained by reading the port property.
 *
 * udpsrc can read from multicast groups by setting the #GstUDPSrc:multicast-group
 * property to the IP address of the multicast group.
 *
 * Alternatively one can provide a custom socket to udpsrc with the #GstUDPSrc:socket
 * property, udpsrc will then not allocate a socket itself but use the provided
 * one.
 *
 * The #GstUDPSrc:caps property is mainly used to give a type to the UDP packet
 * so that they can be autoplugged in GStreamer pipelines. This is very useful
 * for RTP implementations where the contents of the UDP packets is transferred
 * out-of-bounds using SDP or other means.
 *
 * The #GstUDPSrc:buffer-size property is used to change the default kernel
 * buffersizes used for receiving packets. The buffer size may be increased for
 * high-volume connections, or may be decreased to limit the possible backlog of
 * incoming data. The system places an absolute limit on these values, on Linux,
 * for example, the default buffer size is typically 50K and can be increased to
 * maximally 100K.
 *
 * The #GstUDPSrc:skip-first-bytes property is used to strip off an arbitrary
 * number of bytes from the start of the raw udp packet and can be used to strip
 * off proprietary header, for example.
 *
 * The udpsrc is always a live source. It does however not provide a #GstClock,
 * this is left for downstream elements such as an RTP session manager or demuxer
 * (such as an MPEG demuxer). As with all live sources, the captured buffers
 * will have their timestamp set to the current running time of the pipeline.
 *
 * udpsrc implements a #GstURIHandler interface that handles udp://host:port
 * type URIs.
 *
 * If the #GstUDPSrc:timeout property is set to a value bigger than 0, udpsrc
 * will generate an element message named `GstUDPSrcTimeout`
 * if no data was received in the given timeout.
 * 
 * The message's structure contains one field:
 *
 * * #guint64 `timeout`: the timeout in microseconds that expired when waiting for data.
 *
 * The message is typically used to detect that no UDP arrives in the receiver
 * because it is blocked by a firewall.
 *
 * A custom file descriptor can be configured with the
 * #GstUDPSrc:socket property. The socket will be closed when setting
 * the element to READY by default. This behaviour can be overridden
 * with the #GstUDPSrc:close-socket property, in which case the
 * application is responsible for closing the file descriptor.
 *
 * ## Examples
 * |[
 * gst-launch-1.0 -v udpsrc ! fakesink dump=1
 * ]| A pipeline to read from the default port and dump the udp packets.
 * To actually generate udp packets on the default port one can use the
 * udpsink element. When running the following pipeline in another terminal, the
 * above mentioned pipeline should dump data packets to the console.
 * |[
 * gst-launch-1.0 -v audiotestsrc ! udpsink
 * ]|
 * |[
 * gst-launch-1.0 -v udpsrc port=0 ! fakesink
 * ]| read udp packets from a free port.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Needed to get struct in6_pktinfo.
 * Also all these have to be before glib.h is included as
 * otherwise struct in6_pktinfo is not defined completely
 * due to broken glibc headers */
#define _GNU_SOURCE
/* Needed for OSX/iOS to define the IPv6 variants */
#define __APPLE_USE_RFC_3542
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <string.h>
#include "gstudpsrc.h"

#include <gst/net/gstnetaddressmeta.h>

#include <gio/gnetworking.h>

/* Required for other parts of in_pktinfo / in6_pktinfo but only
 * on non-Windows and can be included after glib.h */
#ifndef G_PLATFORM_WIN32
#include <netinet/ip.h>
#endif

/* Control messages for getting the destination address */
#ifdef IP_PKTINFO
GType gst_ip_pktinfo_message_get_type (void);

#define GST_TYPE_IP_PKTINFO_MESSAGE         (gst_ip_pktinfo_message_get_type ())
#define GST_IP_PKTINFO_MESSAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_IP_PKTINFO_MESSAGE, GstIPPktinfoMessage))
#define GST_IP_PKTINFO_MESSAGE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GST_TYPE_IP_PKTINFO_MESSAGE, GstIPPktinfoMessageClass))
#define GST_IS_IP_PKTINFO_MESSAGE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_IP_PKTINFO_MESSAGE))
#define GST_IS_IP_PKTINFO_MESSAGE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GST_TYPE_IP_PKTINFO_MESSAGE))
#define GST_IP_PKTINFO_MESSAGE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_IP_PKTINFO_MESSAGE, GstIPPktinfoMessageClass))

typedef struct _GstIPPktinfoMessage GstIPPktinfoMessage;
typedef struct _GstIPPktinfoMessageClass GstIPPktinfoMessageClass;

struct _GstIPPktinfoMessageClass
{
  GSocketControlMessageClass parent_class;

};

struct _GstIPPktinfoMessage
{
  GSocketControlMessage parent;

  guint ifindex;
#ifndef G_PLATFORM_WIN32
#ifndef __NetBSD__
  struct in_addr spec_dst;
#endif
#endif
  struct in_addr addr;
};

G_DEFINE_TYPE (GstIPPktinfoMessage, gst_ip_pktinfo_message,
    G_TYPE_SOCKET_CONTROL_MESSAGE);

static gsize
gst_ip_pktinfo_message_get_size (GSocketControlMessage * message)
{
  return sizeof (struct in_pktinfo);
}

static int
gst_ip_pktinfo_message_get_level (GSocketControlMessage * message)
{
  return IPPROTO_IP;
}

static int
gst_ip_pktinfo_message_get_msg_type (GSocketControlMessage * message)
{
  return IP_PKTINFO;
}

static GSocketControlMessage *
gst_ip_pktinfo_message_deserialize (gint level,
    gint type, gsize size, gpointer data)
{
  struct in_pktinfo *pktinfo;
  GstIPPktinfoMessage *message;

  if (level != IPPROTO_IP || type != IP_PKTINFO)
    return NULL;

  if (size < sizeof (struct in_pktinfo))
    return NULL;

  pktinfo = data;

  message = g_object_new (GST_TYPE_IP_PKTINFO_MESSAGE, NULL);
  message->ifindex = pktinfo->ipi_ifindex;
#ifndef G_PLATFORM_WIN32
#ifndef __NetBSD__
  message->spec_dst = pktinfo->ipi_spec_dst;
#endif
#endif
  message->addr = pktinfo->ipi_addr;

  return G_SOCKET_CONTROL_MESSAGE (message);
}

static void
gst_ip_pktinfo_message_init (GstIPPktinfoMessage * message)
{
}

static void
gst_ip_pktinfo_message_class_init (GstIPPktinfoMessageClass * class)
{
  GSocketControlMessageClass *scm_class;

  scm_class = G_SOCKET_CONTROL_MESSAGE_CLASS (class);
  scm_class->get_size = gst_ip_pktinfo_message_get_size;
  scm_class->get_level = gst_ip_pktinfo_message_get_level;
  scm_class->get_type = gst_ip_pktinfo_message_get_msg_type;
  scm_class->deserialize = gst_ip_pktinfo_message_deserialize;
}
#endif

#ifdef IPV6_PKTINFO
GType gst_ipv6_pktinfo_message_get_type (void);

#define GST_TYPE_IPV6_PKTINFO_MESSAGE         (gst_ipv6_pktinfo_message_get_type ())
#define GST_IPV6_PKTINFO_MESSAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_IPV6_PKTINFO_MESSAGE, GstIPV6PktinfoMessage))
#define GST_IPV6_PKTINFO_MESSAGE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GST_TYPE_IPV6_PKTINFO_MESSAGE, GstIPV6PktinfoMessageClass))
#define GST_IS_IPV6_PKTINFO_MESSAGE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_IPV6_PKTINFO_MESSAGE))
#define GST_IS_IPV6_PKTINFO_MESSAGE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GST_TYPE_IPV6_PKTINFO_MESSAGE))
#define GST_IPV6_PKTINFO_MESSAGE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_IPV6_PKTINFO_MESSAGE, GstIPV6PktinfoMessageClass))

typedef struct _GstIPV6PktinfoMessage GstIPV6PktinfoMessage;
typedef struct _GstIPV6PktinfoMessageClass GstIPV6PktinfoMessageClass;

struct _GstIPV6PktinfoMessageClass
{
  GSocketControlMessageClass parent_class;

};

struct _GstIPV6PktinfoMessage
{
  GSocketControlMessage parent;

  guint ifindex;
  struct in6_addr addr;
};

G_DEFINE_TYPE (GstIPV6PktinfoMessage, gst_ipv6_pktinfo_message,
    G_TYPE_SOCKET_CONTROL_MESSAGE);

static gsize
gst_ipv6_pktinfo_message_get_size (GSocketControlMessage * message)
{
  return sizeof (struct in6_pktinfo);
}

static int
gst_ipv6_pktinfo_message_get_level (GSocketControlMessage * message)
{
  return IPPROTO_IPV6;
}

static int
gst_ipv6_pktinfo_message_get_msg_type (GSocketControlMessage * message)
{
  return IPV6_PKTINFO;
}

static GSocketControlMessage *
gst_ipv6_pktinfo_message_deserialize (gint level,
    gint type, gsize size, gpointer data)
{
  struct in6_pktinfo *pktinfo;
  GstIPV6PktinfoMessage *message;

  if (level != IPPROTO_IPV6 || type != IPV6_PKTINFO)
    return NULL;

  if (size < sizeof (struct in6_pktinfo))
    return NULL;

  pktinfo = data;

  message = g_object_new (GST_TYPE_IPV6_PKTINFO_MESSAGE, NULL);
  message->ifindex = pktinfo->ipi6_ifindex;
  message->addr = pktinfo->ipi6_addr;

  return G_SOCKET_CONTROL_MESSAGE (message);
}

static void
gst_ipv6_pktinfo_message_init (GstIPV6PktinfoMessage * message)
{
}

static void
gst_ipv6_pktinfo_message_class_init (GstIPV6PktinfoMessageClass * class)
{
  GSocketControlMessageClass *scm_class;

  scm_class = G_SOCKET_CONTROL_MESSAGE_CLASS (class);
  scm_class->get_size = gst_ipv6_pktinfo_message_get_size;
  scm_class->get_level = gst_ipv6_pktinfo_message_get_level;
  scm_class->get_type = gst_ipv6_pktinfo_message_get_msg_type;
  scm_class->deserialize = gst_ipv6_pktinfo_message_deserialize;
}

#endif

#ifdef IP_RECVDSTADDR
GType gst_ip_recvdstaddr_message_get_type (void);

#define GST_TYPE_IP_RECVDSTADDR_MESSAGE         (gst_ip_recvdstaddr_message_get_type ())
#define GST_IP_RECVDSTADDR_MESSAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_IP_RECVDSTADDR_MESSAGE, GstIPRecvdstaddrMessage))
#define GST_IP_RECVDSTADDR_MESSAGE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GST_TYPE_IP_RECVDSTADDR_MESSAGE, GstIPRecvdstaddrMessageClass))
#define GST_IS_IP_RECVDSTADDR_MESSAGE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_IP_RECVDSTADDR_MESSAGE))
#define GST_IS_IP_RECVDSTADDR_MESSAGE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GST_TYPE_IP_RECVDSTADDR_MESSAGE))
#define GST_IP_RECVDSTADDR_MESSAGE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_IP_RECVDSTADDR_MESSAGE, GstIPRecvdstaddrMessageClass))

typedef struct _GstIPRecvdstaddrMessage GstIPRecvdstaddrMessage;
typedef struct _GstIPRecvdstaddrMessageClass GstIPRecvdstaddrMessageClass;

struct _GstIPRecvdstaddrMessageClass
{
  GSocketControlMessageClass parent_class;

};

struct _GstIPRecvdstaddrMessage
{
  GSocketControlMessage parent;

  guint ifindex;
  struct in_addr addr;
};

G_DEFINE_TYPE (GstIPRecvdstaddrMessage, gst_ip_recvdstaddr_message,
    G_TYPE_SOCKET_CONTROL_MESSAGE);

static gsize
gst_ip_recvdstaddr_message_get_size (GSocketControlMessage * message)
{
  return sizeof (struct in_addr);
}

static int
gst_ip_recvdstaddr_message_get_level (GSocketControlMessage * message)
{
  return IPPROTO_IP;
}

static int
gst_ip_recvdstaddr_message_get_msg_type (GSocketControlMessage * message)
{
  return IP_RECVDSTADDR;
}

static GSocketControlMessage *
gst_ip_recvdstaddr_message_deserialize (gint level,
    gint type, gsize size, gpointer data)
{
  struct in_addr *addr;
  GstIPRecvdstaddrMessage *message;

  if (level != IPPROTO_IP || type != IP_RECVDSTADDR)
    return NULL;

  if (size < sizeof (struct in_addr))
    return NULL;

  addr = data;

  message = g_object_new (GST_TYPE_IP_RECVDSTADDR_MESSAGE, NULL);
  message->addr = *addr;

  return G_SOCKET_CONTROL_MESSAGE (message);
}

static void
gst_ip_recvdstaddr_message_init (GstIPRecvdstaddrMessage * message)
{
}

static void
gst_ip_recvdstaddr_message_class_init (GstIPRecvdstaddrMessageClass * class)
{
  GSocketControlMessageClass *scm_class;

  scm_class = G_SOCKET_CONTROL_MESSAGE_CLASS (class);
  scm_class->get_size = gst_ip_recvdstaddr_message_get_size;
  scm_class->get_level = gst_ip_recvdstaddr_message_get_level;
  scm_class->get_type = gst_ip_recvdstaddr_message_get_msg_type;
  scm_class->deserialize = gst_ip_recvdstaddr_message_deserialize;
}
#endif

static gboolean
gst_udpsrc_decide_allocation (GstBaseSrc * bsrc, GstQuery * query)
{
  GstUDPSrc *udpsrc;
  GstBufferPool *pool;
  gboolean update;
  GstStructure *config;
  GstCaps *caps = NULL;

  udpsrc = GST_UDPSRC (bsrc);

  if (gst_query_get_n_allocation_pools (query) > 0) {
    update = TRUE;
  } else {
    update = FALSE;
  }

  pool = gst_buffer_pool_new ();

  config = gst_buffer_pool_get_config (pool);

  gst_query_parse_allocation (query, &caps, NULL);

  gst_buffer_pool_config_set_params (config, caps, udpsrc->mtu, 0, 0);

  gst_buffer_pool_set_config (pool, config);

  if (update)
    gst_query_set_nth_allocation_pool (query, 0, pool, udpsrc->mtu, 0, 0);
  else
    gst_query_add_allocation_pool (query, pool, udpsrc->mtu, 0, 0);

  gst_object_unref (pool);

  return TRUE;
}

/* not 100% correct, but a good upper bound for memory allocation purposes */
#define MAX_IPV4_UDP_PACKET_SIZE (65536 - 8)

GST_DEBUG_CATEGORY_STATIC (udpsrc_debug);
#define GST_CAT_DEFAULT (udpsrc_debug)

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

#define UDP_DEFAULT_PORT                5004
#define UDP_DEFAULT_MULTICAST_GROUP     "0.0.0.0"
#define UDP_DEFAULT_MULTICAST_IFACE     NULL
#define UDP_DEFAULT_URI                 "udp://"UDP_DEFAULT_MULTICAST_GROUP":"G_STRINGIFY(UDP_DEFAULT_PORT)
#define UDP_DEFAULT_CAPS                NULL
#define UDP_DEFAULT_SOCKET              NULL
#define UDP_DEFAULT_BUFFER_SIZE		0
#define UDP_DEFAULT_TIMEOUT             0
#define UDP_DEFAULT_SKIP_FIRST_BYTES	0
#define UDP_DEFAULT_CLOSE_SOCKET       TRUE
#define UDP_DEFAULT_USED_SOCKET        NULL
#define UDP_DEFAULT_AUTO_MULTICAST     TRUE
#define UDP_DEFAULT_REUSE              TRUE
#define UDP_DEFAULT_LOOP               TRUE
#define UDP_DEFAULT_RETRIEVE_SENDER_ADDRESS TRUE
#define UDP_DEFAULT_MTU                (1492)

enum
{
  PROP_0,

  PROP_PORT,
  PROP_MULTICAST_GROUP,
  PROP_MULTICAST_IFACE,
  PROP_URI,
  PROP_CAPS,
  PROP_SOCKET,
  PROP_BUFFER_SIZE,
  PROP_TIMEOUT,
  PROP_SKIP_FIRST_BYTES,
  PROP_CLOSE_SOCKET,
  PROP_USED_SOCKET,
  PROP_AUTO_MULTICAST,
  PROP_REUSE,
  PROP_ADDRESS,
  PROP_LOOP,
  PROP_RETRIEVE_SENDER_ADDRESS,
  PROP_MTU,
};

static void gst_udpsrc_uri_handler_init (gpointer g_iface, gpointer iface_data);

static GstCaps *gst_udpsrc_getcaps (GstBaseSrc * src, GstCaps * filter);
static gboolean gst_udpsrc_close (GstUDPSrc * src);
static gboolean gst_udpsrc_unlock (GstBaseSrc * bsrc);
static gboolean gst_udpsrc_unlock_stop (GstBaseSrc * bsrc);
static GstFlowReturn gst_udpsrc_fill (GstPushSrc * psrc, GstBuffer * outbuf);

static void gst_udpsrc_finalize (GObject * object);

static void gst_udpsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_udpsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn gst_udpsrc_change_state (GstElement * element,
    GstStateChange transition);

#define gst_udpsrc_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstUDPSrc, gst_udpsrc, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER, gst_udpsrc_uri_handler_init));

static void
gst_udpsrc_class_init (GstUDPSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (udpsrc_debug, "udpsrc", 0, "UDP src");

#ifdef IP_PKTINFO
  GST_TYPE_IP_PKTINFO_MESSAGE;
#endif
#ifdef IPV6_PKTINFO
  GST_TYPE_IPV6_PKTINFO_MESSAGE;
#endif
#ifdef IP_RECVDSTADDR
  GST_TYPE_IP_RECVDSTADDR_MESSAGE;
#endif

  gobject_class->set_property = gst_udpsrc_set_property;
  gobject_class->get_property = gst_udpsrc_get_property;
  gobject_class->finalize = gst_udpsrc_finalize;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PORT,
      g_param_spec_int ("port", "Port",
          "The port to receive the packets from, 0=allocate", 0, G_MAXUINT16,
          UDP_DEFAULT_PORT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /* FIXME 2.0: Remove multicast-group property */
#ifndef GST_REMOVE_DEPRECATED
  g_object_class_install_property (gobject_class, PROP_MULTICAST_GROUP,
      g_param_spec_string ("multicast-group", "Multicast Group",
          "The Address of multicast group to join. (DEPRECATED: "
          "Use address property instead)", UDP_DEFAULT_MULTICAST_GROUP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));
#endif
  g_object_class_install_property (gobject_class, PROP_MULTICAST_IFACE,
      g_param_spec_string ("multicast-iface", "Multicast Interface",
          "The network interface on which to join the multicast group."
          "This allows multiple interfaces separated by comma. (\"eth0,eth1\")",
          UDP_DEFAULT_MULTICAST_IFACE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_URI,
      g_param_spec_string ("uri", "URI",
          "URI in the form of udp://multicast_group:port", UDP_DEFAULT_URI,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CAPS,
      g_param_spec_boxed ("caps", "Caps",
          "The caps of the source pad", GST_TYPE_CAPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SOCKET,
      g_param_spec_object ("socket", "Socket",
          "Socket to use for UDP reception. (NULL == allocate)",
          G_TYPE_SOCKET, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BUFFER_SIZE,
      g_param_spec_int ("buffer-size", "Buffer Size",
          "Size of the kernel receive buffer in bytes, 0=default", 0, G_MAXINT,
          UDP_DEFAULT_BUFFER_SIZE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TIMEOUT,
      g_param_spec_uint64 ("timeout", "Timeout",
          "Post a message after timeout nanoseconds (0 = disabled)", 0,
          G_MAXUINT64, UDP_DEFAULT_TIMEOUT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_SKIP_FIRST_BYTES, g_param_spec_int ("skip-first-bytes",
          "Skip first bytes", "number of bytes to skip for each udp packet", 0,
          G_MAXINT, UDP_DEFAULT_SKIP_FIRST_BYTES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CLOSE_SOCKET,
      g_param_spec_boolean ("close-socket", "Close socket",
          "Close socket if passed as property on state change",
          UDP_DEFAULT_CLOSE_SOCKET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_USED_SOCKET,
      g_param_spec_object ("used-socket", "Socket Handle",
          "Socket currently in use for UDP reception. (NULL = no socket)",
          G_TYPE_SOCKET, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_AUTO_MULTICAST,
      g_param_spec_boolean ("auto-multicast", "Auto Multicast",
          "Automatically join/leave multicast groups",
          UDP_DEFAULT_AUTO_MULTICAST,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_REUSE,
      g_param_spec_boolean ("reuse", "Reuse", "Enable reuse of the port",
          UDP_DEFAULT_REUSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_ADDRESS,
      g_param_spec_string ("address", "Address",
          "Address to receive packets for. This is equivalent to the "
          "multicast-group property for now", UDP_DEFAULT_MULTICAST_GROUP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstUDPSrc::loop:
   *
   * Can be used to disable multicast loopback.
   *
   * Since: 1.8
   */
  g_object_class_install_property (gobject_class, PROP_LOOP,
      g_param_spec_boolean ("loop", "Multicast Loopback",
          "Used for setting the multicast loop parameter. TRUE = enable,"
          " FALSE = disable", UDP_DEFAULT_LOOP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstUDPSrc::retrieve-sender-address:
   *
   * Whether to retrieve the sender address and add it to the buffers as
   * meta. Disabling this might result in minor performance improvements
   * in certain scenarios.
   *
   * Since: 1.10
   */
  g_object_class_install_property (gobject_class, PROP_RETRIEVE_SENDER_ADDRESS,
      g_param_spec_boolean ("retrieve-sender-address",
          "Retrieve Sender Address",
          "Whether to retrieve the sender address and add it to buffers as "
          "meta. Disabling this might result in minor performance improvements "
          "in certain scenarios", UDP_DEFAULT_RETRIEVE_SENDER_ADDRESS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstUDPSrc::mtu:
   *
   * Maximum expected packet size. This directly defines the allocation
   * size of the receive buffer pool.
   *
   * In case more data is received, a new #GstMemory is appended to the
   * output buffer, ensuring no data is lost, this however leads to that
   * buffer being freed and reallocated.
   *
   * Since: 1.14
   */
  g_object_class_install_property (gobject_class, PROP_MTU,
      g_param_spec_uint ("mtu", "Expected Maximum Transmission Unit",
          "Maximum expected packet size. This directly defines the allocation"
          "size of the receive buffer pool.",
          0, G_MAXINT, UDP_DEFAULT_MTU,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &src_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "UDP packet receiver", "Source/Network",
      "Receive data over the network via UDP",
      "Wim Taymans <wim@fluendo.com>, "
      "Thijs Vermeir <thijs.vermeir@barco.com>");

  gstelement_class->change_state = gst_udpsrc_change_state;

  gstbasesrc_class->unlock = gst_udpsrc_unlock;
  gstbasesrc_class->unlock_stop = gst_udpsrc_unlock_stop;
  gstbasesrc_class->get_caps = gst_udpsrc_getcaps;
  gstbasesrc_class->decide_allocation = gst_udpsrc_decide_allocation;

  gstpushsrc_class->fill = gst_udpsrc_fill;
}

static void
gst_udpsrc_init (GstUDPSrc * udpsrc)
{
  udpsrc->uri =
      g_strdup_printf ("udp://%s:%u", UDP_DEFAULT_MULTICAST_GROUP,
      UDP_DEFAULT_PORT);

  udpsrc->address = g_strdup (UDP_DEFAULT_MULTICAST_GROUP);
  udpsrc->port = UDP_DEFAULT_PORT;
  udpsrc->socket = UDP_DEFAULT_SOCKET;
  udpsrc->multi_iface = g_strdup (UDP_DEFAULT_MULTICAST_IFACE);
  udpsrc->buffer_size = UDP_DEFAULT_BUFFER_SIZE;
  udpsrc->timeout = UDP_DEFAULT_TIMEOUT;
  udpsrc->skip_first_bytes = UDP_DEFAULT_SKIP_FIRST_BYTES;
  udpsrc->close_socket = UDP_DEFAULT_CLOSE_SOCKET;
  udpsrc->external_socket = (udpsrc->socket != NULL);
  udpsrc->auto_multicast = UDP_DEFAULT_AUTO_MULTICAST;
  udpsrc->used_socket = UDP_DEFAULT_USED_SOCKET;
  udpsrc->reuse = UDP_DEFAULT_REUSE;
  udpsrc->loop = UDP_DEFAULT_LOOP;
  udpsrc->retrieve_sender_address = UDP_DEFAULT_RETRIEVE_SENDER_ADDRESS;
  udpsrc->mtu = UDP_DEFAULT_MTU;

  /* configure basesrc to be a live source */
  gst_base_src_set_live (GST_BASE_SRC (udpsrc), TRUE);
  /* make basesrc output a segment in time */
  gst_base_src_set_format (GST_BASE_SRC (udpsrc), GST_FORMAT_TIME);
  /* make basesrc set timestamps on outgoing buffers based on the running_time
   * when they were captured */
  gst_base_src_set_do_timestamp (GST_BASE_SRC (udpsrc), TRUE);
}

static void
gst_udpsrc_finalize (GObject * object)
{
  GstUDPSrc *udpsrc;

  udpsrc = GST_UDPSRC (object);

  if (udpsrc->caps)
    gst_caps_unref (udpsrc->caps);
  udpsrc->caps = NULL;

  g_free (udpsrc->multi_iface);
  udpsrc->multi_iface = NULL;

  g_free (udpsrc->uri);
  udpsrc->uri = NULL;

  g_free (udpsrc->address);
  udpsrc->address = NULL;

  if (udpsrc->socket)
    g_object_unref (udpsrc->socket);
  udpsrc->socket = NULL;

  if (udpsrc->used_socket)
    g_object_unref (udpsrc->used_socket);
  udpsrc->used_socket = NULL;

  if (udpsrc->extra_mem)
    gst_memory_unref (udpsrc->extra_mem);
  udpsrc->extra_mem = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstCaps *
gst_udpsrc_getcaps (GstBaseSrc * src, GstCaps * filter)
{
  GstUDPSrc *udpsrc;
  GstCaps *caps, *result;

  udpsrc = GST_UDPSRC (src);

  GST_OBJECT_LOCK (src);
  if ((caps = udpsrc->caps))
    gst_caps_ref (caps);
  GST_OBJECT_UNLOCK (src);

  if (caps) {
    if (filter) {
      result = gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
      gst_caps_unref (caps);
    } else {
      result = caps;
    }
  } else {
    result = (filter) ? gst_caps_ref (filter) : gst_caps_new_any ();
  }
  return result;
}

static void
gst_udpsrc_create_cancellable (GstUDPSrc * src)
{
  GPollFD pollfd;

  src->cancellable = g_cancellable_new ();
  src->made_cancel_fd = g_cancellable_make_pollfd (src->cancellable, &pollfd);
}

static void
gst_udpsrc_free_cancellable (GstUDPSrc * src)
{
  if (src->made_cancel_fd) {
    g_cancellable_release_fd (src->cancellable);
    src->made_cancel_fd = FALSE;
  }
  g_object_unref (src->cancellable);
  src->cancellable = NULL;
}

static GstFlowReturn
gst_udpsrc_fill (GstPushSrc * psrc, GstBuffer * outbuf)
{
  GstUDPSrc *udpsrc;
  GSocketAddress *saddr = NULL;
  GSocketAddress **p_saddr;
  gint flags = G_SOCKET_MSG_NONE;
  gboolean try_again;
  GError *err = NULL;
  gssize res;
  gsize offset;
  GSocketControlMessage **msgs = NULL;
  GSocketControlMessage ***p_msgs;
  gint n_msgs = 0, i;
  GstMapInfo info;
  GstMapInfo extra_info;
  GInputVector ivec[2];

  udpsrc = GST_UDPSRC_CAST (psrc);

  /* optimization: use messages only in multicast mode and
   * if we can't let the kernel do the filtering for us */
  p_msgs =
      (g_inet_address_get_is_multicast (g_inet_socket_address_get_address
          (udpsrc->addr))) ? &msgs : NULL;
#ifdef IP_MULTICAST_ALL
  if (g_inet_address_get_family (g_inet_socket_address_get_address
          (udpsrc->addr)) == G_SOCKET_FAMILY_IPV4)
    p_msgs = NULL;
#endif

  /* Retrieve sender address unless we've been configured not to do so */
  p_saddr = (udpsrc->retrieve_sender_address) ? &saddr : NULL;

  if (!gst_buffer_map (outbuf, &info, GST_MAP_READWRITE))
    goto buffer_map_error;

  ivec[0].buffer = info.data;
  ivec[0].size = info.size;

  /* Prepare memory in case the data size exceeds mtu */
  if (udpsrc->extra_mem == NULL) {
    GstBufferPool *pool;
    GstStructure *config;
    GstAllocator *allocator = NULL;
    GstAllocationParams params;

    pool = gst_base_src_get_buffer_pool (GST_BASE_SRC_CAST (psrc));
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_allocator (config, &allocator, &params);

    udpsrc->extra_mem =
        gst_allocator_alloc (allocator, MAX_IPV4_UDP_PACKET_SIZE, &params);

    gst_object_unref (pool);
    gst_structure_free (config);
    if (allocator)
      gst_object_unref (allocator);
  }

  if (!gst_memory_map (udpsrc->extra_mem, &extra_info, GST_MAP_READWRITE))
    goto memory_map_error;

  ivec[1].buffer = extra_info.data;
  ivec[1].size = extra_info.size;

retry:
  if (saddr != NULL) {
    g_object_unref (saddr);
    saddr = NULL;
  }

  do {
    gint64 timeout;

    try_again = FALSE;

    if (udpsrc->timeout)
      timeout = udpsrc->timeout / 1000;
    else
      timeout = -1;

    GST_LOG_OBJECT (udpsrc, "doing select, timeout %" G_GINT64_FORMAT, timeout);

    if (!g_socket_condition_timed_wait (udpsrc->used_socket, G_IO_IN | G_IO_PRI,
            timeout, udpsrc->cancellable, &err)) {
      if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_BUSY)
          || g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
        goto stopped;
      } else if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_TIMED_OUT)) {
        g_clear_error (&err);
        /* timeout, post element message */
        gst_element_post_message (GST_ELEMENT_CAST (udpsrc),
            gst_message_new_element (GST_OBJECT_CAST (udpsrc),
                gst_structure_new ("GstUDPSrcTimeout",
                    "timeout", G_TYPE_UINT64, udpsrc->timeout, NULL)));
      } else {
        goto select_error;
      }

      try_again = TRUE;
    }
  } while (G_UNLIKELY (try_again));

  res =
      g_socket_receive_message (udpsrc->used_socket, p_saddr, ivec, 2,
      p_msgs, &n_msgs, &flags, udpsrc->cancellable, &err);

  if (G_UNLIKELY (res < 0)) {
    /* G_IO_ERROR_HOST_UNREACHABLE for a UDP socket means that a packet sent
     * with udpsink generated a "port unreachable" ICMP response. We ignore
     * that and try again.
     * On Windows we get G_IO_ERROR_CONNECTION_CLOSED instead */
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_HOST_UNREACHABLE) ||
        g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED)) {
      g_clear_error (&err);
      goto retry;
    }
    goto receive_error;
  }

  /* Retry if multicast and the destination address is not ours. We don't want
   * to receive arbitrary packets */
  if (p_msgs) {
    GInetAddress *iaddr = g_inet_socket_address_get_address (udpsrc->addr);
    gboolean skip_packet = FALSE;
    gsize iaddr_size = g_inet_address_get_native_size (iaddr);
    const guint8 *iaddr_bytes = g_inet_address_to_bytes (iaddr);

    for (i = 0; i < n_msgs && !skip_packet; i++) {
#ifdef IP_PKTINFO
      if (GST_IS_IP_PKTINFO_MESSAGE (msgs[i])) {
        GstIPPktinfoMessage *msg = GST_IP_PKTINFO_MESSAGE (msgs[i]);

        if (sizeof (msg->addr) == iaddr_size
            && memcmp (iaddr_bytes, &msg->addr, sizeof (msg->addr)))
          skip_packet = TRUE;
      }
#endif
#ifdef IPV6_PKTINFO
      if (GST_IS_IPV6_PKTINFO_MESSAGE (msgs[i])) {
        GstIPV6PktinfoMessage *msg = GST_IPV6_PKTINFO_MESSAGE (msgs[i]);

        if (sizeof (msg->addr) == iaddr_size
            && memcmp (iaddr_bytes, &msg->addr, sizeof (msg->addr)))
          skip_packet = TRUE;
      }
#endif
#ifdef IP_RECVDSTADDR
      if (GST_IS_IP_RECVDSTADDR_MESSAGE (msgs[i])) {
        GstIPRecvdstaddrMessage *msg = GST_IP_RECVDSTADDR_MESSAGE (msgs[i]);

        if (sizeof (msg->addr) == iaddr_size
            && memcmp (iaddr_bytes, &msg->addr, sizeof (msg->addr)))
          skip_packet = TRUE;
      }
#endif
    }

    for (i = 0; i < n_msgs; i++) {
      g_object_unref (msgs[i]);
    }
    g_free (msgs);

    if (skip_packet) {
      GST_DEBUG_OBJECT (udpsrc,
          "Dropping packet for a different multicast address");
      goto retry;
    }
  }

  gst_buffer_unmap (outbuf, &info);
  gst_memory_unmap (udpsrc->extra_mem, &extra_info);

  /* If this is the case, the buffer will be freed once unreffed,
   * and the buffer pool will have to reallocate a new one.
   */
  if (res > udpsrc->mtu) {
    gst_buffer_append_memory (outbuf, udpsrc->extra_mem);
    udpsrc->extra_mem = NULL;
  }

  offset = udpsrc->skip_first_bytes;

  if (G_UNLIKELY (offset > 0 && res < offset))
    goto skip_error;

  gst_buffer_resize (outbuf, offset, res - offset);

  /* use buffer metadata so receivers can also track the address */
  if (saddr) {
    gst_buffer_add_net_address_meta (outbuf, saddr);
    g_object_unref (saddr);
    saddr = NULL;
  }

  GST_LOG_OBJECT (udpsrc, "read packet of %d bytes", (int) res);

  return GST_FLOW_OK;

  /* ERRORS */
buffer_map_error:
  {
    GST_ELEMENT_ERROR (udpsrc, RESOURCE, READ, (NULL),
        ("Failed to map memory"));
    return GST_FLOW_ERROR;
  }
memory_map_error:
  {
    gst_buffer_unmap (outbuf, &info);
    GST_ELEMENT_ERROR (udpsrc, RESOURCE, READ, (NULL),
        ("Failed to map memory"));
    return GST_FLOW_ERROR;
  }
select_error:
  {
    gst_buffer_unmap (outbuf, &info);
    gst_memory_unmap (udpsrc->extra_mem, &extra_info);
    GST_ELEMENT_ERROR (udpsrc, RESOURCE, READ, (NULL),
        ("select error: %s", err->message));
    g_clear_error (&err);
    return GST_FLOW_ERROR;
  }
stopped:
  {
    gst_buffer_unmap (outbuf, &info);
    gst_memory_unmap (udpsrc->extra_mem, &extra_info);
    GST_DEBUG ("stop called");
    g_clear_error (&err);
    return GST_FLOW_FLUSHING;
  }
receive_error:
  {
    gst_buffer_unmap (outbuf, &info);
    gst_memory_unmap (udpsrc->extra_mem, &extra_info);
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_BUSY) ||
        g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_clear_error (&err);
      return GST_FLOW_FLUSHING;
    } else {
      GST_ELEMENT_ERROR (udpsrc, RESOURCE, READ, (NULL),
          ("receive error %" G_GSSIZE_FORMAT ": %s", res, err->message));
      g_clear_error (&err);
      return GST_FLOW_ERROR;
    }
  }
skip_error:
  {
    GST_ELEMENT_ERROR (udpsrc, STREAM, DECODE, (NULL),
        ("UDP buffer to small to skip header"));
    return GST_FLOW_ERROR;
  }
}

static gboolean
gst_udpsrc_set_uri (GstUDPSrc * src, const gchar * uri, GError ** error)
{
  gchar *address;
  guint16 port;

  if (!gst_udp_parse_uri (uri, &address, &port))
    goto wrong_uri;

  if (port == (guint16) - 1)
    port = UDP_DEFAULT_PORT;

  g_free (src->address);
  src->address = address;
  src->port = port;

  g_free (src->uri);
  src->uri = g_strdup (uri);

  return TRUE;

  /* ERRORS */
wrong_uri:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("error parsing uri %s", uri));
    g_set_error_literal (error, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
        "Could not parse UDP URI");
    return FALSE;
  }
}

static void
gst_udpsrc_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstUDPSrc *udpsrc = GST_UDPSRC (object);

  switch (prop_id) {
    case PROP_BUFFER_SIZE:
      udpsrc->buffer_size = g_value_get_int (value);
      break;
    case PROP_PORT:
      udpsrc->port = g_value_get_int (value);
      g_free (udpsrc->uri);
      udpsrc->uri =
          g_strdup_printf ("udp://%s:%u", udpsrc->address, udpsrc->port);
      break;
    case PROP_MULTICAST_GROUP:
    case PROP_ADDRESS:
    {
      const gchar *group;

      g_free (udpsrc->address);
      if ((group = g_value_get_string (value)))
        udpsrc->address = g_strdup (group);
      else
        udpsrc->address = g_strdup (UDP_DEFAULT_MULTICAST_GROUP);

      g_free (udpsrc->uri);
      udpsrc->uri =
          g_strdup_printf ("udp://%s:%u", udpsrc->address, udpsrc->port);
      break;
    }
    case PROP_MULTICAST_IFACE:
      g_free (udpsrc->multi_iface);

      if (g_value_get_string (value) == NULL)
        udpsrc->multi_iface = g_strdup (UDP_DEFAULT_MULTICAST_IFACE);
      else
        udpsrc->multi_iface = g_value_dup_string (value);
      break;
    case PROP_URI:
      gst_udpsrc_set_uri (udpsrc, g_value_get_string (value), NULL);
      break;
    case PROP_CAPS:
    {
      const GstCaps *new_caps_val = gst_value_get_caps (value);
      GstCaps *new_caps;
      GstCaps *old_caps;

      if (new_caps_val == NULL) {
        new_caps = gst_caps_new_any ();
      } else {
        new_caps = gst_caps_copy (new_caps_val);
      }

      GST_OBJECT_LOCK (udpsrc);
      old_caps = udpsrc->caps;
      udpsrc->caps = new_caps;
      GST_OBJECT_UNLOCK (udpsrc);
      if (old_caps)
        gst_caps_unref (old_caps);

      gst_pad_mark_reconfigure (GST_BASE_SRC_PAD (udpsrc));
      break;
    }
    case PROP_SOCKET:
      if (udpsrc->socket != NULL && udpsrc->socket != udpsrc->used_socket &&
          udpsrc->close_socket) {
        GError *err = NULL;

        if (!g_socket_close (udpsrc->socket, &err)) {
          GST_ERROR ("failed to close socket %p: %s", udpsrc->socket,
              err->message);
          g_clear_error (&err);
        }
      }
      if (udpsrc->socket)
        g_object_unref (udpsrc->socket);
      udpsrc->socket = g_value_dup_object (value);
      GST_DEBUG ("setting socket to %p", udpsrc->socket);
      break;
    case PROP_TIMEOUT:
      udpsrc->timeout = g_value_get_uint64 (value);
      break;
    case PROP_SKIP_FIRST_BYTES:
      udpsrc->skip_first_bytes = g_value_get_int (value);
      break;
    case PROP_CLOSE_SOCKET:
      udpsrc->close_socket = g_value_get_boolean (value);
      break;
    case PROP_AUTO_MULTICAST:
      udpsrc->auto_multicast = g_value_get_boolean (value);
      break;
    case PROP_REUSE:
      udpsrc->reuse = g_value_get_boolean (value);
      break;
    case PROP_LOOP:
      udpsrc->loop = g_value_get_boolean (value);
      break;
    case PROP_RETRIEVE_SENDER_ADDRESS:
      udpsrc->retrieve_sender_address = g_value_get_boolean (value);
      break;
    case PROP_MTU:
      udpsrc->mtu = g_value_get_uint (value);
      break;
    default:
      break;
  }
}

static void
gst_udpsrc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstUDPSrc *udpsrc = GST_UDPSRC (object);

  switch (prop_id) {
    case PROP_BUFFER_SIZE:
      g_value_set_int (value, udpsrc->buffer_size);
      break;
    case PROP_PORT:
      g_value_set_int (value, udpsrc->port);
      break;
    case PROP_MULTICAST_GROUP:
    case PROP_ADDRESS:
      g_value_set_string (value, udpsrc->address);
      break;
    case PROP_MULTICAST_IFACE:
      g_value_set_string (value, udpsrc->multi_iface);
      break;
    case PROP_URI:
      g_value_set_string (value, udpsrc->uri);
      break;
    case PROP_CAPS:
      GST_OBJECT_LOCK (udpsrc);
      gst_value_set_caps (value, udpsrc->caps);
      GST_OBJECT_UNLOCK (udpsrc);
      break;
    case PROP_SOCKET:
      g_value_set_object (value, udpsrc->socket);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint64 (value, udpsrc->timeout);
      break;
    case PROP_SKIP_FIRST_BYTES:
      g_value_set_int (value, udpsrc->skip_first_bytes);
      break;
    case PROP_CLOSE_SOCKET:
      g_value_set_boolean (value, udpsrc->close_socket);
      break;
    case PROP_USED_SOCKET:
      g_value_set_object (value, udpsrc->used_socket);
      break;
    case PROP_AUTO_MULTICAST:
      g_value_set_boolean (value, udpsrc->auto_multicast);
      break;
    case PROP_REUSE:
      g_value_set_boolean (value, udpsrc->reuse);
      break;
    case PROP_LOOP:
      g_value_set_boolean (value, udpsrc->loop);
      break;
    case PROP_RETRIEVE_SENDER_ADDRESS:
      g_value_set_boolean (value, udpsrc->retrieve_sender_address);
      break;
    case PROP_MTU:
      g_value_set_uint (value, udpsrc->mtu);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GInetAddress *
gst_udpsrc_resolve (GstUDPSrc * src, const gchar * address)
{
  GInetAddress *addr;
  GError *err = NULL;
  GResolver *resolver;

  addr = g_inet_address_new_from_string (address);
  if (!addr) {
    GList *results;

    GST_DEBUG_OBJECT (src, "resolving IP address for host %s", address);
    resolver = g_resolver_get_default ();
    results =
        g_resolver_lookup_by_name (resolver, address, src->cancellable, &err);
    if (!results)
      goto name_resolve;
    addr = G_INET_ADDRESS (g_object_ref (results->data));

    g_resolver_free_addresses (results);
    g_object_unref (resolver);
  }
#ifndef GST_DISABLE_GST_DEBUG
  {
    gchar *ip = g_inet_address_to_string (addr);

    GST_DEBUG_OBJECT (src, "IP address for host %s is %s", address, ip);
    g_free (ip);
  }
#endif

  return addr;

name_resolve:
  {
    GST_WARNING_OBJECT (src, "Failed to resolve %s: %s", address, err->message);
    g_clear_error (&err);
    g_object_unref (resolver);
    return NULL;
  }
}

static gint
gst_udpsrc_get_rcvbuf (GstUDPSrc * src)
{
  gint val = 0;

  /* read the value of the receive buffer. Note that on linux this returns
   * 2x the value we set because the kernel allocates extra memory for
   * metadata. The default on Linux is about 100K (which is about 50K
   * without metadata) */
  if (!g_socket_get_option (src->used_socket, SOL_SOCKET, SO_RCVBUF, &val,
          NULL)) {
    GST_DEBUG_OBJECT (src, "could not get udp buffer size");
    return 0;
  }
#ifdef __linux__
  /* Devise by 2 so that the numbers matches when we do get/set */
  val /= 2;
#endif

  return val;
}

/* create a socket for sending to remote machine */
static gboolean
gst_udpsrc_open (GstUDPSrc * src)
{
  GInetAddress *addr, *bind_addr;
  GSocketAddress *bind_saddr;
  GError *err = NULL;

  gst_udpsrc_create_cancellable (src);

  if (src->socket == NULL) {
    /* need to allocate a socket */
    GST_DEBUG_OBJECT (src, "allocating socket for %s:%d", src->address,
        src->port);

    addr = gst_udpsrc_resolve (src, src->address);
    if (!addr)
      goto name_resolve;

    if ((src->used_socket =
            g_socket_new (g_inet_address_get_family (addr),
                G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &err)) == NULL)
      goto no_socket;

    src->external_socket = FALSE;

    GST_DEBUG_OBJECT (src, "got socket %p", src->used_socket);

    if (src->addr)
      g_object_unref (src->addr);
    src->addr =
        G_INET_SOCKET_ADDRESS (g_inet_socket_address_new (addr, src->port));

    GST_DEBUG_OBJECT (src, "binding on port %d", src->port);

    /* For multicast, bind to ANY and join the multicast group later */
    if (g_inet_address_get_is_multicast (addr))
      bind_addr = g_inet_address_new_any (g_inet_address_get_family (addr));
    else
      bind_addr = G_INET_ADDRESS (g_object_ref (addr));

    g_object_unref (addr);

    bind_saddr = g_inet_socket_address_new (bind_addr, src->port);
    g_object_unref (bind_addr);
    if (!g_socket_bind (src->used_socket, bind_saddr, src->reuse, &err)) {
      GST_ERROR_OBJECT (src, "%s: error binding to %s:%d", err->message,
          src->address, src->port);
      goto bind_error;
    }

    g_object_unref (bind_saddr);
    g_socket_set_multicast_loopback (src->used_socket, src->loop);
  } else {
    GInetSocketAddress *local_addr;

    GST_DEBUG_OBJECT (src, "using provided socket %p", src->socket);
    /* we use the configured socket, try to get some info about it */
    src->used_socket = G_SOCKET (g_object_ref (src->socket));
    src->external_socket = TRUE;

    local_addr =
        G_INET_SOCKET_ADDRESS (g_socket_get_local_address (src->used_socket,
            &err));
    if (!local_addr)
      goto getsockname_error;

    addr = gst_udpsrc_resolve (src, src->address);
    if (!addr)
      goto name_resolve;

    /* If bound to ANY and address points to a multicast address, make
     * sure that address is not overridden with ANY but we have the
     * opportunity later to join the multicast address. This ensures that we
     * have the same behaviour as for sockets created by udpsrc */
    if (!src->auto_multicast ||
        !g_inet_address_get_is_any (g_inet_socket_address_get_address
            (local_addr))
        || !g_inet_address_get_is_multicast (addr)) {
      g_object_unref (addr);
      if (src->addr)
        g_object_unref (src->addr);
      src->addr = local_addr;
    } else {
      g_object_unref (local_addr);
      if (src->addr)
        g_object_unref (src->addr);
      src->addr =
          G_INET_SOCKET_ADDRESS (g_inet_socket_address_new (addr, src->port));
      g_object_unref (addr);
    }
  }

  {
    gint val;
    GError *opt_err = NULL;
    gboolean force_rcvbuf G_GNUC_UNUSED = FALSE;

    if (src->buffer_size != 0) {
      GST_INFO_OBJECT (src, "setting udp buffer of %d bytes", src->buffer_size);
      /* set buffer size, Note that on Linux this is typically limited to a
       * maximum of around 100K. Also a minimum of 128 bytes is required on
       * Linux. */
      if (!g_socket_set_option (src->used_socket, SOL_SOCKET, SO_RCVBUF,
              src->buffer_size, &opt_err)) {
        GST_INFO_OBJECT (src,
            "Could not create a buffer of requested %d bytes (%s) try forcing",
            src->buffer_size, opt_err->message);
        g_clear_error (&opt_err);
        force_rcvbuf = TRUE;
      }
    }
#if defined(SO_RCVBUFFORCE)
    val = gst_udpsrc_get_rcvbuf (src);
    if (val < src->buffer_size)
      force_rcvbuf = TRUE;

    if (force_rcvbuf) {
      GST_INFO_OBJECT (src,
          "forcibly setting udp buffer of %d bytes", src->buffer_size);

      /* Will only work with CAP_NET_ADMIN privilege */
      if (!g_socket_set_option (src->used_socket, SOL_SOCKET, SO_RCVBUFFORCE,
              src->buffer_size, &opt_err)) {
        GST_ELEMENT_WARNING (src, RESOURCE, SETTINGS, (NULL),
            ("Could not create a buffer of requested %d bytes (%s). Need net.admin privilege?",
                src->buffer_size, opt_err->message));
        g_clear_error (&opt_err);
      }
    }
#endif

    val = gst_udpsrc_get_rcvbuf (src);
    if (val < src->buffer_size)
      GST_WARNING_OBJECT (src,
          "have udp buffer of %d bytes while %d were requested",
          val, src->buffer_size);
    else
      GST_INFO_OBJECT (src, "have udp buffer of %d bytes", val);
  }

  g_socket_set_broadcast (src->used_socket, TRUE);

  if (src->auto_multicast
      &&
      g_inet_address_get_is_multicast (g_inet_socket_address_get_address
          (src->addr))) {

    if (src->multi_iface) {
      GStrv multi_ifaces = g_strsplit (src->multi_iface, ",", -1);
      gchar **ifaces = multi_ifaces;
      while (*ifaces) {
        g_strstrip (*ifaces);
        GST_DEBUG_OBJECT (src, "joining multicast group %s interface %s",
            src->address, *ifaces);
        if (!g_socket_join_multicast_group (src->used_socket,
                g_inet_socket_address_get_address (src->addr),
                FALSE, *ifaces, &err)) {
          g_strfreev (multi_ifaces);
          goto membership;
        }

        ifaces++;
      }
      g_strfreev (multi_ifaces);
    } else {
      GST_DEBUG_OBJECT (src, "joining multicast group %s", src->address);
      if (!g_socket_join_multicast_group (src->used_socket,
              g_inet_socket_address_get_address (src->addr), FALSE, NULL, &err))
        goto membership;
    }

    if (g_inet_address_get_family (g_inet_socket_address_get_address
            (src->addr)) == G_SOCKET_FAMILY_IPV4) {
#if defined(IP_MULTICAST_ALL)
      if (!g_socket_set_option (src->used_socket, IPPROTO_IP, IP_MULTICAST_ALL,
              0, &err)) {
        GST_WARNING_OBJECT (src, "Failed to disable IP_MULTICAST_ALL: %s",
            err->message);
        g_clear_error (&err);
      }
#elif defined(IP_PKTINFO)
      if (!g_socket_set_option (src->used_socket, IPPROTO_IP, IP_PKTINFO, TRUE,
              &err)) {
        GST_WARNING_OBJECT (src, "Failed to enable IP_PKTINFO: %s",
            err->message);
        g_clear_error (&err);
      }
#elif defined(IP_RECVDSTADDR)
      if (!g_socket_set_option (src->used_socket, IPPROTO_IP, IP_RECVDSTADDR,
              TRUE, &err)) {
        GST_WARNING_OBJECT (src, "Failed to enable IP_RECVDSTADDR: %s",
            err->message);
        g_clear_error (&err);
      }
#else
#pragma message("No API available for getting IPv4 destination address")
      GST_WARNING_OBJECT (src, "No API available for getting IPv4 destination "
          "address, will receive packets for every destination to our port");
#endif
    } else
        if (g_inet_address_get_family (g_inet_socket_address_get_address
            (src->addr)) == G_SOCKET_FAMILY_IPV6) {
#ifdef IPV6_PKTINFO
#ifdef IPV6_RECVPKTINFO
      if (!g_socket_set_option (src->used_socket, IPPROTO_IPV6,
              IPV6_RECVPKTINFO, TRUE, &err)) {
#else
      if (!g_socket_set_option (src->used_socket, IPPROTO_IPV6, IPV6_PKTINFO,
              TRUE, &err)) {
#endif
        GST_WARNING_OBJECT (src, "Failed to enable IPV6_PKTINFO: %s",
            err->message);
        g_clear_error (&err);
      }
#else
#pragma message("No API available for getting IPv6 destination address")
      GST_WARNING_OBJECT (src, "No API available for getting IPv6 destination "
          "address, will receive packets for every destination to our port");
#endif
    }
  }

  /* NOTE: sockaddr_in.sin_port works for ipv4 and ipv6 because sin_port
   * follows ss_family on both */
  {
    GInetSocketAddress *addr;
    guint16 port;

    addr =
        G_INET_SOCKET_ADDRESS (g_socket_get_local_address (src->used_socket,
            &err));
    if (!addr)
      goto getsockname_error;

    port = g_inet_socket_address_get_port (addr);
    GST_DEBUG_OBJECT (src, "bound, on port %d", port);
    if (port != src->port) {
      src->port = port;
      GST_DEBUG_OBJECT (src, "notifying port %d", port);
      g_object_notify (G_OBJECT (src), "port");
    }
    g_object_unref (addr);
  }

  return TRUE;

  /* ERRORS */
name_resolve:
  {
    return FALSE;
  }
no_socket:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("no socket error: %s", err->message));
    g_clear_error (&err);
    g_object_unref (addr);
    return FALSE;
  }
bind_error:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("bind failed: %s", err->message));
    g_clear_error (&err);
    g_object_unref (bind_saddr);
    gst_udpsrc_close (src);
    return FALSE;
  }
membership:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("could not add membership: %s", err->message));
    g_clear_error (&err);
    gst_udpsrc_close (src);
    return FALSE;
  }
getsockname_error:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("getsockname failed: %s", err->message));
    g_clear_error (&err);
    gst_udpsrc_close (src);
    return FALSE;
  }
}

static gboolean
gst_udpsrc_unlock (GstBaseSrc * bsrc)
{
  GstUDPSrc *src;

  src = GST_UDPSRC (bsrc);

  GST_LOG_OBJECT (src, "Flushing");
  g_cancellable_cancel (src->cancellable);

  return TRUE;
}

static gboolean
gst_udpsrc_unlock_stop (GstBaseSrc * bsrc)
{
  GstUDPSrc *src;

  src = GST_UDPSRC (bsrc);

  GST_LOG_OBJECT (src, "No longer flushing");

  gst_udpsrc_free_cancellable (src);
  gst_udpsrc_create_cancellable (src);

  return TRUE;
}

static gboolean
gst_udpsrc_close (GstUDPSrc * src)
{
  GST_DEBUG ("closing sockets");

  if (src->used_socket) {
    if (src->auto_multicast
        &&
        g_inet_address_get_is_multicast (g_inet_socket_address_get_address
            (src->addr))) {
      GError *err = NULL;

      if (src->multi_iface) {
        GStrv multi_ifaces = g_strsplit (src->multi_iface, ",", -1);
        gchar **ifaces = multi_ifaces;
        while (*ifaces) {
          g_strstrip (*ifaces);
          GST_DEBUG_OBJECT (src, "leaving multicast group %s interface %s",
              src->address, *ifaces);
          if (!g_socket_leave_multicast_group (src->used_socket,
                  g_inet_socket_address_get_address (src->addr),
                  FALSE, *ifaces, &err)) {
            GST_ERROR_OBJECT (src, "Failed to leave multicast group: %s",
                err->message);
            g_clear_error (&err);
          }
          ifaces++;
        }
        g_strfreev (multi_ifaces);

      } else {
        GST_DEBUG_OBJECT (src, "leaving multicast group %s", src->address);
        if (!g_socket_leave_multicast_group (src->used_socket,
                g_inet_socket_address_get_address (src->addr), FALSE,
                NULL, &err)) {
          GST_ERROR_OBJECT (src, "Failed to leave multicast group: %s",
              err->message);
          g_clear_error (&err);
        }
      }
    }

    if (src->close_socket || !src->external_socket) {
      GError *err = NULL;
      if (!g_socket_close (src->used_socket, &err)) {
        GST_ERROR_OBJECT (src, "Failed to close socket: %s", err->message);
        g_clear_error (&err);
      }
    }

    g_object_unref (src->used_socket);
    src->used_socket = NULL;
    g_object_unref (src->addr);
    src->addr = NULL;
  }

  gst_udpsrc_free_cancellable (src);

  return TRUE;
}


static GstStateChangeReturn
gst_udpsrc_change_state (GstElement * element, GstStateChange transition)
{
  GstUDPSrc *src;
  GstStateChangeReturn result;

  src = GST_UDPSRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_udpsrc_open (src))
        goto open_failed;
      break;
    default:
      break;
  }
  if ((result =
          GST_ELEMENT_CLASS (parent_class)->change_state (element,
              transition)) == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_udpsrc_close (src);
      break;
    default:
      break;
  }
  return result;
  /* ERRORS */
open_failed:
  {
    GST_DEBUG_OBJECT (src, "failed to open socket");
    return GST_STATE_CHANGE_FAILURE;
  }
failure:
  {
    GST_DEBUG_OBJECT (src, "parent failed state change");
    return result;
  }
}




/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_udpsrc_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_udpsrc_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "udp", NULL };

  return protocols;
}

static gchar *
gst_udpsrc_uri_get_uri (GstURIHandler * handler)
{
  GstUDPSrc *src = GST_UDPSRC (handler);

  return g_strdup (src->uri);
}

static gboolean
gst_udpsrc_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  return gst_udpsrc_set_uri (GST_UDPSRC (handler), uri, error);
}

static void
gst_udpsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_udpsrc_uri_get_type;
  iface->get_protocols = gst_udpsrc_uri_get_protocols;
  iface->get_uri = gst_udpsrc_uri_get_uri;
  iface->set_uri = gst_udpsrc_uri_set_uri;
}
