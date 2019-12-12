/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@temple-baptist.com>
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

#include <string.h>
#include <gst/gst.h>

#include "flx_color.h"

FlxColorSpaceConverter *
flx_colorspace_converter_new (gint width, gint height)
{
  FlxColorSpaceConverter *new = g_malloc (sizeof (FlxColorSpaceConverter));

  new->width = width;
  new->height = height;

  memset (new->palvec, 0, sizeof (new->palvec));
  return new;
}

void
flx_colorspace_converter_destroy (FlxColorSpaceConverter * flxpal)
{
  g_return_if_fail (flxpal != NULL);

  g_free (flxpal);
}

void
flx_colorspace_convert (FlxColorSpaceConverter * flxpal, guchar * src,
    guchar * dest)
{
  guint size, col;

  g_return_if_fail (flxpal != NULL);
  g_return_if_fail (src != dest);


  size = flxpal->width * flxpal->height;

  while (size--) {
    col = (*src++ * 3);

#if G_BYTE_ORDER == G_BIG_ENDIAN
    *dest++ = 0;
    *dest++ = flxpal->palvec[col];
    *dest++ = flxpal->palvec[col + 1];
    *dest++ = flxpal->palvec[col + 2];
#else
    *dest++ = flxpal->palvec[col + 2];
    *dest++ = flxpal->palvec[col + 1];
    *dest++ = flxpal->palvec[col];
    *dest++ = 0;
#endif
  }

}


void
flx_set_palette_vector (FlxColorSpaceConverter * flxpal, guint start, guint num,
    guchar * newpal, gint scale)
{
  guint grab;

  g_return_if_fail (flxpal != NULL);
  g_return_if_fail (start < 0x100);

  grab = ((start + num) > 0x100 ? 0x100 - start : num);

  if (scale) {
    gint i = 0;

    start *= 3;
    while (grab) {
      flxpal->palvec[start++] = newpal[i++] << scale;
      flxpal->palvec[start++] = newpal[i++] << scale;
      flxpal->palvec[start++] = newpal[i++] << scale;
      grab--;
    }
  } else {
    memcpy (&flxpal->palvec[start * 3], newpal, grab * 3);
  }
}

void
flx_set_color (FlxColorSpaceConverter * flxpal, guint colr, guint red,
    guint green, guint blue, gint scale)
{

  g_return_if_fail (flxpal != NULL);
  g_return_if_fail (colr < 0x100);

  flxpal->palvec[(colr * 3)] = red << scale;
  flxpal->palvec[(colr * 3) + 1] = green << scale;
  flxpal->palvec[(colr * 3) + 2] = blue << scale;
}
