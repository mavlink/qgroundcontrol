/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). <qt-info@nokia.com>
    Copyright (C) 2011-2012 Collabora Ltd. <info@collabora.com>

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
 *   @brief Extracted from QtGstreamer to avoid overly complex dependency
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "openglsurfacepainter.h"
#include <QtCore/qmath.h>

#include "glutils.h"

#ifndef GL_TEXTURE0
#  define GL_TEXTURE0    0x84C0
#  define GL_TEXTURE1    0x84C1
#  define GL_TEXTURE2    0x84C2
#endif

#ifndef GL_PROGRAM_ERROR_STRING_ARB
#  define GL_PROGRAM_ERROR_STRING_ARB       0x8874
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#  define GL_UNSIGNED_SHORT_5_6_5 33635
#endif

#ifndef GL_CLAMP_TO_EDGE
#  define GL_CLAMP_TO_EDGE 0x812F
#endif

#define QRECT_TO_GLMATRIX(rect) \
    { \
        GLfloat(rect.left())     , GLfloat(rect.bottom() + 1), \
        GLfloat(rect.right() + 1), GLfloat(rect.bottom() + 1), \
        GLfloat(rect.left())     , GLfloat(rect.top()), \
        GLfloat(rect.right() + 1), GLfloat(rect.top()) \
    }

OpenGLSurfacePainter::OpenGLSurfacePainter()
    : m_textureFormat(0)
    , m_textureInternalFormat(0)
    , m_textureType(0)
    , m_textureCount(0)
    , m_videoColorMatrix(GST_VIDEO_COLOR_MATRIX_UNKNOWN)
{
#ifndef QT_OPENGL_ES
        glActiveTexture = (_glActiveTexture) QGLContext::currentContext()->getProcAddress(QLatin1String("glActiveTexture"));
#endif
}

//static
QSet<GstVideoFormat> OpenGLSurfacePainter::supportedPixelFormats()
{
    return QSet<GstVideoFormat>()
        //also handled by the generic painter on LE
        << GST_VIDEO_FORMAT_BGRA
        << GST_VIDEO_FORMAT_BGRx

        //also handled by the generic painter on BE
        << GST_VIDEO_FORMAT_ARGB
        << GST_VIDEO_FORMAT_xRGB

        //also handled by the generic painter everywhere
        << GST_VIDEO_FORMAT_RGB
        << GST_VIDEO_FORMAT_RGB16

        //not handled by the generic painter
        << GST_VIDEO_FORMAT_BGR
        << GST_VIDEO_FORMAT_v308
        << GST_VIDEO_FORMAT_AYUV
        << GST_VIDEO_FORMAT_YV12
        << GST_VIDEO_FORMAT_I420
        ;
}

