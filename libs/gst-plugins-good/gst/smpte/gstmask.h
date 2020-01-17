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


#ifndef __GST_MASK_H__
#define __GST_MASK_H__

#include <gst/gst.h>

typedef struct _GstMask GstMask;
typedef struct _GstMaskDefinition GstMaskDefinition;

typedef void            (*GstMaskDrawFunc)              (GstMask *mask);
typedef void            (*GstMaskDestroyFunc)           (GstMask *mask);

struct _GstMaskDefinition {
  gint                   type;
  const gchar           *short_name;
  const gchar           *long_name;
  GstMaskDrawFunc        draw_func;
  GstMaskDestroyFunc     destroy_func;
  gconstpointer          user_data;
};

struct _GstMask {
  gint                   type;
  guint32               *data;
  gconstpointer          user_data;

  gint                   width;
  gint                   height;
  gint                   bpp;

  GstMaskDestroyFunc     destroy_func;
};

void                    _gst_mask_init                  (void);
void                    _gst_mask_register              (const GstMaskDefinition *definition);

void                    _gst_mask_default_destroy       (GstMask *mask);

const GList*            gst_mask_get_definitions        (void);
GstMask*                gst_mask_factory_new            (gint type, gboolean invert, gint bpp, gint width, gint height);
void                    gst_mask_destroy                (GstMask *mask);

void _gst_barboxwipes_register (void);

#endif /* __GST_MASK_H__ */
