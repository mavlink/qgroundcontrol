/* GStreamer Wavpack plugin
 * Copyright (c) 2006 Sebastian Dröge <slomo@circular-chaos.org>
 *
 * gstwavpackstreamreader.c: stream reader used for decoding
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
#include <math.h>
#include <gst/gst.h>

#include "gstwavpackstreamreader.h"

GST_DEBUG_CATEGORY_EXTERN (wavpack_debug);
#define GST_CAT_DEFAULT wavpack_debug

static int32_t
gst_wavpack_stream_reader_read_bytes (void *id, void *data, int32_t bcount)
{
  read_id *rid = (read_id *) id;
  uint32_t left = rid->length - rid->position;
  uint32_t to_read = MIN (left, bcount);

  GST_DEBUG ("Trying to read %d of %d bytes from position %d", bcount,
      rid->length, rid->position);

  if (to_read > 0) {
    memmove (data, rid->buffer + rid->position, to_read);
    rid->position += to_read;
    return to_read;
  } else {
    GST_WARNING ("Couldn't read %d bytes", bcount);
    return 0;
  }
}

static uint32_t
gst_wavpack_stream_reader_get_pos (void *id)
{
  GST_DEBUG ("Returning position %d", ((read_id *) id)->position);
  return ((read_id *) id)->position;
}

static int
gst_wavpack_stream_reader_set_pos_abs (void *id, uint32_t pos)
{
  GST_WARNING ("Should not be called: tried to set absolute position to %d",
      pos);
  return -1;
}

static int
gst_wavpack_stream_reader_set_pos_rel (void *id, int32_t delta, int mode)
{
  GST_WARNING ("Should not be called: tried to set relative position to %d"
      " with mode %d", delta, mode);
  return -1;
}

static int
gst_wavpack_stream_reader_push_back_byte (void *id, int c)
{
  read_id *rid = (read_id *) id;

  GST_DEBUG ("Pushing back one byte: 0x%x", c);

  if (rid->position == 0)
    return rid->position;

  rid->position -= 1;
  return rid->position;
}

static uint32_t
gst_wavpack_stream_reader_get_length (void *id)
{
  GST_DEBUG ("Returning length %d", ((read_id *) id)->length);

  return ((read_id *) id)->length;
}

static int
gst_wavpack_stream_reader_can_seek (void *id)
{
  GST_DEBUG ("Can't seek");
  return FALSE;
}

static int32_t
gst_wavpack_stream_reader_write_bytes (void *id, void *data, int32_t bcount)
{
  GST_WARNING ("Should not be called, tried to write %d bytes", bcount);
  return 0;
}

WavpackStreamReader *
gst_wavpack_stream_reader_new (void)
{
  WavpackStreamReader *stream_reader =
      (WavpackStreamReader *) g_malloc0 (sizeof (WavpackStreamReader));
  stream_reader->read_bytes = gst_wavpack_stream_reader_read_bytes;
  stream_reader->get_pos = gst_wavpack_stream_reader_get_pos;
  stream_reader->set_pos_abs = gst_wavpack_stream_reader_set_pos_abs;
  stream_reader->set_pos_rel = gst_wavpack_stream_reader_set_pos_rel;
  stream_reader->push_back_byte = gst_wavpack_stream_reader_push_back_byte;
  stream_reader->get_length = gst_wavpack_stream_reader_get_length;
  stream_reader->can_seek = gst_wavpack_stream_reader_can_seek;
  stream_reader->write_bytes = gst_wavpack_stream_reader_write_bytes;

  return stream_reader;
}
