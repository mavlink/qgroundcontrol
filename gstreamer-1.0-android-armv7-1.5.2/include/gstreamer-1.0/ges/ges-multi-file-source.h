/* GStreamer Editing Services
 * Copyright (C) 2013 Lubosz Sarnecki <lubosz@gmail.com>
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

#ifndef _GES_MULTI_FILE_SOURCE
#define _GES_MULTI_FILE_SOURCE

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-video-source.h>

G_BEGIN_DECLS
#define GES_TYPE_MULTI_FILE_SOURCE ges_multi_file_source_get_type()
#define GES_MULTI_FILE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_MULTI_FILE_SOURCE, GESMultiFileSource))
#define ges_multi_file_source_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_MULTI_FILE_SOURCE, GESMultiFileSourceClass))
#define GES_IS_MULTI_FILE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_MULTI_FILE_SOURCE))
#define GES_IS_MULTI_FILE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_MULTI_FILE_SOURCE))
#define ges_multi_file_source_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_MULTI_FILE_SOURCE, GESMultiFileSourceClass))
typedef struct _GESMultiFileSourcePrivate GESMultiFileSourcePrivate;

/**
 * GESMultiFileSource:
 */
struct _GESMultiFileSource
{
  /*< private > */
  GESVideoSource parent;

  gchar *uri;

  GESMultiFileSourcePrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESMultiFileSourceClass
{
  GESVideoSourceClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_multi_file_source_get_type (void);

GESMultiFileSource *ges_multi_file_source_new (gchar * uri);

#define GES_MULTI_FILE_URI_PREFIX "multifile://"

G_END_DECLS
#endif /* _GES_MULTI_FILE_SOURCE */
