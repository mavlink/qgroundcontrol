/* RTP muxer element for GStreamer
 *
 * gstrtpdtmfmux.h:
 *
 * Copyright (C) <2007> Nokia Corporation.
 *   Contact: Zeeshan Ali <zeeshan.ali@nokia.com>
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *               2000,2005 Wim Taymans <wim@fluendo.com>
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

#ifndef __GST_RTP_DTMF_MUX_H__
#define __GST_RTP_DTMF_MUX_H__

#include <gst/gst.h>
#include "gstrtpmux.h"

G_BEGIN_DECLS
#define GST_TYPE_RTP_DTMF_MUX (gst_rtp_dtmf_mux_get_type())
#define GST_RTP_DTMF_MUX(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_DTMF_MUX, GstRTPDTMFMux))
#define GST_RTP_DTMF_MUX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_DTMF_MUX, GstRTPDTMFMux))
#define GST_IS_RTP_DTMF_MUX(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_DTMF_MUX))
#define GST_IS_RTP_DTMF_MUX_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_DTMF_MUX))
typedef struct _GstRTPDTMFMux GstRTPDTMFMux;
typedef struct _GstRTPDTMFMuxClass GstRTPDTMFMuxClass;

/**
 * GstRTPDTMFMux:
 *
 * The opaque #GstRTPDTMFMux structure.
 */
struct _GstRTPDTMFMux
{
  GstRTPMux mux;

  /* Protected by object lock */
  GstClockTime last_priority_end;
};

struct _GstRTPDTMFMuxClass
{
  GstRTPMuxClass parent_class;

  /* signals */
  void (*locking) (GstElement * element, GstPad * pad);
  void (*unlocked) (GstElement * element, GstPad * pad);
};

GType gst_rtp_dtmf_mux_get_type (void);
gboolean gst_rtp_dtmf_mux_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_RTP_DTMF_MUX_H__ */
