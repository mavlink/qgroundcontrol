/* GStreamer
 *
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2colorbalance.h: color balance interface implementation for V4L2
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

#ifndef __GST_V4L2_COLOR_BALANCE_H__
#define __GST_V4L2_COLOR_BALANCE_H__

#include <gst/gst.h>
#include <gst/video/colorbalance.h>

#include "gstv4l2object.h"

G_BEGIN_DECLS

#define GST_TYPE_V4L2_COLOR_BALANCE_CHANNEL \
  (gst_v4l2_color_balance_channel_get_type ())
#define GST_V4L2_COLOR_BALANCE_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_V4L2_COLOR_BALANCE_CHANNEL, \
                               GstV4l2ColorBalanceChannel))
#define GST_V4L2_COLOR_BALANCE_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_V4L2_COLOR_BALANCE_CHANNEL, \
                            GstV4l2ColorBalanceChannelClass))
#define GST_IS_V4L2_COLOR_BALANCE_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_V4L2_COLOR_BALANCE_CHANNEL))
#define GST_IS_V4L2_COLOR_BALANCE_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_V4L2_COLOR_BALANCE_CHANNEL))

typedef struct _GstV4l2ColorBalanceChannel {
  GstColorBalanceChannel parent;

  guint32 id;
} GstV4l2ColorBalanceChannel;

typedef struct _GstV4l2ColorBalanceChannelClass {
  GstColorBalanceChannelClass parent;
} GstV4l2ColorBalanceChannelClass;

GType gst_v4l2_color_balance_channel_get_type   (void);

const GList *   gst_v4l2_color_balance_list_channels    (GstV4l2Object * v4l2object);

void            gst_v4l2_color_balance_set_value        (GstV4l2Object * v4l2object,
                                                         GstColorBalanceChannel * channel,
                                                         gint value);

gint            gst_v4l2_color_balance_get_value        (GstV4l2Object * v4l2object,
                                                         GstColorBalanceChannel * channel);

#define GST_IMPLEMENT_V4L2_COLOR_BALANCE_METHODS(Type, interface_as_function)         \
                                                                                      \
static const GList *                                                                  \
interface_as_function ## _color_balance_list_channels (GstColorBalance * balance)     \
{                                                                                     \
  Type *this = (Type*) balance;                                                       \
  return gst_v4l2_color_balance_list_channels(this->v4l2object);                      \
}                                                                                     \
                                                                                      \
static void                                                                           \
interface_as_function ## _color_balance_set_value (GstColorBalance * balance,         \
                                                   GstColorBalanceChannel * channel,  \
                                                   gint value)                        \
{                                                                                     \
  Type *this = (Type*) balance;                                                       \
  gst_v4l2_color_balance_set_value(this->v4l2object, channel, value);          \
}                                                                                     \
                                                                                      \
static gint                                                                           \
interface_as_function ## _color_balance_get_value (GstColorBalance * balance,         \
                                                   GstColorBalanceChannel * channel)  \
{                                                                                     \
  Type *this = (Type*) balance;                                                       \
  return gst_v4l2_color_balance_get_value(this->v4l2object, channel);                 \
}                                                                                     \
                                                                                      \
static GstColorBalanceType                                                            \
interface_as_function ## _color_balance_get_balance_type (GstColorBalance * balance)  \
{                                                                                     \
  return GST_COLOR_BALANCE_HARDWARE;                                                  \
}                                                                                     \
                                                                                      \
static void                                                                           \
interface_as_function ## _color_balance_interface_init (GstColorBalanceInterface * iface) \
{                                                                                     \
  /* default virtual functions */                                                     \
  iface->list_channels = interface_as_function ## _color_balance_list_channels;       \
  iface->set_value = interface_as_function ## _color_balance_set_value;               \
  iface->get_value = interface_as_function ## _color_balance_get_value;               \
  iface->get_balance_type = interface_as_function ## _color_balance_get_balance_type; \
}                                                                                     \

G_END_DECLS
#endif /* __GST_V4L2_COLOR_BALANCE_H__ */
