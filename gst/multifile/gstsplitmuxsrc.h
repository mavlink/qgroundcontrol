/* GStreamer Split Muxed File Source
 * Copyright (C) 2014 Jan Schmidt <jan@centricular.com>
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
#ifndef __GST_SPLITMUX_SRC_H__
#define __GST_SPLITMUX_SRC_H__

#include <gst/gst.h>

#include "gstsplitmuxpartreader.h"

G_BEGIN_DECLS

#define GST_TYPE_SPLITMUX_SRC \
  (gst_splitmux_src_get_type())
#define GST_SPLITMUX_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SPLITMUX_SRC,GstSplitMuxSrc))
#define GST_SPLITMUX_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SPLITMUX_SRC,GstSplitMuxSrcClass))
#define GST_IS_SPLITMUX_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SPLITMUX_SRC))
#define GST_IS_SPLITMUX_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SPLITMUX_SRC))

typedef struct _GstSplitMuxSrc GstSplitMuxSrc;
typedef struct _GstSplitMuxSrcClass GstSplitMuxSrcClass;

struct _GstSplitMuxSrc
{
  GstBin parent;

  GMutex lock;
  gboolean     running;

  gchar       *location;  /* OBJECT_LOCK */

  GstSplitMuxPartReader **parts;
  guint        num_parts;
  guint        num_prepared_parts;
  guint        num_created_parts;
  guint        cur_part;

  gboolean async_pending;
  gboolean pads_complete;

  GRWLock pads_rwlock;
  GList  *pads; /* pads_lock */
  guint n_pads;
  guint n_notlinked;

  GstClockTime total_duration;
  GstClockTime end_offset;
  GstSegment play_segment;
  guint32 segment_seqnum;
};

struct _GstSplitMuxSrcClass
{
  GstBinClass parent_class;
};

GType splitmux_src_pad_get_type (void);
#define SPLITMUX_TYPE_SRC_PAD splitmux_src_pad_get_type()
#define SPLITMUX_SRC_PAD_CAST(p) ((SplitMuxSrcPad *)(p))
#define SPLITMUX_SRC_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),SPLITMUX_TYPE_SRC_PAD,SplitMuxSrcPad))

struct _SplitMuxSrcPad
{
  GstPad parent;

  guint cur_part;
  GstSplitMuxPartReader *reader;
  GstPad *part_pad;

  GstSegment segment;

  gboolean set_next_discont;
  gboolean clear_next_discont;

  gboolean sent_stream_start;
  gboolean sent_caps;
  gboolean sent_segment;
};

struct _SplitMuxSrcPadClass
{
  GstPadClass parent;
};

GType gst_splitmux_src_get_type (void);
gboolean register_splitmuxsrc (GstPlugin * plugin);

#define SPLITMUX_SRC_LOCK(s) g_mutex_lock(&(s)->lock)
#define SPLITMUX_SRC_UNLOCK(s) g_mutex_unlock(&(s)->lock)

#define SPLITMUX_SRC_PADS_WLOCK(s) g_rw_lock_writer_lock(&(s)->pads_rwlock)
#define SPLITMUX_SRC_PADS_WUNLOCK(s) g_rw_lock_writer_unlock(&(s)->pads_rwlock)
#define SPLITMUX_SRC_PADS_RLOCK(s) g_rw_lock_reader_lock(&(s)->pads_rwlock)
#define SPLITMUX_SRC_PADS_RUNLOCK(s) g_rw_lock_reader_unlock(&(s)->pads_rwlock)

G_END_DECLS

#endif /* __GST_SPLITMUX_SRC_H__ */
