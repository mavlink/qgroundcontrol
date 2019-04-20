/* GStreamer Editing Services
 * Copyright (C) 2010 Brandon Lewis <brandon.lewis@collabora.co.uk>
 *               2010 Nokia Corporation
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

#ifndef _GES_VIDEO_TRANSITION
#define _GES_VIDEO_TRANSITION

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-transition.h>

G_BEGIN_DECLS

#define GES_TYPE_VIDEO_TRANSITION ges_video_transition_get_type()

#define GES_VIDEO_TRANSITION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_VIDEO_TRANSITION, GESVideoTransition))

#define GES_VIDEO_TRANSITION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_VIDEO_TRANSITION, GESVideoTransitionClass))

#define GES_IS_VIDEO_TRANSITION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_VIDEO_TRANSITION))

#define GES_IS_VIDEO_TRANSITION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_VIDEO_TRANSITION))

#define GES_VIDEO_TRANSITION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_VIDEO_TRANSITION, GESVideoTransitionClass))

typedef struct _GESVideoTransitionPrivate GESVideoTransitionPrivate;

/**
 * GESVideoTransition:
 */

struct _GESVideoTransition {
  GESTransition parent;

  /*< private >*/

  GESVideoTransitionPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESVideoTransitionClass:
 * @parent_class: parent class
 *
 */

struct _GESVideoTransitionClass {
  GESTransitionClass parent_class;

  /*< private >*/
  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_video_transition_get_type               (void);
GESVideoTransition* ges_video_transition_new (void);

gboolean ges_video_transition_set_transition_type (GESVideoTransition * self,
                                                         GESVideoStandardTransitionType type);
GESVideoStandardTransitionType
ges_video_transition_get_transition_type          (GESVideoTransition * trans);

void ges_video_transition_set_border              (GESVideoTransition * self,
                                                         guint value);
gint ges_video_transition_get_border              (GESVideoTransition * self);

void ges_video_transition_set_inverted            (GESVideoTransition * self,
                                                         gboolean inverted);
gboolean ges_video_transition_is_inverted        (GESVideoTransition * self);

G_END_DECLS

#endif /* _GES_TRACK_VIDEO_transition */

