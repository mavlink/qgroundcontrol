/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *               2009 Nokia Corporation
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

#ifndef _GES_LAYER
#define _GES_LAYER

#include <glib-object.h>
#include <ges/ges-types.h>

G_BEGIN_DECLS

#define GES_TYPE_LAYER ges_layer_get_type()

#define GES_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_LAYER, GESLayer))

#define GES_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_LAYER, GESLayerClass))

#define GES_IS_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_LAYER))

#define GES_IS_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_LAYER))

#define GES_LAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_LAYER, GESLayerClass))

typedef struct _GESLayerPrivate GESLayerPrivate;

/**
 * GESLayer:
 * @timeline: the #GESTimeline where this layer is being used.
 */
struct _GESLayer {
  GInitiallyUnowned parent;

  /*< public >*/

  GESTimeline *timeline;

  /*< protected >*/
  guint32 min_nle_priority, max_nle_priority;

  GESLayerPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESLayerClass:
 * @get_objects: method to get the objects contained in the layer
 *
 * Subclasses can override the @get_objects if they can provide a more
 * efficient way of providing the list of contained #GESClip(s).
 */
struct _GESLayerClass {
  /*< private >*/
  GInitiallyUnownedClass parent_class;

  /*< public >*/
  /* virtual methods for subclasses */
  GList *(*get_objects) (GESLayer * layer);

  /*< private >*/
  /* Signals */
  void	(*object_added)		(GESLayer * layer, GESClip * object);
  void	(*object_removed)	(GESLayer * layer, GESClip * object);

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_layer_get_type (void);

GESLayer* ges_layer_new (void);

void     ges_layer_set_timeline  (GESLayer * layer,
					   GESTimeline * timeline);

GESTimeline *
ges_layer_get_timeline           (GESLayer * layer);

gboolean ges_layer_add_clip    (GESLayer * layer,
					   GESClip * clip);
GESClip * ges_layer_add_asset   (GESLayer *layer,
                                                       GESAsset *asset,
                                                       GstClockTime start,
                                                       GstClockTime inpoint,
                                                       GstClockTime duration,
                                                       GESTrackType track_types);

gboolean ges_layer_remove_clip (GESLayer * layer,
					   GESClip * clip);

void     ges_layer_set_priority  (GESLayer * layer,
					   guint priority);

gboolean ges_layer_is_empty      (GESLayer * layer);

guint   ges_layer_get_priority  (GESLayer * layer);

gboolean ges_layer_get_auto_transition (GESLayer * layer);

void ges_layer_set_auto_transition (GESLayer * layer,
					     gboolean auto_transition);

GList*   ges_layer_get_clips   (GESLayer * layer);
GstClockTime ges_layer_get_duration (GESLayer *layer);

G_END_DECLS

#endif /* _GES_LAYER */

