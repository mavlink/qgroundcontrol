/*
 * gstrtpvp9depay.h - Header for GstRtpVP9Depay
 * Copyright (C) 2011 Sjoerd Simons <sjoerd@luon.net>
 * Copyright (C) 2015 Stian Selnes <stian@pexip.com>
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

#ifndef __GST_RTP_VP9_DEPAY_H__
#define __GST_RTP_VP9_DEPAY_H__

#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_VP9_DEPAY \
  (gst_rtp_vp9_depay_get_type())
#define GST_RTP_VP9_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RTP_VP9_DEPAY, GstRtpVP9Depay))
#define GST_RTP_VP9_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_RTP_VP9_DEPAY, \
    GstRtpVP9DepayClass))
#define GST_IS_RTP_VP9_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RTP_VP9_DEPAY))
#define GST_IS_RTP_VP9_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_RTP_VP9_DEPAY))
#define GST_RTP_VP9_DEPAY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTP_VP9_DEPAY, \
    GstRtpVP9DepayClass))

typedef struct _GstRtpVP9Depay GstRtpVP9Depay;
typedef struct _GstRtpVP9DepayClass GstRtpVP9DepayClass;

struct _GstRtpVP9DepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

struct _GstRtpVP9Depay
{
  GstRTPBaseDepayload parent;
  GstAdapter *adapter;
  gboolean started;

  gint ss_width;
  gint ss_height;
  gint last_width;
  gint last_height;
  gboolean caps_sent;
};

GType gst_rtp_vp9_depay_get_type (void);

gboolean gst_rtp_vp9_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* #ifndef __GST_RTP_VP9_DEPAY_H__ */
