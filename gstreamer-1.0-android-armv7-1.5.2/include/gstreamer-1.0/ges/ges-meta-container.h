/* GStreamer Editing Services
 * Copyright (C) 2012 Paul Lange <palango@gmx.de>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _GES_META_CONTAINER
#define _GES_META_CONTAINER

#include <glib-object.h>
#include <gst/gst.h>
#include <ges/ges-types.h>
#include "ges-enums.h"

G_BEGIN_DECLS

#define GES_TYPE_META_CONTAINER                 (ges_meta_container_get_type ())
#define GES_META_CONTAINER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_META_CONTAINER, GESMetaContainer))
#define GES_IS_META_CONTAINER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_META_CONTAINER))
#define GES_META_CONTAINER_GET_INTERFACE (inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GES_TYPE_META_CONTAINER, GESMetaContainerInterface))

/**
 * GES_META_FORMATTER_NAME:
 *
 * Name of a formatter it is used as ID of Formater assets (string)
 *
 * The name of the formatter
 */
#define GES_META_FORMATTER_NAME                       "name"

/**
 * GES_META_DESCRIPTION:
 *
 * The description of an object, can be used in various context (string)
 *
 * The description
 */
#define GES_META_DESCRIPTION                         "description"

/**
 * GES_META_FORMATTER_MIMETYPE:
 *
 * Mimetype used for the file produced by a  formatter (string)
 *
 * The mime type
 */
#define GES_META_FORMATTER_MIMETYPE                   "mimetype"

/**
 * GES_META_FORMATTER_EXTENSION:
 *
 * The extension of the files produced by a formatter (string)
 */
#define GES_META_FORMATTER_EXTENSION                  "extension"

/**
 * GES_META_FORMATTER_VERSION:
 *
 * The version of a formatter (double)
 *
 * The formatter version
 */
#define GES_META_FORMATTER_VERSION                    "version"

/**
 * GES_META_FORMATTER_RANK:
 *
 * The rank of a formatter (GstRank)
 *
 * The rank of a formatter
 */
#define GES_META_FORMATTER_RANK                       "rank"

/**
 * GES_META_VOLUME:
 *
 * The volume, can be used for audio track or layers
 *
 * The volume for a track or a layer, it is register as a float
 */
#define GES_META_VOLUME                              "volume"

/**
 * GES_META_VOLUME_DEFAULT:
 *
 * The default volume
 *
 * The default volume for a track or a layer as a float
 */
#define GES_META_VOLUME_DEFAULT                       1.0

/**
 * GES_META_FORMAT_VERSION:
 *
 * The version of the format in which a project is serialized
 */
#define GES_META_FORMAT_VERSION                       "format-version"

typedef struct _GESMetaContainer          GESMetaContainer;
typedef struct _GESMetaContainerInterface GESMetaContainerInterface;

struct _GESMetaContainerInterface {
  GTypeInterface parent_iface;

  gpointer _ges_reserved[GES_PADDING];
};

GType ges_meta_container_get_type (void);

gboolean
ges_meta_container_set_boolean     (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gboolean value);

gboolean
ges_meta_container_set_int         (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gint value);

gboolean
ges_meta_container_set_uint        (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        guint value);

gboolean
ges_meta_container_set_int64       (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gint64 value);

gboolean
ges_meta_container_set_uint64      (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        guint64 value);

gboolean
ges_meta_container_set_float       (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gfloat value);

gboolean
ges_meta_container_set_double      (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gdouble value);

gboolean
ges_meta_container_set_date        (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        const GDate* value);

gboolean
ges_meta_container_set_date_time   (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        const GstDateTime* value);

gboolean
ges_meta_container_set_string      (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        const gchar* value);

gboolean
ges_meta_container_set_meta            (GESMetaContainer * container,
                                        const gchar* meta_item,
                                        const GValue *value);

gboolean
ges_meta_container_register_meta_boolean (GESMetaContainer *container,
                                          GESMetaFlag flags,
                                          const gchar* meta_item,
                                          gboolean value);

gboolean
ges_meta_container_register_meta_int     (GESMetaContainer *container,
                                          GESMetaFlag flags,
                                          const gchar* meta_item,
                                          gint value);

gboolean
ges_meta_container_register_meta_uint    (GESMetaContainer *container,
                                          GESMetaFlag flags,
                                          const gchar* meta_item,
                                          guint value);

gboolean
ges_meta_container_register_meta_int64   (GESMetaContainer *container,
                                          GESMetaFlag flags,
                                          const gchar* meta_item,
                                          gint64 value);

gboolean
ges_meta_container_register_meta_uint64  (GESMetaContainer *container,
                                          GESMetaFlag flags,
                                          const gchar* meta_item,
                                          guint64 value);

gboolean
ges_meta_container_register_meta_float   (GESMetaContainer *container,
                                          GESMetaFlag flags,
                                          const gchar* meta_item,
                                          gfloat value);

gboolean
ges_meta_container_register_meta_double  (GESMetaContainer *container,
                                          GESMetaFlag flags,
                                          const gchar* meta_item,
                                          gdouble value);

gboolean
ges_meta_container_register_meta_date    (GESMetaContainer *container,
                                          GESMetaFlag flags,
                                          const gchar* meta_item,
                                          const GDate* value);

gboolean
ges_meta_container_register_meta_date_time  (GESMetaContainer *container,
                                             GESMetaFlag flags,
                                             const gchar* meta_item,
                                             const GstDateTime* value);

gboolean
ges_meta_container_register_meta_string     (GESMetaContainer *container,
                                             GESMetaFlag flags,
                                             const gchar* meta_item,
                                             const gchar* value);

gboolean
ges_meta_container_register_meta            (GESMetaContainer *container,
                                             GESMetaFlag flags,
                                             const gchar* meta_item,
                                             const GValue * value);

gboolean
ges_meta_container_check_meta_registered    (GESMetaContainer *container,
                                             const gchar * meta_item,
                                             GESMetaFlag * flags,
                                             GType * type);

gboolean
ges_meta_container_get_boolean     (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gboolean* dest);

gboolean
ges_meta_container_get_int         (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gint* dest);

gboolean
ges_meta_container_get_uint        (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        guint* dest);

gboolean
ges_meta_container_get_int64       (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gint64* dest);

gboolean
ges_meta_container_get_uint64      (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        guint64* dest);

gboolean
ges_meta_container_get_float       (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gfloat* dest);

gboolean
ges_meta_container_get_double      (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        gdouble* dest);

gboolean
ges_meta_container_get_date        (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        GDate** dest);

gboolean
ges_meta_container_get_date_time   (GESMetaContainer *container,
                                        const gchar* meta_item,
                                        GstDateTime** dest);

const gchar *
ges_meta_container_get_string      (GESMetaContainer * container,
                                        const gchar * meta_item);

const GValue *
ges_meta_container_get_meta            (GESMetaContainer * container,
                                        const gchar * key);

typedef void
(*GESMetaForeachFunc)                  (const GESMetaContainer *container,
                                        const gchar *key,
                                        const GValue *value,
                                        gpointer user_data);

void
ges_meta_container_foreach             (GESMetaContainer *container,
                                        GESMetaForeachFunc func,
                                        gpointer user_data);

gchar *
ges_meta_container_metas_to_string     (GESMetaContainer *container);

gboolean
ges_meta_container_add_metas_from_string (GESMetaContainer *container,
                                          const gchar *str);

G_END_DECLS
#endif /* _GES_META_CONTAINER */
