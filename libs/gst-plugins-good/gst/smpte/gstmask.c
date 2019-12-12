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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstmask.h"
#include "paint.h"

static GList *masks = NULL;

void
_gst_mask_init (void)
{
  _gst_barboxwipes_register ();
}

static gint
gst_mask_compare (GstMaskDefinition * def1, GstMaskDefinition * def2)
{
  return (def1->type - def2->type);
}

void
_gst_mask_register (const GstMaskDefinition * definition)
{
  masks =
      g_list_insert_sorted (masks, (gpointer) definition,
      (GCompareFunc) gst_mask_compare);
}

const GList *
gst_mask_get_definitions (void)
{
  return masks;
}

static GstMaskDefinition *
gst_mask_find_definition (gint type)
{
  GList *walk = masks;

  while (walk) {
    GstMaskDefinition *def = (GstMaskDefinition *) walk->data;

    if (def->type == type)
      return def;

    walk = g_list_next (walk);
  }
  return NULL;
}

GstMask *
gst_mask_factory_new (gint type, gboolean invert, gint bpp, gint width,
    gint height)
{
  GstMaskDefinition *definition;
  GstMask *mask = NULL;

  definition = gst_mask_find_definition (type);
  if (definition) {
    mask = g_new0 (GstMask, 1);

    mask->type = definition->type;
    mask->bpp = bpp;
    mask->width = width;
    mask->height = height;
    mask->destroy_func = definition->destroy_func;
    mask->user_data = definition->user_data;
    mask->data = g_malloc (width * height * sizeof (guint32));

    definition->draw_func (mask);

    if (invert) {
      gint i, j;
      guint32 *datap = mask->data;
      guint32 max = (1 << bpp);

      for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
          *datap = max - *datap;
          datap++;
        }
      }
    }
  }

  return mask;
}

void
_gst_mask_default_destroy (GstMask * mask)
{
  g_free (mask->data);
  g_free (mask);
}

void
gst_mask_destroy (GstMask * mask)
{
  if (mask->destroy_func)
    mask->destroy_func (mask);
}
