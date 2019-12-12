/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 * Copyright (C) 2009      David Schleef <ds@schleef.org>
 *
 * gst1394clock.c: Clock for use by IEEE 1394 plugins
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gst1394clock.h"

GST_DEBUG_CATEGORY_STATIC (gst_1394_clock_debug);
#define GST_CAT_DEFAULT gst_1394_clock_debug

static void gst_1394_clock_class_init (Gst1394ClockClass * klass);
static void gst_1394_clock_init (Gst1394Clock * clock);

static GstClockTime gst_1394_clock_get_internal_time (GstClock * clock);

static GstSystemClockClass *parent_class = NULL;

/* static guint gst_1394_clock_signals[LAST_SIGNAL] = { 0 }; */

GType
gst_1394_clock_get_type (void)
{
  static GType clock_type = 0;

  if (!clock_type) {
    static const GTypeInfo clock_info = {
      sizeof (Gst1394ClockClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_1394_clock_class_init,
      NULL,
      NULL,
      sizeof (Gst1394Clock),
      4,
      (GInstanceInitFunc) gst_1394_clock_init,
      NULL
    };

    clock_type = g_type_register_static (GST_TYPE_SYSTEM_CLOCK, "Gst1394Clock",
        &clock_info, 0);
  }
  return clock_type;
}


static void
gst_1394_clock_class_init (Gst1394ClockClass * klass)
{
  GstClockClass *gstclock_class;

  gstclock_class = (GstClockClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gstclock_class->get_internal_time = gst_1394_clock_get_internal_time;

  GST_DEBUG_CATEGORY_INIT (gst_1394_clock_debug, "1394clock", 0, "1394clock");
}

static void
gst_1394_clock_init (Gst1394Clock * clock)
{
  GST_OBJECT_FLAG_SET (clock, GST_CLOCK_FLAG_CAN_SET_MASTER);
}

/**
 * gst_1394_clock_new:
 * @name: the name of the clock
 *
 * Create a new #Gst1394Clock instance.
 *
 * Returns: a new #Gst1394Clock
 */
Gst1394Clock *
gst_1394_clock_new (const gchar * name)
{
  Gst1394Clock *_1394clock =
      GST_1394_CLOCK (g_object_new (GST_TYPE_1394_CLOCK, "name", name,
          "clock-type", GST_CLOCK_TYPE_OTHER, NULL));

  /* Clear floating flag */
  gst_object_ref_sink (_1394clock);

  return _1394clock;
}

static GstClockTime
gst_1394_clock_get_internal_time (GstClock * clock)
{
  Gst1394Clock *_1394clock;
  GstClockTime result;
  guint32 cycle_timer;
  guint64 local_time;

  _1394clock = GST_1394_CLOCK_CAST (clock);

  if (_1394clock->handle != NULL) {
    GST_OBJECT_LOCK (clock);
    raw1394_read_cycle_timer (_1394clock->handle, &cycle_timer, &local_time);

    if (cycle_timer < _1394clock->cycle_timer_lo) {
      GST_LOG_OBJECT (clock, "overflow %u to %u",
          _1394clock->cycle_timer_lo, cycle_timer);

      _1394clock->cycle_timer_hi++;
    }
    _1394clock->cycle_timer_lo = cycle_timer;

    /* get the seconds from the cycleSeconds counter */
    result = (((((guint64) _1394clock->cycle_timer_hi) << 32) |
            cycle_timer) >> 25) * GST_SECOND;
    /* add the microseconds from the cycleCount counter */
    result += (((cycle_timer >> 12) & 0x1fff) * 125) * GST_USECOND;

    GST_LOG_OBJECT (clock, "result %" GST_TIME_FORMAT, GST_TIME_ARGS (result));
    GST_OBJECT_UNLOCK (clock);
  } else {
    result = GST_CLOCK_TIME_NONE;
  }

  return result;
}

void
gst_1394_clock_set_handle (Gst1394Clock * clock, raw1394handle_t handle)
{
  clock->handle = handle;
  clock->cycle_timer_lo = 0;
  clock->cycle_timer_hi = 0;
}

void
gst_1394_clock_unset_handle (Gst1394Clock * clock)
{
  clock->handle = NULL;
}
