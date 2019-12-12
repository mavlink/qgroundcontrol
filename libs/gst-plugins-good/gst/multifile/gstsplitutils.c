/* GStreamer Split Source Utility Functions
 * Copyright (C) 2011 Collabora Ltd. <tim.muller@collabora.co.uk>
 * Copyright (C) 2014 Jan Schmidt <jan@centricular.com>
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

#include <string.h>

#include "gstsplitutils.h"
#include "patternspec.h"

static int
gst_split_util_array_sortfunc (gchar ** a, gchar ** b)
{
  return strcmp (*a, *b);
}

gchar **
gst_split_util_find_files (const gchar * dirname,
    const gchar * basename, GError ** err)
{
  PatternSpec *pspec;
  GPtrArray *files;
  const gchar *name;
  GDir *dir;

  if (dirname == NULL || basename == NULL)
    goto invalid_location;

  GST_INFO ("checking in directory '%s' for pattern '%s'", dirname, basename);

  dir = g_dir_open (dirname, 0, err);
  if (dir == NULL)
    return NULL;

  if (DEFAULT_PATTERN_MATCH_MODE == MATCH_MODE_UTF8 &&
      !g_utf8_validate (basename, -1, NULL)) {
    goto not_utf8;
  }

  /* mode will be AUTO on linux/unix and UTF8 on win32 */
  pspec = pattern_spec_new (basename, DEFAULT_PATTERN_MATCH_MODE);

  files = g_ptr_array_new ();

  while ((name = g_dir_read_name (dir))) {
    GST_TRACE ("check: %s", name);
    if (pattern_match_string (pspec, name)) {
      GST_DEBUG ("match: %s", name);
      g_ptr_array_add (files, g_build_filename (dirname, name, NULL));
    }
  }

  if (files->len == 0)
    goto no_matches;

  g_ptr_array_sort (files, (GCompareFunc) gst_split_util_array_sortfunc);
  g_ptr_array_add (files, NULL);

  pattern_spec_free (pspec);
  g_dir_close (dir);

  return (gchar **) g_ptr_array_free (files, FALSE);

/* ERRORS */
invalid_location:
  {
    g_set_error_literal (err, G_FILE_ERROR, G_FILE_ERROR_INVAL,
        "No filename specified.");
    return NULL;
  }
not_utf8:
  {
    g_dir_close (dir);
    g_set_error_literal (err, G_FILE_ERROR, G_FILE_ERROR_INVAL,
        "Filename pattern must be UTF-8 on Windows.");
    return NULL;
  }
no_matches:
  {
    pattern_spec_free (pspec);
    g_dir_close (dir);
    g_set_error_literal (err, G_FILE_ERROR, G_FILE_ERROR_NOENT,
        "Found no files matching the pattern.");
    return NULL;
  }
}
