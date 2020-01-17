/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2011> Tim-Philipp Müller <tim centricular net>
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


#ifndef __GST_FLAC_DEC_H__
#define __GST_FLAC_DEC_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiodecoder.h>

#include <FLAC/all.h>

G_BEGIN_DECLS

#define GST_TYPE_FLAC_DEC gst_flac_dec_get_type()
#define GST_FLAC_DEC(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_FLAC_DEC, GstFlacDec)
#define GST_FLAC_DEC_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, GST_TYPE_FLAC_DEC, GstFlacDecClass)
#define GST_IS_FLAC_DEC(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, GST_TYPE_FLAC_DEC)
#define GST_IS_FLAC_DEC_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, GST_TYPE_FLAC_DEC)

typedef struct _GstFlacDec GstFlacDec;
typedef struct _GstFlacDecClass GstFlacDecClass;

struct _GstFlacDec {
  GstAudioDecoder  audiodecoder;

  /*< private >*/
  FLAC__StreamDecoder         *decoder;
  GstAdapter                  *adapter;

  gboolean       got_headers; /* have we received all the header buffers yet? */

  GstFlowReturn  last_flow;   /* to marshal flow return from finis_frame to
                               * handle_frame via flac callbacks */

  GstAudioInfo   info;
  gint           channel_reorder_map[8];
  gint           depth;

  /* from the stream info, needed for scanning */
  guint16        min_blocksize;
  guint16        max_blocksize;

  gboolean       do_resync;
  gint           error_count;
};

struct _GstFlacDecClass {
  GstAudioDecoderClass  audiodecoder;
};

GType gst_flac_dec_get_type (void);

G_END_DECLS

#endif /* __GST_FLAC_DEC_H__ */
