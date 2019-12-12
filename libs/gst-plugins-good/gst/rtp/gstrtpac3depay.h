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

#ifndef __GST_RTP_AC3_DEPAY_H__
#define __GST_RTP_AC3_DEPAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_AC3_DEPAY \
  (gst_rtp_ac3_depay_get_type())
#define GST_RTP_AC3_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_AC3_DEPAY,GstRtpAC3Depay))
#define GST_RTP_AC3_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_AC3_DEPAY,GstRtpAC3DepayClass))
#define GST_IS_RTP_AC3_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_AC3_DEPAY))
#define GST_IS_RTP_AC3_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_AC3_DEPAY))

typedef struct _GstRtpAC3Depay GstRtpAC3Depay;
typedef struct _GstRtpAC3DepayClass GstRtpAC3DepayClass;

struct _GstRtpAC3Depay
{
  GstRTPBaseDepayload depayload;
};

struct _GstRtpAC3DepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

GType gst_rtp_ac3_depay_get_type (void);

gboolean gst_rtp_ac3_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_AC3_DEPAY_H__ */
