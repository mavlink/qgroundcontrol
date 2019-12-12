/* GStreamer
 * Copyright (C) <2005> Luca Ognibene <luogni@tin.it>
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

#ifndef __GST_XIMAGEUTIL_H__
#define __GST_XIMAGEUTIL_H__

#include <gst/gst.h>

#ifdef HAVE_XSHM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* HAVE_XSHM */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_XSHM
#include <X11/extensions/XShm.h>
#endif /* HAVE_XSHM */

#include <string.h>
#include <math.h>

G_BEGIN_DECLS

typedef struct _GstXContext GstXContext;
typedef struct _GstXWindow GstXWindow;
typedef struct _GstXImage GstXImage;
typedef struct _GstMetaXImage GstMetaXImage;

/* Global X Context stuff */
/**
 * GstXContext:
 * @disp: the X11 Display of this context
 * @screen: the default Screen of Display @disp
 * @visual: the default Visual of Screen @screen
 * @root: the root Window of Display @disp
 * @white: the value of a white pixel on Screen @screen
 * @black: the value of a black pixel on Screen @screen
 * @depth: the color depth of Display @disp
 * @bpp: the number of bits per pixel on Display @disp
 * @endianness: the endianness of image bytes on Display @disp
 * @width: the width in pixels of Display @disp
 * @height: the height in pixels of Display @disp
 * @widthmm: the width in millimeters of Display @disp
 * @heightmm: the height in millimeters of Display @disp
 * @par_n: the pixel aspect ratio numerator calculated from @width, @widthmm
 * and @height,
 * @par_d: the pixel aspect ratio denumerator calculated from @width, @widthmm
 * and @height,
 * @heightmm ratio
 * @use_xshm: used to known wether of not XShm extension is usable or not even
 * if the Extension is present
 * @caps: the #GstCaps that Display @disp can accept
 *
 * Structure used to store various information collected/calculated for a
 * Display.
 */
struct _GstXContext {
  Display *disp;

  Screen *screen;

  Visual *visual;

  Window root;

  gulong white, black;

  gint depth;
  gint bpp;
  gint endianness;

  gint width, height;
  gint widthmm, heightmm;

  /* these are the output masks
   * for buffers from ximagesrc
   * and are in big endian */
  guint32 r_mask_output, g_mask_output, b_mask_output;

  guint par_n;                  /* calculated pixel aspect ratio numerator */
  guint par_d;                  /* calculated pixel aspect ratio denumerator */

  gboolean use_xshm;

  GstCaps *caps;
};

/**
 * GstXWindow:
 * @win: the Window ID of this X11 window
 * @width: the width in pixels of Window @win
 * @height: the height in pixels of Window @win
 * @internal: used to remember if Window @win was created internally or passed
 * through the #GstXOverlay interface
 * @gc: the Graphical Context of Window @win
 *
 * Structure used to store information about a Window.
 */
struct _GstXWindow {
  Window win;
  gint width, height;
  gboolean internal;
  GC gc;
};

gboolean ximageutil_check_xshm_calls (GstXContext * xcontext);

GstXContext *ximageutil_xcontext_get (GstElement *parent, 
    const gchar *display_name);
void ximageutil_xcontext_clear (GstXContext *xcontext);
void ximageutil_calculate_pixel_aspect_ratio (GstXContext * xcontext);

/* custom ximagesrc buffer, copied from ximagesink */

/* BufferReturnFunc is called when a buffer is finalised */
typedef gboolean (*BufferReturnFunc) (GstElement *parent, GstBuffer *buf);

/**
 * GstMetaXImage:
 * @parent: a reference to the element we belong to
 * @ximage: the XImage of this buffer
 * @width: the width in pixels of XImage @ximage
 * @height: the height in pixels of XImage @ximage
 * @size: the size in bytes of XImage @ximage
 *
 * Extra data attached to buffers containing additional information about an XImage.
 */
struct _GstMetaXImage {
  GstMeta meta;

  /* Reference to the ximagesrc we belong to */
  GstElement *parent;

  XImage *ximage;

#ifdef HAVE_XSHM
  XShmSegmentInfo SHMInfo;
#endif /* HAVE_XSHM */

  gint width, height;
  size_t size;

  BufferReturnFunc return_func;
};

GType gst_meta_ximage_api_get_type (void);
const GstMetaInfo * gst_meta_ximage_get_info (void);
#define GST_META_XIMAGE_GET(buf) ((GstMetaXImage *)gst_buffer_get_meta(buf,gst_meta_ximage_api_get_type()))
#define GST_META_XIMAGE_ADD(buf) ((GstMetaXImage *)gst_buffer_add_meta(buf,gst_meta_ximage_get_info(),NULL))

GstBuffer *gst_ximageutil_ximage_new (GstXContext *xcontext,
  GstElement *parent, int width, int height, BufferReturnFunc return_func);

void gst_ximageutil_ximage_destroy (GstXContext *xcontext, 
  GstBuffer * ximage);
  
/* Call to manually release a buffer */
void gst_ximage_buffer_free (GstBuffer *ximage);

G_END_DECLS 

#endif /* __GST_XIMAGEUTIL_H__ */
