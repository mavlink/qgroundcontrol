/* GStreamer
 * Copyright (C) <2007> Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __GST_RTP_SSRC_DEMUX_H__
#define __GST_RTP_SSRC_DEMUX_H__

#include <gst/gst.h>

#define GST_TYPE_RTP_SSRC_DEMUX            (gst_rtp_ssrc_demux_get_type())
#define GST_RTP_SSRC_DEMUX(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_SSRC_DEMUX,GstRtpSsrcDemux))
#define GST_RTP_SSRC_DEMUX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_SSRC_DEMUX,GstRtpSsrcDemuxClass))
#define GST_IS_RTP_SSRC_DEMUX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_SSRC_DEMUX))
#define GST_IS_RTP_SSRC_DEMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_SSRC_DEMUX))

typedef struct _GstRtpSsrcDemux GstRtpSsrcDemux;
typedef struct _GstRtpSsrcDemuxClass GstRtpSsrcDemuxClass;
typedef struct _GstRtpSsrcDemuxPad GstRtpSsrcDemuxPad;

struct _GstRtpSsrcDemux
{
  GstElement parent;

  GstPad *rtp_sink;
  GstPad *rtcp_sink;

  GRecMutex padlock;
  GSList *srcpads;
};

struct _GstRtpSsrcDemuxClass
{
  GstElementClass parent_class;

  /* signals */
  void (*new_ssrc_pad)     (GstRtpSsrcDemux *demux, guint32 ssrc, GstPad *pad);
  void (*removed_ssrc_pad) (GstRtpSsrcDemux *demux, guint32 ssrc, GstPad *pad);

  /* actions */
  void (*clear_ssrc)       (GstRtpSsrcDemux *demux, guint32 ssrc);
};

GType gst_rtp_ssrc_demux_get_type (void);

#endif /* __GST_RTP_SSRC_DEMUX_H__ */
