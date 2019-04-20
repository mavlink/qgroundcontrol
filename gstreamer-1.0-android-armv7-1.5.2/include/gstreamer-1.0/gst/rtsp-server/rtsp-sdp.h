/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
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

#include <gst/gst.h>
#include <gst/sdp/gstsdpmessage.h>

#include "rtsp-media.h"

#ifndef __GST_RTSP_SDP_H__
#define __GST_RTSP_SDP_H__

G_BEGIN_DECLS

typedef struct {
  gboolean is_ipv6;
  const gchar *server_ip;
} GstSDPInfo;

/* creating SDP */
gboolean            gst_rtsp_sdp_from_media      (GstSDPMessage *sdp, GstSDPInfo *info, GstRTSPMedia * media);

G_END_DECLS

#endif /* __GST_RTSP_SDP_H__ */
