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

#ifndef _GES_TIMELINE_TITLESOURCE
#define _GES_TIMELINE_TITLESOURCE

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-source-clip.h>
#include <ges/ges-track.h>

G_BEGIN_DECLS

#define GES_TYPE_TITLE_CLIP ges_title_clip_get_type()

#define GES_TITLE_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_TITLE_CLIP, GESTitleClip))

#define GES_TITLE_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_TITLE_CLIP, GESTitleClipClass))

#define GES_IS_TITLE_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_TITLE_CLIP))

#define GES_IS_TITLE_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_TITLE_CLIP))

#define GES_TITLE_CLIP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_TITLE_CLIP, GESTitleClipClass))

typedef struct _GESTitleClipPrivate GESTitleClipPrivate;

/**
 * GESTitleClip:
 *
 * Render stand-alone titles in GESLayer.
 */

struct _GESTitleClip {
  GESSourceClip parent;

  /*< private >*/
  GESTitleClipPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESTitleClipClass {
  /*< private >*/
  GESSourceClipClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_title_clip_get_type (void);

void
ges_title_clip_set_text( GESTitleClip * self,
    const gchar * text);

void
ges_title_clip_set_font_desc (GESTitleClip * self,
    const gchar * font_desc);

void
ges_title_clip_set_valignment (GESTitleClip * self,
    GESTextVAlign valign);

void
ges_title_clip_set_halignment (GESTitleClip * self,
    GESTextHAlign halign);

void
ges_title_clip_set_color (GESTitleClip * self,
    guint32 color);

void
ges_title_clip_set_background (GESTitleClip * self,
    guint32 background);

void
ges_title_clip_set_xpos (GESTitleClip * self,
    gdouble position);

void
ges_title_clip_set_ypos (GESTitleClip * self,
    gdouble position);

const gchar*
ges_title_clip_get_font_desc (GESTitleClip * self);

GESTextVAlign
ges_title_clip_get_valignment (GESTitleClip * self);

GESTextHAlign
ges_title_clip_get_halignment (GESTitleClip * self);

const guint32
ges_title_clip_get_text_color (GESTitleClip * self);

const guint32
ges_title_clip_get_background_color (GESTitleClip * self);

const gdouble
ges_title_clip_get_xpos (GESTitleClip * self);

const gdouble
ges_title_clip_get_ypos (GESTitleClip * self);

const gchar* ges_title_clip_get_text (GESTitleClip * self);

GESTitleClip* ges_title_clip_new (void);

G_END_DECLS

#endif /* _GES_TIMELINE_TITLESOURCE */

