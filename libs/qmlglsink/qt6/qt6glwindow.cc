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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <gst/video/video.h>
#include <gst/gl/gstglfuncs.h>
#include "qt6glwindow.h"
#include "gstqt6glutility.h"

#include <QtCore/QDateTime>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickWindow>
#include <QQuickRenderTarget>

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
 * #Qt6GLWindow is an #QQuickWindow that grab QtQuick view to GStreamer OpenGL video buffers.
 */

GST_DEBUG_CATEGORY_STATIC (qt6_gl_window_debug);
#define GST_CAT_DEFAULT qt6_gl_window_debug

struct _Qt6GLWindowPrivate
{
  GMutex lock;
  GCond update_cond;

  GstBuffer *buffer;
  GstVideoInfo v_info;
  GstVideoFrame mapped_frame;
  GstGLBaseMemoryAllocator *gl_allocator;
  GstGLAllocationParams *gl_params;

  gboolean initted;
  gboolean updated;
  gboolean quit;
  gboolean result;
  gboolean useDefaultFbo;

  GstGLDisplay *display;
  GstGLContext *other_context;
  GstGLContext *context;

  guint fbo;

  gboolean new_caps;
  GstBuffer *produced_buffer;
};

Qt6GLWindow::Qt6GLWindow (QWindow * parent, QQuickWindow *src)
  : QQuickWindow( parent ), source (src)
{
  QGuiApplication *app = static_cast<QGuiApplication *> (QCoreApplication::instance ());
  static gsize _debug;

  g_assert (app != NULL);

  if (g_once_init_enter (&_debug)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "qt6glwindow", 0, "Qt6 GL QuickWindow");
    g_once_init_leave (&_debug, 1);
  }

  this->priv = g_new0 (Qt6GLWindowPrivate, 1);

  g_mutex_init (&this->priv->lock);
  g_cond_init (&this->priv->update_cond);

  this->priv->display = gst_qml6_get_gl_display(FALSE);
  this->priv->result = TRUE;

  connect (source, SIGNAL(beforeRendering()), this, SLOT(beforeRendering()), Qt::DirectConnection);
  connect (source, SIGNAL(afterRendering()), this, SLOT(afterRendering()), Qt::DirectConnection);
  if (source->isSceneGraphInitialized())
    source->scheduleRenderJob(new RenderJob(std::bind(&Qt6GLWindow::onSceneGraphInitialized, this)), QQuickWindow::BeforeSynchronizingStage);
  else
    connect (source, SIGNAL(sceneGraphInitialized()), this, SLOT(onSceneGraphInitialized()), Qt::DirectConnection);

  connect (source, SIGNAL(sceneGraphInvalidated()), this, SLOT(onSceneGraphInvalidated()), Qt::DirectConnection);

  GST_DEBUG ("%p init Qt Window", this->priv->display);
}

Qt6GLWindow::~Qt6GLWindow()
{
  GST_DEBUG ("deinit Qt Window");
  g_mutex_clear (&this->priv->lock);
  g_cond_clear (&this->priv->update_cond);
  gst_clear_object (&this->priv->other_context);
  gst_clear_buffer (&this->priv->buffer);
  gst_clear_buffer (&this->priv->produced_buffer);
  gst_clear_object (&this->priv->display);
  gst_clear_object (&this->priv->context);
  gst_clear_object (&this->priv->gl_allocator);

  if (this->priv->gl_params)
    gst_gl_allocation_params_free (this->priv->gl_params);
  this->priv->gl_params = NULL;

  g_free (this->priv);
  this->priv = NULL;
}

void
Qt6GLWindow::beforeRendering()
{
  g_mutex_lock (&this->priv->lock);

  if (!this->priv->context) {
    GST_LOG ("no GStreamer GL context set yet, skipping frame");
    g_mutex_unlock (&this->priv->lock);
    return;
  }

  QSize size = source->size();

  if (!this->priv->gl_allocator)
    this->priv->gl_allocator =
        (GstGLBaseMemoryAllocator *) gst_gl_memory_allocator_get_default (this->priv->context);

  if (GST_VIDEO_INFO_WIDTH (&this->priv->v_info) != size.width()
      || GST_VIDEO_INFO_HEIGHT (&this->priv->v_info) != size.height()) {
    this->priv->new_caps = TRUE;

    gst_video_info_set_format (&this->priv->v_info, GST_VIDEO_FORMAT_RGBA,
        size.width(), size.height());

    if (this->priv->gl_params) {
      GstGLVideoAllocationParams *gl_vid_params = (GstGLVideoAllocationParams *) this->priv->gl_params;
      if (GST_VIDEO_INFO_WIDTH (gl_vid_params->v_info) != source->width()
            || GST_VIDEO_INFO_HEIGHT (gl_vid_params->v_info) != source->height())
        this->priv->gl_params = NULL;
      gst_clear_buffer (&this->priv->buffer);
    }
  }

  if (!this->priv->gl_params) {
    this->priv->gl_params = (GstGLAllocationParams *)
        gst_gl_video_allocation_params_new (this->priv->context, NULL,
        &this->priv->v_info, 0, NULL, GST_GL_TEXTURE_TARGET_2D, GST_GL_RGBA);
  }

  if (!this->priv->buffer) {
    GstGLMemory *gl_mem =
        (GstGLMemory *) gst_gl_base_memory_alloc (this->priv->gl_allocator,
        this->priv->gl_params);
    this->priv->buffer = gst_buffer_new ();
    gst_buffer_append_memory (this->priv->buffer, (GstMemory *) gl_mem);
  }

  if (!gst_video_frame_map (&this->priv->mapped_frame, &this->priv->v_info,
        this->priv->buffer, (GstMapFlags) (GST_MAP_WRITE | GST_MAP_GL))) {
    GST_WARNING ("failed map video frame");
    gst_clear_buffer (&this->priv->buffer);
    return;
  }

  if (!this->priv->useDefaultFbo) {
    guint tex_id = *(guint *) this->priv->mapped_frame.data[0];

    source->setRenderTarget(QQuickRenderTarget::fromOpenGLTexture(tex_id, source->size()));
  } else if (this->priv->useDefaultFbo) {
    GST_DEBUG ("use default fbo for render target");
    source->setRenderTarget(QQuickRenderTarget());
  }

  g_mutex_unlock (&this->priv->lock);
}

