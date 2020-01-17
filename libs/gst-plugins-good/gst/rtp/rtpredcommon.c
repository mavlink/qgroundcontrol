/* GStreamer plugin for forward error correction
 * Copyright (C) 2017 Pexip
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Mikhail Fludkov <misha@pexip.com>
 */

#include "rtpredcommon.h"

gsize
rtp_red_block_header_get_length (gboolean is_redundant)
{
  return is_redundant ? sizeof (RedBlockHeader) : 1;
}

gboolean
rtp_red_block_is_redundant (gpointer red_block)
{
  return ((RedBlockHeader *) red_block)->F;
}

guint8
rtp_red_block_get_payload_type (gpointer red_block)
{
  return ((RedBlockHeader *) red_block)->pt;
}

guint16
rtp_red_block_get_payload_length (gpointer red_block)
{
  RedBlockHeader *hdr = (RedBlockHeader *) red_block;
  return (hdr->length_hi << 8) | hdr->length_lo;
}

guint16
rtp_red_block_get_timestamp_offset (gpointer red_block)
{
  RedBlockHeader *hdr = (RedBlockHeader *) red_block;
  return (hdr->timestamp_offset_hi << 6) | hdr->timestamp_offset_lo;
}

void
rtp_red_block_set_payload_type (gpointer red_block, guint8 pt)
{
  ((RedBlockHeader *) red_block)->pt = pt;
}

void
rtp_red_block_set_is_redundant (gpointer red_block, gboolean is_redundant)
{
  ((RedBlockHeader *) red_block)->F = is_redundant;
}

void
rtp_red_block_set_timestamp_offset (gpointer red_block,
    guint16 timestamp_offset)
{
  RedBlockHeader *hdr = (RedBlockHeader *) red_block;

  g_assert (rtp_red_block_is_redundant (red_block));
  g_assert_cmpint (timestamp_offset, <=, RED_BLOCK_TIMESTAMP_OFFSET_MAX);

  hdr->timestamp_offset_lo = timestamp_offset & 0x3f;
  hdr->timestamp_offset_hi = timestamp_offset >> 6;
}

void
rtp_red_block_set_payload_length (gpointer red_block, guint16 length)
{
  RedBlockHeader *hdr = (RedBlockHeader *) red_block;

  g_assert (rtp_red_block_is_redundant (red_block));
  g_assert_cmpint (length, <=, RED_BLOCK_LENGTH_MAX);

  hdr->length_lo = length & 0xff;
  hdr->length_hi = length >> 8;
}
