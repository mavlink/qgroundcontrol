/* RTP Retransmission receiver element for GStreamer
 *
 * gstrtprtxreceive.h:
 *
 * Copyright (C) 2013 Collabora Ltd.
 *   @author Julien Isorce <julien.isorce@collabora.co.uk>
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

#ifndef __GST_RTP_RTX_RECEIVE_H__
#define __GST_RTP_RTX_RECEIVE_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbuffer.h>

G_BEGIN_DECLS
#define GST_TYPE_RTP_RTX_RECEIVE (gst_rtp_rtx_receive_get_type())
#define GST_RTP_RTX_RECEIVE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_RTX_RECEIVE, GstRtpRtxReceive))
#define GST_RTP_RTX_RECEIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_RTX_RECEIVE, GstRtpRtxReceiveClass))
#define GST_RTP_RTX_RECEIVE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTP_RTX_RECEIVE, GstRtpRtxReceiveClass))
#define GST_IS_RTP_RTX_RECEIVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_RTX_RECEIVE))
#define GST_IS_RTP_RTX_RECEIVE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_RTX_RECEIVE))
typedef struct _GstRtpRtxReceive GstRtpRtxReceive;
typedef struct _GstRtpRtxReceiveClass GstRtpRtxReceiveClass;

struct _GstRtpRtxReceive
{
  GstElement element;

  /* pad */
  GstPad *sinkpad;
  GstPad *srcpad;

  /* retrieve associated master stream from rtx stream
   * it also works to retrieve rtx stream from master stream
   * as we make sure all ssrc are unique */
  GHashTable *ssrc2_ssrc1_map;

  /* contains seqnum of request packets of whom their ssrc have
   * not been associated to a rtx stream yet */
  GHashTable *seqnum_ssrc1_map;

  /* rtx pt (uint) -> origin pt (uint) */
  GHashTable *rtx_pt_map;
  /* origin pt (string) -> rtx pt (uint) */
  GstStructure *rtx_pt_map_structure;

  /* statistics */
  guint num_rtx_requests;
  guint num_rtx_packets;
  guint num_rtx_assoc_packets;

  GstClockTime last_time;
};

struct _GstRtpRtxReceiveClass
{
  GstElementClass parent_class;
};


GType gst_rtp_rtx_receive_get_type (void);
gboolean gst_rtp_rtx_receive_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_RTP_RTX_RECEIVE_H__ */
