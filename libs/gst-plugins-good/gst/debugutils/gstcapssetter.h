/* GStreamer Element
 * Copyright (C) 2006-2009 Mark Nauwelaerts <mnauw@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1307, USA.
 */


#ifndef __GST_CAPS_SETTER_H__
#define __GST_CAPS_SETTER_H__

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_CAPS_SETTER \
  (gst_caps_setter_get_type())
#define GST_CAPS_SETTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CAPS_SETTER,GstCapsSetter))
#define GST_CAPS_SETTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CAPS_SETTER,GstCapsSetterClass))
#define GST_IS_CAPS_SETTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CAPS_SETTER))
#define GST_IS_CAPS_SETTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CAPS_SETTER))

GType gst_caps_setter_get_type (void);

typedef struct _GstCapsSetter GstCapsSetter;
typedef struct _GstCapsSetterClass GstCapsSetterClass;

struct _GstCapsSetter
{
  GstBaseTransform parent;

  /* < private > */
  /* properties */
  GstCaps *caps;
  gboolean join;
  gboolean replace;
};


struct _GstCapsSetterClass
{
  GstBaseTransformClass parent_class;
};

G_END_DECLS

#endif /* __GST_CAPS_SETTER_H__ */
