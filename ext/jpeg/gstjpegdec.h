/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2012 Collabora Ltd.
 *	Author : Edward Hervey <edward@collabora.com>
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


#ifndef __GST_JPEG_DEC_H__
#define __GST_JPEG_DEC_H__


#include <setjmp.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideodecoder.h>
#include <gst/base/gstadapter.h>

/* this is a hack hack hack to get around jpeglib header bugs... */
#ifdef HAVE_STDLIB_H
# undef HAVE_STDLIB_H
#endif
#include <stdio.h>
#include <jpeglib.h>

G_BEGIN_DECLS

#define GST_TYPE_JPEG_DEC \
  (gst_jpeg_dec_get_type())
#define GST_JPEG_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_JPEG_DEC,GstJpegDec))
#define GST_JPEG_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_JPEG_DEC,GstJpegDecClass))
#define GST_IS_JPEG_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_JPEG_DEC))
#define GST_IS_JPEG_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_JPEG_DEC))

typedef struct _GstJpegDec           GstJpegDec;
typedef struct _GstJpegDecClass      GstJpegDecClass;

struct GstJpegDecErrorMgr {
  struct jpeg_error_mgr    pub;   /* public fields */
  jmp_buf                  setjmp_buffer;
};

struct GstJpegDecSourceMgr {
  struct jpeg_source_mgr   pub;   /* public fields */
  GstJpegDec              *dec;
};

/* Can't use GstBaseTransform, because GstBaseTransform
 * doesn't handle the N buffers in, 1 buffer out case,
 * but only the 1-in 1-out case */
struct _GstJpegDec {
  GstVideoDecoder decoder;

  /* negotiated state */
  GstVideoCodecState *input_state;
  GstVideoCodecFrame *current_frame;
  GstMapInfo current_frame_map;

  /* parse state */
  gboolean saw_header;
  gint     parse_entropy_len;
  gint     parse_resync;

  /* properties */
  gint     idct_method;
  gint     max_errors;  /* ATOMIC */

  struct jpeg_decompress_struct cinfo;
  struct GstJpegDecErrorMgr     jerr;
  struct GstJpegDecSourceMgr    jsrc;

  /* arrays for indirect decoding */
  gboolean idr_width_allocated;
  guchar *idr_y[16],*idr_u[16],*idr_v[16];
  /* scratch buffer for direct decoding overflow */
  guchar *scratch;
  guint scratch_size;
  /* current (parsed) image size */
  guint    rem_img_len;
};

struct _GstJpegDecClass {
  GstVideoDecoderClass decoder_class;
};

GType gst_jpeg_dec_get_type(void);


G_END_DECLS


#endif /* __GST_JPEG_DEC_H__ */
