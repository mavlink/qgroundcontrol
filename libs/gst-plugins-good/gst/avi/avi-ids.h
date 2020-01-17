/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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

#ifndef __GST_AVI_H__
#define __GST_AVI_H__

#include <gst/gst.h>

typedef struct _gst_riff_avih {
  guint32 us_frame;          /* microsec per frame */
  guint32 max_bps;           /* byte/s overall */
  guint32 pad_gran;          /* pad_granularity */
  guint32 flags;
/* flags values */
#define GST_RIFF_AVIH_HASINDEX       0x00000010 /* has idx1 chunk */
#define GST_RIFF_AVIH_MUSTUSEINDEX   0x00000020 /* must use idx1 chunk to determine order */
#define GST_RIFF_AVIH_ISINTERLEAVED  0x00000100 /* AVI file is interleaved */
#define GST_RIFF_AVIH_TRUSTCKTYPE    0x00000800 /* Use CKType to find key frames */
#define GST_RIFF_AVIH_WASCAPTUREFILE 0x00010000 /* specially allocated used for capturing real time video */
#define GST_RIFF_AVIH_COPYRIGHTED    0x00020000 /* contains copyrighted data */
  guint32 tot_frames;        /* # of frames (all) */
  guint32 init_frames;       /* initial frames (???) */
  guint32 streams;
  guint32 bufsize;           /* suggested buffer size */
  guint32 width;
  guint32 height;
  guint32 scale;
  guint32 rate;
  guint32 start;
  guint32 length;
} gst_riff_avih;

/* vprp (video properties) ODML header */
/* see ODML spec for some/more explanation */
#define GST_RIFF_TAG_vprp GST_MAKE_FOURCC ('v','p','r','p')
#define GST_RIFF_DXSB GST_MAKE_FOURCC ('D','X','S','B')
#define GST_RIFF_VPRP_VIDEO_FIELDS        (2)

typedef struct _gst_riff_vprp_video_field_desc {
  guint32 compressed_bm_height;
  guint32 compressed_bm_width;
  guint32 valid_bm_height;
  guint32 valid_bm_width;
  guint32 valid_bm_x_offset;
  guint32 valid_bm_y_offset;
  guint32 video_x_t_offset;
  guint32 video_y_start;
} gst_riff_vprp_video_field_desc;

typedef struct _gst_riff_vprp {
  guint32 format_token;      /* whether fields defined by standard */
  guint32 standard;          /* video display standard, UNKNOWN, PAL, etc */
  guint32 vert_rate;         /* vertical refresh rate */
  guint32 hor_t_total;       /* width */
  guint32 vert_lines;        /* height */
  guint32 aspect;            /* aspect ratio high word:low word */
  guint32 width;             /* active width */
  guint32 height;            /* active height */
  guint32 fields;            /* field count */
  gst_riff_vprp_video_field_desc field_info[GST_RIFF_VPRP_VIDEO_FIELDS];
} gst_riff_vprp;

#endif /* __GST_AVI_H__ */
