/*  GStreamer RTP SBC payloader
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasepayload.h>
#include <gst/rtp/gstrtpbuffer.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_SBC_PAY \
  (gst_rtp_sbc_pay_get_type())
#define GST_RTP_SBC_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_SBC_PAY,\
                              GstRtpSBCPay))
#define GST_RTP_SBC_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_SBC_PAY,\
                           GstRtpSBCPayClass))
#define GST_IS_RTP_SBC_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_SBC_PAY))
#define GST_IS_RTP_SBC_PAY_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_SBC_PAY))

typedef struct _GstRtpSBCPay GstRtpSBCPay;
typedef struct _GstRtpSBCPayClass GstRtpSBCPayClass;

struct _GstRtpSBCPay {
  GstRTPBasePayload base;

  GstAdapter *adapter;
  GstClockTime last_timestamp;

  guint frame_length;
  GstClockTime frame_duration;

  guint min_frames;
};

struct _GstRtpSBCPayClass {
  GstRTPBasePayloadClass parent_class;
};

GType gst_rtp_sbc_pay_get_type(void);

gboolean gst_rtp_sbc_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS
