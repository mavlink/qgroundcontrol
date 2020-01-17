/* GStreamer
 *
 * Copyright (c) 2008,2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 * Copyright (c) 2008-2017 Collabora Ltd
 *  @author: Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *  @author: Vincent Penquerc'h <vincent.penquerch@collabora.com>
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

#ifndef __GST_FLV_MUX_H__
#define __GST_FLV_MUX_H__

#include <gst/gst.h>
#include <gst/base/gstaggregator.h>

G_BEGIN_DECLS

#define GST_TYPE_FLV_MUX_PAD (gst_flv_mux_pad_get_type())
#define GST_FLV_MUX_PAD(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_FLV_MUX_PAD, GstFlvMuxPad))
#define GST_FLV_MUX_PAD_CAST(obj) ((GstFlvMuxPad *)(obj))
#define GST_FLV_MUX_PAD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_FLV_MUX_PAD, GstFlvMuxPad))
#define GST_IS_FLV_MUX_PAD(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_FLV_MUX_PAD))
#define GST_IS_FLV_MUX_PAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_FLV_MUX_PAD))

typedef struct _GstFlvMuxPad GstFlvMuxPad;
typedef struct _GstFlvMuxPadClass GstFlvMuxPadClass;
typedef struct _GstFlvMux GstFlvMux;
typedef struct _GstFlvMuxClass GstFlvMuxClass;

#define GST_TYPE_FLV_MUX \
  (gst_flv_mux_get_type ())
#define GST_FLV_MUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_FLV_MUX, GstFlvMux))
#define GST_FLV_MUX_CAST(obj) ((GstFlvMux *)obj)
#define GST_FLV_MUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_FLV_MUX, GstFlvMuxClass))
#define GST_IS_FLV_MUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_FLV_MUX))
#define GST_IS_FLV_MUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_FLV_MUX))

struct _GstFlvMuxPad
{
  GstAggregatorPad aggregator_pad;

  guint codec;
  guint rate;
  guint width;
  guint channels;
  GstBuffer *codec_data;

  guint bitrate;

  GstClockTime last_timestamp;
  gint64 pts;
  gint64 dts;

  gboolean info_changed;
};

struct _GstFlvMuxPadClass {
  GstAggregatorPadClass parent;
};

typedef enum
{
  GST_FLV_MUX_STATE_HEADER,
  GST_FLV_MUX_STATE_DATA
} GstFlvMuxState;

struct _GstFlvMux {
  GstAggregator   aggregator;

  GstPad         *srcpad;

  /* <private> */
  GstFlvMuxState state;
  GstFlvMuxPad *audio_pad;
  GstFlvMuxPad *video_pad;
  gboolean streamable;
  gchar *metadatacreator;
  gchar *encoder;

  GstTagList *tags;
  gboolean new_tags;
  GList *index;
  guint64 byte_count;
  guint64 duration;
  gint64 first_timestamp;
  GstClockTime last_dts;

  gboolean sent_header;
};

struct _GstFlvMuxClass {
  GstAggregatorClass parent;
};

GType    gst_flv_mux_pad_get_type(void);
GType    gst_flv_mux_get_type    (void);

G_END_DECLS

#endif /* __GST_FLV_MUX_H__ */
