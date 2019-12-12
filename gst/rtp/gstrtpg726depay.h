/* GStreamer
 * Copyright (C) 2005 Edgard Lima <edgard.lima@gmail.com>
 * Copyright (C) 2008 Axis Communications AB <dev-gstreamer@axis.com>
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

#ifndef __GST_RTP_G726_DEPAY_H__
#define __GST_RTP_G726_DEPAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

typedef struct _GstRtpG726Depay GstRtpG726Depay;
typedef struct _GstRtpG726DepayClass GstRtpG726DepayClass;

#define GST_TYPE_RTP_G726_DEPAY \
  (gst_rtp_g726_depay_get_type())
#define GST_RTP_G726_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_G726_DEPAY,GstRtpG726Depay))
#define GST_RTP_G726_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_G726_DEPAY,GstRtpG726DepayClass))
#define GST_IS_RTP_G726_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_G726_DEPAY))
#define GST_IS_RTP_G726_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_G726_DEPAY))

struct _GstRtpG726Depay
{
  GstRTPBaseDepayload depayload;

  gboolean aal2;
  gboolean force_aal2;
  gint bitrate;
  guint block_align;
};

struct _GstRtpG726DepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

GType gst_rtp_g726_depay_get_type (void);

gboolean gst_rtp_g726_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_RTP_G726_DEPAY_H__ */
