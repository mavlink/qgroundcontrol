/*
 * GStreamer
 * Copyright (C) 2023 Matthew Waters <matthew@centricular.com>
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
#include "gstqsg6material.h"
#include <private/qrhi_p.h>

#define GST_CAT_DEFAULT gst_qsg_texture_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

/* matrix colour conversion code from vkvideoconvert.c */
typedef struct
{
  double dm[4][4];
} Matrix4;

static void
matrix_debug (const Matrix4 * s)
{
  GST_DEBUG ("[%f %f %f %f]", s->dm[0][0], s->dm[0][1], s->dm[0][2],
      s->dm[0][3]);
  GST_DEBUG ("[%f %f %f %f]", s->dm[1][0], s->dm[1][1], s->dm[1][2],
      s->dm[1][3]);
  GST_DEBUG ("[%f %f %f %f]", s->dm[2][0], s->dm[2][1], s->dm[2][2],
      s->dm[2][3]);
  GST_DEBUG ("[%f %f %f %f]", s->dm[3][0], s->dm[3][1], s->dm[3][2],
      s->dm[3][3]);
}

static void
matrix_to_float (const Matrix4 * m, float *ret)
{
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      ret[j * 4 + i] = m->dm[i][j];
    }
  }
}

static void
matrix_set_identity (Matrix4 * m)
{
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      m->dm[i][j] = (i == j);
    }
  }
}

static void
matrix_copy (Matrix4 * d, const Matrix4 * s)
{
  gint i, j;

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      d->dm[i][j] = s->dm[i][j];
}

/* Perform 4x4 matrix multiplication:
 *  - @dst@ = @a@ * @b@
 *  - @dst@ may be a pointer to @a@ andor @b@
 */
static void
matrix_multiply (Matrix4 * dst, Matrix4 * a, Matrix4 * b)
{
  Matrix4 tmp;
  int i, j, k;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      double x = 0;
      for (k = 0; k < 4; k++) {
        x += a->dm[i][k] * b->dm[k][j];
      }
      tmp.dm[i][j] = x;
    }
  }
  matrix_copy (dst, &tmp);
}

static void
matrix_offset_components (Matrix4 * m, double a1, double a2, double a3)
{
  Matrix4 a;

  matrix_set_identity (&a);
  a.dm[0][3] = a1;
  a.dm[1][3] = a2;
  a.dm[2][3] = a3;
  matrix_debug (&a);
  matrix_multiply (m, &a, m);
}

static void
matrix_scale_components (Matrix4 * m, double a1, double a2, double a3)
{
  Matrix4 a;

  matrix_set_identity (&a);
  a.dm[0][0] = a1;
  a.dm[1][1] = a2;
  a.dm[2][2] = a3;
  matrix_multiply (m, &a, m);
}

static void
matrix_YCbCr_to_RGB (Matrix4 * m, double Kr, double Kb)
{
  double Kg = 1.0 - Kr - Kb;
  Matrix4 k = {
    {
          {1., 0., 2 * (1 - Kr), 0.},
          {1., -2 * Kb * (1 - Kb) / Kg, -2 * Kr * (1 - Kr) / Kg, 0.},
          {1., 2 * (1 - Kb), 0., 0.},
          {0., 0., 0., 1.},
        }
  };

  matrix_multiply (m, &k, m);
}

static void
convert_to_RGB (GstVideoInfo *info, Matrix4 * m)
{
  {
    const GstVideoFormatInfo *uinfo;
    gint offset[4], scale[4], depth[4];
    guint i;

    uinfo = gst_video_format_get_info (GST_VIDEO_INFO_FORMAT (info));

    /* bring color components to [0..1.0] range */
    gst_video_color_range_offsets (info->colorimetry.range, uinfo, offset,
        scale);

    for (i = 0; i < uinfo->n_components; i++)
      depth[i] = (1 << uinfo->depth[i]) - 1;

    matrix_offset_components (m, -offset[0] / (float) depth[0],
        -offset[1] / (float) depth[1], -offset[2] / (float) depth[2]);
    matrix_scale_components (m, depth[0] / ((float) scale[0]),
        depth[1] / ((float) scale[1]), depth[2] / ((float) scale[2]));
    GST_DEBUG ("to RGB scale/offset matrix");
    matrix_debug (m);
  }

  if (GST_VIDEO_INFO_IS_YUV (info)) {
    gdouble Kr, Kb;

    if (gst_video_color_matrix_get_Kr_Kb (info->colorimetry.matrix, &Kr, &Kb))
      matrix_YCbCr_to_RGB (m, Kr, Kb);
    GST_DEBUG ("to RGB matrix");
    matrix_debug (m);
  }
}

class GstQSGTexture : public QSGTexture {
public:
  GstQSGTexture(QRhiTexture *);
  ~GstQSGTexture();

