/* GStreamer Split File Source
 * Copyright (C) 2011 Collabora Ltd. <tim.muller@collabora.co.uk>
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
#ifndef __GST_SPLIT_FILE_SRC_H__
#define __GST_SPLIT_FILE_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GST_TYPE_SPLIT_FILE_SRC \
  (gst_split_file_src_get_type())
#define GST_SPLIT_FILE_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SPLIT_FILE_SRC,GstSplitFileSrc))
#define GST_SPLIT_FILE_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SPLIT_FILE_SRC,GstSplitFileSrcClass))
#define GST_IS_SPLIT_FILE_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SPLIT_FILE_SRC))
#define GST_IS_SPLIT_FILE_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SPLIT_FILE_SRC))

typedef struct _GstFilePart GstFilePart;
typedef struct _GstSplitFileSrc GstSplitFileSrc;
typedef struct _GstSplitFileSrcClass GstSplitFileSrcClass;

struct _GstFilePart
{
  GFileInputStream  *stream;
  gchar             *path;
  guint64            start; /* inclusive */
  guint64            stop;  /* inclusive */
};

struct _GstSplitFileSrc
{
  GstBaseSrc   parent;

  gchar       *location;  /* OBJECT_LOCK */

  GstFilePart *parts;
  guint        num_parts;

  guint        cur_part;  /* part used last (likely also to be used next) */

  GCancellable *cancellable; /* so we can interrupt blocking operations */
};

struct _GstSplitFileSrcClass
{
  GstBaseSrcClass parent_class;
};

GType gst_split_file_src_get_type (void);

G_END_DECLS

#endif /* __GST_SPLIT_FILE_SRC_H__ */
