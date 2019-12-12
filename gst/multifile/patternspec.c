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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "patternspec.h"
#include <string.h>

typedef enum
{
  MATCH_ALL,                    /* "*A?A*" */
  MATCH_ALL_TAIL,               /* "*A?AA" */
  MATCH_HEAD,                   /* "AAAA*" */
  MATCH_TAIL,                   /* "*AAAA" */
  MATCH_EXACT,                  /* "AAAAA" */
  MATCH_LAST
} MatchType;

struct _PatternSpec
{
  MatchMode match_mode;
  MatchType match_type;
  guint pattern_length;
  guint min_length;
  guint max_length;
  gchar *pattern;
};

static inline gchar *
raw_strreverse (const gchar * str, gssize size)
{
  g_assert (size > 0);
  return g_strreverse (g_strndup (str, size));
}

static inline gboolean
pattern_ph_match (const gchar * match_pattern, MatchMode match_mode,
    const gchar * match_string, gboolean * wildcard_reached_p)
{
  register const gchar *pattern, *string;
  register gchar ch;

  pattern = match_pattern;
  string = match_string;

  ch = *pattern;
  pattern++;
  while (ch) {
    switch (ch) {
      case '?':
        if (!*string)
          return FALSE;
        if (match_mode == MATCH_MODE_UTF8)
          string = g_utf8_next_char (string);
        else
          ++string;
        break;

      case '*':
        *wildcard_reached_p = TRUE;
        do {
          ch = *pattern;
          pattern++;
          if (ch == '?') {
            if (!*string)
              return FALSE;
            if (match_mode == MATCH_MODE_UTF8)
              string = g_utf8_next_char (string);
            else
              ++string;
          }
        }
        while (ch == '*' || ch == '?');
        if (!ch)
          return TRUE;
        do {
          gboolean next_wildcard_reached = FALSE;
          while (ch != *string) {
            if (!*string)
              return FALSE;
            if (match_mode == MATCH_MODE_UTF8)
              string = g_utf8_next_char (string);
            else
              ++string;
          }
          string++;
          if (pattern_ph_match (pattern, match_mode, string,
                  &next_wildcard_reached))
            return TRUE;
          if (next_wildcard_reached)
            /* the forthcoming pattern substring up to the next wildcard has
             * been matched, but a mismatch occurred for the rest of the
             * pattern, following the next wildcard.
             * there's no need to advance the current match position any
             * further if the rest pattern will not match.
             */
            return FALSE;
        }
        while (*string);
        break;

      default:
        if (ch == *string)
          string++;
        else
          return FALSE;
        break;
    }

    ch = *pattern;
    pattern++;
  }

  return *string == 0;
}

static gboolean
pattern_match (PatternSpec * pspec, guint string_length,
    const gchar * string, const gchar * string_reversed)
{
  MatchMode match_mode;

  g_assert (pspec != NULL);
  g_assert (string != NULL);

  if (string_length < pspec->min_length || string_length > pspec->max_length)
    return FALSE;

  match_mode = pspec->match_mode;
  if (match_mode == MATCH_MODE_AUTO) {
    if (!g_utf8_validate (string, string_length, NULL))
      match_mode = MATCH_MODE_RAW;
    else
      match_mode = MATCH_MODE_UTF8;
  }

  switch (pspec->match_type) {
      gboolean dummy;
    case MATCH_ALL:
      return pattern_ph_match (pspec->pattern, match_mode, string, &dummy);
    case MATCH_ALL_TAIL:
      if (string_reversed)
        return pattern_ph_match (pspec->pattern, match_mode, string_reversed,
            &dummy);
      else {
        gboolean result;
        gchar *tmp;
        if (match_mode == MATCH_MODE_UTF8) {
          tmp = g_utf8_strreverse (string, string_length);
        } else {
          tmp = raw_strreverse (string, string_length);
        }
        result = pattern_ph_match (pspec->pattern, match_mode, tmp, &dummy);
        g_free (tmp);
        return result;
      }
    case MATCH_HEAD:
      if (pspec->pattern_length == string_length)
        return memcmp (pspec->pattern, string, string_length) == 0;
      else if (pspec->pattern_length)
        return memcmp (pspec->pattern, string, pspec->pattern_length) == 0;
      else
        return TRUE;
    case MATCH_TAIL:
      if (pspec->pattern_length)
        /* compare incl. NUL terminator */
        return memcmp (pspec->pattern,
            string + (string_length - pspec->pattern_length),
            pspec->pattern_length + 1) == 0;
      else
        return TRUE;
    case MATCH_EXACT:
      if (pspec->pattern_length != string_length)
        return FALSE;
      else
        return memcmp (pspec->pattern, string, string_length) == 0;
    default:
      g_return_val_if_fail (pspec->match_type < MATCH_LAST, FALSE);
      return FALSE;
  }
}

