/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * dice.c: a 'dicing' effect
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Library Public License.
 *
 * I suppose this looks similar to PuzzleTV, but it's not. The screen is
 * divided into small squares, each of which is rotated either 0, 90, 180 or
 * 270 degrees.  The amount of rotation for each square is chosen at random.
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

#ifndef __GST_DICE_H__
#define __GST_DICE_H__

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_DICETV \
  (gst_dicetv_get_type())
#define GST_DICETV(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DICETV,GstDiceTV))
#define GST_DICETV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DICETV,GstDiceTVClass))
#define GST_IS_DICETV(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DICETV))
#define GST_IS_DICETV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DICETV))

typedef struct _GstDiceTV GstDiceTV;
typedef struct _GstDiceTVClass GstDiceTVClass;

struct _GstDiceTV
{
  GstVideoFilter videofilter;

  /* < private > */
  guint8 *dicemap;

  gint g_cube_bits;
  gint g_cube_size;
  gint g_map_height;
  gint g_map_width;
};

struct _GstDiceTVClass
{
  GstVideoFilterClass parent_class;
};

GType gst_dicetv_get_type (void);

G_END_DECLS

#endif /* __GST_DICE_H__ */
