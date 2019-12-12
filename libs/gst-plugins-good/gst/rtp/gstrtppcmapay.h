/* GStreamer
 * Copyright (C) <2005> Edgard Lima <edgard.lima@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more 
 */


#ifndef __GST_RTP_PCMA_PAY_H__
#define __GST_RTP_PCMA_PAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbaseaudiopayload.h>

G_BEGIN_DECLS

typedef struct _GstRtpPcmaPay GstRtpPcmaPay;
typedef struct _GstRtpPcmaPayClass GstRtpPcmaPayClass;

#define GST_TYPE_RTP_PCMA_PAY \
  (gst_rtp_pcma_pay_get_type())
#define GST_RTP_PCMA_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_PCMA_PAY,GstRtpPcmaPay))
#define GST_RTP_PCMA_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_PCMA_PAY,GstRtpPcmaPayClass))
#define GST_IS_RTP_PCMA_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_PCMA_PAY))
#define GST_IS_RTP_PCMA_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_PCMA_PAY))

struct _GstRtpPcmaPay
{
  GstRTPBaseAudioPayload audiopayload;
};

struct _GstRtpPcmaPayClass
{
  GstRTPBaseAudioPayloadClass parent_class;
};

GType gst_rtp_pcma_pay_get_type (void);

gboolean gst_rtp_pcma_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_PCMA_PAY_H__ */
