/* * Gstreamer
 *
 * Copyright (C) <2013> Thibault Saunier <thibault.saunier@collabora.com>
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
#ifndef GES_GROUP_H
#define GES_GROUP_H

#include <glib-object.h>
#include <ges/ges-types.h>
#include "ges-clip.h"
#include "ges-container.h"

G_BEGIN_DECLS

#define GES_TYPE_GROUP (ges_group_get_type ())
#define GES_GROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_GROUP, GESGroup))
#define GES_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_GROUP, GESGroupClass))
#define GES_IS_GROUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_GROUP))
#define GES_IS_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_GROUP))
#define GES_GROUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_GROUP, GESGroupClass))

typedef struct _GESGroupPrivate GESGroupPrivate;

struct _GESGroup {
  GESContainer parent;

  /*< private >*/
  GESGroupPrivate *priv;

  gpointer _ges_reserved[GES_PADDING];
};

struct _GESGroupClass {
  GESContainerClass parent_class;

  gpointer _ges_reserved[GES_PADDING];
};

GType ges_group_get_type          (void);
GESGroup *ges_group_new           (void);

G_END_DECLS
#endif /* _GES_GROUP_H */
