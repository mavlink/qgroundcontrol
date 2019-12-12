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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <gst/video/video.h>
#include <gst/gl/gstglfuncs.h>
#include "qtwindow.h"
#include "gstqsgtexture.h"
#include "gstqtglutility.h"

#include <QtCore/QDateTime>
#include <QtCore/QRunnable>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickWindow>
#include <QOpenGLFramebufferObject>

/* compatibility definitions... */
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif

/**
 * SECTION:
 *
 * #QtGLWindow is an #QQuickWindow that grab QtQuick view to GStreamer OpenGL video buffers.
 */

GST_DEBUG_CATEGORY_STATIC (qt_window_debug);
#define GST_CAT_DEFAULT qt_window_debug

struct _QtGLWindowPrivate
{
  GMutex lock;
  GCond update_cond;

  GstBuffer *buffer;
  GstCaps *caps;
  GstVideoInfo v_info;

  gboolean initted;
  gboolean updated;
  gboolean quit;
  gboolean result;
  gboolean useDefaultFbo;

  GstGLDisplay *display;
  GstGLContext *other_context;

  GLuint fbo;

  /* frames that qmlview rendered in its gl thread */
  quint64 frames_rendered;
  quint64 start;
  quint64 stop;
};

class InitQtGLContext : public QRunnable
{
public:
  InitQtGLContext(QtGLWindow *window);
  void run();

private:
  QtGLWindow *window_;
};

InitQtGLContext::InitQtGLContext(QtGLWindow *window) :
  window_(window)
{
}

void InitQtGLContext::run()
{
  window_->onSceneGraphInitialized();
}

QtGLWindow::QtGLWindow ( QWindow * parent, QQuickWindow *src ) :
  QQuickWindow( parent ), source (src)
{
  QGuiApplication *app = static_cast<QGuiApplication *> (QCoreApplication::instance ());
  static volatile gsize _debug;

  g_assert (app != NULL);

  if (g_once_init_enter (&_debug)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "qtglwindow", 0, "Qt GL QuickWindow");
    g_once_init_leave (&_debug, 1);
  }

  this->priv = g_new0 (QtGLWindowPrivate, 1);

  g_mutex_init (&this->priv->lock);
  g_cond_init (&this->priv->update_cond);

  this->priv->display = gst_qt_get_gl_display();

  connect (source, SIGNAL(beforeRendering()), this, SLOT(beforeRendering()), Qt::DirectConnection);
  connect (source, SIGNAL(afterRendering()), this, SLOT(afterRendering()), Qt::DirectConnection);
  connect (app, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()), Qt::DirectConnection);
  if (source->isSceneGraphInitialized())
    source->scheduleRenderJob(new InitQtGLContext(this), QQuickWindow::BeforeSynchronizingStage);
  else
    connect (source, SIGNAL(sceneGraphInitialized()), this, SLOT(onSceneGraphInitialized()), Qt::DirectConnection);

  connect (source, SIGNAL(sceneGraphInvalidated()), this, SLOT(onSceneGraphInvalidated()), Qt::DirectConnection);

  GST_DEBUG ("%p init Qt Window", this->priv->display);
}

QtGLWindow::~QtGLWindow()
{
  GST_DEBUG ("deinit Qt Window");
  g_mutex_clear (&this->priv->lock);
  g_cond_clear (&this->priv->update_cond);
  if (this->priv->other_context)
    gst_object_unref(this->priv->other_context);
  if (this->priv->display)
    gst_object_unref(this->priv->display);
  g_free (this->priv);
  this->priv = NULL;
}

void
QtGLWindow::beforeRendering()
{
  unsigned int width, height;

  g_mutex_lock (&this->priv->lock);

  static volatile gsize once = 0;
  if (g_once_init_enter(&once)) {
    this->priv->start = QDateTime::currentDateTime().toMSecsSinceEpoch();
    g_once_init_leave(&once,1);
  }

  if (!fbo && !this->priv->useDefaultFbo) {

    width = source->width();
    height = source->height();

    GST_DEBUG ("create new framebuffer object %dX%d", width, height);

    fbo.reset(new QOpenGLFramebufferObject (width, height,
          QOpenGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGBA));

    source->setRenderTarget(fbo.data());
  } else if (this->priv->useDefaultFbo) {
    GST_DEBUG ("use default fbo for render target");
    fbo.reset(NULL);
    source->setRenderTarget(NULL);
  }

  g_mutex_unlock (&this->priv->lock);
}


