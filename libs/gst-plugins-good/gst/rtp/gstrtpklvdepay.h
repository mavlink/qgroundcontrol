/* GStreamer RTP KLV Depayloader
 * Copyright (C) 2014-2015 Tim-Philipp Müller <tim@centricular.com>>
 * Copyright (C) 2014-2015 Centricular Ltd
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

#ifndef __GST_RTP_KLV_DEPAY_H__
#define __GST_RTP_KLV_DEPAY_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_KLV_DEPAY \
  (gst_rtp_klv_depay_get_type())
#define GST_RTP_KLV_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_KLV_DEPAY,GstRtpKlvDepay))
#define GST_RTP_KLV_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_KLV_DEPAY,GstRtpKlvDepayClass))
#define GST_IS_RTP_KLV_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_KLV_DEPAY))
#define GST_IS_RTP_KLV_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_KLV_DEPAY))

typedef struct _GstRtpKlvDepay GstRtpKlvDepay;
typedef struct _GstRtpKlvDepayClass GstRtpKlvDepayClass;

struct _GstRtpKlvDepay
{
  GstRTPBaseDepayload depayload;

  GstAdapter *adapter;
  gboolean    resync;
  gint        last_marker_seq;   /* -1 if unset, otherwise 0-G_MAXUINT16 */
  gint64      last_rtp_ts;       /* -1 if unset, otherwise 0-G_MAXUINT32 */
};

struct _GstRtpKlvDepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

G_GNUC_INTERNAL GType     gst_rtp_klv_depay_get_type (void);

G_GNUC_INTERNAL gboolean  gst_rtp_klv_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_KLV_DEPAY_H__ */
