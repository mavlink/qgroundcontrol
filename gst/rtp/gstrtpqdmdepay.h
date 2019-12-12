/* GStreamer
 * Copyright (C) <2009> Edward Hervey <bilboed@bilboed.com>
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

#ifndef __GST_RTP_QDM2_DEPAY_H__
#define __GST_RTP_QDM2_DEPAY_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasedepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_QDM2_DEPAY \
  (gst_rtp_qdm2_depay_get_type())
#define GST_RTP_QDM2_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_QDM2_DEPAY,GstRtpQDM2Depay))
#define GST_RTP_QDM2_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_QDM2_DEPAY,GstRtpQDM2DepayClass))
#define GST_IS_RTP_QDM2_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_QDM2_DEPAY))
#define GST_IS_RTP_QDM2_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_QDM2_DEPAY))

typedef struct _GstRtpQDM2Depay GstRtpQDM2Depay;
typedef struct _GstRtpQDM2DepayClass GstRtpQDM2DepayClass;

typedef struct _QDM2Packet {
  guint8* data;
  guint offs;		/* Starts at 4 to give room for the prefix */
} QDM2Packet;

#define MAX_SCRAMBLED_PACKETS 64

struct _GstRtpQDM2Depay
{
  GstRTPBaseDepayload depayload;

  GstAdapter *adapter;

  guint16 nextseq;
  gboolean configured;

  GstClockTime timestamp; /* Timestamp of current incoming data */
  GstClockTime ptimestamp; /* Timestamp of data stored in the adapter */

  guint32 channs;
  guint32 samplerate;
  guint32 bitrate;
  guint32 blocksize;
  guint32 framesize;
  guint32 packetsize;

  guint nbpackets;	/* Number of packets to unscramble */

  QDM2Packet *packets[MAX_SCRAMBLED_PACKETS];
};

struct _GstRtpQDM2DepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

GType gst_rtp_qdm2_depay_get_type (void);

gboolean gst_rtp_qdm2_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_QDM2_DEPAY_H__ */
