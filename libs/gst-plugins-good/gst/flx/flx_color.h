/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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

#ifndef __FLX_COLOR_H__
#define __FLX_COLOR_H__

#include <gst/gst.h>

G_BEGIN_DECLS

typedef enum {
  FLX_COLORSPACE_RGB8,
  FLX_COLORSPACE_RGB32,
} FlxColorSpaceType;


typedef struct _FlxColorSpaceConverter FlxColorSpaceConverter;

struct _FlxColorSpaceConverter {
  guint      width;
  guint      height;
  guchar      palvec[768];
};

void flx_colorspace_converter_destroy(FlxColorSpaceConverter *flxpal);
void flx_colorspace_convert(FlxColorSpaceConverter *flxpal, guchar *src, guchar *dest);
FlxColorSpaceConverter * flx_colorspace_converter_new(gint width, gint height);

void flx_set_palette_vector(FlxColorSpaceConverter *flxpal, guint start, guint num, 
          guchar *newpal, gint scale);
void flx_set_color(FlxColorSpaceConverter *flxpal, guint colr, guint red, guint green,
          guint blue, gint scale);

G_END_DECLS

#endif /* __FLX_COLOR_H__ */
