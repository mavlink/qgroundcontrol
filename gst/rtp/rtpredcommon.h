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

#ifndef __RTP_RED_COMMON_H__
#define __RTP_RED_COMMON_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _RedBlockHeader RedBlockHeader;

/* RFC 2198 */
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |F|   block PT  |  timestamp offset         |   block length    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct _RedBlockHeader {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  guint pt:7;
  guint F:1;

  guint timestamp_offset_hi: 8;

  guint length_hi: 2;
  guint timestamp_offset_lo: 6;

  guint length_lo: 8;
#elif G_BYTE_ORDER == G_BIG_ENDIAN
  guint F:1;
  guint pt:7;

  guint timestamp_offset_hi: 8;

  guint timestamp_offset_lo: 6;
  guint length_hi: 2;

  guint length_lo: 8;
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
};

#define RED_BLOCK_TIMESTAMP_OFFSET_MAX ((1<<14) - 1)
#define RED_BLOCK_LENGTH_MAX ((1<<10) - 1)

gsize    rtp_red_block_header_get_length    (gboolean is_redundant);
gboolean rtp_red_block_is_redundant         (gpointer red_block);
void     rtp_red_block_set_payload_type     (gpointer red_block, guint8 pt);
void     rtp_red_block_set_timestamp_offset (gpointer red_block, guint16 timestamp_offset);
void     rtp_red_block_set_payload_length   (gpointer red_block, guint16 length);
guint16  rtp_red_block_get_timestamp_offset (gpointer red_block);
guint8   rtp_red_block_get_payload_type     (gpointer red_block);
void     rtp_red_block_set_is_redundant     (gpointer red_block, gboolean is_redundant);
guint16  rtp_red_block_get_payload_length   (gpointer red_block);

G_END_DECLS

#endif
