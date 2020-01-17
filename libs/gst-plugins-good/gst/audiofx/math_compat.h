/* 
 * GStreamer
 * Copyright (C) 2008 Sebastian Dröge <slomo@circular-chaos.org>
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

#ifndef __MATH_COMPAT_H__
#define __MATH_COMPAT_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <math.h>

#ifndef HAVE_ASINH
static inline gdouble
asinh (gdouble x)
{
  return log(x + sqrt (x * x + 1));
}
#endif

#ifndef HAVE_SINH
static inline gdouble
sinh (gdouble x)
{
  return 0.5 * (exp (x) - exp (-x));
}
#endif

#ifndef HAVE_COSH
static inline gdouble
cosh (gdouble x)
{
  return 0.5 * (exp (x) + exp (-x));
}
#endif

#endif /* __MATH_COMPAT_H__ */
