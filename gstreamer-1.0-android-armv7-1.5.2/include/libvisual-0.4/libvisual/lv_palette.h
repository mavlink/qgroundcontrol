/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_palette.h,v 1.15 2006/01/22 13:23:37 synap Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_PALETTE_H
#define _LV_PALETTE_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_color.h>

VISUAL_BEGIN_DECLS

#define VISUAL_PALETTE(obj)				(VISUAL_CHECK_CAST ((obj), VisPalette))

typedef struct _VisPalette VisPalette;

/**
 * Data type to describe the palette for an 8 bits screen depth.
 * 
 * To access the RGB value of a certain indexed color simply do:
 * pal->colors[index].(r,g,b)
 *
 * @see visual_palette_new
 */
struct _VisPalette {
	VisObject	 object;	/**< The VisObject data. */
	int		 ncolors;	/**< Number of color entries in palette. */
	VisColor	*colors;	/**< Pointer to the colors. */
};

/* prototypes */
VisPalette *visual_palette_new (int ncolors);
int visual_palette_init (VisPalette *pal);

int visual_palette_copy (VisPalette *dest, VisPalette *src);

int visual_palette_allocate_colors (VisPalette *pal, int ncolors);
int visual_palette_free_colors (VisPalette *pal);

int visual_palette_blend (VisPalette *dest, VisPalette *src1, VisPalette *src2, float rate);
VisColor *visual_palette_color_cycle (VisPalette *pal, float rate);

int visual_palette_find_color (VisPalette *pal, VisColor *color);

VISUAL_END_DECLS

#endif /* _LV_PALETTE_H */
