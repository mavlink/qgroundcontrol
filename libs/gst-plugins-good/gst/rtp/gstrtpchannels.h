/* GStreamer
 * Copyright (C) <2008> Wim Taymans <wim.taymans@gmail.com>
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

#include <string.h>
#include <stdlib.h>

#include <gst/audio/audio.h>

#ifndef __GST_RTP_CHANNELS_H__
#define __GST_RTP_CHANNELS_H__

typedef struct
{
  const gchar                   *name;
  gint                           channels;
  const GstAudioChannelPosition *pos;
} GstRTPChannelOrder;

#define channel_orders gst_rtp_channel_orders
G_GNUC_INTERNAL extern const GstRTPChannelOrder gst_rtp_channel_orders[];

const GstRTPChannelOrder *   gst_rtp_channels_get_by_pos     (gint channels,
                                                              const GstAudioChannelPosition *pos);
const GstRTPChannelOrder *   gst_rtp_channels_get_by_order   (gint channels,
                                                              const gchar *order);
const GstRTPChannelOrder *   gst_rtp_channels_get_by_index   (gint channels, guint idx);

void                         gst_rtp_channels_create_default (gint channels, GstAudioChannelPosition *pos);

#endif /* __GST_RTP_CHANNELS_H__ */
