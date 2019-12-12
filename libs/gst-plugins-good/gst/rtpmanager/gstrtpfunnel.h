/* RTP funnel element for GStreamer
 *
 * gstrtpfunnel.h:
 *
 * Copyright (C) <2017> Pexip.
 *   Contact: Havard Graff <havard@pexip.com>
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
#ifndef __GST_RTP_FUNNEL_H__
#define __GST_RTP_FUNNEL_H__

#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _GstRtpFunnelClass GstRtpFunnelClass;
typedef struct _GstRtpFunnel GstRtpFunnel;

#define GST_TYPE_RTP_FUNNEL (gst_rtp_funnel_get_type())
#define GST_RTP_FUNNEL_CAST(obj) ((GstRtpFunnel *)(obj))

GType gst_rtp_funnel_get_type (void);

typedef struct _GstRtpFunnelPadClass GstRtpFunnelPadClass;
typedef struct _GstRtpFunnelPad GstRtpFunnelPad;

#define GST_TYPE_RTP_FUNNEL_PAD (gst_rtp_funnel_pad_get_type())
#define GST_RTP_FUNNEL_PAD_CAST(obj) ((GstRtpFunnelPad *)(obj))

GType gst_rtp_funnel_pad_get_type (void);

G_END_DECLS

#endif /* __GST_RTP_FUNNEL_H__ */
