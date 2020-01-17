/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2012  Collabora Ltd.
 *
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

#ifndef __GST_RTP_SBC_DEPAY_H
#define __GST_RTP_SBC_DEPAY_H

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasedepayload.h>
#include <gst/audio/audio.h>

G_BEGIN_DECLS
#define GST_TYPE_RTP_SBC_DEPAY \
	(gst_rtp_sbc_depay_get_type())
#define GST_RTP_SBC_DEPAY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_SBC_DEPAY,\
		GstRtpSbcDepay))
#define GST_RTP_SBC_DEPAY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_SBC_DEPAY,\
		GstRtpSbcDepayClass))
#define GST_IS_RTP_SBC_DEPAY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_SBC_DEPAY))
#define GST_IS_RTP_SBC_DEPAY_CLASS(obj) \
	(G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_SBC_DEPAY))
typedef struct _GstRtpSbcDepay GstRtpSbcDepay;
typedef struct _GstRtpSbcDepayClass GstRtpSbcDepayClass;

struct _GstRtpSbcDepay
{
  GstRTPBaseDepayload base;

  int rate;
  GstAdapter *adapter;
  gboolean ignore_timestamps;

  /* Timestamp tracking when ignoring input timestamps */
  GstAudioStreamAlign *stream_align;
};

struct _GstRtpSbcDepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

GType gst_rtp_sbc_depay_get_type (void);

gboolean gst_rtp_sbc_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif
