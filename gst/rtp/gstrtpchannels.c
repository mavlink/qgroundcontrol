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

#include "gstrtpchannels.h"

/* 
 * RTP channel positions as discussed in RFC 3551 and also RFC 3555
 *
 * We can't really represent the described channel positions in GStreamer but we
 * implement a (very rough) approximation here.
 */

static const GstAudioChannelPosition pos_4_1[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT
};

static const GstAudioChannelPosition pos_4_2[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE1
};

static const GstAudioChannelPosition pos_4_3[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE1
};

static const GstAudioChannelPosition pos_5_1[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER
};

static const GstAudioChannelPosition pos_6_1[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE1
};

static const GstAudioChannelPosition pos_6_2[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE1,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT
};

static const GstAudioChannelPosition pos_8_1[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE1,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
  GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT
};

static const GstAudioChannelPosition pos_8_2[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE1,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
  GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT
};

static const GstAudioChannelPosition pos_8_3[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE1,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
  GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT
};

static const GstAudioChannelPosition pos_def_1[] = {
  GST_AUDIO_CHANNEL_POSITION_MONO
};

static const GstAudioChannelPosition pos_def_2[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT
};

static const GstAudioChannelPosition pos_def_3[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER
};

static const GstAudioChannelPosition pos_def_4[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_LFE1
};

static const GstAudioChannelPosition pos_def_5[] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT
};

static const GstAudioChannelPosition pos_def_6[] = {
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_LFE1
};

const GstRTPChannelOrder gst_rtp_channel_orders[] = {
  /* 4 channels */
  {"DV.LRLsRs", 4, pos_4_1},
  {"DV.LRCS", 4, pos_4_2},
  {"DV.LRCWo", 4, pos_4_3},
  /* 5 channels */
  {"DV.LRLsRsC", 5, pos_5_1},
  /* 6 channels */
  {"DV.LRLsRsCS", 6, pos_6_1},
  {"DV.LmixRmixTWoQ1Q2", 6, pos_6_2},
  /* 8 channels */
  {"DV.LRCWoLsRsLmixRmix", 8, pos_8_1},
  {"DV.LRCWoLs1Rs1Ls2Rs2", 8, pos_8_2},
  {"DV.LRCWoLsRsLcRc", 8, pos_8_3},

  /* default layouts */
  {NULL, 1, pos_def_1},
  {NULL, 2, pos_def_2},
  {NULL, 3, pos_def_3},
  {NULL, 4, pos_def_4},
  {NULL, 5, pos_def_5},
  {NULL, 6, pos_def_6},

  /* terminator, invalid entry */
  {NULL, 0, NULL},
};

static gboolean
check_channels (const GstRTPChannelOrder * order,
    const GstAudioChannelPosition * pos)
{
  gint i, j;
  gboolean res = TRUE;

  for (i = 0; i < order->channels; i++) {
    for (j = 0; j < order->channels; j++) {
      if (order->pos[j] == pos[i])
        break;
    }
    if (j == order->channels)
      return FALSE;
  }
  return res;
}

/**
 * gst_rtp_channels_get_by_pos:
 * @channels: the amount of channels
 * @pos: a channel layout
 *
 * Return a description of the channel layout.
 *
 * Returns: a #GstRTPChannelOrder with the channel information or NULL when @pos
 * is not a valid layout.
 */
const GstRTPChannelOrder *
gst_rtp_channels_get_by_pos (gint channels, const GstAudioChannelPosition * pos)
{
  gint i;
  const GstRTPChannelOrder *res = NULL;

  g_return_val_if_fail (pos != NULL, NULL);

  for (i = 0; channel_orders[i].pos; i++) {
    if (channel_orders[i].channels != channels)
      continue;

    if (check_channels (&channel_orders[i], pos)) {
      res = &channel_orders[i];
      break;
    }
  }
  return res;
}

/**
 * gst_rtp_channels_create_default:
 * @channels: the amount of channels
 * @order: a channel order
 *
 * Get the channel order info the @order and @channels.
 *
 * Returns: a #GstRTPChannelOrder with the channel information or NULL when
 * @order is not a know layout for @channels.
 */
const GstRTPChannelOrder *
gst_rtp_channels_get_by_order (gint channels, const gchar * order)
{
  gint i;
  const GstRTPChannelOrder *res = NULL;

  for (i = 0; channel_orders[i].pos; i++) {
    if (channel_orders[i].channels != channels)
      continue;

    /* no name but channels match, continue */
    if (!channel_orders[i].name || !order) {
      res = &channel_orders[i];
      break;
    }

    /* compare names */
    if (g_ascii_strcasecmp (channel_orders[i].name, order)) {
      res = &channel_orders[i];
      break;
    }
  }
  return res;
}

/**
 * gst_rtp_channels_get_by_index:
 * @channels: the amount of channels
 * @idx: the channel index to retrieve
 *
 * Get the allowed channel order descriptions for @channels. @idx can be used to
 * retrieve the desired index.
 *
 * Returns: a #GstRTPChannelOrder at @idx, NULL when there are no valid channel
 * orders.
 */
const GstRTPChannelOrder *
gst_rtp_channels_get_by_index (gint channels, guint idx)
{
  gint i;
  const GstRTPChannelOrder *res = NULL;

  for (i = 0; channel_orders[i].pos; i++) {
    if (channel_orders[i].channels != channels)
      continue;

    if (idx == 0) {
      res = &channel_orders[i];
      break;
    }
    idx--;
  }
  return res;
}


/**
 * gst_rtp_channels_create_default:
 * @channels: the amount of channels
 *
 * Create a default none channel mapping for @channels.
 *
 * Returns: a #GstAudioChannelPosition with all the channel position info set to
 * #GST_AUDIO_CHANNEL_POSITION_NONE.
 */
void
gst_rtp_channels_create_default (gint channels, GstAudioChannelPosition * posn)
{
  gint i;

  g_return_if_fail (channels > 0);

  for (i = 0; i < channels; i++)
    posn[i] = GST_AUDIO_CHANNEL_POSITION_NONE;
}
