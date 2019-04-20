/* GStreamer
 * Copyright (C) 2010 Wim Taymans <wim.taymans at gmail.com>
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

#ifndef __GST_RTSP_PERMISSIONS_H__
#define __GST_RTSP_PERMISSIONS_H__

typedef struct _GstRTSPPermissions GstRTSPPermissions;

G_BEGIN_DECLS

GType gst_rtsp_permissions_get_type (void);

#define GST_TYPE_RTSP_PERMISSIONS        (gst_rtsp_permissions_get_type ())
#define GST_IS_RTSP_PERMISSIONS(obj)     (GST_IS_MINI_OBJECT_TYPE (obj, GST_TYPE_RTSP_PERMISSIONS))
#define GST_RTSP_PERMISSIONS_CAST(obj)   ((GstRTSPPermissions*)(obj))
#define GST_RTSP_PERMISSIONS(obj)        (GST_RTSP_PERMISSIONS_CAST(obj))

/**
 * GstRTSPPermissions:
 *
 * The opaque permissions structure. It is used to define the permissions
 * of objects in different roles.
 */
struct _GstRTSPPermissions {
  GstMiniObject mini_object;
};

/* refcounting */
/**
 * gst_rtsp_permissions_ref:
 * @permissions: The permissions to refcount
 *
 * Increase the refcount of this permissions.
 *
 * Returns: (transfer full): @permissions (for convenience when doing assignments)
 */
#ifdef _FOOL_GTK_DOC_
G_INLINE_FUNC GstRTSPPermissions * gst_rtsp_permissions_ref (GstRTSPPermissions * permissions);
#endif

static inline GstRTSPPermissions *
gst_rtsp_permissions_ref (GstRTSPPermissions * permissions)
{
  return (GstRTSPPermissions *) gst_mini_object_ref (GST_MINI_OBJECT_CAST (permissions));
}

/**
 * gst_rtsp_permissions_unref:
 * @permissions: (transfer full): the permissions to refcount
 *
 * Decrease the refcount of an permissions, freeing it if the refcount reaches 0.
 */
#ifdef _FOOL_GTK_DOC_
G_INLINE_FUNC void gst_rtsp_permissions_unref (GstRTSPPermissions * permissions);
#endif

static inline void
gst_rtsp_permissions_unref (GstRTSPPermissions * permissions)
{
  gst_mini_object_unref (GST_MINI_OBJECT_CAST (permissions));
}


GstRTSPPermissions *  gst_rtsp_permissions_new             (void);

void                  gst_rtsp_permissions_add_role        (GstRTSPPermissions *permissions,
                                                            const gchar *role,
                                                            const gchar *fieldname, ...);
void                  gst_rtsp_permissions_add_role_valist (GstRTSPPermissions *permissions,
                                                            const gchar *role,
                                                            const gchar *fieldname,
                                                            va_list var_args);
void                  gst_rtsp_permissions_remove_role     (GstRTSPPermissions *permissions,
                                                            const gchar *role);
const GstStructure *  gst_rtsp_permissions_get_role        (GstRTSPPermissions *permissions,
                                                            const gchar *role);

gboolean              gst_rtsp_permissions_is_allowed      (GstRTSPPermissions *permissions,
                                                            const gchar *role, const gchar *permission);

G_END_DECLS

#endif /* __GST_RTSP_PERMISSIONS_H__ */