void OpenGLSurfacePainter::updateColors(int brightness, int contrast, int hue, int saturation)
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

    switch (m_videoColorMatrix) {
#if 0
    //I have no idea what this is - it's not needed currently in this code
    case BufferFormat::YCbCr_JPEG:
        m_colorMatrix *= QMatrix4x4(
                    1.0,  0.000,  1.402, -0.701,
                    1.0, -0.344, -0.714,  0.529,
                    1.0,  1.772,  0.000, -0.886,
                    0.0,  0.000,  0.000,  1.0000);
        break;
#endif
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

void OpenGLSurfacePainter::paint(quint8 *data,
        const BufferFormat & /*frameFormat*/,
        QPainter *painter,
        const PaintAreas & areas)
{
    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (!funcs)
        return;

    // if these are enabled, we need to reenable them after beginNativePainting()
    // has been called, as they may get disabled
    bool stencilTestEnabled = funcs->glIsEnabled(GL_STENCIL_TEST);
    bool scissorTestEnabled = funcs->glIsEnabled(GL_SCISSOR_TEST);

    painter->beginNativePainting();

    if (stencilTestEnabled)
        funcs->glEnable(GL_STENCIL_TEST);
    if (scissorTestEnabled)
        funcs->glEnable(GL_SCISSOR_TEST);

    const GLfloat vertexCoordArray[] = QRECT_TO_GLMATRIX(areas.videoArea);

    const GLfloat txLeft    = areas.sourceRect.left();
    const GLfloat txRight   = areas.sourceRect.right();
    const GLfloat txTop     = areas.sourceRect.top();
    const GLfloat txBottom  = areas.sourceRect.bottom();

    const GLfloat textureCoordArray[] =
    {
        txLeft , txBottom,
        txRight, txBottom,
        txLeft , txTop,
        txRight, txTop
    };

    for (int i = 0; i < m_textureCount; ++i) {
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
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    paintImpl(painter, vertexCoordArray, textureCoordArray);

    painter->endNativePainting();
    painter->fillRect(areas.blackArea1, Qt::black);
    painter->fillRect(areas.blackArea2, Qt::black);
}

void OpenGLSurfacePainter::initRgbTextureInfo(
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

void OpenGLSurfacePainter::initYuv420PTextureInfo(const QSize &size)
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
}

void OpenGLSurfacePainter::initYv12TextureInfo(const QSize &size)
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
    m_textureOffsets[1] = bytesPerLine * size.height() + bytesPerLine2 * size.height()/2;
    m_textureWidths[2] = bytesPerLine2;
    m_textureHeights[2] = size.height() / 2;
    m_textureOffsets[2] = bytesPerLine * size.height();
}

#ifndef QT_OPENGL_ES

# ifndef GL_FRAGMENT_PROGRAM_ARB
#  define GL_FRAGMENT_PROGRAM_ARB           0x8804
#  define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
# endif

// Interprets the RGBA texture as in fact being BGRx and paints it.
static const char *qt_arbfp_bgrxShaderProgram =
    "!!ARBfp1.0\n"
    "PARAM matrix[4] = { program.local[0..2],"
    "{ 0.0, 0.0, 0.0, 1.0 } };\n"
    "TEMP bgrx;\n"
    "TEX bgrx.xyz, fragment.texcoord[0], texture[0], 2D;\n"
    "MOV bgrx.w, matrix[3].w;\n"
    "DP4 result.color.x, bgrx.zyxw, matrix[0];\n"
    "DP4 result.color.y, bgrx.zyxw, matrix[1];\n"
    "DP4 result.color.z, bgrx.zyxw, matrix[2];\n"
    "END";

// Interprets the RGBA texture as in fact being BGRA and paints it.
static const char *qt_arbfp_bgraShaderProgram =
    "!!ARBfp1.0\n"
    "PARAM matrix[4] = { program.local[0..2],"
    "{ 0.0, 0.0, 0.0, 1.0 } };\n"
    "TEMP bgra;\n"
    "TEX bgra, fragment.texcoord[0], texture[0], 2D;\n"
    "MOV bgra.w, matrix[3].w;\n"
    "DP4 result.color.x, bgra.zyxw, matrix[0];\n"
    "DP4 result.color.y, bgra.zyxw, matrix[1];\n"
    "DP4 result.color.z, bgra.zyxw, matrix[2];\n"
    "TEX result.color.w, fragment.texcoord[0], texture, 2D;\n"
    "END";

// Interprets the RGBA texture as in fact being xRGB and paints it.
static const char *qt_arbfp_xrgbShaderProgram =
    "!!ARBfp1.0\n"
    "PARAM matrix[4] = { program.local[0..2],"
    "{ 0.0, 0.0, 0.0, 1.0 } };\n"
    "TEMP xrgb;\n"
    "TEX xrgb, fragment.texcoord[0], texture[0], 2D;\n"
    "MOV xrgb.x, matrix[3].w;\n"
    "DP4 result.color.x, xrgb.yzwx, matrix[0];\n"
    "DP4 result.color.y, xrgb.yzwx, matrix[1];\n"
    "DP4 result.color.z, xrgb.yzwx, matrix[2];\n"
    "END";

// Interprets the RGBA texture as in fact being ARGB and paints it.
static const char *qt_arbfp_argbShaderProgram =
    "!!ARBfp1.0\n"
    "PARAM matrix[4] = { program.local[0..2],"
    "{ 0.0, 0.0, 0.0, 1.0 } };\n"
    "TEMP argb;\n"
    "TEX argb, fragment.texcoord[0], texture[0], 2D;\n"
    "MOV argb.x, matrix[3].w;\n"
    "DP4 result.color.x, argb.yzwx, matrix[0];\n"
    "DP4 result.color.y, argb.yzwx, matrix[1];\n"
    "DP4 result.color.z, argb.yzwx, matrix[2];\n"
    "TEX result.color.w, fragment.texcoord[0], texture, 2D;\n"
    "END";

// Paints RGB frames without doing any color channel flipping.
static const char *qt_arbfp_rgbxShaderProgram =
    "!!ARBfp1.0\n"
    "PARAM matrix[4] = { program.local[0..2],"
    "{ 0.0, 0.0, 0.0, 1.0 } };\n"
    "TEMP rgb;\n"
    "TEX rgb.xyz, fragment.texcoord[0], texture[0], 2D;\n"
    "MOV rgb.w, matrix[3].w;\n"
    "DP4 result.color.x, rgb, matrix[0];\n"
    "DP4 result.color.y, rgb, matrix[1];\n"
    "DP4 result.color.z, rgb, matrix[2];\n"
    "END";

// Paints a YUV420P or YV12 frame.
static const char *qt_arbfp_yuvPlanarShaderProgram =
    "!!ARBfp1.0\n"
    "PARAM matrix[4] = { program.local[0..2],"
    "{ 0.0, 0.0, 0.0, 1.0 } };\n"
    "TEMP yuv;\n"
    "TEX yuv.x, fragment.texcoord[0], texture[0], 2D;\n"
    "TEX yuv.y, fragment.texcoord[0], texture[1], 2D;\n"
    "TEX yuv.z, fragment.texcoord[0], texture[2], 2D;\n"
    "MOV yuv.w, matrix[3].w;\n"
    "DP4 result.color.x, yuv, matrix[0];\n"
    "DP4 result.color.y, yuv, matrix[1];\n"
    "DP4 result.color.z, yuv, matrix[2];\n"
    "END";



ArbFpSurfacePainter::ArbFpSurfacePainter()
    : OpenGLSurfacePainter()
    , m_programId(0)
{
    const QGLContext *context = QGLContext::currentContext();
    glProgramStringARB  = (_glProgramStringARB)  context->getProcAddress(QLatin1String("glProgramStringARB"));
    glBindProgramARB    = (_glBindProgramARB)    context->getProcAddress(QLatin1String("glBindProgramARB"));
    glDeleteProgramsARB = (_glDeleteProgramsARB) context->getProcAddress(QLatin1String("glDeleteProgramsARB"));
    glGenProgramsARB    = (_glGenProgramsARB)    context->getProcAddress(QLatin1String("glGenProgramsARB"));
    glProgramLocalParameter4fARB = (_glProgramLocalParameter4fARB) context->getProcAddress(QLatin1String("glProgramLocalParameter4fARB"));
}

void ArbFpSurfacePainter::init(const BufferFormat &format)
{
    Q_ASSERT(m_textureCount == 0);
    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (!funcs)
        return;

    const char *program = 0;

    switch (format.videoFormat()) {
    case GST_VIDEO_FORMAT_BGRx:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        program = qt_arbfp_bgrxShaderProgram;
        break;
    case GST_VIDEO_FORMAT_xRGB:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        program = qt_arbfp_xrgbShaderProgram;
        break;
    case GST_VIDEO_FORMAT_BGRA:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        program = qt_arbfp_bgraShaderProgram;
        break;
    case GST_VIDEO_FORMAT_ARGB:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        program = qt_arbfp_argbShaderProgram;
        break;
    case GST_VIDEO_FORMAT_RGB:
        initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, format.frameSize());
        program = qt_arbfp_rgbxShaderProgram;
        break;
    case GST_VIDEO_FORMAT_BGR:
        initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, format.frameSize());
        program = qt_arbfp_bgrxShaderProgram;
        break;
    //NOTE: unlike the other formats, this is endianness-dependent,
    //but using GL_UNSIGNED_SHORT_5_6_5 ensures that it's handled correctly
    case GST_VIDEO_FORMAT_RGB16:
        initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, format.frameSize());
        program = qt_arbfp_rgbxShaderProgram;
        break;
    case GST_VIDEO_FORMAT_v308:
        initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, format.frameSize());
        program = qt_arbfp_rgbxShaderProgram;
        break;
    case GST_VIDEO_FORMAT_AYUV:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        program = qt_arbfp_argbShaderProgram;
        break;
    case GST_VIDEO_FORMAT_YV12:
        initYv12TextureInfo(format.frameSize());
        program = qt_arbfp_yuvPlanarShaderProgram;
        break;
    case GST_VIDEO_FORMAT_I420:
        initYuv420PTextureInfo(format.frameSize());
        program = qt_arbfp_yuvPlanarShaderProgram;
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    m_videoColorMatrix = format.colorMatrix();

    glGenProgramsARB(1, &m_programId);

    GLenum glError = funcs->glGetError();
    if (glError != GL_NO_ERROR) {
        throw QString("ARBfb Shader allocation error ") +
            QString::number(static_cast<int>(glError), 16);
    } else {
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_programId);
        glProgramStringARB(
                GL_FRAGMENT_PROGRAM_ARB,
                GL_PROGRAM_FORMAT_ASCII_ARB,
                qstrlen(program),
                reinterpret_cast<const GLvoid *>(program));

        if ((glError = funcs->glGetError()) != GL_NO_ERROR) {
            const GLubyte* errorString = funcs->glGetString(GL_PROGRAM_ERROR_STRING_ARB);
            glDeleteProgramsARB(1, &m_programId);
            m_textureCount = 0;
            m_programId = 0;
            throw QString("ARBfp Shader compile error ") +
                QString::number(static_cast<int>(glError), 16) +
                reinterpret_cast<const char *>(errorString);
        } else {
            funcs->glGenTextures(m_textureCount, m_textureIds);
        }
    }
}