  qint64 comparisonKey() const override;
  bool hasAlphaChannel() const override;
  bool hasMipmaps() const override { return false; };
  bool isAtlasTexture() const override { return false; };
  QSize textureSize() const override;

  QRhiTexture *rhiTexture() const override;

private:
  QRhiTexture *m_texture;
  bool m_has_alpha;
};

GstQSGTexture::GstQSGTexture(QRhiTexture * texture)
  : m_texture(texture)
{
  switch (texture->format()) {
    case QRhiTexture::RGBA8:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
    case QRhiTexture::RGB10A2:
#endif
    case QRhiTexture::RGBA16F:
    case QRhiTexture::RGBA32F:
      this->m_has_alpha = true;
      break;
    default:
      this->m_has_alpha = false;
  }
}

GstQSGTexture::~GstQSGTexture()
{
  if (m_texture) {
    delete m_texture;
    m_texture = nullptr;
  }
}

qint64
GstQSGTexture::comparisonKey() const
{
  if (this->m_texture)
      return qint64(qintptr(this->m_texture));

  return qint64(qintptr(this));
}

bool
GstQSGTexture::hasAlphaChannel() const
{
  return m_has_alpha;
}

QSize
GstQSGTexture::textureSize() const
{
  // XXX: currently unused
  return QSize(0, 0);
}

QRhiTexture *
GstQSGTexture::rhiTexture() const
{
  return m_texture;
}

class GstQSGMaterialShader : public QSGMaterialShader {
public:
  GstQSGMaterialShader(GstVideoFormat v_format);
  ~GstQSGMaterialShader();

  bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
  void updateSampledImage(RenderState &state, int binding, QSGTexture **texture, QSGMaterial *newMaterial, QSGMaterial *) override;

private:
  GstVideoFormat v_format;
  QSGTexture *m_textures[GST_VIDEO_MAX_PLANES];
};

GstQSGMaterialShader::GstQSGMaterialShader(GstVideoFormat v_format)
  : v_format(v_format)
{
  setShaderFileName(VertexStage, ":/org/freedesktop/gstreamer/qml6/vertex.vert.qsb");

  switch (v_format) {
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_RGB:
      setShaderFileName(FragmentStage, ":/org/freedesktop/gstreamer/qml6/RGBA.frag.qsb");
      break;
    case GST_VIDEO_FORMAT_YV12:
      setShaderFileName(FragmentStage, ":/org/freedesktop/gstreamer/qml6/YUV_TRIPLANAR.frag.qsb");
      break;
    default:
      g_assert_not_reached ();
  }

  m_textures[0] = nullptr;
  m_textures[1] = nullptr;
  m_textures[2] = nullptr;
  m_textures[3] = nullptr;
}

GstQSGMaterialShader::~GstQSGMaterialShader()
{
  for (int i = 0; i < 4; i++) {
    if (m_textures[i]) {
      delete m_textures[i];
      m_textures[i] = nullptr;
    }
  }
}

bool
GstQSGMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
  const GstVideoFormatInfo *finfo = gst_video_format_get_info (v_format);
  bool changed = false;
  QByteArray *buf = state.uniformData();
  Q_ASSERT(buf->size() >= 84);

  GST_TRACE ("%p new material %p old material %p", this, newMaterial, oldMaterial);

  if (state.isMatrixDirty()) {
    const QMatrix4x4 m = state.combinedMatrix();
    memcpy(buf->data(), m.constData(), 64);
    changed = true;
  }

  if (state.isOpacityDirty()) {
    const float opacity = state.opacity();
    memcpy(buf->data() + 144, &opacity, 4);
    changed = true;
  }

  auto *mat = static_cast<GstQSGMaterial *>(newMaterial);
  if (oldMaterial != newMaterial || mat->uniforms.dirty) {
    memcpy(buf->data() + 64, &mat->uniforms.input_swizzle, 4 * sizeof (int));
    memcpy(buf->data() + 80, mat->uniforms.color_matrix.constData(), 64);
    mat->uniforms.dirty = false;
    changed = true;
  }

  for (guint i = 0; i < GST_VIDEO_MAX_PLANES; i++) {
    if (this->m_textures[i]) {
      delete this->m_textures[i];
      this->m_textures[i] = nullptr;
    }
    if (i < finfo->n_planes)
      this->m_textures[i] = mat->bind(this, state.rhi(), state.resourceUpdateBatch(), i, v_format);
  }

  return changed;
}

void
GstQSGMaterialShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
    QSGMaterial *newMaterial, QSGMaterial *)
{
  *texture = this->m_textures[binding - 1];
  GST_TRACE ("%p binding:%d texture %p", this, binding, *texture);
}