void
Qt6GLWindow::afterRendering()
{
  gboolean ret;
  guint width, height;
  const GstGLFuncs *gl;
  GstGLSyncMeta *sync_meta;

  g_mutex_lock (&this->priv->lock);

  if (!this->priv->buffer) {
    GST_LOG ("no buffer created in beforeRendering(), skipping");
    g_mutex_unlock (&this->priv->lock);
    return;
  }

  width = GST_VIDEO_INFO_WIDTH (&this->priv->v_info);
  height = GST_VIDEO_INFO_HEIGHT (&this->priv->v_info);

  gst_gl_context_activate (this->priv->other_context, TRUE);
  gl = this->priv->other_context->gl_vtable;

  if (!this->priv->useDefaultFbo) {
      gst_video_frame_unmap (&this->priv->mapped_frame);
    ret = TRUE;
  } else {
    gl->BindFramebuffer (GL_READ_FRAMEBUFFER, 0);

    ret = gst_gl_context_check_framebuffer_status (this->priv->other_context, GL_READ_FRAMEBUFFER);
    if (!ret) {
      GST_ERROR ("FBO errors");
      goto errors;
    }

    guint dst_tex = *(guint *) this->priv->mapped_frame.data[0];
    gl->BindTexture (GL_TEXTURE_2D, dst_tex);
    if (gl->BlitFramebuffer) {
      gl->BindFramebuffer (GL_DRAW_FRAMEBUFFER, this->priv->fbo);
      gl->FramebufferTexture2D (GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, dst_tex, 0);

      ret = gst_gl_context_check_framebuffer_status (this->priv->other_context, GL_DRAW_FRAMEBUFFER);
      if (!ret) {
        GST_ERROR ("FBO errors");
        goto errors;
      }
      gl->ReadBuffer (GL_BACK);
      gl->BlitFramebuffer (0, 0, width, height,
          0, 0, width, height,
          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    } else {
      gl->CopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, width, height, 0);
    }
  }

  gst_video_frame_unmap (&this->priv->mapped_frame);
  gl->BindFramebuffer (GL_FRAMEBUFFER, 0);

  if (this->priv->context) {
    sync_meta = gst_buffer_get_gl_sync_meta (this->priv->buffer);
    if (!sync_meta) {
      sync_meta = gst_buffer_add_gl_sync_meta (this->priv->context, this->priv->buffer);
    }
    gst_gl_sync_meta_set_sync_point (sync_meta, this->priv->other_context);
  }

  GST_DEBUG ("rendering finished");

done:
  gst_gl_context_activate (this->priv->other_context, FALSE);

  this->priv->result = ret;
  this->priv->produced_buffer = this->priv->buffer;
  this->priv->buffer = NULL;
  this->priv->updated = TRUE;
  g_cond_signal (&this->priv->update_cond);
  g_mutex_unlock (&this->priv->lock);
  return;

errors:
  gl->BindFramebuffer (GL_FRAMEBUFFER, 0);
  gst_video_frame_unmap (&this->priv->mapped_frame);
  goto done;
}