PatternSpec *
pattern_spec_new (const gchar * pattern, MatchMode match_mode)
{
  PatternSpec *pspec;
  gboolean seen_joker = FALSE, seen_wildcard = FALSE, more_wildcards = FALSE;
  gint hw_pos = -1, tw_pos = -1, hj_pos = -1, tj_pos = -1;
  gboolean follows_wildcard = FALSE;
  guint pending_jokers = 0;
  const gchar *s;
  gchar *d;
  guint i;

  g_assert (pattern != NULL);
  g_assert (match_mode != MATCH_MODE_UTF8
      || g_utf8_validate (pattern, -1, NULL));

  /* canonicalize pattern and collect necessary stats */
  pspec = g_new (PatternSpec, 1);
  pspec->match_mode = match_mode;
  pspec->pattern_length = strlen (pattern);
  pspec->min_length = 0;
  pspec->max_length = 0;
  pspec->pattern = g_new (gchar, pspec->pattern_length + 1);

  if (pspec->match_mode == MATCH_MODE_AUTO) {
    if (!g_utf8_validate (pattern, -1, NULL))
      pspec->match_mode = MATCH_MODE_RAW;
  }

  d = pspec->pattern;
  for (i = 0, s = pattern; *s != 0; s++) {
    switch (*s) {
      case '*':
        if (follows_wildcard) { /* compress multiple wildcards */
          pspec->pattern_length--;
          continue;
        }
        follows_wildcard = TRUE;
        if (hw_pos < 0)
          hw_pos = i;
        tw_pos = i;
        break;
      case '?':
        pending_jokers++;
        pspec->min_length++;
        if (pspec->match_mode == MATCH_MODE_RAW) {
          pspec->max_length += 1;
        } else {
          pspec->max_length += 4;       /* maximum UTF-8 character length */
        }
        continue;
      default:
        for (; pending_jokers; pending_jokers--, i++) {
          *d++ = '?';
          if (hj_pos < 0)
            hj_pos = i;
          tj_pos = i;
        }
        follows_wildcard = FALSE;
        pspec->min_length++;
        pspec->max_length++;
        break;
    }
    *d++ = *s;
    i++;
  }
  for (; pending_jokers; pending_jokers--) {
    *d++ = '?';
    if (hj_pos < 0)
      hj_pos = i;
    tj_pos = i;
  }
  *d++ = 0;
  seen_joker = hj_pos >= 0;
  seen_wildcard = hw_pos >= 0;
  more_wildcards = seen_wildcard && hw_pos != tw_pos;
  if (seen_wildcard)
    pspec->max_length = G_MAXUINT;

  /* special case sole head/tail wildcard or exact matches */
  if (!seen_joker && !more_wildcards) {
    if (pspec->pattern[0] == '*') {
      pspec->match_type = MATCH_TAIL;
      memmove (pspec->pattern, pspec->pattern + 1, --pspec->pattern_length);
      pspec->pattern[pspec->pattern_length] = 0;
      return pspec;
    }
    if (pspec->pattern_length > 0 &&
        pspec->pattern[pspec->pattern_length - 1] == '*') {
      pspec->match_type = MATCH_HEAD;
      pspec->pattern[--pspec->pattern_length] = 0;
      return pspec;
    }
    if (!seen_wildcard) {
      pspec->match_type = MATCH_EXACT;
      return pspec;
    }
  }

  /* now just need to distinguish between head or tail match start */
  tw_pos = pspec->pattern_length - 1 - tw_pos;  /* last pos to tail distance */
  tj_pos = pspec->pattern_length - 1 - tj_pos;  /* last pos to tail distance */
  if (seen_wildcard)
    pspec->match_type = tw_pos > hw_pos ? MATCH_ALL_TAIL : MATCH_ALL;
  else                          /* seen_joker */
    pspec->match_type = tj_pos > hj_pos ? MATCH_ALL_TAIL : MATCH_ALL;
  if (pspec->match_type == MATCH_ALL_TAIL) {
    gchar *tmp = pspec->pattern;

    if (pspec->match_mode == MATCH_MODE_RAW) {
      pspec->pattern = raw_strreverse (pspec->pattern, pspec->pattern_length);
    } else {
      pspec->pattern =
          g_utf8_strreverse (pspec->pattern, pspec->pattern_length);
    }
    g_free (tmp);
  }
  return pspec;
}

void
pattern_spec_free (PatternSpec * pspec)
{
  g_assert (pspec != NULL);

  g_free (pspec->pattern);
  g_free (pspec);
}

gboolean
pattern_match_string (PatternSpec * pspec, const gchar * string)
{
  return pattern_match (pspec, strlen (string), string, NULL);
}
