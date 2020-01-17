/* GStreamer
 * Copyright (C) 2008 Stefan Kost <ensonic@users.sf.net>
 *
 * gsttaginject.h:
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


#ifndef __GST_TAG_INJECT_H__
#define __GST_TAG_INJECT_H__


#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS
#define GST_TYPE_TAG_INJECT \
  (gst_tag_inject_get_type())
#define GST_TAG_INJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TAG_INJECT,GstTagInject))
#define GST_TAG_INJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TAG_INJECT,GstTagInjectClass))
#define GST_IS_TAG_INJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TAG_INJECT))
#define GST_IS_TAG_INJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TAG_INJECT))
typedef struct _GstTagInject GstTagInject;
typedef struct _GstTagInjectClass GstTagInjectClass;

/**
 * GstTagInject:
 *
 * Opaque #GstTagInject data structure
 */
struct _GstTagInject
{
  GstBaseTransform element;

  /*< private > */
  GstTagList *tags;
  gboolean tags_sent;
};

struct _GstTagInjectClass
{
  GstBaseTransformClass parent_class;
};

GType gst_tag_inject_get_type (void);

G_END_DECLS
#endif /* __GST_TAG_INJECT_H__ */
