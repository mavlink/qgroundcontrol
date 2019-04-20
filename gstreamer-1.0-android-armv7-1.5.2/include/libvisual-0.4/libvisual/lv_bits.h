/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_bits.h,v 1.3 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_ENDIANESS_H
#define _LV_ENDIANESS_H

#include <libvisual/lvconfig.h>
#include <libvisual/lv_defines.h>

VISUAL_BEGIN_DECLS

/**
 * Macros to convert LE <-> BE
 * 'I' stands for integer here.
 */
#define VISUAL_ENDIAN_LE_BE_I16(w) (\
	(((w) & 0xff00) >> 8) |\
	(((w) & 0x00ff) << 8) )

#define VISUAL_ENDIAN_LE_BE_I32(w) (\
	(((w) & 0x000000ff) << 24) | \
	(((w) & 0x0000ff00) << 8) | \
	(((w) & 0x00ff0000) >> 8) | \
	(((w) & 0xff000000) >> 24) )


#if VISUAL_BIG_ENDIAN & VISUAL_LITTLE_ENDIAN
#	error determining system endianess.
#endif

/**
 * Arch. dependant definitions
 */

#if VISUAL_BIG_ENDIAN
#	define VISUAL_ENDIAN_BEI16(x) (x)
#	define VISUAL_ENDIAN_BEI32(x) (x)
#	define VISUAL_ENDIAN_LEI16(x) VISUAL_ENDIAN_LE_BE_I16(x)
#	define VISUAL_ENDIAN_LEI32(x) VISUAL_ENDIAN_LE_BE_I32(x)
#endif

#if VISUAL_LITTLE_ENDIAN
#	define VISUAL_ENDIAN_LEI16(x) (x)
#	define VISUAL_ENDIAN_LEI32(x) (x)
#	define VISUAL_ENDIAN_BEI16(x) VISUAL_ENDIAN_LE_BE_I16(x)
#	define VISUAL_ENDIAN_BEI32(x) VISUAL_ENDIAN_LE_BE_I32(x)
#endif

/**
 * Macro to check if 'x' is aligned on 'y' bytes. This macro will fail
 * when supplied with '1'. However you wouldn't want to do that anyway.
 */
#define VISUAL_ALIGNED(x, y)	(!(((unsigned long) x) & (y - 1)))

VISUAL_END_DECLS

#endif /* _LV_ENDIANESS_H */
