/* GStreamer Tuner
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * tunernorm.c: tuner norm object design
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tunernorm.h"

/**
 * SECTION:gsttunernorm
 * @title: TunErnorm.h
 * @short_description: Encapsulates information about the data format(s)
 * for a #GstTunerChannel.
 *
 * The #GstTunerNorm object is created by an element implementing the
 * #GstTuner interface and encapsulates the selection of a capture/output format
 * for a selected #GstTunerChannel.
 *
 */

enum
{
  /* FILL ME */
  LAST_SIGNAL
};

G_DEFINE_TYPE (GstTunerNorm, gst_tuner_norm, G_TYPE_OBJECT);

static void gst_tuner_norm_dispose (GObject * object);


/*static guint signals[LAST_SIGNAL] = { 0 };*/

static void
gst_tuner_norm_class_init (GstTunerNormClass * klass)
{
  GObjectClass *object_klass = (GObjectClass *) klass;


  object_klass->dispose = gst_tuner_norm_dispose;
}

static void
gst_tuner_norm_init (GstTunerNorm * norm)
{
  norm->label = NULL;
  g_value_init (&norm->framerate, GST_TYPE_FRACTION);
}

static void
gst_tuner_norm_dispose (GObject * object)
{
  GstTunerNorm *norm = GST_TUNER_NORM (object);

  if (norm->label) {
    g_free (norm->label);
    norm->label = NULL;
  }

  G_OBJECT_CLASS (gst_tuner_norm_parent_class)->dispose (object);
}
