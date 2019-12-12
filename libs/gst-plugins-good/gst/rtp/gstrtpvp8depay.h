/*
 * gstrtpvp8depay.h - Header for GstRtpVP8Depay
 * Copyright (C) 2011 Sjoerd Simons <sjoerd@luon.net>
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

#ifndef __GST_RTP_VP8_DEPAY_H__
#define __GST_RTP_VP8_DEPAY_H__

#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_VP8_DEPAY \
  (gst_rtp_vp8_depay_get_type())
#define GST_RTP_VP8_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RTP_VP8_DEPAY, GstRtpVP8Depay))
#define GST_RTP_VP8_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_RTP_VP8_DEPAY, \
    GstRtpVP8DepayClass))
#define GST_IS_RTP_VP8_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RTP_VP8_DEPAY))
#define GST_IS_RTP_VP8_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_RTP_VP8_DEPAY))
#define GST_RTP_VP8_DEPAY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTP_VP8_DEPAY, \
    GstRtpVP8DepayClass))

typedef struct _GstRtpVP8Depay GstRtpVP8Depay;
typedef struct _GstRtpVP8DepayClass GstRtpVP8DepayClass;

struct _GstRtpVP8DepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

struct _GstRtpVP8Depay
{
  GstRTPBaseDepayload parent;
  GstAdapter *adapter;
  gboolean started;

  gboolean waiting_for_keyframe;
  gint last_profile;
  gint last_width;
  gint last_height;

  gboolean wait_for_keyframe;
};

GType gst_rtp_vp8_depay_get_type (void);

gboolean gst_rtp_vp8_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* #ifndef __GST_RTP_VP8_DEPAY_H__ */
