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


#ifndef GES_CLIP_ASSET_H
#define GES_CLIP_ASSET_H

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-asset.h>

G_BEGIN_DECLS

#define GES_TYPE_CLIP_ASSET (ges_clip_asset_get_type ())
#define GES_CLIP_ASSET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_CLIP_ASSET, GESClipAsset))
#define GES_CLIP_ASSET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_CLIP_ASSET, GESClipAssetClass))
#define GES_IS_CLIP_ASSET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_CLIP_ASSET))
#define GES_IS_CLIP_ASSET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_CLIP_ASSET))
#define GES_CLIP_ASSET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_CLIP_ASSET, GESClipAssetClass))

typedef struct _GESClipAssetPrivate GESClipAssetPrivate;

struct _GESClipAsset
{
  GESAsset parent;

  /* <private> */
  GESClipAssetPrivate *priv;

  gpointer _ges_reserved[GES_PADDING];
};

struct _GESClipAssetClass
{
  GESAssetClass parent;

  gpointer _ges_reserved[GES_PADDING];
};

GType ges_clip_asset_get_type (void);
void ges_clip_asset_set_supported_formats         (GESClipAsset *self,
                                                              GESTrackType supportedformats);
GESTrackType ges_clip_asset_get_supported_formats (GESClipAsset *self);

G_END_DECLS
#endif /* _GES_CLIP_ASSET_H */