void ArbFpSurfacePainter::cleanup()
{
    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (funcs)
    {
        funcs->glDeleteTextures(m_textureCount, m_textureIds);
        glDeleteProgramsARB(1, &m_programId);
    }
    m_textureCount = 0;
    m_programId = 0;
}

void ArbFpSurfacePainter::paintImpl(const QPainter *painter,
        const GLfloat *vertexCoordArray,
        const GLfloat *textureCoordArray)
{
    Q_UNUSED(painter);
    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (!funcs)
        return;

    funcs->glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_programId);

    glProgramLocalParameter4fARB(
            GL_FRAGMENT_PROGRAM_ARB,
            0,
            m_colorMatrix(0, 0),
            m_colorMatrix(0, 1),
            m_colorMatrix(0, 2),
            m_colorMatrix(0, 3));
    glProgramLocalParameter4fARB(
            GL_FRAGMENT_PROGRAM_ARB,
            1,
            m_colorMatrix(1, 0),
            m_colorMatrix(1, 1),
            m_colorMatrix(1, 2),
            m_colorMatrix(1, 3));
    glProgramLocalParameter4fARB(
            GL_FRAGMENT_PROGRAM_ARB,
            2,
            m_colorMatrix(2, 0),
            m_colorMatrix(2, 1),
            m_colorMatrix(2, 2),
            m_colorMatrix(2, 3));

    funcs->glActiveTexture(GL_TEXTURE0);
    funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);

    if (m_textureCount == 3) {
        funcs->glActiveTexture(GL_TEXTURE1);
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
        funcs->glActiveTexture(GL_TEXTURE2);
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);
        funcs->glActiveTexture(GL_TEXTURE0);
    }

    funcs->glVertexPointer(2, GL_FLOAT, 0, vertexCoordArray);
    funcs->glTexCoordPointer(2, GL_FLOAT, 0, textureCoordArray);

    funcs->glEnableClientState(GL_VERTEX_ARRAY);
    funcs->glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    funcs->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    funcs->glDisableClientState(GL_VERTEX_ARRAY);
    funcs->glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    funcs->glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

