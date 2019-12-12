/* GStreamer
 * Copyright (C) 2015 Sebastian Dröge <sebastian@centricular.com>
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

#ifndef __GST_RTP_UTILS_H__
#define __GST_RTP_UTILS_H__

#include <gst/gst.h>
#include <gst/base/gstbitreader.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
void gst_rtp_copy_meta (GstElement * element, GstBuffer *outbuf, GstBuffer *inbuf, GQuark copy_tag);

G_GNUC_INTERNAL
void gst_rtp_copy_audio_meta (gpointer element, GstBuffer *outbuf, GstBuffer *inbuf);

G_GNUC_INTERNAL
void gst_rtp_copy_video_meta (gpointer element, GstBuffer *outbuf, GstBuffer *inbuf);

G_GNUC_INTERNAL
void gst_rtp_drop_meta (GstElement * element, GstBuffer *buf, GQuark keep_tag);

G_GNUC_INTERNAL
void gst_rtp_drop_non_audio_meta (gpointer element, GstBuffer * buf);

G_GNUC_INTERNAL
void gst_rtp_drop_non_video_meta (gpointer element, GstBuffer * buf);

G_GNUC_INTERNAL
gboolean gst_rtp_read_golomb (GstBitReader * br, guint32 * value);

G_GNUC_INTERNAL extern GQuark rtp_quark_meta_tag_video;
G_GNUC_INTERNAL extern GQuark rtp_quark_meta_tag_audio;

G_END_DECLS

#endif /* __GST_RTP_UTILS_H__ */
