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

#ifndef _GES_URI_CLIP
#define _GES_URI_CLIP

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-source-clip.h>
#include <ges/ges-track.h>

G_BEGIN_DECLS

#define GES_TYPE_URI_CLIP ges_uri_clip_get_type()

#define GES_URI_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_URI_CLIP, GESUriClip))

#define GES_URI_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_URI_CLIP, GESUriClipClass))

#define GES_IS_URI_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_URI_CLIP))

#define GES_IS_URI_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_URI_CLIP))

#define GES_URI_CLIP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_URI_CLIP, GESUriClipClass))

typedef struct _GESUriClipPrivate GESUriClipPrivate;

struct _GESUriClip {
  GESSourceClip parent;

  /*< private >*/
  GESUriClipPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESUriClipClass:
 */

struct _GESUriClipClass {
  /*< private >*/
  GESSourceClipClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_uri_clip_get_type (void);

void
ges_uri_clip_set_mute (GESUriClip * self, gboolean mute);

void
ges_uri_clip_set_is_image (GESUriClip * self,
    gboolean is_image);

gboolean ges_uri_clip_is_muted (GESUriClip * self);
gboolean ges_uri_clip_is_image (GESUriClip * self);
const gchar *ges_uri_clip_get_uri (GESUriClip * self);

GESUriClip* ges_uri_clip_new (gchar *uri);

G_END_DECLS

#endif /* _GES_URI_CLIP */

