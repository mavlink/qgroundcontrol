/* GStreamer Editing Services
 * Copyright (C) 2009 Brandon Lewis <brandon.lewis@collabora.co.uk>
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

#ifndef _GES_TL_TESTSOURCE
#define _GES_TL_TESTSOURCE

#include <glib-object.h>
#include <ges/ges-enums.h>
#include <ges/ges-types.h>
#include <ges/ges-source-clip.h>
#include <ges/ges-track.h>

G_BEGIN_DECLS

#define GES_TYPE_TEST_CLIP ges_test_clip_get_type()

#define GES_TEST_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_TEST_CLIP, GESTestClip))

#define GES_TEST_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_TEST_CLIP, GESTestClipClass))

#define GES_IS_TEST_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_TEST_CLIP))

#define GES_IS_TEST_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_TEST_CLIP))

#define GES_TEST_CLIP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_TEST_CLIP, GESTestClipClass))

typedef struct _GESTestClipPrivate GESTestClipPrivate;

/**
 * GESTestClip:
 * 
 */

struct _GESTestClip {

  GESSourceClip parent;

  /*< private >*/
  GESTestClipPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESTestClipClass:
 */

struct _GESTestClipClass {
  /*< private >*/
  GESSourceClipClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_test_clip_get_type (void);

void
ges_test_clip_set_mute (GESTestClip * self, gboolean mute);

void
ges_test_clip_set_vpattern (GESTestClip * self,
    GESVideoTestPattern vpattern);

void
ges_test_clip_set_frequency (GESTestClip * self, gdouble freq);

void
ges_test_clip_set_volume (GESTestClip * self,
    gdouble volume);


GESVideoTestPattern
ges_test_clip_get_vpattern (GESTestClip * self);

gboolean ges_test_clip_is_muted (GESTestClip * self);
gdouble ges_test_clip_get_frequency (GESTestClip * self);
gdouble ges_test_clip_get_volume (GESTestClip * self);

GESTestClip* ges_test_clip_new (void);
GESTestClip* ges_test_clip_new_for_nick(gchar * nick);

G_END_DECLS

#endif /* _GES_TL_TESTSOURCE */