#endif

static const char *qt_glsl_vertexShaderProgram =
        "attribute highp vec4 vertexCoordArray;\n"
        "attribute highp vec2 textureCoordArray;\n"
        "uniform highp mat4 positionMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "   gl_Position = positionMatrix * vertexCoordArray;\n"
        "   textureCoord = textureCoordArray;\n"
        "}\n";

// Interprets the RGBA texture as in fact being BGRx and paints it.
static const char *qt_glsl_bgrxShaderProgram =
        "uniform sampler2D texRgb;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "    highp vec4 color = vec4(texture2D(texRgb, textureCoord.st).bgr, 1.0);\n"
        "    gl_FragColor = colorMatrix * color;\n"
        "}\n";

// Interprets the RGBA texture as in fact being BGRA and paints it.
static const char *qt_glsl_bgraShaderProgram =
        "uniform sampler2D texRgb;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "    highp vec4 color = vec4(texture2D(texRgb, textureCoord.st).bgr, 1.0);\n"
        "    color = colorMatrix * color;\n"
        "    gl_FragColor = vec4(color.rgb, texture2D(texRgb, textureCoord.st).a);\n"
        "}\n";

// Interprets the RGBA texture as in fact being xRGB and paints it.
static const char *qt_glsl_xrgbShaderProgram =
        "uniform sampler2D texRgb;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "    highp vec4 color = vec4(texture2D(texRgb, textureCoord.st).gba, 1.0);\n"
        "    gl_FragColor = colorMatrix * color;\n"
        "}\n";

