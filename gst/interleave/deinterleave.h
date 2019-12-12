/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2005 Wim Taymans <wim@fluendo.com>
 *                    2007 Andy Wingo <wingo at pobox.com>
 *                    2008 Sebastian Dröge <slomo@circular-chaos.org>
 *
 * deinterleave.c: deinterleave samples
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

#ifndef __DEINTERLEAVE_H__
#define __DEINTERLEAVE_H__

G_BEGIN_DECLS

#include <gst/gst.h>
#include <gst/audio/audio.h>

#define GST_TYPE_DEINTERLEAVE            (gst_deinterleave_get_type())
#define GST_DEINTERLEAVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DEINTERLEAVE,GstDeinterleave))
#define GST_DEINTERLEAVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DEINTERLEAVE,GstDeinterleaveClass))
#define GST_DEINTERLEAVE_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_DEINTERLEAVE,GstDeinterleaveClass))
#define GST_IS_DEINTERLEAVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DEINTERLEAVE))
#define GST_IS_DEINTERLEAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DEINTERLEAVE))

typedef struct _GstDeinterleave GstDeinterleave;
typedef struct _GstDeinterleaveClass GstDeinterleaveClass;

typedef void (*GstDeinterleaveFunc) (gpointer out, gpointer in, guint stride, guint nframes);

struct _GstDeinterleave
{
  GstElement element;

  /*< private > */
  GList *srcpads;
  GstCaps *sinkcaps;
  GstAudioInfo audio_info;
  gboolean keep_positions;

  GstPad *sink;

  GstDeinterleaveFunc func;

  GList *pending_events;
};

struct _GstDeinterleaveClass
{
  GstElementClass parent_class;
};

GType gst_deinterleave_get_type (void);

G_END_DECLS

#endif /* __DEINTERLEAVE_H__ */
