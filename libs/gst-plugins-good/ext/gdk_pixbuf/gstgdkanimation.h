/*
 * Copyright (C) 2003 Benjamin Otte <in7y118@public.uni-hamburg.de>
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

#ifndef __GST_LOADER_H__
#define __GST_LOADER_H__

#include <gst/gst.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-animation.h>
#include <gdk-pixbuf/gdk-pixbuf-io.h>
#include <stdio.h>

G_BEGIN_DECLS

/* how many bytes we need to have available before we dare to start a new iteration */
#define GST_GDK_BUFFER_SIZE (102400)
/* how far behind we need to be before we attempt to seek */
#define GST_GDK_MAX_DELAY_TO_SEEK (GST_SECOND / 4)


#define GST_TYPE_GDK_ANIMATION                  (gst_gdk_animation_get_type())
#define GST_GDK_ANIMATION(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GDK_ANIMATION,GstGdkAnimation))
#define GST_GDK_ANIMATION_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GDK_ANIMATION,GstGdkAnimationClass))
#define GST_IS_GDK_ANIMATION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GDK_ANIMATION))
#define GST_IS_GDK_ANIMATION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GDK_ANIMATION))

typedef struct _GstGdkAnimation      GstGdkAnimation;
typedef struct _GstGdkAnimationClass GstGdkAnimationClass;

typedef struct _GstGdkAnimationIter GstGdkAnimationIter;
typedef struct _GstGdkAnimationIterClass GstGdkAnimationIterClass;

struct _GstGdkAnimation
{
  GdkPixbufAnimation            parent;

  /* name of temporary buffer file */
  gchar *                       temp_location;
  /* file descriptor to temporary file or 0 if we're done writing */
  int                           temp_fd;

  /* size of image */
  gint                          width;
  gint                          height;
  gint                          bpp;
  /* static image we use */
  GdkPixbuf *                   pixbuf;
};

struct _GstGdkAnimationClass 
{
  GdkPixbufAnimationClass       parent_class;
};

GType                   gst_gdk_animation_get_type      (void);

GstGdkAnimation *       gst_gdk_animation_new           (GError **error);

gboolean                gst_gdk_animation_add_data      (GstGdkAnimation *      ani,
                                                         const guint8 *         data,
                                                         guint                  size);
void                    gst_gdk_animation_done_adding   (GstGdkAnimation *      ani);


#define GST_TYPE_GDK_ANIMATION_ITER             (gst_gdk_animation_iter_get_type ())
#define GST_GDK_ANIMATION_ITER(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_GDK_ANIMATION_ITER, GstGdkAnimationIter))
#define GST_IS_GDK_ANIMATION_ITER(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_GDK_ANIMATION_ITER))

#define GST_GDK_ANIMATION_ITER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GDK_ANIMATION_ITER, GstGdkAnimationIterClass))
#define GST_IS_GDK_ANIMATION_ITER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GDK_ANIMATION_ITER))
#define GST_GDK_ANIMATION_ITER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GDK_ANIMATION_ITER, GstGdkAnimationIterClass))

struct _GstGdkAnimationIter {
  GdkPixbufAnimationIter        parent;
        
  /* our animation */
  GstGdkAnimation *             ani;
  /* start timeval */
  GTimeVal                      start;
  /* timestamp of last buffer */
  GstClockTime                  last_timestamp;
  
  /* pipeline we're using */
  GstElement *                  pipeline;
  gboolean                      eos;
  gboolean                      just_seeked;
  
  /* current image and the buffers containing the data */
  GdkPixbuf *                   pixbuf;
  GQueue *                      buffers;
};

struct _GstGdkAnimationIterClass {
  GdkPixbufAnimationIterClass   parent_class;
};

GType gst_gdk_animation_iter_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GST_GDK_ANIMATION_H__ */
