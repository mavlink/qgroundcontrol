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

#ifndef __GST_RTP_AMR_DEPAY_H__
#define __GST_RTP_AMR_DEPAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_AMR_DEPAY \
  (gst_rtp_amr_depay_get_type())
#define GST_RTP_AMR_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_AMR_DEPAY,GstRtpAMRDepay))
#define GST_RTP_AMR_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_AMR_DEPAY,GstRtpAMRDepayClass))
#define GST_IS_RTP_AMR_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_AMR_DEPAY))
#define GST_IS_RTP_AMR_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_AMR_DEPAY))

typedef struct _GstRtpAMRDepay GstRtpAMRDepay;
typedef struct _GstRtpAMRDepayClass GstRtpAMRDepayClass;

typedef enum {
  GST_RTP_AMR_DP_MODE_INVALID = 0,
  GST_RTP_AMR_DP_MODE_NB      = 1,
  GST_RTP_AMR_DP_MODE_WB      = 2
} GstRtpAMRDepayMode;

struct _GstRtpAMRDepay
{
  GstRTPBaseDepayload depayload;

  GstRtpAMRDepayMode mode;

  gboolean octet_align;
  guint8   mode_set;
  gint     mode_change_period;
  gboolean mode_change_neighbor;
  gint     maxptime;
  gboolean crc;
  gboolean robust_sorting;
  gboolean interleaving;
  gint     ptime;
  gint     channels;
};

struct _GstRtpAMRDepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

GType gst_rtp_amr_depay_get_type (void);

gboolean gst_rtp_amr_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_AMR_DEPAY_H__ */
