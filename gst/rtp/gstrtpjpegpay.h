/* GStreamer
 * Copyright (C) 2008 Axis Communications <dev-gstreamer@axis.com>
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

#ifndef __GST_RTP_JPEG_PAY_H__
#define __GST_RTP_JPEG_PAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasepayload.h>

G_BEGIN_DECLS
#define GST_TYPE_RTP_JPEG_PAY \
  (gst_rtp_jpeg_pay_get_type())
#define GST_RTP_JPEG_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_JPEG_PAY,GstRtpJPEGPay))
#define GST_RTP_JPEG_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_JPEG_PAY,GstRtpJPEGPayClass))
#define GST_IS_RTP_JPEG_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_JPEG_PAY))
#define GST_IS_RTP_JPEG_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_JPEG_PAY))
typedef struct _GstRtpJPEGPay GstRtpJPEGPay;
typedef struct _GstRtpJPEGPayClass GstRtpJPEGPayClass;

struct _GstRtpJPEGPay
{
  GstRTPBasePayload payload;

  guint8 quality;
  guint8 type;

  gint height;
  gint width;

  guint8 quant;
};

struct _GstRtpJPEGPayClass
{
  GstRTPBasePayloadClass parent_class;
};

GType gst_rtp_jpeg_pay_get_type (void);

gboolean gst_rtp_jpeg_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_RTP_JPEG_PAY_H__ */
