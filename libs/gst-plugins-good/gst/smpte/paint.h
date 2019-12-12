/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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

#ifndef __GST_SMPTE_PAINT_H__
#define __GST_SMPTE_PAINT_H__

#include <glib.h>

void    gst_smpte_paint_vbox            (guint32 *dest, gint stride, 
                                         gint x0, gint y0, gint c0, 
                                         gint x1, gint y1, gint c1);
void    gst_smpte_paint_hbox            (guint32 *dest, gint stride, 
                                         gint x0, gint y0, gint c0, 
                                         gint x1, gint y1, gint c1);

void    gst_smpte_paint_triangle_linear (guint32 *dest, gint stride,
                                         gint x0, gint y0, gint c0,
                                         gint x1, gint y1, gint c1, 
                                         gint x2, gint y2, gint c2);

void    gst_smpte_paint_triangle_clock  (guint32 *dest, gint stride,
                                         gint x0, gint y0, gint c0,
                                         gint x1, gint y1, gint c1, 
                                         gint x2, gint y2, gint c2);

void    gst_smpte_paint_box_clock       (guint32 *dest, gint stride,
                                         gint x0, gint y0, gint c0,
                                         gint x1, gint y1, gint c1,
                                         gint x2, gint y2, gint c2);

#endif /* __GST_SMPTE_PAINT_H__ */
