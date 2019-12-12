/*
 * ISO File Format parsing library
 *
 * gstisoff.h
 *
 * Copyright (C) 2015 Samsung Electronics. All rights reserved.
 *   Author: Thiago Santos <thiagoss@osg.samsung.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library (COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "qtdemux_debug.h"
#include "gstisoff.h"
#include <gst/base/gstbytereader.h>

#define GST_CAT_DEFAULT qtdemux_debug

void
gst_isoff_qt_sidx_parser_init (GstSidxParser * parser)
{
  parser->status = GST_ISOFF_QT_SIDX_PARSER_INIT;
  parser->cumulative_entry_size = 0;
  parser->sidx.entries = NULL;
  parser->sidx.entries_count = 0;
}

void
gst_isoff_qt_sidx_parser_clear (GstSidxParser * parser)
{
  g_free (parser->sidx.entries);
  parser->sidx.entries = NULL;
}

static void
gst_isoff_qt_parse_sidx_entry (GstSidxBoxEntry * entry, GstByteReader * reader)
{
  guint32 aux;

  aux = gst_byte_reader_get_uint32_be_unchecked (reader);
  entry->ref_type = aux >> 31;
  entry->size = aux & 0x7FFFFFFF;
  entry->duration = gst_byte_reader_get_uint32_be_unchecked (reader);
  aux = gst_byte_reader_get_uint32_be_unchecked (reader);
  entry->starts_with_sap = aux >> 31;
  entry->sap_type = ((aux >> 28) & 0x7);
  entry->sap_delta_time = aux & 0xFFFFFFF;
}

GstIsoffParserResult
gst_isoff_qt_sidx_parser_add_data (GstSidxParser * parser,
    const guint8 * buffer, gint length, guint * consumed)
{
  GstIsoffParserResult res = GST_ISOFF_QT_PARSER_OK;
  GstByteReader reader;
  gsize remaining;
  guint32 fourcc;

  gst_byte_reader_init (&reader, buffer, length);

  switch (parser->status) {
    case GST_ISOFF_QT_SIDX_PARSER_INIT:
      if (gst_byte_reader_get_remaining (&reader) < GST_ISOFF_QT_FULL_BOX_SIZE) {
        break;
      }

      parser->size = gst_byte_reader_get_uint32_be_unchecked (&reader);
      fourcc = gst_byte_reader_get_uint32_le_unchecked (&reader);
      if (fourcc != GST_ISOFF_QT_FOURCC_SIDX) {
        res = GST_ISOFF_QT_PARSER_UNEXPECTED;
        gst_byte_reader_set_pos (&reader, 0);
        break;
      }
      if (parser->size == 1) {
        if (gst_byte_reader_get_remaining (&reader) < 12) {
          gst_byte_reader_set_pos (&reader, 0);
          break;
        }

        parser->size = gst_byte_reader_get_uint64_be_unchecked (&reader);
      }
      if (parser->size == 0) {
        res = GST_ISOFF_QT_PARSER_ERROR;
        gst_byte_reader_set_pos (&reader, 0);
        break;
      }
      parser->sidx.version = gst_byte_reader_get_uint8_unchecked (&reader);
      parser->sidx.flags = gst_byte_reader_get_uint24_le_unchecked (&reader);

      parser->status = GST_ISOFF_QT_SIDX_PARSER_HEADER;

    case GST_ISOFF_QT_SIDX_PARSER_HEADER:
      remaining = gst_byte_reader_get_remaining (&reader);
      if (remaining < 12 + (parser->sidx.version == 0 ? 8 : 16)) {
        break;
      }

      parser->sidx.ref_id = gst_byte_reader_get_uint32_be_unchecked (&reader);
      parser->sidx.timescale =
          gst_byte_reader_get_uint32_be_unchecked (&reader);
      if (parser->sidx.version == 0) {
        parser->sidx.earliest_pts =
            gst_byte_reader_get_uint32_be_unchecked (&reader);
        parser->sidx.first_offset = parser->sidx.earliest_pts =
            gst_byte_reader_get_uint32_be_unchecked (&reader);
      } else {
        parser->sidx.earliest_pts =
            gst_byte_reader_get_uint64_be_unchecked (&reader);
        parser->sidx.first_offset =
            gst_byte_reader_get_uint64_be_unchecked (&reader);
      }
      /* skip 2 reserved bytes */
      gst_byte_reader_skip_unchecked (&reader, 2);
      parser->sidx.entries_count =
          gst_byte_reader_get_uint16_be_unchecked (&reader);

      GST_LOG ("Timescale: %" G_GUINT32_FORMAT, parser->sidx.timescale);
      GST_LOG ("Earliest pts: %" G_GUINT64_FORMAT, parser->sidx.earliest_pts);
      GST_LOG ("First offset: %" G_GUINT64_FORMAT, parser->sidx.first_offset);

      parser->cumulative_pts =
          gst_util_uint64_scale_int_round (parser->sidx.earliest_pts,
          GST_SECOND, parser->sidx.timescale);

      if (parser->sidx.entries_count) {
        parser->sidx.entries =
            g_malloc (sizeof (GstSidxBoxEntry) * parser->sidx.entries_count);
      }
      parser->sidx.entry_index = 0;

      parser->status = GST_ISOFF_QT_SIDX_PARSER_DATA;

    case GST_ISOFF_QT_SIDX_PARSER_DATA:
      while (parser->sidx.entry_index < parser->sidx.entries_count) {
        GstSidxBoxEntry *entry =
            &parser->sidx.entries[parser->sidx.entry_index];

        remaining = gst_byte_reader_get_remaining (&reader);
        if (remaining < 12)
          break;

        entry->offset = parser->cumulative_entry_size;
        entry->pts = parser->cumulative_pts;
        gst_isoff_qt_parse_sidx_entry (entry, &reader);
        entry->duration = gst_util_uint64_scale_int_round (entry->duration,
            GST_SECOND, parser->sidx.timescale);
        parser->cumulative_entry_size += entry->size;
        parser->cumulative_pts += entry->duration;

        GST_LOG ("Sidx entry %d) offset: %" G_GUINT64_FORMAT ", pts: %"
            GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT " - size %"
            G_GUINT32_FORMAT, parser->sidx.entry_index, entry->offset,
            GST_TIME_ARGS (entry->pts), GST_TIME_ARGS (entry->duration),
            entry->size);

        parser->sidx.entry_index++;
      }

      if (parser->sidx.entry_index == parser->sidx.entries_count)
        parser->status = GST_ISOFF_QT_SIDX_PARSER_FINISHED;
      else
        break;
    case GST_ISOFF_QT_SIDX_PARSER_FINISHED:
      parser->sidx.entry_index = 0;
      res = GST_ISOFF_QT_PARSER_DONE;
      break;
  }

  *consumed = gst_byte_reader_get_pos (&reader);
  return res;
}

GstIsoffParserResult
gst_isoff_qt_sidx_parser_add_buffer (GstSidxParser * parser, GstBuffer * buffer,
    guint * consumed)
{
  GstIsoffParserResult res = GST_ISOFF_QT_PARSER_OK;
  GstMapInfo info;

  if (!gst_buffer_map (buffer, &info, GST_MAP_READ)) {
    *consumed = 0;
    return GST_ISOFF_QT_PARSER_ERROR;
  }

  res =
      gst_isoff_qt_sidx_parser_add_data (parser, info.data, info.size,
      consumed);

  gst_buffer_unmap (buffer, &info);
  return res;
}
