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


#ifndef __GST_JPEGENC_H__
#define __GST_JPEGENC_H__


#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideoencoder.h>
/* this is a hack hack hack to get around jpeglib header bugs... */
#ifdef HAVE_STDLIB_H
# undef HAVE_STDLIB_H
#endif
#include <stdio.h>
#include <jpeglib.h>

G_BEGIN_DECLS
#define GST_TYPE_JPEGENC \
  (gst_jpegenc_get_type())
#define GST_JPEGENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_JPEGENC,GstJpegEnc))
#define GST_JPEGENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_JPEGENC,GstJpegEncClass))
#define GST_IS_JPEGENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_JPEGENC))
#define GST_IS_JPEGENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_JPEGENC))

typedef struct _GstJpegEnc GstJpegEnc;
typedef struct _GstJpegEncClass GstJpegEncClass;

struct _GstJpegEnc
{
  GstVideoEncoder encoder;

  GstVideoCodecState *input_state;
  GstVideoFrame current_vframe;
  GstVideoCodecFrame *current_frame;
  GstFlowReturn res;

  gboolean input_caps_changed;

  guint channels;

  gint inc[GST_VIDEO_MAX_COMPONENTS];
  gint cwidth[GST_VIDEO_MAX_COMPONENTS];
  gint cheight[GST_VIDEO_MAX_COMPONENTS];
  gint h_samp[GST_VIDEO_MAX_COMPONENTS];
  gint v_samp[GST_VIDEO_MAX_COMPONENTS];
  gint h_max_samp;
  gint v_max_samp;
  gboolean planar;
  gint sof_marker;
  /* the video buffer */
  gint bufsize;
  /* the jpeg line buffer */
  guchar **line[3];
  /* indirect encoding line buffers */
  guchar *row[3][4 * DCTSIZE];

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct jpeg_destination_mgr jdest;

  /* properties */
  gint quality;
  gint smoothing;
  gint idct_method;
  gboolean snapshot;

  GstMemory *output_mem;
  GstMapInfo output_map;
};

struct _GstJpegEncClass
{
  GstVideoEncoderClass parent_class;
};

GType gst_jpegenc_get_type (void);

G_END_DECLS
#endif /* __GST_JPEGENC_H__ */
