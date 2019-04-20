 
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * gst-editing-services
 * Copyright (C) 2013 Thibault Saunier <thibault.saunier@collabora.com>
 *
 gst-editing-services is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gst-editing-services is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.";
 */

#ifndef _GES_EFFECT_ASSET_H_
#define _GES_EFFECT_ASSET_H_

#include <glib-object.h>
#include "ges-track-element-asset.h"

G_BEGIN_DECLS

#define GES_TYPE_EFFECT_ASSET             (ges_effect_asset_get_type ())
#define GES_EFFECT_ASSET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_EFFECT_ASSET, GESEffectAsset))
#define GES_EFFECT_ASSET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_EFFECT_ASSET, GESEffectAssetClass))
#define GES_IS_EFFECT_ASSET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_EFFECT_ASSET))
#define GES_IS_EFFECT_ASSET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_EFFECT_ASSET))
#define GES_EFFECT_ASSET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_EFFECT_ASSET, GESEffectAssetClass))

typedef struct _GESEffectAssetClass GESEffectAssetClass;
typedef struct _GESEffectAsset GESEffectAsset;
typedef struct _GESEffectAssetPrivate GESEffectAssetPrivate;


struct _GESEffectAssetClass
{
  GESTrackElementAssetClass parent_class;

  gpointer _ges_reserved[GES_PADDING];
};

struct _GESEffectAsset
{
  GESTrackElementAsset parent_instance;

  GESEffectAssetPrivate *priv;

  gpointer _ges_reserved[GES_PADDING];
};

GType ges_effect_asset_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* _GES_EFFECT_ASSET_H_ */

