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

#include "rtsp-media-factory.h"

#ifndef __GST_RTSP_MOUNT_POINTS_H__
#define __GST_RTSP_MOUNT_POINTS_H__

G_BEGIN_DECLS

#define GST_TYPE_RTSP_MOUNT_POINTS              (gst_rtsp_mount_points_get_type ())
#define GST_IS_RTSP_MOUNT_POINTS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_MOUNT_POINTS))
#define GST_IS_RTSP_MOUNT_POINTS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_MOUNT_POINTS))
#define GST_RTSP_MOUNT_POINTS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_MOUNT_POINTS, GstRTSPMountPointsClass))
#define GST_RTSP_MOUNT_POINTS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_MOUNT_POINTS, GstRTSPMountPoints))
#define GST_RTSP_MOUNT_POINTS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_MOUNT_POINTS, GstRTSPMountPointsClass))
#define GST_RTSP_MOUNT_POINTS_CAST(obj)         ((GstRTSPMountPoints*)(obj))
#define GST_RTSP_MOUNT_POINTS_CLASS_CAST(klass) ((GstRTSPMountPointsClass*)(klass))

typedef struct _GstRTSPMountPoints GstRTSPMountPoints;
typedef struct _GstRTSPMountPointsClass GstRTSPMountPointsClass;
typedef struct _GstRTSPMountPointsPrivate GstRTSPMountPointsPrivate;

/**
 * GstRTSPMountPoints:
 *
 * Creates a #GstRTSPMediaFactory object for a given url.
 */
struct _GstRTSPMountPoints {
  GObject       parent;

  /*< private >*/
  GstRTSPMountPointsPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

/**
 * GstRTSPMountPointsClass:
 * @make_path: make a path from the given url.
 *
 * The class for the media mounts object.
 */
struct _GstRTSPMountPointsClass {
  GObjectClass  parent_class;

  gchar * (*make_path) (GstRTSPMountPoints *mounts,
                        const GstRTSPUrl *url);

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType                 gst_rtsp_mount_points_get_type       (void);

/* creating a mount points */
GstRTSPMountPoints *  gst_rtsp_mount_points_new            (void);

gchar *               gst_rtsp_mount_points_make_path      (GstRTSPMountPoints *mounts,
                                                            const GstRTSPUrl * url);
/* finding a media factory */
GstRTSPMediaFactory * gst_rtsp_mount_points_match          (GstRTSPMountPoints *mounts,
                                                            const gchar *path,
                                                            gint * matched);
/* managing media to a mount point */
void                  gst_rtsp_mount_points_add_factory    (GstRTSPMountPoints *mounts,
                                                            const gchar *path,
                                                            GstRTSPMediaFactory *factory);
void                  gst_rtsp_mount_points_remove_factory (GstRTSPMountPoints *mounts,
                                                            const gchar *path);

G_END_DECLS

#endif /* __GST_RTSP_MOUNT_POINTS_H__ */
