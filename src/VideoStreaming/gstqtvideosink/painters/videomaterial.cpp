/*
    Copyright (C) 2011-2013 Collabora Ltd. <info@collabora.com>
    Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
    Copyright (C) 2013 basysKom GmbH <info@basyskom.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 2.1
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 *   @brief Extracted (and fixed) from QtGstreamer to avoid overly complex dependency
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "videomaterial.h"

#include <qmath.h>
#include <QOpenGLContext>
#include <QtQuick/QSGMaterialShader>

#include "glutils.h"

static const char * const qtvideosink_glsl_vertexShader =
    "uniform highp mat4 qt_Matrix;                      \n"
    "attribute highp vec4 qt_VertexPosition;            \n"
    "attribute highp vec2 qt_VertexTexCoord;            \n"
    "varying highp vec2 qt_TexCoord;                    \n"
    "void main() {                                      \n"
    "    qt_TexCoord = qt_VertexTexCoord;               \n"
    "    gl_Position = qt_Matrix * qt_VertexPosition;   \n"
    "}";

inline const char * qtvideosink_glsl_bgrxFragmentShader()
{
    return
    "uniform sampler2D rgbTexture;\n"
    "uniform lowp float opacity;\n"
    "uniform mediump mat4 colorMatrix;\n"
    "varying highp vec2 qt_TexCoord;\n"
    "void main(void)\n"
    "{\n"
    "    highp vec4 color = vec4(texture2D(rgbTexture, qt_TexCoord.st).bgr, 1.0);\n"
    "    gl_FragColor = colorMatrix * color * opacity;\n"
    "}\n";
}

inline const char * qtvideosink_glsl_xrgbFragmentShader()
{
    return
    "uniform sampler2D rgbTexture;\n"
    "uniform lowp float opacity;\n"
    "uniform mediump mat4 colorMatrix;\n"
    "varying highp vec2 qt_TexCoord;\n"
    "void main(void)\n"
    "{\n"
    "    highp vec4 color = vec4(texture2D(rgbTexture, qt_TexCoord.st).gba, 1.0);\n"
    "    gl_FragColor = colorMatrix * color * opacity;\n"
    "}\n";
}

inline const char * qtvideosink_glsl_rgbxFragmentShader()
{
    return
    "uniform sampler2D rgbTexture;\n"
    "uniform lowp float opacity;\n"
    "uniform mediump mat4 colorMatrix;\n"
    "varying highp vec2 qt_TexCoord;\n"
    "void main(void)\n"
    "{\n"
    "    highp vec4 color = vec4(texture2D(rgbTexture, qt_TexCoord.st).rgb, 1.0);\n"
    "    gl_FragColor = colorMatrix * color * opacity;\n"
    "}\n";
}

inline const char * qtvideosink_glsl_yuvPlanarFragmentShader()
{
    return
    "uniform sampler2D yTexture;\n"
    "uniform sampler2D uTexture;\n"
    "uniform sampler2D vTexture;\n"
    "uniform mediump mat4 colorMatrix;\n"
    "uniform lowp float opacity;\n"
    "varying highp vec2 qt_TexCoord;\n"
    "void main(void)\n"
    "{\n"
    "    highp vec4 color = vec4(\n"
    "           texture2D(yTexture, qt_TexCoord.st).r,\n"
    "           texture2D(uTexture, qt_TexCoord.st).r,\n"
    "           texture2D(vTexture, qt_TexCoord.st).r,\n"
    "           1.0);\n"
    "    gl_FragColor = colorMatrix * color * opacity;\n"
    "}\n";
}

class VideoMaterialShader : public QSGMaterialShader
{
public:
    virtual void updateState(const RenderState &state,
        QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
    {
        Q_UNUSED(oldMaterial);

        VideoMaterial *material = static_cast<VideoMaterial *>(newMaterial);
        if (m_id_rgbTexture > 0) {
            program()->setUniformValue(m_id_rgbTexture, 0);
        } else {
            program()->setUniformValue(m_id_yTexture, 0);
            program()->setUniformValue(m_id_uTexture, 1);
            program()->setUniformValue(m_id_vTexture, 2);
        }

        if (state.isOpacityDirty()) {
            material->setFlag(QSGMaterial::Blending,
                qFuzzyCompare(state.opacity(), 1.0f) ? false : true);
            program()->setUniformValue(m_id_opacity, GLfloat(state.opacity()));
        }

        if (state.isMatrixDirty())
            program()->setUniformValue(m_id_matrix, state.combinedMatrix());

        program()->setUniformValue(m_id_colorMatrix, material->m_colorMatrix);

        material->bind();
    }

    virtual char const *const *attributeNames() const {
        static const char *names[] = {
            "qt_VertexPosition",
            "qt_VertexTexCoord",
            0
        };
        return names;
    }

protected:
    virtual void initialize() {
        m_id_matrix = program()->uniformLocation("qt_Matrix");
        m_id_rgbTexture = program()->uniformLocation("rgbTexture");
        m_id_yTexture = program()->uniformLocation("yTexture");
        m_id_uTexture = program()->uniformLocation("uTexture");
        m_id_vTexture = program()->uniformLocation("vTexture");
        m_id_colorMatrix = program()->uniformLocation("colorMatrix");
        m_id_opacity = program()->uniformLocation("opacity");
    }

    virtual const char *vertexShader() const {
        return qtvideosink_glsl_vertexShader;
    }

    int m_id_matrix;
    int m_id_rgbTexture;
    int m_id_yTexture;
    int m_id_uTexture;
    int m_id_vTexture;
    int m_id_colorMatrix;
    int m_id_opacity;
};

template <const char * (*FragmentShader)()>
class VideoMaterialShaderImpl : public VideoMaterialShader
{
protected:
    virtual const char *fragmentShader() const {
        return FragmentShader();
    }
};

template <const char *(*FragmentShader)()>
class VideoMaterialImpl : public VideoMaterial
{
public:
    virtual QSGMaterialType *type() const {
        static QSGMaterialType theType;
        return &theType;
    }

    virtual QSGMaterialShader *createShader() const {
        return new VideoMaterialShaderImpl<FragmentShader>;
    }
};

VideoMaterial *VideoMaterial::create(const BufferFormat & format)
{
    VideoMaterial *material = NULL;

    switch (format.videoFormat()) {
    // BGRx
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
        material = new VideoMaterialImpl<qtvideosink_glsl_bgrxFragmentShader>;
        material->initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        break;
    case GST_VIDEO_FORMAT_BGR:
        material = new VideoMaterialImpl<qtvideosink_glsl_bgrxFragmentShader>;
        material->initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, format.frameSize());
        break;

    // xRGB
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_AYUV:
        material = new VideoMaterialImpl<qtvideosink_glsl_xrgbFragmentShader>;
        material->initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        break;

    // RGBx
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_v308:
        material = new VideoMaterialImpl<qtvideosink_glsl_rgbxFragmentShader>;
        material->initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, format.frameSize());
        break;
    case GST_VIDEO_FORMAT_RGB16:
        material = new VideoMaterialImpl<qtvideosink_glsl_rgbxFragmentShader>;
        material->initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, format.frameSize());
        break;

    // YUV 420 planar
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
        material = new VideoMaterialImpl<qtvideosink_glsl_yuvPlanarFragmentShader>;
        material->initYuv420PTextureInfo(
            (format.videoFormat() == GST_VIDEO_FORMAT_YV12) /* uvSwapped */,
            format.frameSize());
        break;

    default:
        Q_ASSERT(false);
        break;
    }

    material->init(format.colorMatrix());
    return material;
}

