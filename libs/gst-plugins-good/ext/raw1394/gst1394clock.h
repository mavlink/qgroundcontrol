/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2005 Wim Taymans <wim@fluendo.com>
 * Copyright (C) 2009      David Schleef <ds@schleef.org>
 *
 * gst1394clock.h: Clock for use by the IEEE 1394
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

#ifndef __GST_1394_CLOCK_H__
#define __GST_1394_CLOCK_H__

#include <gst/gst.h>
#include <gst/gstsystemclock.h>

#include <libraw1394/raw1394.h>

G_BEGIN_DECLS

#define GST_TYPE_1394_CLOCK \
  (gst_1394_clock_get_type())
#define GST_1394_CLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_1394_CLOCK,Gst1394Clock))
#define GST_1394_CLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_1394_CLOCK,Gst1394ClockClass))
#define GST_IS_1394_CLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_1394_CLOCK))
#define GST_IS_1394_CLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_1394_CLOCK))
#define GST_1394_CLOCK_CAST(obj) \
  ((Gst1394Clock*)(obj))

typedef struct _Gst1394Clock Gst1394Clock;
typedef struct _Gst1394ClockClass Gst1394ClockClass;

/**
 * Gst1394Clock:
 * @clock: parent #GstSystemClock
 *
 * Opaque #Gst1394Clock.
 */
struct _Gst1394Clock {
  GstSystemClock clock;

  raw1394handle_t handle;

  guint32 cycle_timer_lo;
  guint32 cycle_timer_hi;
};

struct _Gst1394ClockClass {
  GstSystemClockClass parent_class;
};

GType           gst_1394_clock_get_type        (void);
Gst1394Clock*   gst_1394_clock_new             (const gchar *name);
void            gst_1394_clock_set_handle      (Gst1394Clock *clock,
    raw1394handle_t handle);
void            gst_1394_clock_unset_handle      (Gst1394Clock *clock);

G_END_DECLS

#endif /* __GST_1394_CLOCK_H__ */
