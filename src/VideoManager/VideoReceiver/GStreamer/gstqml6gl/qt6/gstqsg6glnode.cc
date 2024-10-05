/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/*
 * GStreamer
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

#include "gstqsg6glnode.h"

#include <QtQuick/QSGTextureProvider>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTexture>

#define GST_CAT_DEFAULT gst_qsg_texture_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

GstQSG6OpenGLNode::GstQSG6OpenGLNode(QQuickItem * item)
{
  static gsize _debug;

  if (g_once_init_enter (&_debug)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "qtqsgtexture", 0,
        "Qt Scenegraph Texture");
    g_once_init_leave (&_debug, 1);
  }

  gst_video_info_init (&this->v_info);

  this->buffer_ = NULL;
  this->sync_buffer_ = gst_buffer_new ();
  this->dummy_tex_ = nullptr;
  // TODO; handle windowChanged?
  this->window_ = item->window();
}

GstQSG6OpenGLNode::~GstQSG6OpenGLNode()
{
  gst_buffer_replace (&this->buffer_, NULL);
  gst_buffer_replace (&this->sync_buffer_, NULL);
  this->buffer_was_bound = FALSE;
  delete this->dummy_tex_;
  this->dummy_tex_ = nullptr;
}

QSGTexture *
GstQSG6OpenGLNode::texture() const
{
  return QSGSimpleTextureNode::texture();
}

/* only called from the streaming thread with scene graph thread blocked */
void
GstQSG6OpenGLNode::setCaps (GstCaps * caps)
{
  GST_LOG ("%p setCaps %" GST_PTR_FORMAT, this, caps);

  if (caps)
    gst_video_info_from_caps (&this->v_info, caps);
  else
    gst_video_info_init (&this->v_info);
}

/* only called from the streaming thread with scene graph thread blocked */
GstBuffer *
GstQSG6OpenGLNode::getBuffer ()
{
  GstBuffer *buffer = NULL;

  if (this->buffer_)
    buffer = gst_buffer_ref (this->buffer_);

  return buffer;
}

/* only called from the streaming thread with scene graph thread blocked */
void
GstQSG6OpenGLNode::setBuffer (GstBuffer * buffer)
{
  GstGLContext *qt_context = NULL;
  gboolean buffer_changed;

  GST_LOG ("%p setBuffer %" GST_PTR_FORMAT, this, buffer);
  /* FIXME: update more state here */
  buffer_changed = gst_buffer_replace (&this->buffer_, buffer);

  if (buffer_changed) {
    GstGLContext *context;
    GstGLSyncMeta *sync_meta;
    GstMemory *mem;
    guint tex_id;
    QQuickWindow::CreateTextureOptions options = QQuickWindow::TextureHasAlphaChannel;
    QSGTexture *texture = nullptr;
    QSize texSize;

    qt_context = gst_gl_context_get_current();
    if (!qt_context)
      goto use_dummy_tex;

    if (!this->buffer_)
      goto use_dummy_tex;
    if (GST_VIDEO_INFO_FORMAT (&this->v_info) == GST_VIDEO_FORMAT_UNKNOWN)
      goto use_dummy_tex;

    this->mem_ = gst_buffer_peek_memory (this->buffer_, 0);
    if (!this->mem_)
      goto use_dummy_tex;

    /* FIXME: should really lock the memory to prevent write access */
    if (!gst_video_frame_map (&this->v_frame, &this->v_info, this->buffer_,
          (GstMapFlags) (GST_MAP_READ | GST_MAP_GL))) {
      // TODO(zdanek) - this happens when format of the video changes
      //g_assert_not_reached ();
      GST_ERROR ("Failed to map video frame");
      goto use_dummy_tex;
    }

    mem = gst_buffer_peek_memory (this->buffer_, 0);
    g_assert (gst_is_gl_memory (mem));

    context = ((GstGLBaseMemory *)mem)->context;

    sync_meta = gst_buffer_get_gl_sync_meta (this->sync_buffer_);
    if (!sync_meta)
      sync_meta = gst_buffer_add_gl_sync_meta (context, this->sync_buffer_);

    gst_gl_sync_meta_set_sync_point (sync_meta, context);

    gst_gl_sync_meta_wait (sync_meta, qt_context);

    tex_id = *(guint *) this->v_frame.data[0];
    GST_LOG ("%p binding Qt texture %u", this, tex_id);

    texSize = QSize(GST_VIDEO_FRAME_WIDTH (&this->v_frame), GST_VIDEO_FRAME_HEIGHT (&this->v_frame));
    // XXX: ideally, we would like to subclass the relevant texture object
    // ourselves but this is good enough for now
    texture = QNativeInterface::QSGOpenGLTexture::fromNative(tex_id, this->window_, texSize, options);

    setTexture(texture);
    setOwnsTexture(true);
    markDirty(QSGNode::DirtyMaterial);

    gst_video_frame_unmap (&this->v_frame);

    /* Texture was successfully bound, so we do not need
     * to use the dummy texture */
  }

  if (!texture()) {
use_dummy_tex:
    /* Create dummy texture if not already present. */
    if (this->dummy_tex_ == nullptr) {
      /* Make this a black 64x64 pixel RGBA texture.
       * This size and format is supported pretty much everywhere, so these
       * are a safe pick. (64 pixel sidelength must be supported according
       * to the GLES2 spec, table 6.18.)
       * Set min/mag filters to GL_LINEAR to make sure no mipmapping is used. */
      const int tex_sidelength = 64;
      QImage image(tex_sidelength, tex_sidelength, QImage::Format_ARGB32);
      image.fill(QColor(0, 0, 0, 255));

      this->dummy_tex_ = this->window_->createTextureFromImage(image);
    }

    g_assert (this->dummy_tex_ != nullptr);

    if (texture() != this->dummy_tex_) {
      setTexture(this->dummy_tex_);
      setOwnsTexture(false);
      markDirty(QSGNode::DirtyMaterial);
    }

    GST_LOG ("%p binding fallback dummy Qt texture %p", this, this->dummy_tex_);
  }
}