#define DEFINE_MATERIAL(format) \
class G_PASTE(GstQSGMaterial_,format) : public GstQSGMaterial { \
public: \
  G_PASTE(GstQSGMaterial_,format)(); \
  ~G_PASTE(GstQSGMaterial_,format)(); \
  QSGMaterialType *type() const override { static QSGMaterialType type; return &type; }; \
}; \
G_PASTE(GstQSGMaterial_,format)::G_PASTE(GstQSGMaterial_,format)() {} \
G_PASTE(GstQSGMaterial_,format)::~G_PASTE(GstQSGMaterial_,format)() {}

DEFINE_MATERIAL(RGBA_SWIZZLE);
DEFINE_MATERIAL(YUV_TRIPLANAR);

GstQSGMaterial *
GstQSGMaterial::new_for_format(GstVideoFormat format)
{
  const GstVideoFormatInfo *finfo = gst_video_format_get_info (format);

  if (GST_VIDEO_FORMAT_INFO_IS_RGB (finfo) && finfo->n_planes == 1) {
    return static_cast<GstQSGMaterial *>(new GstQSGMaterial_RGBA_SWIZZLE());
  }

  switch (format) {
    case GST_VIDEO_FORMAT_YV12:
      return static_cast<GstQSGMaterial *>(new GstQSGMaterial_YUV_TRIPLANAR());
    default:
      g_assert_not_reached ();
  }
}

GstQSGMaterial::GstQSGMaterial ()
{
  static gsize _debug;

  if (g_once_init_enter (&_debug)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "qtqsg6material", 0,
        "Qt6 Scenegraph Material");
    g_once_init_leave (&_debug, 1);
  }

  g_weak_ref_init (&this->qt_context_ref_, NULL);
  gst_video_info_init (&this->v_info);
  memset (&this->v_frame, 0, sizeof (this->v_frame));

  this->buffer_ = NULL;
  this->buffer_was_bound = false;
  this->sync_buffer_ = gst_buffer_new ();

  this->uniforms.dirty = true;
}

GstQSGMaterial::~GstQSGMaterial ()
{
  g_weak_ref_clear (&this->qt_context_ref_);
  gst_buffer_replace (&this->buffer_, NULL);
  gst_buffer_replace (&this->sync_buffer_, NULL);
  this->buffer_was_bound = false;

  if (this->v_frame.buffer) {
    gst_video_frame_unmap (&this->v_frame);
    memset (&this->v_frame, 0, sizeof (this->v_frame));
  }
}

bool
GstQSGMaterial::compatibleWith(GstVideoInfo * v_info)
{
  if (GST_VIDEO_INFO_FORMAT (&this->v_info) != GST_VIDEO_INFO_FORMAT (v_info))
    return false;

  return true;
}

QSGMaterialShader *
GstQSGMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
  GstVideoFormat v_format = GST_VIDEO_INFO_FORMAT (&this->v_info);

  return new GstQSGMaterialShader(v_format);
}

/* only called from the streaming thread with scene graph thread blocked */
void
GstQSGMaterial::setCaps (GstCaps * caps)
{
  GST_LOG ("%p setCaps %" GST_PTR_FORMAT, this, caps);

  gst_video_info_from_caps (&this->v_info, caps);
}

/* only called from the streaming thread with scene graph thread blocked */
gboolean
GstQSGMaterial::setBuffer (GstBuffer * buffer)
{
  GST_LOG ("%p setBuffer %" GST_PTR_FORMAT, this, buffer);
  /* FIXME: update more state here */
  if (!gst_buffer_replace (&this->buffer_, buffer))
    return FALSE;

  this->buffer_was_bound = false;

  g_weak_ref_set (&this->qt_context_ref_, gst_gl_context_get_current ());

  if (this->v_frame.buffer) {
    gst_video_frame_unmap (&this->v_frame);
    memset (&this->v_frame, 0, sizeof (this->v_frame));
  }

  if (this->buffer_) {
    if (!gst_video_frame_map (&this->v_frame, &this->v_info, this->buffer_,
          (GstMapFlags) (GST_MAP_READ | GST_MAP_GL))) {
      g_assert_not_reached ();
      return FALSE;
    }
    gst_gl_video_format_swizzle(GST_VIDEO_INFO_FORMAT (&this->v_info), this->uniforms.input_swizzle);

    Matrix4 m;
    float matrix_data[16] = { 0.0, };

    matrix_set_identity (&m);
    convert_to_RGB (&this->v_info, &m);
    matrix_debug (&m);
    matrix_to_float (&m, matrix_data);

    this->uniforms.color_matrix = QMatrix4x4(matrix_data);
    this->uniforms.dirty = true;
  }

  return TRUE;
}

/* only called from the streaming thread with scene graph thread blocked */
GstBuffer *
GstQSGMaterial::getBuffer (bool * was_bound)
{
  GstBuffer *buffer = NULL;

  if (this->buffer_)
    buffer = gst_buffer_ref (this->buffer_);
  if (was_bound)
    *was_bound = this->buffer_was_bound;

  return buffer;
}

