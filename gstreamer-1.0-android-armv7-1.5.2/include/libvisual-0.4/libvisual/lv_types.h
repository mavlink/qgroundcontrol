/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_types.h,v 1.11 2006/02/13 20:54:08 synap Exp $
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

#ifndef _LV_TYPES_H
#define _LV_TYPES_H

#include <libvisual/lv_defines.h>

#if defined(VISUAL_OS_WIN32)
#include <stdint.h>
#else
#include <sys/types.h>
#endif /* !VISUAL_OS_WIN32 */

VISUAL_BEGIN_DECLS

#define VISUAL_CHECK_CAST(uiobj, cast)		((cast*) (uiobj))

#define VISUAL_TABLESIZE(table)			(sizeof (table) / sizeof (table[0]))

#if !defined(VISUAL_OS_WIN32)
#ifndef uint8_t
#define uint8_t		u_int8_t
#endif

#ifndef uint16_t
#define uint16_t	u_int16_t
#endif

#ifndef uint32_t
#define uint32_t	u_int32_t
#endif
#endif /* !VISUAL_OS_WIN32 */

VISUAL_END_DECLS

#endif /* _LV_TYPES_H */
