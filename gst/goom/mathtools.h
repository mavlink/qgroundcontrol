/* Goom Project
 * Copyright (C) <2003> Jean-Christophe Hoelt <jeko@free.fr>
 *
 * goom_core.c:Contains the core of goom's work.
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

 #ifndef MATHTOOLS_H
#define MATHTOOLS_H

#include <glib.h>

#define _double2fixmagic (68719476736.0*1.5)
/* 2^36 * 1.5,  (52-_shiftamt=36) uses limited precisicion to floor */
#define _shiftamt 16
/* 16.16 fixed point representation */

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define iexp_				0
#define iman_				1
#else
#define iexp_				1
#define iman_				0
#endif /* BigEndian_ */

/* TODO: this optimization is very efficient: put it again when all works
#ifdef HAVE_MMX
#define F2I(dbl,i) {double d = dbl + _double2fixmagic; i = ((int*)&d)[iman_] >> _shiftamt;}
#else*/
#define F2I(dbl,i) i=(int)dbl;
/*#endif*/

#if 0
#define SINCOS(f,s,c) \
  __asm__ __volatile__ ("fsincos" : "=t" (c), "=u" (s) : "0" (f))
#else
#define SINCOS(f,s,c) {s=sin(f);c=cos(f);}
#endif

extern float sin256[256];
extern float cos256[256];

#endif

