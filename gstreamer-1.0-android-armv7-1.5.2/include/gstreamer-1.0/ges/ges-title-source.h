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

#ifndef _GES_TITLE_SOURCE
#define _GES_TITLE_SOURCE

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-video-source.h>

G_BEGIN_DECLS

#define GES_TYPE_TITLE_SOURCE ges_title_source_get_type()

#define GES_TITLE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_TITLE_SOURCE, GESTitleSource))

#define GES_TITLE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_TITLE_SOURCE, GESTitleSourceClass))

#define GES_IS_TITLE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_TITLE_SOURCE))

#define GES_IS_TITLE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_TITLE_SOURCE))

#define GES_TITLE_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_TITLE_SOURCE, GESTitleSourceClass))

typedef struct _GESTitleSourcePrivate GESTitleSourcePrivate;

/** 
 * GESTitleSource:
 *
 */
struct _GESTitleSource {
  GESVideoSource parent;

  /*< private >*/
  GESTitleSourcePrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESTitleSourceClass:
 * @parent_class: parent class
 */

struct _GESTitleSourceClass {
  GESVideoSourceClass parent_class;

  /*< private >*/

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING - 1];
};

GType ges_title_source_get_type (void);

void ges_title_source_set_text (GESTitleSource *self,
				     const gchar *text);

void ges_title_source_set_font_desc (GESTitleSource *self,
					  const gchar *font_desc);

void ges_title_source_set_halignment (GESTitleSource *self,
					   GESTextHAlign halign);

void ges_title_source_set_valignment (GESTitleSource *self,
					   GESTextVAlign valign);

void ges_title_source_set_text_color (GESTitleSource *self,
					   guint32 color);
void ges_title_source_set_background_color (GESTitleSource *self,
					   guint32 color);
void ges_title_source_set_xpos (GESTitleSource *self,
					   gdouble position);
void ges_title_source_set_ypos (GESTitleSource *self,
					   gdouble position);

const gchar *ges_title_source_get_text (GESTitleSource *source);
const gchar *ges_title_source_get_font_desc (GESTitleSource *source);
GESTextHAlign ges_title_source_get_halignment (GESTitleSource *source);
GESTextVAlign ges_title_source_get_valignment (GESTitleSource *source);
const guint32 ges_title_source_get_text_color (GESTitleSource *source);
const guint32 ges_title_source_get_background_color (GESTitleSource *source);
const gdouble ges_title_source_get_xpos (GESTitleSource *source);
const gdouble ges_title_source_get_ypos (GESTitleSource *source);

G_END_DECLS

#endif /* _GES_TITLE_SOURCE */

