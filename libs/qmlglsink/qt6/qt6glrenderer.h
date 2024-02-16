/*
 * GStreamer
 * Copyright (C) 2022 Matthew Waters <matthew@cenricular.com>
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

#ifndef __GST_QT6_GL_RENDER_H__
#define __GST_QT6_GL_RENDER_H__

#include <QThread>
#include <QMutex>

#include <gst/gl/gl.h>

QT_FORWARD_DECLARE_CLASS(QOpenGLContext)
QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)
QT_FORWARD_DECLARE_CLASS(QQuickRenderControl)
QT_FORWARD_DECLARE_CLASS(QQuickWindow)
QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QQmlComponent)
QT_FORWARD_DECLARE_CLASS(QQuickItem)
QT_FORWARD_DECLARE_CLASS(GstAnimationDriver)
QT_FORWARD_DECLARE_CLASS(GstBackingSurface)

class GstQt6QuickRenderer : public QObject
{
    Q_OBJECT

public:
    GstQt6QuickRenderer();
    ~GstQt6QuickRenderer();

    /* initialize the GStreamer/Qt integration.  On failure returns false
     * and fills @error.
     * Must be called with @context not wrapped and current in the current
     * thread  */
    bool init (GstGLContext * context, GError ** error);

    /* set the qml scene.  returns false and fills @error on failure */
    bool setQmlScene (const gchar * scene, GError ** error);

    void setSize(int w, int h);

    GstGLMemory *generateOutput(GstClockTime input_ns);

    /* cleanup any resources.  Any use of this object after calling this
     * function may result in undefined behaviour */
    void cleanup();

    /* retrieve the rootItem from the qml scene.  Only valid after
     * setQmlScene() has been successfully called */
    QQuickItem *rootItem() const;

private slots:
    void initializeQml();

private:
    void init();
    void ensureFbo();

    void updateSizes();

    static void render_gst_gl_c (GstGLContext * context, GstQt6QuickRenderer * self) { self->renderGstGL (); }
    void renderGstGL ();

    static void initialize_gst_gl_c (GstGLContext * context, GstQt6QuickRenderer * self) { self->initializeGstGL (); }
    void initializeGstGL ();

    static void stop_c (GstGLContext * context, GstQt6QuickRenderer * self) { self->stopGL (); }
    void stopGL ();
    static void stop_after_c (GstGLContext * context, GstQt6QuickRenderer * self) { self->stopAfterGL (); }
    void stopAfterGL ();

    static void activate_context_c (GstGLContext * context, GstQt6QuickRenderer * self) { self->activateContext (); }
    void activateContext ();

    static void deactivate_context_c (GstGLContext * context, GstQt6QuickRenderer * self) { self->deactivateContext (); }
    void deactivateContext ();

    GstGLContext *gl_context;
    QQuickWindow *m_quickWindow;
    QQuickRenderControl *m_renderControl;
    QQmlEngine *m_qmlEngine;
    QQmlComponent *m_qmlComponent;
    QQuickItem *m_rootItem;

    GstGLBaseMemoryAllocator *gl_allocator;
    GstGLAllocationParams *gl_params;
    GstVideoInfo v_info;
    GstGLMemory *gl_mem;

    QString m_errorString;
    struct SharedRenderData *m_sharedRenderData;
};

class CreateSurfaceWorker : public QObject
{
  Q_OBJECT

public:
  CreateSurfaceWorker (struct SharedRenderData * rdata);
  ~CreateSurfaceWorker ();

  bool event(QEvent *ev) override;

private:
  struct SharedRenderData *m_sharedRenderData;
};

#endif /* __GST_QT6_GL_RENDER_H__ */