void
GstQSGMaterial::setFiltering(QSGTexture::Filtering filtering)
{
  m_filtering = filtering;
}

static QRhiTexture::Format
video_format_to_rhi_format (GstVideoFormat format, guint plane)
{
  switch (format) {
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_BGRA:
      return QRhiTexture::RGBA8;
    case GST_VIDEO_FORMAT_YV12:
      return QRhiTexture::RED_OR_ALPHA8;
    default:
      g_assert_not_reached ();
  }
}

QSGTexture *
GstQSGMaterial::bind(GstQSGMaterialShader *shader, QRhi * rhi, QRhiResourceUpdateBatch *res_updates, guint plane, GstVideoFormat v_format)
{
  GstGLContext *qt_context, *context;
  GstMemory *mem;
  GstGLMemory *gl_mem;
  GstGLSyncMeta *sync_meta;
  gboolean use_dummy_tex = TRUE;
  guint tex_id;
  GstQSGTexture *ret;
  QRhiTexture *rhi_tex;
  QSize tex_size;

  qt_context = GST_GL_CONTEXT (g_weak_ref_get (&this->qt_context_ref_));
  if (!qt_context)
    goto out;

  if (!this->buffer_)
    goto out;
  if (GST_VIDEO_INFO_FORMAT (&this->v_info) == GST_VIDEO_FORMAT_UNKNOWN)
    goto out;

  mem = gst_buffer_peek_memory (this->buffer_, plane);
  g_assert (gst_is_gl_memory (mem));
  gl_mem = (GstGLMemory *) mem;
  context = ((GstGLBaseMemory *)mem)->context;

  /* Texture was successfully bound, so we do not need
   * to use the dummy texture */
  use_dummy_tex = FALSE;

  this->buffer_was_bound = true;
  tex_id = *(guint *) this->v_frame.data[plane];

  tex_size = QSize(gst_gl_memory_get_texture_width(gl_mem), gst_gl_memory_get_texture_height (gl_mem));

  rhi_tex = rhi->newTexture (video_format_to_rhi_format (v_format, plane), tex_size, 1, {});
  rhi_tex->createFrom({(guint64) tex_id, 0});

  sync_meta = gst_buffer_get_gl_sync_meta (this->sync_buffer_);
  if (!sync_meta)
    sync_meta = gst_buffer_add_gl_sync_meta (context, this->sync_buffer_);

  gst_gl_sync_meta_set_sync_point (sync_meta, context);

  gst_gl_sync_meta_wait (sync_meta, qt_context);

  GST_LOG ("%p binding GL texture %u for plane %d", this, tex_id, plane);

out:
  if (G_UNLIKELY (use_dummy_tex)) {
    /* Create dummy texture if not already present.
     * Use the Qt RHI functions instead of the GstGL ones.
     */

    /* Make this a black 64x64 pixel RGBA texture.
     * This size and format is supported pretty much everywhere, so these
     * are a safe pick. (64 pixel sidelength must be supported according
     * to the GLES2 spec, table 6.18.)
     * Set min/mag filters to GL_LINEAR to make sure no mipmapping is used. */
    const int tex_sidelength = 64;
    std::vector < char > dummy_data (tex_sidelength * tex_sidelength * 4, 0);

    rhi_tex = rhi->newTexture (video_format_to_rhi_format (v_format, plane), QSize(tex_sidelength, tex_sidelength), 1, {});

    switch (v_format) {
      case GST_VIDEO_FORMAT_RGBA:
      case GST_VIDEO_FORMAT_BGRA:
      case GST_VIDEO_FORMAT_RGB:
        break;
      case GST_VIDEO_FORMAT_YV12:
        if (plane == 1 || plane == 2) {
          char *data = dummy_data.data();
          for (gsize j = 0; j < tex_sidelength; j++) {
            for (gsize k = 0; k < tex_sidelength; k++) {
              data[(j * tex_sidelength + k) * 4 + 0] = 0x7F;
            }
          }
        }
        break;
      default:
        g_assert_not_reached ();
        break;
    }

    QRhiTextureSubresourceUploadDescription sub_desc;

    sub_desc.setData(QByteArray::fromRawData(dummy_data.data(), dummy_data.size()));

    QRhiTextureUploadEntry entry(0, 0, sub_desc);
    QRhiTextureUploadDescription desc({ entry });
    res_updates->uploadTexture(rhi_tex, desc);

    GST_LOG ("%p binding for plane %d fallback dummy Qt texture", this, plane);
  }

  ret = new GstQSGTexture(rhi_tex);
  ret->setFiltering(m_filtering);

  gst_clear_object (&qt_context);

  return static_cast<QSGTexture *>(ret);
}
