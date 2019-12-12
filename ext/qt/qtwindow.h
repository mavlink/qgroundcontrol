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

#ifndef __QT_WINDOW_H__
#define __QT_WINDOW_H__

#include <gst/gst.h>
#include <gst/gl/gl.h>

#include "gstqtgl.h"
#include <QtQuick/QQuickWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

typedef struct _QtGLWindowPrivate QtGLWindowPrivate;

class InitQtGLContext;

class QtGLWindow : public QQuickWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    QtGLWindow (QWindow * parent = NULL, QQuickWindow *source = NULL);
    ~QtGLWindow ();
    bool getGeometry (int * width, int * height);

    /* private for C interface ... */
    QtGLWindowPrivate *priv;

private Q_SLOTS:
    void beforeRendering ();
    void afterRendering ();
    void onSceneGraphInitialized ();
    void onSceneGraphInvalidated ();
    void aboutToQuit();

private:
    friend class InitQtGLContext;
    QQuickWindow * source;
    QScopedPointer<QOpenGLFramebufferObject> fbo;
};

extern "C"
{
gboolean        qt_window_set_buffer (QtGLWindow * qt_window, GstBuffer * buffer);
gboolean        qt_window_set_caps (QtGLWindow * qt_window, GstCaps * caps);
GstGLContext *  qt_window_get_qt_context (QtGLWindow * qt_window);
GstGLDisplay *  qt_window_get_display (QtGLWindow * qt_window);
gboolean        qt_window_is_scenegraph_initialized (QtGLWindow * qt_window);
void            qt_window_use_default_fbo (QtGLWindow * qt_window, gboolean useDefaultFbo);
}

#endif /* __QT_WINDOW_H__ */