void
QtGLWindow::afterRendering()
{
  GstVideoFrame gl_frame;
  GstVideoInfo *info;
  GstGLContext *context;
  gboolean ret;
  guint width, height;
  const GstGLFuncs *gl;
  GLuint dst_tex;

  g_mutex_lock (&this->priv->lock);

  this->priv->frames_rendered++;

  if(!this->priv->buffer || this->priv->updated == TRUE) {
    GST_DEBUG ("skip this frame");
    g_mutex_unlock (&this->priv->lock);
    return;
  }

  GST_DEBUG ("copy buffer %p",this->priv->buffer);

  width = GST_VIDEO_INFO_WIDTH (&this->priv->v_info);
  height = GST_VIDEO_INFO_HEIGHT (&this->priv->v_info);
  info = &this->priv->v_info;
  context = this->priv->other_context;

  gst_gl_context_activate (context, TRUE);
  gl = context->gl_vtable;

  ret = gst_video_frame_map (&gl_frame, info, this->priv->buffer,
      (GstMapFlags) (GST_MAP_WRITE | GST_MAP_GL));

  if (!ret) {
    this->priv->buffer = NULL;
    GST_ERROR ("Failed to map video frame");
    goto errors;
  }

  gl->BindFramebuffer (GL_READ_FRAMEBUFFER, this->source->renderTargetId());

  ret = gst_gl_context_check_framebuffer_status (context, GL_READ_FRAMEBUFFER);
  if (!ret) {
    GST_ERROR ("FBO errors");
    goto errors;
  }

  dst_tex = *(guint *) gl_frame.data[0];
  GST_DEBUG ("qml render target id %d, render to tex %d %dX%d", 
      this->source->renderTargetId(), dst_tex, width,height);

  gl->BindTexture (GL_TEXTURE_2D, dst_tex);
  if (gl->BlitFramebuffer) {
    gl->BindFramebuffer (GL_DRAW_FRAMEBUFFER, this->priv->fbo);
    gl->FramebufferTexture2D (GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
              GL_TEXTURE_2D, dst_tex, 0);

    ret = gst_gl_context_check_framebuffer_status (context, GL_DRAW_FRAMEBUFFER);
    if (!ret) {
      GST_ERROR ("FBO errors");
      goto errors;
    }
    if (this->priv->useDefaultFbo)
      gl->ReadBuffer (GL_BACK);
    else
      gl->ReadBuffer (GL_COLOR_ATTACHMENT0);
    gl->BlitFramebuffer (0, 0, width, height,
        0, 0, width, height,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
  } else {
    gl->CopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, width, height, 0);
  }

  GST_DEBUG ("rendering finished");

errors:
  gl->BindFramebuffer (GL_FRAMEBUFFER, 0);
  gst_video_frame_unmap (&gl_frame);

  gst_gl_context_activate (context, FALSE);

  this->priv->result = ret;
  this->priv->updated = TRUE;
  g_cond_signal (&this->priv->update_cond);
  g_mutex_unlock (&this->priv->lock);
}

void
QtGLWindow::aboutToQuit()
{
  g_mutex_lock (&this->priv->lock);

  this->priv->updated = TRUE;
  this->priv->quit = TRUE;
  g_cond_signal (&this->priv->update_cond);

  this->priv->stop = QDateTime::currentDateTime().toMSecsSinceEpoch();
  qint64 duration = this->priv->stop - this->priv->start;
  float fps = ((float)this->priv->frames_rendered / duration * 1000);

  GST_DEBUG("about to quit, total refresh frames (%lld) in (%0.3f) seconds, fps: %0.3f",
      this->priv->frames_rendered, (float)duration / 1000, fps);

  g_mutex_unlock (&this->priv->lock);
}