VideoMaterial::VideoMaterial()
    : m_frame(0)
    , m_textureCount(0)
    , m_textureFormat(0)
    , m_textureInternalFormat(0)
    , m_textureType(0)
    , m_colorMatrixType(GST_VIDEO_COLOR_MATRIX_UNKNOWN)
{
    memset(m_textureIds, 0, sizeof(m_textureIds));
    setFlag(Blending, false);
}

VideoMaterial::~VideoMaterial()
{
    if (!m_textureSize.isEmpty())
    {
        QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
        if (funcs)
        {
            funcs->glDeleteTextures(m_textureCount, m_textureIds);
        }
    }
    gst_buffer_replace(&m_frame, NULL);
}

int VideoMaterial::compare(const QSGMaterial *other) const
{
    const VideoMaterial *m = static_cast<const VideoMaterial *>(other);
    int d = m_textureIds[0] - m->m_textureIds[0];
    if (d || m_textureCount == 1)
        return d;
    else if ((d = m_textureIds[1] - m->m_textureIds[1]) != 0)
        return d;
    else
        return m_textureIds[2] - m->m_textureIds[2];
}

void VideoMaterial::initRgbTextureInfo(
        GLenum internalFormat, GLuint format, GLenum type, const QSize &size)
{
#ifndef QT_OPENGL_ES
    //make sure we get 8 bits per component, at least on the desktop GL where we can
    switch(internalFormat) {
    case GL_RGBA:
        internalFormat = GL_RGBA8;
        break;
    case GL_RGB:
        internalFormat = GL_RGB8;
        break;
    default:
        break;
    }
#endif

    m_textureInternalFormat = internalFormat;
    m_textureFormat = format;
    m_textureType = type;
    m_textureCount = 1;
    m_textureWidths[0] = size.width();
    m_textureHeights[0] = size.height();
    m_textureOffsets[0] = 0;
}

