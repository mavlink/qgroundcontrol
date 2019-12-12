/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_FLAC_ENC_H__
#define __GST_FLAC_ENC_H__

#include <gst/gst.h>
#include <gst/audio/gstaudioencoder.h>

#include <FLAC/all.h>

G_BEGIN_DECLS

#define GST_TYPE_FLAC_ENC (gst_flac_enc_get_type())
#define GST_FLAC_ENC(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_FLAC_ENC, GstFlacEnc)
#define GST_FLAC_ENC_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, GST_TYPE_FLAC_ENC, GstFlacEncClass)
#define GST_IS_FLAC_ENC(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, GST_TYPE_FLAC_ENC)
#define GST_IS_FLAC_ENC_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, GST_TYPE_FLAC_ENC)

typedef struct _GstFlacEnc GstFlacEnc;
typedef struct _GstFlacEncClass GstFlacEncClass;

struct _GstFlacEnc {
  GstAudioEncoder  element;

  /* < private > */

  GstFlowReturn  last_flow; /* save flow from last push so we can pass the
                             * correct flow return upstream in case the push
                             * fails for some reason */

  guint64        offset;
  gint           quality;
  gboolean       stopped;
  guint           padding;
  gint            seekpoints;

  FLAC__StreamEncoder *encoder;

  FLAC__StreamMetadata **meta;

  GstTagList *     tags;
  GstToc *         toc;

  guint64          samples_in;
  guint64          samples_out;
  gboolean         eos;
  /* queue headers until we have them all so we can add streamheaders to caps */
  gboolean         got_headers;
  GList           *headers;

  gint             channel_reorder_map[8];
};

struct _GstFlacEncClass {
  GstAudioEncoderClass parent_class;
};

GType gst_flac_enc_get_type(void);

G_END_DECLS

#endif /* __GST_FLAC_ENC_H__ */
