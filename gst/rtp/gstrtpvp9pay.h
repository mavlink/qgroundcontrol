/*
 * gstrtpvp9pay.h - Header for GstRtpVP9Pay
 * Copyright (C) 2011 Sjoerd Simons <sjoerd@luon.net>
 * Copyright (C) 2015 Stian Selnes <stian@pexip.com>
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

#ifndef __GST_RTP_VP9_PAY_H__
#define __GST_RTP_VP9_PAY_H__

#include <gst/rtp/gstrtpbasepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_VP9_PAY \
  (gst_rtp_vp9_pay_get_type())
#define GST_RTP_VP9_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RTP_VP9_PAY, GstRtpVP9Pay))
#define GST_RTP_VP9_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_RTP_VP9_PAY, GstRtpVP9PayClass))
#define GST_IS_RTP_VP9_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RTP_VP9_PAY))
#define GST_IS_RTP_VP9_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_RTP_VP9_PAY))
#define GST_RTP_VP9_PAY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTP_VP9_PAY, GstRtpVP9PayClass))

typedef struct _GstRtpVP9Pay GstRtpVP9Pay;
typedef struct _GstRtpVP9PayClass GstRtpVP9PayClass;
typedef enum _VP9PictureIDMode VP9PictureIDMode;

enum _VP9PictureIDMode {
  VP9_PAY_NO_PICTURE_ID,
  VP9_PAY_PICTURE_ID_7BITS,
  VP9_PAY_PICTURE_ID_15BITS,
};

struct _GstRtpVP9PayClass
{
  GstRTPBasePayloadClass parent_class;
};

struct _GstRtpVP9Pay
{
  GstRTPBasePayload parent;
  gboolean is_keyframe;
  guint width;
  guint height;
  VP9PictureIDMode picture_id_mode;
  guint16 picture_id;
};

GType gst_rtp_vp9_pay_get_type (void);

gboolean gst_rtp_vp9_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* #ifndef __GST_RTP_VP9_PAY_H__ */