void VideoMaterial::initYuv420PTextureInfo(bool uvSwapped, const QSize &size)
{
    int bytesPerLine = (size.width() + 3) & ~3;
    int bytesPerLine2 = (size.width() / 2 + 3) & ~3;

    m_textureInternalFormat = GL_LUMINANCE;
    m_textureFormat = GL_LUMINANCE;
    m_textureType = GL_UNSIGNED_BYTE;
    m_textureCount = 3;
    m_textureWidths[0] = bytesPerLine;
    m_textureHeights[0] = size.height();
    m_textureOffsets[0] = 0;
    m_textureWidths[1] = bytesPerLine2;
    m_textureHeights[1] = size.height() / 2;
    m_textureOffsets[1] = bytesPerLine * size.height();
    m_textureWidths[2] = bytesPerLine2;
    m_textureHeights[2] = size.height() / 2;
    m_textureOffsets[2] = bytesPerLine * size.height() + bytesPerLine2 * size.height()/2;

    if (uvSwapped)
      qSwap (m_textureOffsets[1], m_textureOffsets[2]);
}

void VideoMaterial::init(GstVideoColorMatrix colorMatrixType)
{
    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (funcs)
    {
        funcs->glGenTextures(m_textureCount, m_textureIds);
        m_colorMatrixType = colorMatrixType;
        updateColors(0, 0, 0, 0);
    }
}

void VideoMaterial::setCurrentFrame(GstBuffer *buffer)
{
    QMutexLocker lock(&m_frameMutex);
    gst_buffer_replace(&m_frame, buffer);
}

