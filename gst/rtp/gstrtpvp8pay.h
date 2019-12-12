/*
 * gstrtpvp8pay.h - Header for GstRtpVP8Pay
 * Copyright (C) 2011 Sjoerd Simons <sjoerd@luon.net>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GST_RTP_VP8_PAY_H__
#define __GST_RTP_VP8_PAY_H__

#include <gst/rtp/gstrtpbasepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_VP8_PAY \
  (gst_rtp_vp8_pay_get_type())
#define GST_RTP_VP8_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RTP_VP8_PAY, GstRtpVP8Pay))
#define GST_RTP_VP8_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_RTP_VP8_PAY, GstRtpVP8PayClass))
#define GST_IS_RTP_VP8_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RTP_VP8_PAY))
#define GST_IS_RTP_VP8_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_RTP_VP8_PAY))
#define GST_RTP_VP8_PAY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTP_VP8_PAY, GstRtpVP8PayClass))

typedef struct _GstRtpVP8Pay GstRtpVP8Pay;
typedef struct _GstRtpVP8PayClass GstRtpVP8PayClass;
typedef enum _PictureIDMode PictureIDMode;

enum _PictureIDMode {
  VP8_PAY_NO_PICTURE_ID,
  VP8_PAY_PICTURE_ID_7BITS,
  VP8_PAY_PICTURE_ID_15BITS,
};

struct _GstRtpVP8PayClass
{
  GstRTPBasePayloadClass parent_class;
};

struct _GstRtpVP8Pay
{
  GstRTPBasePayload parent;
  gboolean is_keyframe;
  gint n_partitions;
  /* Treat frame header & tag & partition size block as the first partition,
   * folowed by max. 8 data partitions. last offset is the end of the buffer */
  guint partition_offset[10];
  guint partition_size[9];
  PictureIDMode picture_id_mode;
  guint16 picture_id;
};

GType gst_rtp_vp8_pay_get_type (void);

gboolean gst_rtp_vp8_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* #ifndef __GST_RTP_VP8_PAY_H__ */
