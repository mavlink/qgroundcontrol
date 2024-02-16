/*
 * GStreamer
 * Copyright (C) 2016 Freescale Semiconductor, Inc. All rights reserved.
 * Copyright (C) 2022 Matthew Waters <matthew@centricular.com>
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

#ifndef __QT6_GL_WINDOW_H__
#define __QT6_GL_WINDOW_H__

#include <gst/gst.h>
#include <gst/gl/gl.h>

#include "gstqt6gl.h"
#include <QtQuick/QQuickWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

typedef struct _Qt6GLWindowPrivate Qt6GLWindowPrivate;

class Qt6GLWindow : public QQuickWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    Qt6GLWindow (QWindow * parent = NULL, QQuickWindow *source = NULL);
    ~Qt6GLWindow ();
    bool getGeometry (int * width, int * height);

    /* private for C interface ... */
    Qt6GLWindowPrivate *priv;

private Q_SLOTS:
    void beforeRendering ();
    void afterRendering ();
    void onSceneGraphInitialized ();
    void onSceneGraphInvalidated ();

private:
    QQuickWindow * source;
};

extern "C"
{
GstBuffer *     qt6_gl_window_take_buffer (Qt6GLWindow * qt6_window, GstCaps ** updated_caps);
GstGLContext *  qt6_gl_window_get_qt_context (Qt6GLWindow * qt6_window);
GstGLContext *  qt6_gl_window_get_context (Qt6GLWindow * qt6_window);
gboolean        qt6_gl_window_set_context (Qt6GLWindow * qt6_window, GstGLContext * context);
GstGLDisplay *  qt6_gl_window_get_display (Qt6GLWindow * qt6_window);
gboolean        qt6_gl_window_is_scenegraph_initialized (Qt6GLWindow * qt6_window);
void            qt6_gl_window_use_default_fbo (Qt6GLWindow * qt6_window, gboolean useDefaultFbo);
void            qt6_gl_window_unlock(Qt6GLWindow* qt6_window);
void            qt6_gl_window_unlock_stop(Qt6GLWindow* qt6_window);
}

#endif /* __QT6_GL_WINDOW_H__ */
