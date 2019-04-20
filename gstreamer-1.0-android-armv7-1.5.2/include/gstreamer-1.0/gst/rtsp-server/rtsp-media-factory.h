/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
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
#include <gst/rtsp/gstrtspurl.h>

#include "rtsp-media.h"
#include "rtsp-permissions.h"
#include "rtsp-address-pool.h"

#ifndef __GST_RTSP_MEDIA_FACTORY_H__
#define __GST_RTSP_MEDIA_FACTORY_H__

G_BEGIN_DECLS

/* types for the media factory */
#define GST_TYPE_RTSP_MEDIA_FACTORY              (gst_rtsp_media_factory_get_type ())
#define GST_IS_RTSP_MEDIA_FACTORY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_MEDIA_FACTORY))
#define GST_IS_RTSP_MEDIA_FACTORY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_MEDIA_FACTORY))
#define GST_RTSP_MEDIA_FACTORY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_MEDIA_FACTORY, GstRTSPMediaFactoryClass))
#define GST_RTSP_MEDIA_FACTORY(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_MEDIA_FACTORY, GstRTSPMediaFactory))
#define GST_RTSP_MEDIA_FACTORY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_MEDIA_FACTORY, GstRTSPMediaFactoryClass))
#define GST_RTSP_MEDIA_FACTORY_CAST(obj)         ((GstRTSPMediaFactory*)(obj))
#define GST_RTSP_MEDIA_FACTORY_CLASS_CAST(klass) ((GstRTSPMediaFactoryClass*)(klass))

typedef struct _GstRTSPMediaFactory GstRTSPMediaFactory;
typedef struct _GstRTSPMediaFactoryClass GstRTSPMediaFactoryClass;
typedef struct _GstRTSPMediaFactoryPrivate GstRTSPMediaFactoryPrivate;

/**
 * GstRTSPMediaFactory:
 *
 * The definition and logic for constructing the pipeline for a media. The media
 * can contain multiple streams like audio and video.
 */
struct _GstRTSPMediaFactory {
  GObject            parent;

  /*< private >*/
  GstRTSPMediaFactoryPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

/**
 * GstRTSPMediaFactoryClass:
 * @gen_key: convert @url to a key for caching shared #GstRTSPMedia objects.
 *       The default implementation of this function will use the complete URL
 *       including the query parameters to return a key.
 * @create_element: Construct and return a #GstElement that is a #GstBin containing
 *       the elements to use for streaming the media. The bin should contain
 *       payloaders pay\%d for each stream. The default implementation of this
 *       function returns the bin created from the launch parameter.
 * @construct: the vmethod that will be called when the factory has to create the
 *       #GstRTSPMedia for @url. The default implementation of this
 *       function calls create_element to retrieve an element and then looks for
 *       pay\%d to create the streams.
 * @create_pipeline: create a new pipeline or re-use an existing one and
 *       add the #GstRTSPMedia's element created by @construct to the pipeline.
 * @configure: configure the media created with @construct. The default
 *       implementation will configure the 'shared' property of the media.
 * @media_constructed: signal emited when a media was constructed
 * @media_configure: signal emited when a media should be configured
 *
 * The #GstRTSPMediaFactory class structure.
 */
struct _GstRTSPMediaFactoryClass {
  GObjectClass  parent_class;

  gchar *         (*gen_key)            (GstRTSPMediaFactory *factory, const GstRTSPUrl *url);

  GstElement *    (*create_element)     (GstRTSPMediaFactory *factory, const GstRTSPUrl *url);
  GstRTSPMedia *  (*construct)          (GstRTSPMediaFactory *factory, const GstRTSPUrl *url);
  GstElement *    (*create_pipeline)    (GstRTSPMediaFactory *factory, GstRTSPMedia *media);
  void            (*configure)          (GstRTSPMediaFactory *factory, GstRTSPMedia *media);

  /* signals */
  void            (*media_constructed)  (GstRTSPMediaFactory *factory, GstRTSPMedia *media);
  void            (*media_configure)    (GstRTSPMediaFactory *factory, GstRTSPMedia *media);

