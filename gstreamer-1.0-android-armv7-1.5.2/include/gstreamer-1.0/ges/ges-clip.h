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

#ifndef _GES_CLIP
#define _GES_CLIP

#include <glib-object.h>
#include <gst/gst.h>
#include <ges/ges-timeline-element.h>
#include <ges/ges-container.h>
#include <ges/ges-types.h>
#include <ges/ges-track.h>

G_BEGIN_DECLS

#define GES_TYPE_CLIP             ges_clip_get_type()
#define GES_CLIP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_CLIP, GESClip))
#define GES_CLIP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_CLIP, GESClipClass))
#define GES_IS_CLIP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_CLIP))
#define GES_IS_CLIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_CLIP))
#define GES_CLIP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_CLIP, GESClipClass))

typedef struct _GESClipPrivate GESClipPrivate;

/**
 * GESFillTrackElementFunc:
 * @clip: the #GESClip controlling the track elements
 * @track_element: the #GESTrackElement
 * @nleobj: the GNonLin object that needs to be filled.
 *
 * A function that will be called when the GNonLin object of a corresponding
 * track element needs to be filled.
 *
 * The implementer of this function shall add the proper #GstElement to @nleobj
 * using gst_bin_add().
 *
 * Returns: TRUE if the implementer succesfully filled the @nleobj, else #FALSE.
 */
typedef gboolean (*GESFillTrackElementFunc) (GESClip *clip, GESTrackElement *track_element,
                                             GstElement *nleobj);

/**
 * GESCreateTrackElementFunc:
 * @clip: a #GESClip
 * @type: a #GESTrackType
 *
 * Creates the 'primary' track element for this @clip.
 *
 * Subclasses should implement this method if they only provide a
 * single #GESTrackElement per track.
 *
 * If the subclass needs to create more than one #GESTrackElement for a
 * given track, then it should implement the 'create_track_elements'
 * method instead.
 *
 * The implementer of this function shall return the proper #GESTrackElement
 * that should be controlled by @clip for the given @track.
 *
 * The returned #GESTrackElement will be automatically added to the list
 * of objects controlled by the #GESClip.
 *
 * Returns: the #GESTrackElement to be used, or %NULL if it can't provide one
 * for the given @track.
 */
typedef GESTrackElement *(*GESCreateTrackElementFunc) (GESClip * clip, GESTrackType type);

/**
 * GESCreateTrackElementsFunc:
 * @clip: a #GESClip
 * @type: a #GESTrackType
 *
 * Create all track elements this clip handles for this type of track.
 *
 * Subclasses should implement this method if they potentially need to
 * return more than one #GESTrackElement(s) for a given #GESTrack.
 *
 * Returns: %TRUE on success %FALSE on failure.
 */

typedef GList * (*GESCreateTrackElementsFunc) (GESClip * clip, GESTrackType type);

/**
 * GESClip:
 *
 * The #GESClip base class.
 */
struct _GESClip
{
  GESContainer    parent;

  /*< private >*/
  GESClipPrivate *priv;

  /* Padding for API extension */
  gpointer       _ges_reserved[GES_PADDING_LARGE];
};

/**
 * GESClipClass:
 * @create_track_element: method to create a single #GESTrackElement for a given #GESTrack.
 * @create_track_elements: method to create multiple #GESTrackElements for a
 * #GESTrack.
 *
 * Subclasses can override the @create_track_element.
 */
struct _GESClipClass
{
  /*< private > */
  GESContainerClass          parent_class;

  /*< public > */
  GESCreateTrackElementFunc  create_track_element;
  GESCreateTrackElementsFunc create_track_elements;

  /*< private >*/
  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING_LARGE];
};

/****************************************************
 *                  Standard                        *
 ****************************************************/
GType ges_clip_get_type (void);

/****************************************************
 *                TrackElement handling             *
 ****************************************************/
GESTrackType      ges_clip_get_supported_formats  (GESClip *clip);
void              ges_clip_set_supported_formats  (GESClip *clip, GESTrackType       supportedformats);
GESTrackElement*  ges_clip_add_asset              (GESClip *clip, GESAsset *asset);
GESTrackElement*  ges_clip_find_track_element     (GESClip *clip, GESTrack *track,
                                                   GType type);
GList *           ges_clip_find_track_elements    (GESClip * clip, GESTrack * track,
                                                   GESTrackType track_type, GType type);

/****************************************************
 *                     Layer                        *
 ****************************************************/
GESLayer* ges_clip_get_layer              (GESClip *clip);
gboolean          ges_clip_move_to_layer          (GESClip *clip, GESLayer  *layer);

/****************************************************
 *                   Effects                        *
 ****************************************************/
GList*   ges_clip_get_top_effects           (GESClip *clip);
gint     ges_clip_get_top_effect_position   (GESClip *clip, GESBaseEffect *effect);
gint     ges_clip_get_top_effect_index   (GESClip *clip, GESBaseEffect *effect);
gboolean ges_clip_set_top_effect_priority   (GESClip *clip, GESBaseEffect *effect,
                                             guint newpriority);
gboolean ges_clip_set_top_effect_index   (GESClip *clip, GESBaseEffect *effect,
                                             guint newindex);

/****************************************************
 *                   Editing                        *
 ****************************************************/
GESClip* ges_clip_split  (GESClip *clip, guint64  position);

G_END_DECLS
#endif /* _GES_CLIP */
