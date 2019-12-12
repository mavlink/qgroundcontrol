/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __GST_RTP_H263P_PAY_H__
#define __GST_RTP_H263P_PAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasepayload.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_H263P_PAY \
  (gst_rtp_h263p_pay_get_type())
#define GST_RTP_H263P_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_H263P_PAY,GstRtpH263PPay))
#define GST_RTP_H263P_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_H263P_PAY,GstRtpH263PPayClass))
#define GST_IS_RTP_H263P_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_H263P_PAY))
#define GST_IS_RTP_H263P_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_H263P_PAY))

typedef struct _GstRtpH263PPay GstRtpH263PPay;
typedef struct _GstRtpH263PPayClass GstRtpH263PPayClass;

typedef enum
{
  GST_FRAGMENTATION_MODE_NORMAL = 0,
  GST_FRAGMENTATION_MODE_SYNC   = 1
} GstFragmentationMode;

struct _GstRtpH263PPay
{
  GstRTPBasePayload    payload;

  GstAdapter          *adapter;
  GstClockTime         first_timestamp;
  GstClockTime         first_duration;
  GstFragmentationMode fragmentation_mode;
};

struct _GstRtpH263PPayClass
{
  GstRTPBasePayloadClass parent_class;
};

GType gst_rtp_h263p_pay_get_type (void);

gboolean gst_rtp_h263p_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_H263P_PAY_H__ */
