/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_defines.h,v 1.7 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_DEFINES_H
#define _LV_DEFINES_H

#ifdef __cplusplus
# define VISUAL_C_LINKAGE extern "C"
#else
# define VISUAL_C_LINKAGE
#endif /* __cplusplus */

#ifdef __cplusplus
# define VISUAL_BEGIN_DECLS	VISUAL_C_LINKAGE {
# define VISUAL_END_DECLS	}
#else
# define VISUAL_BEGIN_DECLS
# define VISUAL_END_DECLS
#endif /* __cplusplus */

#ifdef NULL
#undef NULL
#endif

/**
 * NULL define.
 */
#define NULL	((void *) 0)

#ifndef FALSE
/**
 * FALSE define.
 */
#define FALSE	(0)
#endif

#ifndef TRUE
/**
 * TRUE define.
 */
#define TRUE	(!FALSE)
#endif

/* Compiler specific optimalization macros */
#if __GNUC__ >= 3
# ifndef inline
#  define inline		inline __attribute__ ((always_inline))
#endif
# ifndef __malloc
#  define __malloc		__attribute__ ((malloc))
# endif
# ifndef __packed
#  define __packed		__attribute__ ((packed))
# endif
# define VIS_LIKELY(x)		__builtin_expect (!!(x), 1)
# define VIS_UNLIKELY(x)	__builtin_expect (!!(x), 0)
#else
# ifndef inline
#  define inline		/* no inline */
# endif
# ifndef __malloc
#  define __malloc		/* no malloc */
# endif
# ifndef __packed
#  define __packed		/* no packed */
# endif
# define VIS_LIKELY(x)		(x)
# define VIS_UNLIKELY(x)	(x)
#endif

#endif /* _LV_DEFINES_H */
