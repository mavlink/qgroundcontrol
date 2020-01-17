/*
 * GStreamer
 * Copyright (C) 2016 Freescale Semiconductor, Inc. All rights reserved.
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

#ifndef __QT_GL_UTILS_H__
#define __QT_GL_UTILS_H__

#include <gst/gst.h>
#include <gst/gl/gl.h>

G_BEGIN_DECLS

GstGLDisplay * gst_qt_get_gl_display ();
gboolean       gst_qt_get_gl_wrapcontext (GstGLDisplay * display,
    GstGLContext **wrap_glcontext, GstGLContext **context, void * wgl_device);

G_END_DECLS
#endif /* __QT_GL_UTILS_H__ */