void VideoMaterial::updateColors(int brightness, int contrast, int hue, int saturation)
{
    const qreal b = brightness / 200.0;
    const qreal c = contrast / 100.0 + 1.0;
    const qreal h = hue / 100.0;
    const qreal s = saturation / 100.0 + 1.0;

    const qreal cosH = qCos(M_PI * h);
    const qreal sinH = qSin(M_PI * h);

    const qreal h11 =  0.787 * cosH - 0.213 * sinH + 0.213;
    const qreal h21 = -0.213 * cosH + 0.143 * sinH + 0.213;
    const qreal h31 = -0.213 * cosH - 0.787 * sinH + 0.213;

    const qreal h12 = -0.715 * cosH - 0.715 * sinH + 0.715;
    const qreal h22 =  0.285 * cosH + 0.140 * sinH + 0.715;
    const qreal h32 = -0.715 * cosH + 0.715 * sinH + 0.715;

    const qreal h13 = -0.072 * cosH + 0.928 * sinH + 0.072;
    const qreal h23 = -0.072 * cosH - 0.283 * sinH + 0.072;
    const qreal h33 =  0.928 * cosH + 0.072 * sinH + 0.072;

    const qreal sr = (1.0 - s) * 0.3086;
    const qreal sg = (1.0 - s) * 0.6094;
    const qreal sb = (1.0 - s) * 0.0820;

    const qreal sr_s = sr + s;
    const qreal sg_s = sg + s;
    const qreal sb_s = sr + s;

    const float m4 = (s + sr + sg + sb) * (0.5 - 0.5 * c + b);

    m_colorMatrix(0, 0) = c * (sr_s * h11 + sg * h21 + sb * h31);
    m_colorMatrix(0, 1) = c * (sr_s * h12 + sg * h22 + sb * h32);
    m_colorMatrix(0, 2) = c * (sr_s * h13 + sg * h23 + sb * h33);
    m_colorMatrix(0, 3) = m4;

    m_colorMatrix(1, 0) = c * (sr * h11 + sg_s * h21 + sb * h31);
    m_colorMatrix(1, 1) = c * (sr * h12 + sg_s * h22 + sb * h32);
    m_colorMatrix(1, 2) = c * (sr * h13 + sg_s * h23 + sb * h33);
    m_colorMatrix(1, 3) = m4;

    m_colorMatrix(2, 0) = c * (sr * h11 + sg * h21 + sb_s * h31);
    m_colorMatrix(2, 1) = c * (sr * h12 + sg * h22 + sb_s * h32);
    m_colorMatrix(2, 2) = c * (sr * h13 + sg * h23 + sb_s * h33);
    m_colorMatrix(2, 3) = m4;

    m_colorMatrix(3, 0) = 0.0;
    m_colorMatrix(3, 1) = 0.0;
    m_colorMatrix(3, 2) = 0.0;
    m_colorMatrix(3, 3) = 1.0;

    switch (m_colorMatrixType) {
    case GST_VIDEO_COLOR_MATRIX_BT709:
/*
 *  This is bogus (Gus Grubba 20150706)
        m_colorMatrix *= QMatrix4x4(
                    1.164,  0.000,  1.793, -0.5727,
                    1.164, -0.534, -0.213,  0.3007,
                    1.164,  2.115,  0.000, -1.1302,
                    0.0,    0.000,  0.000,  1.0000);
        break;
*/
    case GST_VIDEO_COLOR_MATRIX_BT601:
        m_colorMatrix *= QMatrix4x4(
                    1.164f,  0.000f,  1.596f, -0.8708f,
                    1.164f, -0.392f, -0.813f,  0.5296f,
                    1.164f,  2.017f,  0.000f, -1.081f,
                    0.0f,    0.000f,  0.000f,  1.0000f);
        break;
    default:
        break;
    }
}

void VideoMaterial::bind()
{
    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (!funcs)
        return;

    GstBuffer *frame = NULL;

    m_frameMutex.lock();
    if (m_frame)
      frame = gst_buffer_ref(m_frame);
    m_frameMutex.unlock();

    if (frame) {
        GstMapInfo info;
        gst_buffer_map(frame, &info, GST_MAP_READ);
        funcs->glActiveTexture(GL_TEXTURE1);
        bindTexture(1, info.data);
        funcs->glActiveTexture(GL_TEXTURE2);
        bindTexture(2, info.data);
        funcs->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
        bindTexture(0, info.data);
        gst_buffer_unmap(frame, &info);
        gst_buffer_unref(frame);
    } else {
        funcs->glActiveTexture(GL_TEXTURE1);
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
        funcs->glActiveTexture(GL_TEXTURE2);
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);
        funcs->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
    }
}

void VideoMaterial::bindTexture(int i, const quint8 *data)
{
    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (!funcs)
        return;

    funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[i]);
    funcs->glTexImage2D(
        GL_TEXTURE_2D,
        0,
        m_textureInternalFormat,
        m_textureWidths[i],
        m_textureHeights[i],
        0,
        m_textureFormat,
        m_textureType,
        data + m_textureOffsets[i]);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

