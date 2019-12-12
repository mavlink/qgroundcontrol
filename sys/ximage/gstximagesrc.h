/* screenshotsrc: Screenshot plugin for GStreamer
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

#ifndef __GST_XIMAGE_SRC_H__
#define __GST_XIMAGE_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include "ximageutil.h"

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif
#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif

G_BEGIN_DECLS

#define GST_TYPE_XIMAGE_SRC (gst_ximage_src_get_type())
#define GST_XIMAGE_SRC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_XIMAGE_SRC,GstXImageSrc))
#define GST_XIMAGE_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_XIMAGE_SRC,GstXImageSrc))
#define GST_IS_XIMAGE_SRC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_XIMAGE_SRC))
#define GST_IS_XIMAGE_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_XIMAGE_SRC))

typedef struct _GstXImageSrc GstXImageSrc;
typedef struct _GstXImageSrcClass GstXImageSrcClass;

GType gst_ximage_src_get_type (void) G_GNUC_CONST;

struct _GstXImageSrc
{
  GstPushSrc parent;

  /* Information on display */
  GstXContext *xcontext;
  gint x;
  gint y;
  gint width;
  gint height;

  Window xwindow;
  gchar *display_name;

  /* Window selection */
  guint64 xid;
  gchar *xname;

  /* Desired output framerate */
  gint fps_n;
  gint fps_d;

  /* for framerate sync */
  GstClockID clock_id;
  gint64 last_frame_no;

  /* Protect X Windows calls */
  GMutex  x_lock;

  /* Gathered pool of emitted buffers */
  GMutex  pool_lock;
  GSList *buffer_pool;

  /* XFixes and XDamage support */
  gboolean have_xfixes;
  gboolean have_xdamage;
  gboolean show_pointer;
  gboolean use_damage;

  /* co-ordinates for start and end */
  guint startx;
  guint starty;
  guint endx;
  guint endy;

  /* whether to use remote friendly calls */
  gboolean remote;

#ifdef HAVE_XFIXES
  int fixes_event_base;
  XFixesCursorImage *cursor_image;
#endif
#ifdef HAVE_XDAMAGE
  Damage damage;
  int damage_event_base;
  XserverRegion damage_region;
  GC damage_copy_gc;
  GstBuffer *last_ximage;
#endif
};

struct _GstXImageSrcClass
{
  GstPushSrcClass parent_class;
};

G_END_DECLS

#endif /* __GST_XIMAGE_SRC_H__ */