void
QtGLWindow::onSceneGraphInitialized()
{
  GST_DEBUG ("scene graph initialization with Qt GL context %p",
      this->source->openglContext ());

  this->priv->initted = gst_qt_get_gl_wrapcontext (this->priv->display,
      &this->priv->other_context, NULL);

  if (this->priv->initted && this->priv->other_context) {
    const GstGLFuncs *gl;

    gst_gl_context_activate (this->priv->other_context, TRUE);
    gl = this->priv->other_context->gl_vtable;

    gl->GenFramebuffers (1, &this->priv->fbo);

    gst_gl_context_activate (this->priv->other_context, FALSE);
  }

  GST_DEBUG ("%p created wrapped GL context %" GST_PTR_FORMAT, this,
      this->priv->other_context);
}

void
QtGLWindow::onSceneGraphInvalidated()
{
  GST_DEBUG ("scene graph invalidated");

  if (this->priv->fbo && this->priv->other_context) {
    const GstGLFuncs *gl;

    gst_gl_context_activate (this->priv->other_context, TRUE);
    gl = this->priv->other_context->gl_vtable;

    gl->DeleteFramebuffers (1, &this->priv->fbo);

    gst_gl_context_activate (this->priv->other_context, FALSE);
  }
}

bool
QtGLWindow::getGeometry(int * width, int * height)
{
  if (width == NULL || height == NULL)
    return FALSE;

  *width = this->source->width();
  *height = this->source->height();

  return TRUE;
}

GstGLContext *
qt_window_get_qt_context (QtGLWindow * qt_window)
{
  g_return_val_if_fail (qt_window != NULL, NULL);

  if (!qt_window->priv->other_context)
    return NULL;

  return (GstGLContext *) gst_object_ref (qt_window->priv->other_context);
}

GstGLDisplay *
qt_window_get_display (QtGLWindow * qt_window)
{
  g_return_val_if_fail (qt_window != NULL, NULL);

  if (!qt_window->priv->display)
    return NULL;

  return (GstGLDisplay *) gst_object_ref (qt_window->priv->display);
}

gboolean
qt_window_is_scenegraph_initialized (QtGLWindow * qt_window)
{
  g_return_val_if_fail (qt_window != NULL, FALSE);

  return qt_window->priv->initted;
}

gboolean
qt_window_set_caps (QtGLWindow * qt_window, GstCaps * caps)
{
  GstVideoInfo v_info;

  g_return_val_if_fail (qt_window != NULL, FALSE);
  g_return_val_if_fail (GST_IS_CAPS (caps), FALSE);
  g_return_val_if_fail (gst_caps_is_fixed (caps), FALSE);

  if (qt_window->priv->caps && gst_caps_is_equal_fixed (qt_window->priv->caps, caps))
    return TRUE;

  if (!gst_video_info_from_caps (&v_info, caps))
    return FALSE;

  g_mutex_lock (&qt_window->priv->lock);

  gst_caps_replace (&qt_window->priv->caps, caps);

  qt_window->priv->v_info = v_info;

  g_mutex_unlock (&qt_window->priv->lock);

  return TRUE;
}

gboolean
qt_window_set_buffer (QtGLWindow * qt_window, GstBuffer * buffer)
{
  g_return_val_if_fail (qt_window != NULL, FALSE);
  g_return_val_if_fail (qt_window->priv->initted, FALSE);
  gboolean ret;

  g_mutex_lock (&qt_window->priv->lock);

  if (qt_window->priv->quit){
    GST_DEBUG("about to quit, drop this buffer");
    g_mutex_unlock (&qt_window->priv->lock);
    return TRUE;
  }

  qt_window->priv->updated = FALSE;
  qt_window->priv->buffer = buffer;

  while (!qt_window->priv->updated) 
    g_cond_wait (&qt_window->priv->update_cond, &qt_window->priv->lock);
  
  ret = qt_window->priv->result;

  g_mutex_unlock (&qt_window->priv->lock);

  return ret;
}

void
qt_window_use_default_fbo (QtGLWindow * qt_window, gboolean useDefaultFbo)
{
  g_return_if_fail (qt_window != NULL);

  g_mutex_lock (&qt_window->priv->lock);

  GST_DEBUG ("set to use default fbo %d", useDefaultFbo);
  qt_window->priv->useDefaultFbo = useDefaultFbo;

  g_mutex_unlock (&qt_window->priv->lock);
}