void
Qt6GLWindow::onSceneGraphInitialized()
{
  QSGRendererInterface *renderer = source->rendererInterface();
  if (!renderer)
    return;

  if (renderer->graphicsApi() != QSGRendererInterface::GraphicsApi::OpenGL) {
    GST_WARNING ("%p scene graph initialized with a non-OpenGL renderer interface", this);
    return;
  }

  this->priv->initted = gst_qml6_get_gl_wrapcontext (this->priv->display,
      &this->priv->other_context, &this->priv->context);

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
Qt6GLWindow::onSceneGraphInvalidated()
{
  GST_DEBUG ("scene graph invalidated");

  if (this->priv->fbo && this->priv->other_context) {
    const GstGLFuncs *gl;

    gst_gl_context_activate (this->priv->other_context, TRUE);
    gl = this->priv->other_context->gl_vtable;

    gl->DeleteFramebuffers (1, &this->priv->fbo);

    gst_gl_context_activate (this->priv->other_context, FALSE);
  }

  gst_clear_buffer (&this->priv->buffer);
  gst_clear_buffer (&this->priv->produced_buffer);
}

bool
Qt6GLWindow::getGeometry(int * width, int * height)
{
  if (width == NULL || height == NULL)
    return FALSE;

  *width = this->source->width();
  *height = this->source->height();

  return TRUE;
}

GstGLContext *
qt6_gl_window_get_qt_context (Qt6GLWindow * qt6_gl_window)
{
  g_return_val_if_fail (qt6_gl_window != NULL, NULL);

  if (!qt6_gl_window->priv->other_context)
    return NULL;

  return (GstGLContext *) gst_object_ref (qt6_gl_window->priv->other_context);
}

GstGLDisplay *
qt6_gl_window_get_display (Qt6GLWindow * qt6_gl_window)
{
  g_return_val_if_fail (qt6_gl_window != NULL, NULL);

  if (!qt6_gl_window->priv->display)
    return NULL;

  return (GstGLDisplay *) gst_object_ref (qt6_gl_window->priv->display);
}

GstGLContext *
qt6_gl_window_get_context (Qt6GLWindow * qt6_gl_window)
{
  g_return_val_if_fail (qt6_gl_window != NULL, NULL);

  if (!qt6_gl_window->priv->context)
    return NULL;

  return (GstGLContext *) gst_object_ref (qt6_gl_window->priv->context);
}

gboolean
qt6_gl_window_set_context (Qt6GLWindow * qt6_gl_window, GstGLContext * context)
{
  g_return_val_if_fail (qt6_gl_window != NULL, FALSE);

  if (qt6_gl_window->priv->context && qt6_gl_window->priv->context != context)
    return FALSE;

  gst_object_replace ((GstObject **) &qt6_gl_window->priv->context, (GstObject *) context);

  return TRUE;
}

gboolean
qt6_gl_window_is_scenegraph_initialized (Qt6GLWindow * qt6_gl_window)
{
  g_return_val_if_fail (qt6_gl_window != NULL, FALSE);

  return qt6_gl_window->priv->initted;
}

GstBuffer *
qt6_gl_window_take_buffer (Qt6GLWindow * qt6_gl_window, GstCaps ** updated_caps)
{
  g_return_val_if_fail (qt6_gl_window != NULL, FALSE);
  g_return_val_if_fail (qt6_gl_window->priv->initted, FALSE);
  GstBuffer *ret;

  g_mutex_lock (&qt6_gl_window->priv->lock);

  if (qt6_gl_window->priv->quit){
    GST_DEBUG("about to quit, drop this buffer");
    g_mutex_unlock (&qt6_gl_window->priv->lock);
    return NULL;
  }

  while (!qt6_gl_window->priv->produced_buffer && qt6_gl_window->priv->result)
    g_cond_wait (&qt6_gl_window->priv->update_cond, &qt6_gl_window->priv->lock);

  ret = qt6_gl_window->priv->produced_buffer;
  qt6_gl_window->priv->produced_buffer = NULL;

  if (qt6_gl_window->priv->new_caps) {
    *updated_caps = gst_video_info_to_caps (&qt6_gl_window->priv->v_info);
    gst_caps_set_features (*updated_caps, 0,
        gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_GL_MEMORY));
    qt6_gl_window->priv->new_caps = FALSE;
  }

  g_mutex_unlock (&qt6_gl_window->priv->lock);

  return ret;
}

void
qt6_gl_window_use_default_fbo (Qt6GLWindow * qt6_gl_window, gboolean useDefaultFbo)
{
  g_return_if_fail (qt6_gl_window != NULL);

  g_mutex_lock (&qt6_gl_window->priv->lock);

  GST_DEBUG ("set to use default fbo %d", useDefaultFbo);
  qt6_gl_window->priv->useDefaultFbo = useDefaultFbo;

  g_mutex_unlock (&qt6_gl_window->priv->lock);
}

void
qt6_gl_window_unlock(Qt6GLWindow* qt6_gl_window)
{
  g_mutex_lock(&qt6_gl_window->priv->lock);

  GST_DEBUG("unlock window");
  qt6_gl_window->priv->result = FALSE;
  g_cond_signal(&qt6_gl_window->priv->update_cond);

  g_mutex_unlock(&qt6_gl_window->priv->lock);
}

void
qt6_gl_window_unlock_stop(Qt6GLWindow* qt6_gl_window)
{
  g_mutex_lock(&qt6_gl_window->priv->lock);

  GST_DEBUG("unlock stop window");
  qt6_gl_window->priv->result = TRUE;
  g_cond_signal(&qt6_gl_window->priv->update_cond);

  g_mutex_unlock(&qt6_gl_window->priv->lock);
}
