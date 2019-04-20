/* GStreamer Editing Services
 *
 * Copyright (C) <2015> Thibault Saunier <tsaunier@gnome.org>
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

#ifndef _GES_COMMAND_LINE_FORMATTER_H_
#define _GES_COMMAND_LINE_FORMATTER_H_

#include <glib-object.h>
#include "ges-formatter.h"

G_BEGIN_DECLS

#define GES_TYPE_COMMAND_LINE_FORMATTER             (ges_command_line_formatter_get_type ())
#define GES_COMMAND_LINE_FORMATTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_COMMAND_LINE_FORMATTER, GESCommandLineFormatter))
#define GES_COMMAND_LINE_FORMATTER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_COMMAND_LINE_FORMATTER, GESCommandLineFormatterClass))
#define GES_IS_COMMAND_LINE_FORMATTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_COMMAND_LINE_FORMATTER))
#define GES_IS_COMMAND_LINE_FORMATTER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_COMMAND_LINE_FORMATTER))
#define GES_COMMAND_LINE_FORMATTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_COMMAND_LINE_FORMATTER, GESCommandLineFormatterClass))

typedef struct _GESCommandLineFormatterClass GESCommandLineFormatterClass;
typedef struct _GESCommandLineFormatter GESCommandLineFormatter;
typedef struct _GESCommandLineFormatterPrivate GESCommandLineFormatterPrivate;


struct _GESCommandLineFormatterClass
{
    GESFormatterClass parent_class;
};

struct _GESCommandLineFormatter
{
    GESFormatter parent_instance;

    GESCommandLineFormatterPrivate *priv;
};

GType ges_command_line_formatter_get_type (void);

G_END_DECLS

#endif /* _GES_COMMAND_LINE_FORMATTER_H_ */

