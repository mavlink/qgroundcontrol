/* Farsight
 * Copyright (C) 2006 Marcel Moreaux <marcelm@spacelabs.nl>
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

#ifndef __GSTRTPDVDEPAY_H__
#define __GSTRTPDVDEPAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

/* #define's don't like whitespacey bits */
#define GST_TYPE_RTP_DV_DEPAY (gst_rtp_dv_depay_get_type())
#define GST_RTP_DV_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_DV_DEPAY,GstRTPDVDepay))
#define GST_RTP_DV_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_DV_DEPAY,GstRTPDVDepay))
#define GST_IS_RTP_DV_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_DV_DEPAY))
#define GST_IS_RTP_DV_DEPAY_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_DV_DEPAY))

typedef struct _GstRTPDVDepay      GstRTPDVDepay;
typedef struct _GstRTPDVDepayClass GstRTPDVDepayClass;

struct _GstRTPDVDepay
{
  GstRTPBaseDepayload parent;

  GstBuffer *acc;
  guint frame_size;
  guint32 prev_ts;
  guint8 header_mask;

  gint width, height;
  gint rate_num, rate_denom;
};

struct _GstRTPDVDepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

GType gst_rtp_dv_depay_get_type (void);

gboolean gst_rtp_dv_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GSTRTPDVDEPAY_H__ */
