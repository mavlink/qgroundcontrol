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

#ifndef __GST_ISOFF_QT_H__
#define __GST_ISOFF_QT_H__

#include <gst/gst.h>

G_BEGIN_DECLS

typedef enum {
  GST_ISOFF_QT_PARSER_OK,
  GST_ISOFF_QT_PARSER_DONE,
  GST_ISOFF_QT_PARSER_UNEXPECTED,
  GST_ISOFF_QT_PARSER_ERROR
} GstIsoffParserResult;

/* this is the minimum size, it can be larger if it
 * uses extended size or type */
#define GST_ISOFF_QT_FULL_BOX_SIZE 12

#define GST_ISOFF_QT_FOURCC_SIDX GST_MAKE_FOURCC('s','i','d','x')
typedef struct _GstSidxBoxEntry
{
  gboolean ref_type;
  guint32 size;
  GstClockTime duration;
  gboolean starts_with_sap;
  guint8 sap_type;
  guint32 sap_delta_time;

  guint64 offset;
  GstClockTime pts;
} GstSidxBoxEntry;

typedef struct _GstSidxBox
{
  guint8 version;
  guint32 flags;

  guint32 ref_id;
  guint32 timescale;
  guint64 earliest_pts;
  guint64 first_offset;

  gint entry_index;
  gint entries_count;

  GstSidxBoxEntry *entries;
} GstSidxBox;

typedef enum _GstSidxParserStatus
{
  GST_ISOFF_QT_SIDX_PARSER_INIT,
  GST_ISOFF_QT_SIDX_PARSER_HEADER,
  GST_ISOFF_QT_SIDX_PARSER_DATA,
  GST_ISOFF_QT_SIDX_PARSER_FINISHED
} GstSidxParserStatus;

typedef struct _GstSidxParser
{
  GstSidxParserStatus status;

  guint64 size;
  guint64 cumulative_entry_size;
  guint64 cumulative_pts;

  GstSidxBox sidx;
} GstSidxParser;

void gst_isoff_qt_sidx_parser_init (GstSidxParser * parser);
void gst_isoff_qt_sidx_parser_clear (GstSidxParser * parser);
GstIsoffParserResult gst_isoff_qt_sidx_parser_add_data (GstSidxParser * parser, const guint8 * buffer, gint length, guint * consumed);
GstIsoffParserResult gst_isoff_qt_sidx_parser_add_buffer (GstSidxParser * parser, GstBuffer * buf, guint * consumed);

G_END_DECLS

#endif /* __GST_ISOFF_QT_H__ */

