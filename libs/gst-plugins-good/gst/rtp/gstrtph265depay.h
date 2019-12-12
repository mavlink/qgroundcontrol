/* GStreamer
 * Copyright (C) <2006> Wim Taymans <wim.taymans@gmail.com>
 * Copyright (C) <2014> Jurgen Slowack <jurgenslowack@gmail.com>
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

#ifndef __GST_RTP_H265_DEPAY_H__
#define __GST_RTP_H265_DEPAY_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/rtp/gstrtpbasedepayload.h>
#include "gstrtph265types.h"

G_BEGIN_DECLS
#define GST_TYPE_RTP_H265_DEPAY \
  (gst_rtp_h265_depay_get_type())
#define GST_RTP_H265_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_H265_DEPAY,GstRtpH265Depay))
#define GST_RTP_H265_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_H265_DEPAY,GstRtpH265DepayClass))
#define GST_IS_RTP_H265_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_H265_DEPAY))
#define GST_IS_RTP_H265_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_H265_DEPAY))
typedef struct _GstRtpH265Depay GstRtpH265Depay;
typedef struct _GstRtpH265DepayClass GstRtpH265DepayClass;

#define GST_H265_VPS_NUT 32
#define GST_H265_SPS_NUT 33
#define GST_H265_PPS_NUT 34

typedef enum
{
  GST_H265_STREAM_FORMAT_UNKNOWN,
  GST_H265_STREAM_FORMAT_BYTESTREAM,
  GST_H265_STREAM_FORMAT_HVC1,
  GST_H265_STREAM_FORMAT_HEV1
} GstH265StreamFormat;

struct _GstRtpH265Depay
{
  GstRTPBaseDepayload depayload;

  const gchar *stream_format;
  GstH265StreamFormat output_format;  /* bytestream, hvc1 or hev1 */
  gboolean byte_stream;

  GstBuffer *codec_data;
  GstAdapter *adapter;
  gboolean wait_start;

  /* nal merging */
  gboolean merge;
  GstAdapter *picture_adapter;
  gboolean picture_start;
  GstClockTime last_ts;
  gboolean last_keyframe;

  /* Work around broken payloaders wrt. Fragmentation Units */
  guint8 current_fu_type;
  GstClockTime fu_timestamp;
  gboolean fu_marker;

  /* misc */
  GPtrArray *vps;
  GPtrArray *sps;
  GPtrArray *pps;
  gboolean new_codec_data;

  /* downstream allocator */
  GstAllocator *allocator;
  GstAllocationParams params;
};

struct _GstRtpH265DepayClass
{
  GstRTPBaseDepayloadClass parent_class;
};

typedef struct
{
  GstElement *element;
  GstBuffer *outbuf;
  GQuark copy_tag;
} CopyMetaData;

typedef struct
{
  GstElement *element;
  GQuark keep_tag;
} DropMetaData;

GType gst_rtp_h265_depay_get_type (void);

gboolean gst_rtp_h265_depay_plugin_init (GstPlugin * plugin);

gboolean gst_rtp_h265_add_vps_sps_pps (GstElement * rtph265, GPtrArray * vps,
    GPtrArray * sps, GPtrArray * pps, GstBuffer * nal);

G_END_DECLS
#endif /* __GST_RTP_H265_DEPAY_H__ */
