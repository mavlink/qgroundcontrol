/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim@fluendo.com>
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

#ifndef __GST_MULTIUDPSINK_H__
#define __GST_MULTIUDPSINK_H__

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#include "gstudpnetutils.h"

#define GST_TYPE_MULTIUDPSINK            (gst_multiudpsink_get_type())
#define GST_MULTIUDPSINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MULTIUDPSINK,GstMultiUDPSink))
#define GST_MULTIUDPSINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MULTIUDPSINK,GstMultiUDPSinkClass))
#define GST_IS_MULTIUDPSINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MULTIUDPSINK))
#define GST_IS_MULTIUDPSINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MULTIUDPSINK))
#define GST_MULTIUDPSINK_CAST(obj)       ((GstMultiUDPSink*)(obj))

typedef struct _GstMultiUDPSink GstMultiUDPSink;
typedef struct _GstMultiUDPSinkClass GstMultiUDPSinkClass;

typedef GOutputMessage GstOutputMessage;

typedef struct {
  gint ref_count;         /* for memory management */
  gint add_count;         /* how often this address has been added */

  GSocketAddress *addr;
  gchar *host;
  gint port;

  /* Per-client stats */
  guint64 bytes_sent;
  guint64 packets_sent;
  guint64 connect_time;
  guint64 disconnect_time;
} GstUDPClient;

/* sends udp packets to multiple host/port pairs.
 */
struct _GstMultiUDPSink {
  GstBaseSink parent;

  GSocket       *used_socket, *used_socket_v6;

  GCancellable  *cancellable;
  gboolean       made_cancel_fd;

  /* client management */
  GMutex         client_lock;
  GList         *clients;
  guint          num_v4_unique;  /* number IPv4 clients (excluding duplicates) */
  guint          num_v4_all;     /* number IPv4 clients (including duplicates) */
  guint          num_v6_unique;  /* number IPv6 clients (excluding duplicates) */
  guint          num_v6_all;     /* number IPv6 clients (including duplicates) */
  GList         *clients_to_be_removed;

  /* pre-allocated scrap space for render function */
  GOutputVector    *vecs;
  guint             n_vecs;
  GstMapInfo       *maps;
  guint             n_maps;
  GstOutputMessage *messages;
  guint             n_messages;

  /* properties */
  guint64        bytes_to_serve;
  guint64        bytes_served;
  GSocket       *socket, *socket_v6;
  gboolean       close_socket;

  gboolean       external_socket;

  gboolean       auto_multicast;
  gchar         *multi_iface;
  gint           ttl;
  gint           ttl_mc;
  gboolean       loop;
  gboolean       force_ipv4;
  gint           qos_dscp;

  gboolean       send_duplicates;
  gint           buffer_size;
  gchar         *bind_address;
  gint           bind_port;
};

struct _GstMultiUDPSinkClass {
  GstBaseSinkClass parent_class;

  /* element methods */
  void          (*add)          (GstMultiUDPSink *sink, const gchar *host, gint port);
  void          (*remove)       (GstMultiUDPSink *sink, const gchar *host, gint port);
  void          (*clear)        (GstMultiUDPSink *sink);
  GstStructure* (*get_stats)    (GstMultiUDPSink *sink, const gchar *host, gint port);

  /* signals */
  void          (*client_added) (GstElement *element, const gchar *host, gint port);
  void          (*client_removed) (GstElement *element, const gchar *host, gint port);
};

GType gst_multiudpsink_get_type(void);

void            gst_multiudpsink_add            (GstMultiUDPSink *sink, const gchar *host, gint port);
void            gst_multiudpsink_remove         (GstMultiUDPSink *sink, const gchar *host, gint port);
void            gst_multiudpsink_clear          (GstMultiUDPSink *sink);
GstStructure*   gst_multiudpsink_get_stats      (GstMultiUDPSink *sink, const gchar *host, gint port);

G_END_DECLS

#endif /* __GST_MULTIUDPSINK_H__ */
