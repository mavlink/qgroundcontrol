/* GStreamer plugin for forward error correction
 * Copyright (C) 2017 Pexip
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Mikhail Fludkov <misha@pexip.com>
 */

#ifndef __GST_RTP_ULPFEC_DEC_H__
#define __GST_RTP_ULPFEC_DEC_H__

#include <gst/gst.h>

#include "rtpstorage.h"

G_BEGIN_DECLS

#define GST_TYPE_RTP_ULPFEC_DEC \
  (gst_rtp_ulpfec_dec_get_type())
#define GST_RTP_ULPFEC_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_ULPFEC_DEC,GstRtpUlpFecDec))
#define GST_RTP_ULPFEC_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_ULPFEC_DEC,GstRtpUlpFecDecClass))
#define GST_IS_RTP_ULPFEC_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_ULPFEC_DEC))
#define GST_IS_RTP_ULPFEC_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_ULPFEC_DEC))

typedef struct _GstRtpUlpFecDec GstRtpUlpFecDec;
typedef struct _GstRtpUlpFecDecClass GstRtpUlpFecDecClass;

struct _GstRtpUlpFecDecClass {
  GstElementClass parent_class;
};

struct _GstRtpUlpFecDec {
  GstElement parent;
  GstPad *srcpad;
  GstPad *sinkpad;

  /* properties */
  guint8 fec_pt;
  RtpStorage *storage;
  gsize packets_recovered;
  gsize packets_unrecovered;

  /* internal stuff */
  GstFlowReturn chain_return_val;
  gboolean unset_discont_flag;
  gboolean have_caps_ssrc;
  gboolean have_caps_pt;
  guint32 caps_ssrc;
  guint8 caps_pt;
  GList *info_media;
  GPtrArray *info_fec;
  GArray *info_arr;
  GArray *scratch_buf;
  gboolean lost_packet_from_storage;
  gboolean lost_packet_returned;
  guint16 next_seqnum;

  /* stats */
  gsize fec_packets_received;
  gsize fec_packets_rejected;
  gsize packets_rejected;
};

GType gst_rtp_ulpfec_dec_get_type (void);

G_END_DECLS

#endif /* __GST_RTP_ULPFEC_DEC_H__ */
