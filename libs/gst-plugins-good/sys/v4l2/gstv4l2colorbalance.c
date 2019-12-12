/* GStreamer
 *
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2colorbalance.c: color balance interface implementation for V4L2
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

#include <gst/gst.h>
#include "gstv4l2colorbalance.h"
#include "gstv4l2object.h"

#define gst_v4l2_color_balance_channel_parent_class parent_class
G_DEFINE_TYPE (GstV4l2ColorBalanceChannel,
    gst_v4l2_color_balance_channel, GST_TYPE_COLOR_BALANCE_CHANNEL);

static void
gst_v4l2_color_balance_channel_class_init (GstV4l2ColorBalanceChannelClass *
    klass)
{
}

static void
gst_v4l2_color_balance_channel_init (GstV4l2ColorBalanceChannel * channel)
{
  channel->id = (guint32) - 1;
}

static G_GNUC_UNUSED gboolean
gst_v4l2_color_balance_contains_channel (GstV4l2Object * v4l2object,
    GstV4l2ColorBalanceChannel * v4l2channel)
{
  const GList *item;

  for (item = v4l2object->colors; item != NULL; item = item->next)
    if (item->data == v4l2channel)
      return TRUE;

  return FALSE;
}

const GList *
gst_v4l2_color_balance_list_channels (GstV4l2Object * v4l2object)
{
  return v4l2object->colors;
}

void
gst_v4l2_color_balance_set_value (GstV4l2Object * v4l2object,
    GstColorBalanceChannel * channel, gint value)
{

  GstV4l2ColorBalanceChannel *v4l2channel =
      GST_V4L2_COLOR_BALANCE_CHANNEL (channel);

  /* assert that we're opened and that we're using a known item */
  g_return_if_fail (GST_V4L2_IS_OPEN (v4l2object));
  g_return_if_fail (gst_v4l2_color_balance_contains_channel (v4l2object,
          v4l2channel));

  gst_v4l2_set_attribute (v4l2object, v4l2channel->id, value);
}

gint
gst_v4l2_color_balance_get_value (GstV4l2Object * v4l2object,
    GstColorBalanceChannel * channel)
{
  GstV4l2ColorBalanceChannel *v4l2channel =
      GST_V4L2_COLOR_BALANCE_CHANNEL (channel);
  gint value;

  /* assert that we're opened and that we're using a known item */
  g_return_val_if_fail (GST_V4L2_IS_OPEN (v4l2object), 0);
  g_return_val_if_fail (gst_v4l2_color_balance_contains_channel (v4l2object,
          v4l2channel), 0);

  if (!gst_v4l2_get_attribute (v4l2object, v4l2channel->id, &value))
    return 0;

  return value;
}
