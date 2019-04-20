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

#ifndef _GES_VIDEO_TEST_SOURCE
#define _GES_VIDEO_TEST_SOURCE

#include <glib-object.h>
#include <ges/ges-enums.h>
#include <ges/ges-types.h>
#include <ges/ges-video-source.h>

G_BEGIN_DECLS

#define GES_TYPE_VIDEO_TEST_SOURCE ges_video_test_source_get_type()

#define GES_VIDEO_TEST_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_VIDEO_TEST_SOURCE, GESVideoTestSource))

#define GES_VIDEO_TEST_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_VIDEO_TEST_SOURCE, GESVideoTestSourceClass))

#define GES_IS_VIDEO_TEST_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_VIDEO_TEST_SOURCE))

#define GES_IS_VIDEO_TEST_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_VIDEO_TEST_SOURCE))

#define GES_VIDEO_TEST_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_VIDEO_TEST_SOURCE, GESVideoTestSourceClass))

typedef struct _GESVideoTestSourcePrivate GESVideoTestSourcePrivate;

/**
 * GESVideoTestSource:
 */
struct _GESVideoTestSource {
  /*< private >*/
  GESVideoSource parent;

  GESVideoTestSourcePrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESVideoTestSourceClass {
  GESVideoSourceClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_video_test_source_get_type (void);

void
ges_video_test_source_set_pattern(GESVideoTestSource *self,
					GESVideoTestPattern pattern);
GESVideoTestPattern
ges_video_test_source_get_pattern (GESVideoTestSource *source);

G_END_DECLS

#endif /* _GES_VIDEO_TEST_SOURCE */