// Interprets the RGBA texture as in fact being ARGB and paints it.
static const char *qt_glsl_argbShaderProgram =
        "uniform sampler2D texRgb;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "    highp vec4 color = vec4(texture2D(texRgb, textureCoord.st).gba, 1.0);\n"
        "    color = colorMatrix * color;\n"
        "    gl_FragColor = vec4(color.rgb, texture2D(texRgb, textureCoord.st).r);\n"
        "}\n";

// Paints RGB frames without doing any color channel flipping.
static const char *qt_glsl_rgbxShaderProgram =
        "uniform sampler2D texRgb;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "    highp vec4 color = vec4(texture2D(texRgb, textureCoord.st).rgb, 1.0);\n"
        "    gl_FragColor = colorMatrix * color;\n"
        "}\n";

// Paints planar yuv frames.
static const char *qt_glsl_yuvPlanarShaderProgram =
        "uniform sampler2D texY;\n"
        "uniform sampler2D texU;\n"
        "uniform sampler2D texV;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "    highp vec4 color = vec4(\n"
        "           texture2D(texY, textureCoord.st).r,\n"
        "           texture2D(texU, textureCoord.st).r,\n"
        "           texture2D(texV, textureCoord.st).r,\n"
        "           1.0);\n"
        "    gl_FragColor = colorMatrix * color;\n"
        "}\n";


GlslSurfacePainter::GlslSurfacePainter()
    : OpenGLSurfacePainter()
{
}

void GlslSurfacePainter::init(const BufferFormat &format)
{
    Q_ASSERT(m_textureCount == 0);

    const char *fragmentProgram = 0;

    switch (format.videoFormat()) {
    case GST_VIDEO_FORMAT_BGRx:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        fragmentProgram = qt_glsl_bgrxShaderProgram;
        break;
    case GST_VIDEO_FORMAT_xRGB:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        fragmentProgram = qt_glsl_xrgbShaderProgram;
        break;
    case GST_VIDEO_FORMAT_BGRA:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        fragmentProgram = qt_glsl_bgraShaderProgram;
        break;
    case GST_VIDEO_FORMAT_ARGB:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        fragmentProgram = qt_glsl_argbShaderProgram;
        break;
    case GST_VIDEO_FORMAT_RGB:
        initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, format.frameSize());
        fragmentProgram = qt_glsl_rgbxShaderProgram;
        break;
    case GST_VIDEO_FORMAT_BGR:
        initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, format.frameSize());
        fragmentProgram = qt_glsl_bgrxShaderProgram;
        break;
    //NOTE: unlike the other formats, this is endianness-dependent,
    //but using GL_UNSIGNED_SHORT_5_6_5 ensures that it's handled correctly
    case GST_VIDEO_FORMAT_RGB16:
        initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, format.frameSize());
        fragmentProgram = qt_glsl_rgbxShaderProgram;
        break;
    case GST_VIDEO_FORMAT_v308:
        initRgbTextureInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, format.frameSize());
        fragmentProgram = qt_glsl_rgbxShaderProgram;
        break;
    case GST_VIDEO_FORMAT_AYUV:
        initRgbTextureInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, format.frameSize());
        fragmentProgram = qt_glsl_argbShaderProgram;
        break;
    case GST_VIDEO_FORMAT_YV12:
        initYv12TextureInfo(format.frameSize());
        fragmentProgram = qt_glsl_yuvPlanarShaderProgram;
        break;
    case GST_VIDEO_FORMAT_I420:
        initYuv420PTextureInfo(format.frameSize());
        fragmentProgram = qt_glsl_yuvPlanarShaderProgram;
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    m_videoColorMatrix = format.colorMatrix();

    if (!m_program.addShaderFromSourceCode(QGLShader::Vertex, qt_glsl_vertexShaderProgram)) {
        throw QString("Vertex shader compile error ") + m_program.log();
    }

    if (!m_program.addShaderFromSourceCode(QGLShader::Fragment, fragmentProgram)) {
        throw QString("Shader compile error ") + m_program.log();
    }

    if(!m_program.link()) {
        throw QString("Shader link error ") + m_program.log();
    }

    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (funcs)
        funcs->glGenTextures(m_textureCount, m_textureIds);
}

