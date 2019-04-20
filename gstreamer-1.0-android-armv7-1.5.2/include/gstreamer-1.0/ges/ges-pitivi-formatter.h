/* GStreamer Editing Services Pitivi Formatter
 * Copyright (C) 2011-2012 Mathieu Duponchelle <seeed@laposte.net>
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

#ifndef _GES_PITIVI_FORMATTER
#define _GES_PITIVI_FORMATTER

#define GES_TYPE_PITIVI_FORMATTER ges_pitivi_formatter_get_type()

#define GES_PITIVI_FORMATTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_PITIVI_FORMATTER, GESPitiviFormatter))

#define GES_PITIVI_FORMATTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_PITIVI_FORMATTER, GESPitiviFormatterClass))

#define GES_IS_PITIVI_FORMATTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_PITIVI_FORMATTER))

#define GES_IS_PITIVI_FORMATTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_PITIVI_FORMATTER))

#define GES_PITIVI_FORMATTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_PITIVI_FORMATTER, GESPitiviFormatterClass))

typedef struct _GESPitiviFormatterPrivate GESPitiviFormatterPrivate;


/**
 * GESPitiviFormatter:
 *
 * Serializes a #GESTimeline to a file using the xptv Pitivi file format
 */
struct _GESPitiviFormatter {
  GESFormatter parent;

  /*< public > */
  /*< private >*/
  GESPitiviFormatterPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESPitiviFormatterClass
{
  /*< private >*/
  GESFormatterClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_pitivi_formatter_get_type (void);
GESPitiviFormatter *ges_pitivi_formatter_new (void);

#endif /* _GES_PITIVI_FORMATTER */
