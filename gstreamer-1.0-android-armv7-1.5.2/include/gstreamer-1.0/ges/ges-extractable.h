/* GStreamer Editing Services
 *
 * Copyright (C) 2012 Thibault Saunier <thibault.saunier@collabora.com>
 * Copyright (C) 2012 Volodymyr Rudyi <vladimir.rudoy@gmail.com>
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
#ifndef _GES_EXTRACTABLE_
#define _GES_EXTRACTABLE_

#include <glib-object.h>
#include <gio/gio.h>
#include <ges/ges-types.h>
#include <ges/ges-asset.h>

G_BEGIN_DECLS

/* GESExtractable interface declarations */
#define GES_TYPE_EXTRACTABLE                (ges_extractable_get_type ())
#define GES_EXTRACTABLE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_EXTRACTABLE, GESExtractable))
#define GES_IS_EXTRACTABLE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_EXTRACTABLE))
#define GES_EXTRACTABLE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GES_TYPE_EXTRACTABLE, GESExtractableInterface))

GType ges_extractable_get_type (void);

/**
 * GESExtractableCheckId:
 * @type: The #GType to check @id for:
 * @id: The id to check
 * @error: An error that can be set if needed
 *
 * Returns: The ID to use for the asset or %NULL if @id is not valid
 */

typedef gchar* (*GESExtractableCheckId) (GType type, const gchar *id,
    GError **error);

/**
 * GESExtractable:
 */
struct _GESExtractableInterface
{
  GTypeInterface parent;

  GType asset_type;

  GESExtractableCheckId check_id;
  gboolean can_update_asset;

  void (*set_asset)                  (GESExtractable *self,
                                         GESAsset *asset);

  gboolean (*set_asset_full)         (GESExtractable *self,
                                      GESAsset *asset);

  GParameter *(*get_parameters_from_id) (const gchar *id,
                                         guint *n_params);

  gchar * (*get_id)                     (GESExtractable *self);

  GType (*get_real_extractable_type)    (GType wanted_type,
                                         const gchar *id);

  gboolean (*register_metas)            (GESExtractableInterface *self,
                                         GObjectClass *klass,
                                         GESAsset *asset);

  gpointer _ges_reserved[GES_PADDING];
};

GESAsset* ges_extractable_get_asset      (GESExtractable *self);
gboolean ges_extractable_set_asset              (GESExtractable *self,
                                                GESAsset *asset);

gchar * ges_extractable_get_id                 (GESExtractable *self);

G_END_DECLS
#endif /* _GES_EXTRACTABLE_ */
