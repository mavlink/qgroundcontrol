/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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

#include <vector>
#include <stdio.h>

#include <gst/video/video.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>
#include "gstqsgtexture.h"

#define GST_CAT_DEFAULT gst_qsg_texture_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

GstQSGTexture::GstQSGTexture ()
{
  static volatile gsize _debug;

  initializeOpenGLFunctions();

  if (g_once_init_enter (&_debug)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "qtqsgtexture", 0,
        "Qt Scenegraph Texture");
    g_once_init_leave (&_debug, 1);
  }

  gst_video_info_init (&this->v_info);
  this->buffer_ = NULL;
  this->qt_context_ = NULL;
  this->sync_buffer_ = gst_buffer_new ();
  this->dummy_tex_id_ = 0;
}

GstQSGTexture::~GstQSGTexture ()
{
  gst_buffer_replace (&this->buffer_, NULL);
  gst_buffer_replace (&this->sync_buffer_, NULL);
  if (this->dummy_tex_id_ && QOpenGLContext::currentContext ()) {
    QOpenGLContext::currentContext ()->functions ()->glDeleteTextures (1,
        &this->dummy_tex_id_);
  }
}

/* only called from the streaming thread with scene graph thread blocked */
void
GstQSGTexture::setCaps (GstCaps * caps)
{
  GST_LOG ("%p setCaps %" GST_PTR_FORMAT, this, caps);

  gst_video_info_from_caps (&this->v_info, caps);
}

/* only called from the streaming thread with scene graph thread blocked */
gboolean
GstQSGTexture::setBuffer (GstBuffer * buffer)
{
  GST_LOG ("%p setBuffer %" GST_PTR_FORMAT, this, buffer);
  /* FIXME: update more state here */
  if (!gst_buffer_replace (&this->buffer_, buffer))
    return FALSE;

  this->qt_context_ = gst_gl_context_get_current ();

  return TRUE;
}

/* only called from qt's scene graph render thread */
void
GstQSGTexture::bind ()
{
  const GstGLFuncs *gl;
  GstGLContext *context;
  GstGLSyncMeta *sync_meta;
  GstMemory *mem;
  guint tex_id;
  gboolean use_dummy_tex = TRUE;

  if (!this->qt_context_)
    return;

  gst_gl_context_activate (this->qt_context_, TRUE);

  if (!this->buffer_)
    goto out;
  if (GST_VIDEO_INFO_FORMAT (&this->v_info) == GST_VIDEO_FORMAT_UNKNOWN)
    goto out;

  this->mem_ = gst_buffer_peek_memory (this->buffer_, 0);
  if (!this->mem_)
    goto out;

  g_assert (this->qt_context_);
  gl = this->qt_context_->gl_vtable;

  /* FIXME: should really lock the memory to prevent write access */
  if (!gst_video_frame_map (&this->v_frame, &this->v_info, this->buffer_,
        (GstMapFlags) (GST_MAP_READ | GST_MAP_GL))) {
    g_assert_not_reached ();
    goto out;
  }

  mem = gst_buffer_peek_memory (this->buffer_, 0);
  g_assert (gst_is_gl_memory (mem));

  context = ((GstGLBaseMemory *)mem)->context;

  sync_meta = gst_buffer_get_gl_sync_meta (this->sync_buffer_);
  if (!sync_meta)
    sync_meta = gst_buffer_add_gl_sync_meta (context, this->sync_buffer_);

  gst_gl_sync_meta_set_sync_point (sync_meta, context);

  gst_gl_sync_meta_wait (sync_meta, this->qt_context_);

  tex_id = *(guint *) this->v_frame.data[0];
  GST_LOG ("%p binding Qt texture %u", this, tex_id);

  gl->BindTexture (GL_TEXTURE_2D, tex_id);

  gst_video_frame_unmap (&this->v_frame);

  /* Texture was successfully bound, so we do not need
   * to use the dummy texture */
  use_dummy_tex = FALSE;

out:
  if (G_UNLIKELY (use_dummy_tex)) {
    QOpenGLContext *qglcontext = QOpenGLContext::currentContext ();
    QOpenGLFunctions *funcs = qglcontext->functions ();

    /* Create dummy texture if not already present.
     * Use the Qt OpenGL functions instead of the GstGL ones,
     * since we are using the Qt OpenGL context here, and we must
     * be able to delete the texture in the destructor. */
    if (this->dummy_tex_id_ == 0) {
      /* Make this a black 64x64 pixel RGBA texture.
       * This size and format is supported pretty much everywhere, so these
       * are a safe pick. (64 pixel sidelength must be supported according
       * to the GLES2 spec, table 6.18.)
       * Set min/mag filters to GL_LINEAR to make sure no mipmapping is used. */
      const int tex_sidelength = 64;
      std::vector < guint8 > dummy_data (tex_sidelength * tex_sidelength * 4, 0);

      funcs->glGenTextures (1, &this->dummy_tex_id_);
      funcs->glBindTexture (GL_TEXTURE_2D, this->dummy_tex_id_);
      funcs->glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      funcs->glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      funcs->glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, tex_sidelength,
          tex_sidelength, 0, GL_RGBA, GL_UNSIGNED_BYTE, &dummy_data[0]);
    }

    g_assert (this->dummy_tex_id_ != 0);

    funcs->glBindTexture (GL_TEXTURE_2D, this->dummy_tex_id_);
  }

  gst_gl_context_activate (this->qt_context_, FALSE);
}

/* can be called from any thread */
int
GstQSGTexture::textureId () const
{
  int tex_id = 0;

  if (this->buffer_) {
    GstMemory *mem = gst_buffer_peek_memory (this->buffer_, 0);

    tex_id = ((GstGLMemory *) mem)->tex_id;
  }

  GST_LOG ("%p get texture id %u", this, tex_id);

  return tex_id;
}

/* can be called from any thread */
QSize
GstQSGTexture::textureSize () const
{
  if (GST_VIDEO_INFO_FORMAT (&this->v_info) == GST_VIDEO_FORMAT_UNKNOWN)
    return QSize (0, 0);

  GST_TRACE ("%p get texture size %ux%u", this, this->v_info.width,
      this->v_info.height);

  return QSize (this->v_info.width, this->v_info.height);
}

/* can be called from any thread */
bool
GstQSGTexture::hasAlphaChannel () const
{
  /* FIXME: support RGB textures */
  return true;
}

/* can be called from any thread */
bool
GstQSGTexture::hasMipmaps () const
{
  return false;
}
