/* gstgoom.c: implementation of goom drawing element
 * Copyright (C) <2001> Richard Boulton <richard@tartarus.org>
 *           (C) <2006> Wim Taymans <wim at fluendo dot com>
 * Copyright (C) <2015> Luis de Bethencourt <luis@debethencourt.com>
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

#ifndef __GST_GOOM_H__
#define __GST_GOOM_H__

#include <gst/pbutils/gstaudiovisualizer.h>

#include "goom_core.h"

G_BEGIN_DECLS

#define GOOM2K1_SAMPLES 512

#define GST_TYPE_GOOM2K1            (gst_goom2k1_get_type())
#define GST_GOOM2K1(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GOOM2K1,GstGoom2k1))
#define GST_GOOM2K1_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GOOM2K1,GstGoom2k1Class))
#define GST_IS_GOOM2K1(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GOOM2K1))
#define GST_IS_GOOM2K1_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GOOM2K1))

typedef struct _GstGoom2k1 GstGoom2k1;
typedef struct _GstGoom2k1Class GstGoom2k1Class;

struct _GstGoom2k1
{
  GstAudioVisualizer parent;

  /* input tracking */
  gint channels;

  /* video state */
  gint width;
  gint height;

  /* goom stuff */
  GoomData goomdata;
};

struct _GstGoom2k1Class
{
  GstAudioVisualizerClass parent_class;
};

GType gst_goom2k1_get_type (void);
gboolean gst_goom2k1_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_GOOM_H__ */

