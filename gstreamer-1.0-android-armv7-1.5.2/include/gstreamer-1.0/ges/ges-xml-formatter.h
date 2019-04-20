/* Gstreamer Editing Services
 *
 * Copyright (C) <2012> Thibault Saunier <thibault.saunier@collabora.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "ges-base-xml-formatter.h"

#ifndef GES_XML_FORMATTER_H
#define GES_XML_FORMATTER_H

G_BEGIN_DECLS
#define GES_TYPE_XML_FORMATTER (ges_xml_formatter_get_type ())
#define GES_XML_FORMATTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_XML_FORMATTER, GESXmlFormatter))
#define GES_XML_FORMATTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_XML_FORMATTER, GESXmlFormatterClass))
#define GES_IS_XML_FORMATTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_XML_FORMATTER))
#define GES_IS_XML_FORMATTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_XML_FORMATTER))
#define GES_XML_FORMATTER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_XML_FORMATTER, GESXmlFormatterClass))
typedef struct _GESXmlFormatterPrivate GESXmlFormatterPrivate;

typedef struct
{
  GESBaseXmlFormatter parent;

  GESXmlFormatterPrivate *priv;

  gpointer _ges_reserved[GES_PADDING];
} GESXmlFormatter;

typedef struct
{
  GESBaseXmlFormatterClass parent;

  gpointer _ges_reserved[GES_PADDING];
} GESXmlFormatterClass;

GType ges_xml_formatter_get_type (void);

G_END_DECLS
#endif /* _GES_XML_FORMATTER_H */
