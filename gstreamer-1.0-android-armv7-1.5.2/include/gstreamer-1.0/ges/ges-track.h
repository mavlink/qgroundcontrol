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

#ifndef _GES_TRACK
#define _GES_TRACK

#include <glib-object.h>
#include <gst/gst.h>
#include <ges/ges-types.h>
#include <ges/ges-enums.h>

G_BEGIN_DECLS

#define GES_TYPE_TRACK            ges_track_get_type()
#define GES_TRACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_TRACK, GESTrack))
#define GES_TRACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_TRACK, GESTrackClass))
#define GES_IS_TRACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_TRACK))
#define GES_IS_TRACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_TRACK))
#define GES_TRACK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_TRACK, GESTrackClass))

typedef struct _GESTrackPrivate GESTrackPrivate;

/**
 * GESCreateElementForGapFunc:
 * @track: the #GESTrack
 *
 * A function that will be called to create the #GstElement that will be used
 * as a source to fill the gaps in @track.
 *
 * Returns: A #GstElement (must be a source) that will be used to
 * fill the gaps (periods of time in @track that containes no source).
 */
typedef GstElement* (*GESCreateElementForGapFunc) (GESTrack *track);

/**
 * GESTrack:
 * @type: a #GESTrackType indicting the basic type of the track.
 */
struct _GESTrack
{
  GstBin parent;

  /*< public >*/
  /* READ-ONLY */
  GESTrackType     type;

  /*< private >*/
  GESTrackPrivate* priv;
  /* Padding for API extension */
  gpointer         _ges_reserved[GES_PADDING];
};

/**
 * GESTrackClass:
 */
struct _GESTrackClass
{
  /*< private >*/
  GstBinClass parent_class;

  GstElement * (*get_mixing_element) (GESTrack *track);

  /* Padding for API extension */
  gpointer    _ges_reserved[GES_PADDING];
};

const GstCaps*     ges_track_get_caps                        (GESTrack *track);
GList*             ges_track_get_elements                    (GESTrack *track);
const GESTimeline* ges_track_get_timeline                    (GESTrack *track);
gboolean           ges_track_commit                          (GESTrack *track);
void               ges_track_set_timeline                    (GESTrack *track, GESTimeline *timeline);
gboolean           ges_track_add_element                     (GESTrack *track, GESTrackElement *object);
gboolean           ges_track_remove_element                  (GESTrack *track, GESTrackElement *object);
void               ges_track_set_create_element_for_gap_func (GESTrack *track, GESCreateElementForGapFunc func);
void               ges_track_set_mixing                      (GESTrack *track, gboolean mixing);
gboolean           ges_track_get_mixing                      (GESTrack *track);
void               ges_track_set_restriction_caps            (GESTrack *track, const GstCaps *caps);
void               ges_track_update_restriction_caps         (GESTrack *track, const GstCaps *caps);

/* standard methods */
GType              ges_track_get_type                        (void);
GESTrack*          ges_track_new                             (GESTrackType type, GstCaps * caps);

G_END_DECLS

#endif /* _GES_TRACK */
