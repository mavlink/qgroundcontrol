/*
 * Copyright (C) 2014 Collabora Ltd.
 *     Author: Nicolas Dufresne <nicolas@ndufresne.ca>
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
 *
 */

#include <glib.h>

#ifndef __TYPES_COMPAT_H__
#define __TYPES_COMPAT_H__

#ifdef   __linux__
#include <linux/types.h>
#include <asm/ioctl.h>

#else /* One of the BSDs */
/* From linux/types.h */
#ifndef __bitwise__
#  ifdef __CHECKER__
#    define __bitwise__ __attribute__((bitwise))
#  else
#    define __bitwise__
#  endif
#endif

#ifndef __bitwise
#  ifdef __CHECK_ENDIAN__
#    define __bitwise __bitwise__
#  else
#    define __bitwise
#  endif
#endif

typedef guint8  __u8;
typedef guint16 __u16;
typedef gint32  __s32;
typedef guint32 __u32;
typedef gint64  __s64;
typedef guint64 __u64;
typedef guint32 __bitwise __le32;

#endif

#endif /* __TYPES_COMPAT_H__ */
