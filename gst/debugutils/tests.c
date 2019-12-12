/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <otte@gnome.org>
 *
 * includes code based on glibc 2.2.3's crypt/md5.c,
 * Copyright (C) 1995, 1996, 1997, 1999, 2000 Free Software Foundation, Inc. 
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
#include "config.h"
#endif

#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 *** LENGTH ***
 */

typedef struct
{
  gint64 value;
}
LengthTest;

static GParamSpec *
length_get_spec (const GstTestInfo * info, gboolean compare_value)
{
  if (compare_value) {
    return g_param_spec_int64 ("expected-length", "expected length",
        "expected length of stream", -1, G_MAXINT64, -1,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  } else {
    return g_param_spec_int64 ("length", "length", "length of stream",
        -1, G_MAXINT64, -1, G_PARAM_READABLE);
  }
}

static gpointer
length_new (const GstTestInfo * info)
{
  return g_new0 (LengthTest, 1);
}

static void
length_add (gpointer test, GstBuffer * buffer)
{
  LengthTest *t = test;

  t->value += gst_buffer_get_size (buffer);
}

static gboolean
length_finish (gpointer test, GValue * value)
{
  LengthTest *t = test;

  if (g_value_get_int64 (value) == -1)
    return TRUE;

  return t->value == g_value_get_int64 (value);
}

static void
length_get_value (gpointer test, GValue * value)
{
  LengthTest *t = test;

  g_value_set_int64 (value, t ? t->value : -1);
}

/*
 *** BUFFER COUNT ***
 */

static GParamSpec *
buffer_count_get_spec (const GstTestInfo * info, gboolean compare_value)
{
  if (compare_value) {
    return g_param_spec_int64 ("expected-buffer-count", "expected buffer count",
        "expected number of buffers in stream",
        -1, G_MAXINT64, -1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  } else {
    return g_param_spec_int64 ("buffer-count", "buffer count",
        "number of buffers in stream", -1, G_MAXINT64, -1, G_PARAM_READABLE);
  }
}

static void
buffer_count_add (gpointer test, GstBuffer * buffer)
{
  LengthTest *t = test;

  t->value++;
}

/*
 *** TIMESTAMP / DURATION MATCHING ***
 */

typedef struct
{
  guint64 diff;
  guint count;
  GstClockTime expected;
}
TimeDurTest;

static GParamSpec *
timedur_get_spec (const GstTestInfo * info, gboolean compare_value)
{
  if (compare_value) {
    return g_param_spec_int64 ("allowed-timestamp-deviation",
        "allowed timestamp deviation",
        "allowed average difference in usec between timestamp of next buffer "
        "and expected timestamp from analyzing last buffer",
        -1, G_MAXINT64, -1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  } else {
    return g_param_spec_int64 ("timestamp-deviation",
        "timestamp deviation",
        "average difference in usec between timestamp of next buffer "
        "and expected timestamp from analyzing last buffer",
        -1, G_MAXINT64, -1, G_PARAM_READABLE);
  }
}

static gpointer
timedur_new (const GstTestInfo * info)
{
  TimeDurTest *ret = g_new0 (TimeDurTest, 1);

  ret->expected = GST_CLOCK_TIME_NONE;

  return ret;
}

static void
timedur_add (gpointer test, GstBuffer * buffer)
{
  TimeDurTest *t = test;

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer) &&
      GST_CLOCK_TIME_IS_VALID (t->expected)) {
    t->diff +=
        ABS (GST_CLOCK_DIFF (t->expected, GST_BUFFER_TIMESTAMP (buffer)));
    t->count++;
  }
  if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer) &&
      GST_BUFFER_DURATION_IS_VALID (buffer)) {
    t->expected = GST_BUFFER_TIMESTAMP (buffer) + GST_BUFFER_DURATION (buffer);
  } else {
    t->expected = GST_CLOCK_TIME_NONE;
  }
}

static gboolean
timedur_finish (gpointer test, GValue * value)
{
  TimeDurTest *t = test;

  if (g_value_get_int64 (value) == -1)
    return TRUE;

  return (t->diff / MAX (1, t->count)) <= g_value_get_int64 (value);
}

static void
timedur_get_value (gpointer test, GValue * value)
{
  TimeDurTest *t = test;

  g_value_set_int64 (value, t ? (t->diff / MAX (1, t->count)) : -1);
}

/*
 *** MD5 ***
 */

static GParamSpec *
md5_get_spec (const GstTestInfo * info, gboolean compare_value)
{
  if (compare_value) {
    return g_param_spec_string ("expected-md5", "expected md5",
        "expected md5 of processing the whole data",
        "---", G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  } else {
    return g_param_spec_string ("md5", "md5",
        "md5 of processing the whole data", "---", G_PARAM_READABLE);
  }
}

static gpointer
md5_new (const GstTestInfo * info)
{
  return g_checksum_new (G_CHECKSUM_MD5);
}

static void
md5_add (gpointer checksum, GstBuffer * buffer)
{
  GstMapInfo map;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  g_checksum_update (checksum, map.data, map.size);
  gst_buffer_unmap (buffer, &map);
}

static gboolean
md5_finish (gpointer checksum, GValue * value)
{
  const gchar *expected, *result;

  expected = g_value_get_string (value);
  result = g_checksum_get_string (checksum);

  if (g_str_equal (expected, "---"))
    return TRUE;
  if (g_str_equal (expected, result))
    return TRUE;
  return FALSE;
}

static void
md5_get_value (gpointer checksum, GValue * value)
{
  if (!checksum) {
    g_value_set_string (value, "---");
  } else {
    g_value_set_string (value, g_checksum_get_string (checksum));
  }
}

static void
md5_free (gpointer checksum)
{
  g_checksum_free (checksum);
}

/*
 *** TESTINFO ***
 */

const GstTestInfo tests[] = {
  {length_get_spec, length_new, length_add,
      length_finish, length_get_value, g_free},
  {buffer_count_get_spec, length_new, buffer_count_add,
      length_finish, length_get_value, g_free},
  {timedur_get_spec, timedur_new, timedur_add,
      timedur_finish, timedur_get_value, g_free},
  {md5_get_spec, md5_new, md5_add,
      md5_finish, md5_get_value, md5_free}
};
