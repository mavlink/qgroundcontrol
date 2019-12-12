/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <otte@gnome.org>
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

#include <gst/gst.h>

#ifndef __GST_TESTS_H__
#define __GST_TESTS_H__


typedef struct _GstTestInfo GstTestInfo;

struct _GstTestInfo
{
  GParamSpec *(*get_spec) (const GstTestInfo * info, gboolean compare_value);
    gpointer (*new) (const GstTestInfo * info);
  void (*add) (gpointer test, GstBuffer * buffer);
    gboolean (*finish) (gpointer test, GValue * value);
  void (*get_value) (gpointer test, GValue * value);
  void (*free) (gpointer test);
};

extern const GstTestInfo tests[];
/* keep up to date! */
#define TESTS_COUNT (4)


#endif /* __GST_TESTS_H__ */
