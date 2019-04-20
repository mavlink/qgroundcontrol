/* GStreamer
 * Copyright (C) 2012 Wim Taymans <wim.taymans at gmail.com>
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

#include <gst/gst.h>

#ifndef __GST_RTSP_ADDRESS_POOL_H__
#define __GST_RTSP_ADDRESS_POOL_H__

G_BEGIN_DECLS

#define GST_TYPE_RTSP_ADDRESS_POOL              (gst_rtsp_address_pool_get_type ())
#define GST_IS_RTSP_ADDRESS_POOL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_ADDRESS_POOL))
#define GST_IS_RTSP_ADDRESS_POOL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_ADDRESS_POOL))
#define GST_RTSP_ADDRESS_POOL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_ADDRESS_POOL, GstRTSPAddressPoolClass))
#define GST_RTSP_ADDRESS_POOL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_ADDRESS_POOL, GstRTSPAddressPool))
#define GST_RTSP_ADDRESS_POOL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_ADDRESS_POOL, GstRTSPAddressPoolClass))
#define GST_RTSP_ADDRESS_POOL_CAST(obj)         ((GstRTSPAddressPool*)(obj))
#define GST_RTSP_ADDRESS_POOL_CLASS_CAST(klass) ((GstRTSPAddressPoolClass*)(klass))

/**
 * GstRTSPAddressPoolResult:
 * @GST_RTSP_ADDRESS_POOL_OK: no error
 * @GST_RTSP_ADDRESS_POOL_EINVAL:invalid arguments were provided to a function
 * @GST_RTSP_ADDRESS_POOL_ERESERVED: the addres has already been reserved
 * @GST_RTSP_ADDRESS_POOL_ERANGE: the address is not in the pool
 * @GST_RTSP_ADDRESS_POOL_ELAST: last error
 *
 * Result codes from RTSP address pool functions.
 */
typedef enum {
  GST_RTSP_ADDRESS_POOL_OK         =  0,
  /* errors */
  GST_RTSP_ADDRESS_POOL_EINVAL     = -1,
  GST_RTSP_ADDRESS_POOL_ERESERVED  = -2,
  GST_RTSP_ADDRESS_POOL_ERANGE     = -3,

  GST_RTSP_ADDRESS_POOL_ELAST      = -4,
} GstRTSPAddressPoolResult;


typedef struct _GstRTSPAddress GstRTSPAddress;

typedef struct _GstRTSPAddressPool GstRTSPAddressPool;
typedef struct _GstRTSPAddressPoolClass GstRTSPAddressPoolClass;
typedef struct _GstRTSPAddressPoolPrivate GstRTSPAddressPoolPrivate;

/**
 * GstRTSPAddress:
 * @pool: the #GstRTSPAddressPool owner of this address
 * @address: the address
 * @port: the port number
 * @n_ports: number of ports
 * @ttl: TTL or 0 for unicast addresses
 *
 * An address
 */
struct _GstRTSPAddress {
  GstRTSPAddressPool *pool;

  gchar *address;
  guint16 port;
  gint n_ports;
  guint8 ttl;

  /*<private >*/
  gpointer priv;
};

GType gst_rtsp_address_get_type        (void);

GstRTSPAddress * gst_rtsp_address_copy (GstRTSPAddress *addr);
void             gst_rtsp_address_free (GstRTSPAddress *addr);

/**
 * GstRTSPAddressFlags:
 * @GST_RTSP_ADDRESS_FLAG_NONE: no flags
 * @GST_RTSP_ADDRESS_FLAG_IPV4: an IPv4 address
 * @GST_RTSP_ADDRESS_FLAG_IPV6: and IPv6 address
 * @GST_RTSP_ADDRESS_FLAG_EVEN_PORT: address with an even port
 * @GST_RTSP_ADDRESS_FLAG_MULTICAST: a multicast address
 * @GST_RTSP_ADDRESS_FLAG_UNICAST: a unicast address
 *
 * Flags used to control allocation of addresses
 */
typedef enum {
  GST_RTSP_ADDRESS_FLAG_NONE      = 0,
  GST_RTSP_ADDRESS_FLAG_IPV4      = (1 << 0),
  GST_RTSP_ADDRESS_FLAG_IPV6      = (1 << 1),
  GST_RTSP_ADDRESS_FLAG_EVEN_PORT = (1 << 2),
  GST_RTSP_ADDRESS_FLAG_MULTICAST = (1 << 3),
  GST_RTSP_ADDRESS_FLAG_UNICAST   = (1 << 4),
} GstRTSPAddressFlags;

/**
 * GST_RTSP_ADDRESS_POOL_ANY_IPV4:
 *
 * Used with gst_rtsp_address_pool_add_range() to bind to all
 * IPv4 addresses
 */
#define GST_RTSP_ADDRESS_POOL_ANY_IPV4  "0.0.0.0"

/**
 * GST_RTSP_ADDRESS_POOL_ANY_IPV6:
 *
 * Used with gst_rtsp_address_pool_add_range() to bind to all
 * IPv6 addresses
 */
#define GST_RTSP_ADDRESS_POOL_ANY_IPV6  "::"

/**
 * GstRTSPAddressPool:
 * @parent: the parent GObject
 *
 * An address pool, all member are private
 */
struct _GstRTSPAddressPool {
  GObject       parent;

  /*< private >*/
  GstRTSPAddressPoolPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

/**
 * GstRTSPAddressPoolClass:
 *
 * Opaque Address pool class.
 */
struct _GstRTSPAddressPoolClass {
  GObjectClass  parent_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType                  gst_rtsp_address_pool_get_type        (void);

/* create a new address pool */
GstRTSPAddressPool *   gst_rtsp_address_pool_new             (void);

void                   gst_rtsp_address_pool_clear           (GstRTSPAddressPool * pool);
void                   gst_rtsp_address_pool_dump            (GstRTSPAddressPool * pool);

gboolean               gst_rtsp_address_pool_add_range       (GstRTSPAddressPool * pool,
                                                              const gchar *min_address,
                                                              const gchar *max_address,
                                                              guint16 min_port,
                                                              guint16 max_port,
                                                              guint8 ttl);

GstRTSPAddress *       gst_rtsp_address_pool_acquire_address (GstRTSPAddressPool * pool,
                                                              GstRTSPAddressFlags flags,
                                                              gint n_ports);

GstRTSPAddressPoolResult  gst_rtsp_address_pool_reserve_address (GstRTSPAddressPool * pool,
                                                              const gchar *ip_address,
                                                              guint port,
                                                              guint n_ports,
                                                              guint ttl,
                                                              GstRTSPAddress ** address);

gboolean               gst_rtsp_address_pool_has_unicast_addresses (GstRTSPAddressPool * pool);

G_END_DECLS

#endif /* __GST_RTSP_ADDRESS_POOL_H__ */
