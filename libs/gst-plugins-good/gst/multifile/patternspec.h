/* GPattern copy that supports raw (non-utf8) matching
 * based on: GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997, 1999  Peter Mattis, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __PATTERN_SPEC_H__
#define __PATTERN_SPEC_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  MATCH_MODE_AUTO = 0,
  MATCH_MODE_UTF8,
  MATCH_MODE_RAW
} MatchMode;

typedef struct _PatternSpec PatternSpec;

PatternSpec * pattern_spec_new       (const gchar  * pattern,
                                      MatchMode      match_mode);

void          pattern_spec_free      (PatternSpec  * pspec);

gboolean      pattern_match_string   (PatternSpec  * pspec,
                                      const gchar  * string);

G_END_DECLS

#endif /* __PATTERN_SPEC_H__ */