  /*< private >*/
  gpointer         _gst_reserved[GST_PADDING_LARGE];
};

GType                 gst_rtsp_media_factory_get_type     (void);

/* creating the factory */
GstRTSPMediaFactory * gst_rtsp_media_factory_new          (void);

/* configuring the factory */
void                  gst_rtsp_media_factory_set_launch       (GstRTSPMediaFactory *factory,
                                                               const gchar *launch);
gchar *               gst_rtsp_media_factory_get_launch       (GstRTSPMediaFactory *factory);

void                  gst_rtsp_media_factory_set_permissions  (GstRTSPMediaFactory *factory,
                                                               GstRTSPPermissions *permissions);
GstRTSPPermissions *  gst_rtsp_media_factory_get_permissions  (GstRTSPMediaFactory *factory);
void                  gst_rtsp_media_factory_add_role         (GstRTSPMediaFactory *factory,
                                                               const gchar *role,
                                                               const gchar *fieldname, ...);

void                  gst_rtsp_media_factory_set_shared       (GstRTSPMediaFactory *factory,
                                                               gboolean shared);
gboolean              gst_rtsp_media_factory_is_shared        (GstRTSPMediaFactory *factory);

void                  gst_rtsp_media_factory_set_suspend_mode (GstRTSPMediaFactory *factory,
                                                               GstRTSPSuspendMode mode);
GstRTSPSuspendMode    gst_rtsp_media_factory_get_suspend_mode (GstRTSPMediaFactory *factory);

void                  gst_rtsp_media_factory_set_eos_shutdown (GstRTSPMediaFactory *factory,
                                                               gboolean eos_shutdown);
gboolean              gst_rtsp_media_factory_is_eos_shutdown  (GstRTSPMediaFactory *factory);

void                  gst_rtsp_media_factory_set_profiles     (GstRTSPMediaFactory *factory,
                                                               GstRTSPProfile profiles);
GstRTSPProfile        gst_rtsp_media_factory_get_profiles     (GstRTSPMediaFactory *factory);

void                  gst_rtsp_media_factory_set_protocols    (GstRTSPMediaFactory *factory,
                                                               GstRTSPLowerTrans protocols);
GstRTSPLowerTrans     gst_rtsp_media_factory_get_protocols    (GstRTSPMediaFactory *factory);

void                  gst_rtsp_media_factory_set_address_pool (GstRTSPMediaFactory * factory,
                                                               GstRTSPAddressPool * pool);
GstRTSPAddressPool *  gst_rtsp_media_factory_get_address_pool (GstRTSPMediaFactory * factory);

void                  gst_rtsp_media_factory_set_buffer_size  (GstRTSPMediaFactory * factory,
                                                               guint size);
guint                 gst_rtsp_media_factory_get_buffer_size  (GstRTSPMediaFactory * factory);
void                  gst_rtsp_media_factory_set_retransmission_time (GstRTSPMediaFactory * factory,
                                                                      GstClockTime time);
GstClockTime          gst_rtsp_media_factory_get_retransmission_time (GstRTSPMediaFactory * factory);

void                  gst_rtsp_media_factory_set_latency      (GstRTSPMediaFactory * factory,
                                                               guint                 latency);
guint                 gst_rtsp_media_factory_get_latency      (GstRTSPMediaFactory * factory);

void                  gst_rtsp_media_factory_set_transport_mode (GstRTSPMediaFactory *factory,
                                                                 GstRTSPTransportMode mode);
GstRTSPTransportMode  gst_rtsp_media_factory_get_transport_mode (GstRTSPMediaFactory *factory);

void                  gst_rtsp_media_factory_set_media_gtype  (GstRTSPMediaFactory * factory,
                                                               GType media_gtype);
GType                 gst_rtsp_media_factory_get_media_gtype  (GstRTSPMediaFactory * factory);

/* creating the media from the factory and a url */
GstRTSPMedia *        gst_rtsp_media_factory_construct        (GstRTSPMediaFactory *factory,
                                                               const GstRTSPUrl *url);

GstElement *          gst_rtsp_media_factory_create_element   (GstRTSPMediaFactory *factory,
                                                               const GstRTSPUrl *url);

G_END_DECLS

#endif /* __GST_RTSP_MEDIA_FACTORY_H__ */
