/* Copyright 2005 Jan Schmidt <thaytan@mad.scientist.com>
 *           2006 Michael Smith <msmith@fluendo.com>
 * Copyright (C) 2003-2004 Benjamin Otte <otte@gnome.org>
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

#ifndef __GST_ICYDEMUX_H__
#define __GST_ICYDEMUX_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/base/gsttypefindhelper.h>

G_BEGIN_DECLS

#define GST_TYPE_ICYDEMUX \
  (gst_icydemux_get_type())
#define GST_ICYDEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ICYDEMUX,GstICYDemux))
#define GST_ICYDEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ICYDEMUX,GstICYDemuxClass))
#define GST_IS_ICYDEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ICYDEMUX))
#define GST_IS_ICYDEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ICYDEMUX))

typedef struct _GstICYDemux      GstICYDemux;
typedef struct _GstICYDemuxClass GstICYDemuxClass;

struct _GstICYDemux
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  /* Interval between metadata updates */
  gint meta_interval;

  /* Remaining bytes until the next metadata update */
  gint remaining;

  /* When 'remaining' is zero, this holds the number of bytes of metadata we
   * still need to read, or zero if we don't yet know (which means we need to
   * read one byte, after which we can initialise this properly) */
  gint meta_remaining;

  /* Caps for the data enclosed */
  GstCaps *src_caps;

  /* True if we're still typefinding */
  gboolean typefinding;

  GstTagList *cached_tags;
  GList *cached_events;

  GstAdapter *meta_adapter;

  GstBuffer *typefind_buf;

  /* upstream HTTP Content-Type */
  gchar *content_type;
};

struct _GstICYDemuxClass 
{
  GstElementClass parent_class;
};

GType gst_icydemux_get_type (void);

G_END_DECLS

#endif /* __GST_ICYDEMUX_H__ */
