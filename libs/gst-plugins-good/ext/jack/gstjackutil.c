/* GStreamer Jack utility functions
 * Copyright (C) 2010 Tristan Matthews <tristan@sat.qc.ca>
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

#include "gstjackutil.h"
#include <gst/audio/audio.h>

static const GstAudioChannelPosition default_positions[8][8] = {
  /* 1 channel */
  {
        GST_AUDIO_CHANNEL_POSITION_MONO,
      },
  /* 2 channels */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
      },
  /* 3 channels (2.1) */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_LFE1,        /* or FRONT_CENTER for 3.0? */
      },
  /* 4 channels (4.0 or 3.1?) */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
      },
  /* 5 channels */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
      },
  /* 6 channels */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
      },
  /* 7 channels */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
        GST_AUDIO_CHANNEL_POSITION_REAR_CENTER,
      },
  /* 8 channels */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
        GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
        GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
      }
};


/* if channels are less than or equal to 8, we set a default layout,
 * otherwise set layout to an array of GST_AUDIO_CHANNEL_POSITION_NONE */
void
gst_jack_set_layout (GstAudioRingBuffer * buffer, GstAudioRingBufferSpec * spec)
{
  gint i;

  if (spec->info.channels <= 8) {
    for (i = 0; i < spec->info.channels; i++)
      spec->info.position[i] = default_positions[spec->info.channels - 1][i];
    gst_audio_channel_positions_to_valid_order (spec->info.position,
        spec->info.channels);
    gst_audio_ring_buffer_set_channel_positions (buffer,
        default_positions[spec->info.channels - 1]);
  } else {
    spec->info.flags |= GST_AUDIO_FLAG_UNPOSITIONED;
    for (i = 0; i < G_N_ELEMENTS (spec->info.position); i++)
      spec->info.position[i] = GST_AUDIO_CHANNEL_POSITION_NONE;
    gst_audio_ring_buffer_set_channel_positions (buffer, spec->info.position);
  }

  gst_caps_unref (spec->caps);
  spec->caps = gst_audio_info_to_caps (&spec->info);
}
