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

#ifndef _GES_OVERLAY_TEXT_CLIP
#define _GES_OVERLAY_TEXT_CLIP

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-overlay-clip.h>
#include <ges/ges-track.h>

G_BEGIN_DECLS
#define GES_TYPE_OVERLAY_TEXT_CLIP ges_text_overlay_clip_get_type()
#define GES_OVERLAY_TEXT_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_OVERLAY_TEXT_CLIP, GESTextOverlayClip))
#define GES_OVERLAY_TEXT_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_OVERLAY_TEXT_CLIP, GESTextOverlayClipClass))
#define GES_IS_OVERLAY_TEXT_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_OVERLAY_TEXT_CLIP))
#define GES_IS_OVERLAY_TEXT_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_OVERLAY_TEXT_CLIP))
#define GES_OVERLAY_TEXT_CLIP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_OVERLAY_TEXT_CLIP, GESTextOverlayClipClass))
typedef struct _GESTextOverlayClipPrivate GESTextOverlayClipPrivate;

/**
 * GESTextOverlayClip:
 * 
 */

struct _GESTextOverlayClip
{
  GESOverlayClip parent;

  /*< private > */
  GESTextOverlayClipPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESTextOverlayClipClass:
 */

struct _GESTextOverlayClipClass
{
  /*< private > */

  GESOverlayClipClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_text_overlay_clip_get_type (void);

void
ges_text_overlay_clip_set_text (GESTextOverlayClip * self,
    const gchar * text);

void
ges_text_overlay_clip_set_font_desc (GESTextOverlayClip * self,
    const gchar * font_desc);

void
ges_text_overlay_clip_set_valign (GESTextOverlayClip * self,
    GESTextVAlign valign);

void
ges_text_overlay_clip_set_halign (GESTextOverlayClip * self,
    GESTextHAlign halign);

void
ges_text_overlay_clip_set_color (GESTextOverlayClip * self,
    guint32 color);

void
ges_text_overlay_clip_set_xpos (GESTextOverlayClip * self,
    gdouble position);

void
ges_text_overlay_clip_set_ypos (GESTextOverlayClip * self,
    gdouble position);

const gchar *ges_text_overlay_clip_get_text (GESTextOverlayClip * self);

const gchar *ges_text_overlay_clip_get_font_desc (GESTextOverlayClip *
    self);

GESTextVAlign
ges_text_overlay_clip_get_valignment (GESTextOverlayClip * self);

const guint32
ges_text_overlay_clip_get_color (GESTextOverlayClip * self);

const gdouble
ges_text_overlay_clip_get_xpos (GESTextOverlayClip * self);

const gdouble
ges_text_overlay_clip_get_ypos (GESTextOverlayClip * self);

GESTextHAlign
ges_text_overlay_clip_get_halignment (GESTextOverlayClip * self);

GESTextOverlayClip *ges_text_overlay_clip_new (void);

G_END_DECLS
#endif /* _GES_TL_OVERLAY */