void GlslSurfacePainter::cleanup()
{
    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (funcs)
    {
        funcs->glDeleteTextures(m_textureCount, m_textureIds);
        m_program.removeAllShaders();
    }
    m_textureCount = 0;
}

void GlslSurfacePainter::paintImpl(const QPainter *painter,
        const GLfloat *vertexCoordArray,
        const GLfloat *textureCoordArray)
{
    const int deviceWidth   = painter->device()->width();
    const int deviceHeight  = painter->device()->height();

    const QTransform transform = painter->deviceTransform();

    const GLfloat wfactor = 2.0 / deviceWidth;
    const GLfloat hfactor = -2.0 / deviceHeight;

    const GLfloat positionMatrix[4][4] =
    {
        {
            /*(0,0)*/ GLfloat(wfactor * transform.m11() - transform.m13()),
            /*(0,1)*/ GLfloat(hfactor * transform.m12() + transform.m13()),
            /*(0,2)*/ 0.0,
            /*(0,3)*/ GLfloat(transform.m13())
        }, {
            /*(1,0)*/ GLfloat(wfactor * transform.m21() - transform.m23()),
            /*(1,1)*/ GLfloat(hfactor * transform.m22() + transform.m23()),
            /*(1,2)*/ 0.0,
            /*(1,3)*/ GLfloat(transform.m23())
        }, {
            /*(2,0)*/ 0.0,
            /*(2,1)*/ 0.0,
            /*(2,2)*/ -1.0,
            /*(2,3)*/ 0.0
        }, {
            /*(3,0)*/ GLfloat(wfactor * transform.dx() - transform.m33()),
            /*(3,1)*/ GLfloat(hfactor * transform.dy() + transform.m33()),
            /*(3,2)*/ 0.0,
            /*(3,3)*/ GLfloat(transform.m33())
        }
    };

    m_program.bind();

    m_program.enableAttributeArray("vertexCoordArray");
    m_program.enableAttributeArray("textureCoordArray");
    m_program.setAttributeArray("vertexCoordArray", vertexCoordArray, 2);
    m_program.setAttributeArray("textureCoordArray", textureCoordArray, 2);
    m_program.setUniformValue("positionMatrix", positionMatrix);

    QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
    if (!funcs)
        return;

    if (m_textureCount == 3) {
        funcs->glActiveTexture(GL_TEXTURE0);
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
        funcs->glActiveTexture(GL_TEXTURE1);
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
        funcs->glActiveTexture(GL_TEXTURE2);
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);
        funcs->glActiveTexture(GL_TEXTURE0);

        m_program.setUniformValue("texY", 0);
        m_program.setUniformValue("texU", 1);
        m_program.setUniformValue("texV", 2);
    } else {
        funcs->glActiveTexture(GL_TEXTURE0);
        funcs->glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);

        m_program.setUniformValue("texRgb", 0);
    }
    m_program.setUniformValue("colorMatrix", m_colorMatrix);

    funcs->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program.release();
}
