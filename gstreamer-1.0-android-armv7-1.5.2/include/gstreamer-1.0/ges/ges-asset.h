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

#ifndef _GES_ASSET_
#define _GES_ASSET_

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-enums.h>
#include <gio/gio.h>
#include <gst/gst.h>

G_BEGIN_DECLS
#define GES_TYPE_ASSET ges_asset_get_type()
#define GES_ASSET(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_ASSET, GESAsset))
#define GES_ASSET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_ASSET, GESAssetClass))
#define GES_IS_ASSET(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_ASSET))
#define GES_IS_ASSET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_ASSET))
#define GES_ASSET_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_ASSET, GESAssetClass))

typedef enum
{
  GES_ASSET_LOADING_ERROR,
  GES_ASSET_LOADING_ASYNC,
  GES_ASSET_LOADING_OK
} GESAssetLoadingReturn;

typedef struct _GESAssetPrivate GESAssetPrivate;

GType ges_asset_get_type (void);

struct _GESAsset
{
  GObject parent;

  /* <private> */
  GESAssetPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESAssetClass
{
  GObjectClass parent;

  GESAssetLoadingReturn    (*start_loading)     (GESAsset *self,
                                                 GError **error);
  GESExtractable*          (*extract)           (GESAsset *self,
                                                 GError **error);
  /* Let subclasses know that we proxied an asset */
  void                     (*inform_proxy)      (GESAsset *self,
                                                 const gchar *proxy_id);
  /* Ask subclasses for a new ID for @self when the asset failed loading
   * This function returns %FALSE when the ID could be updated or %TRUE
   * otherwize */
  gboolean                 (*request_id_update) (GESAsset *self,
                                                 gchar **proposed_new_id,
                                                 GError *error) ;
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_asset_get_extractable_type (GESAsset * self);
void ges_asset_request_async         (GType extractable_type,
                                      const gchar * id,
                                      GCancellable *cancellable,
                                      GAsyncReadyCallback callback,
                                      gpointer user_data);
GESAsset * ges_asset_request         (GType extractable_type,
                                      const gchar * id,
                                      GError **error);
const gchar * ges_asset_get_id       (GESAsset* self);
GESAsset * ges_asset_request_finish  (GAsyncResult *res,
                                      GError **error);
GESExtractable * ges_asset_extract   (GESAsset * self,
                                      GError **error);
GList * ges_list_assets              (GType filter);

G_END_DECLS
#endif /* _GES_ASSET */
