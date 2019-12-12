/* GStreamer
 * Copyright (C) 2007 Thomas Vander Stichele <thomas at apestaart dot org>
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
#  include "config.h"
#endif

#include <glib.h>

#include "fnv1hash.h"

/* This file implements FNV-1 hashing used in the Ogg payload encoders
 * to generate the 24-bit ident value based on the header pages.
 * See http://isthe.com/chongo/tech/comp/fnv/
 */

#define MASK_24 (((guint32) 1 << 24) -1)

#define FNV1_HASH_32_INIT ((guint32) 0x811C9DC5L)
//2166136261L)
#define FNV1_HASH_32_PRIME 16777619

guint32
fnv1_hash_32_new (void)
{
  return FNV1_HASH_32_INIT;
}

guint32
fnv1_hash_32_update (guint32 hash, const guchar * data, guint length)
{
  guint i;
  const guchar *p = data;

  for (i = 0; i < length; ++i, ++p) {
    hash *= FNV1_HASH_32_PRIME;
    hash ^= *p;
  }

  return hash;
}

guint32
fnv1_hash_32_to_24 (guint32 hash)
{
  return (hash >> 24) ^ (hash & MASK_24);
}
