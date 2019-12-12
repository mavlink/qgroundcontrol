/* Farsight
 * Copyright (C) 2006 Marcel Moreaux <marcelm@spacelabs.nl>
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


#ifndef __GSTRTPDVPAY_H__
#define __GSTRTPDVPAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasepayload.h>

G_BEGIN_DECLS

typedef struct _GstRTPDVPay GstRTPDVPay;
typedef struct _GstRTPDVPayClass GstRTPDVPayClass;

#define GST_TYPE_RTP_DV_PAY \
  (gst_rtp_dv_pay_get_type())
#define GST_RTP_DV_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_DV_PAY,GstRTPDVPay))
#define GST_RTP_DV_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_DV_PAY,GstRTPDVPay))
#define GST_IS_RTP_DV_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_DV_PAY))
#define GST_IS_RTP_DV_PAY_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_DV_PAY))

typedef enum
{
  GST_DV_PAY_MODE_VIDEO,
  GST_DV_PAY_MODE_BUNDLED,
  GST_DV_PAY_MODE_AUDIO
} GstDVPayMode;

struct _GstRTPDVPay
{
  GstRTPBasePayload payload;

  gboolean negotiated;
  GstDVPayMode mode;
};

struct _GstRTPDVPayClass
{
  GstRTPBasePayloadClass parent_class;
};

GType gst_rtp_dv_pay_get_type (void);

gboolean gst_rtp_dv_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GSTRTPDVPAY_H__ */
