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

#ifndef OPENGLSURFACEPAINTER_H
#define OPENGLSURFACEPAINTER_H

#ifndef GST_QT_VIDEO_SINK_NO_OPENGL

#include "abstractsurfacepainter.h"
#include <QGLShaderProgram>

#ifndef Q_WS_MAC
# ifndef APIENTRYP
#   ifdef APIENTRY
#     define APIENTRYP APIENTRY *
#   else
#     define APIENTRY
#     define APIENTRYP *
#   endif
# endif
#else
# define APIENTRY
# define APIENTRYP *
#endif

class OpenGLSurfacePainter : public AbstractSurfacePainter
{
public:
    OpenGLSurfacePainter();

    static QSet<GstVideoFormat> supportedPixelFormats();

    virtual bool supportsFormat(GstVideoFormat format) const {
        return supportedPixelFormats().contains(format);
    }

    virtual void updateColors(int brightness, int contrast, int hue, int saturation);
    virtual void paint(quint8 *data, const BufferFormat & frameFormat,
                       QPainter *painter, const PaintAreas & areas);

protected:
    void initRgbTextureInfo(GLenum internalFormat, GLuint format, GLenum type, const QSize &size);
    void initYuv420PTextureInfo(const QSize &size);
    void initYv12TextureInfo(const QSize &size);

    virtual void paintImpl(const QPainter *painter,
                           const GLfloat *vertexCoordArray,
                           const GLfloat *textureCoordArray) = 0;

#ifndef QT_OPENGL_ES
    typedef void (APIENTRY *_glActiveTexture) (GLenum);
    _glActiveTexture glActiveTexture;
#endif

    GLenum m_textureFormat;
    GLuint m_textureInternalFormat;
    GLenum m_textureType;
    int m_textureCount;
    GLuint m_textureIds[3];
    int m_textureWidths[3];
    int m_textureHeights[3];
    int m_textureOffsets[3];

    QMatrix4x4 m_colorMatrix;
    GstVideoColorMatrix m_videoColorMatrix;
};

#ifndef QT_OPENGL_ES

class ArbFpSurfacePainter : public OpenGLSurfacePainter
{
public:
    ArbFpSurfacePainter();

    virtual void init(const BufferFormat & format);
    virtual void cleanup();

protected:
    virtual void paintImpl(const QPainter *painter,
                           const GLfloat *vertexCoordArray,
                           const GLfloat *textureCoordArray);

private:
    typedef void (APIENTRY *_glProgramStringARB) (GLenum, GLenum, GLsizei, const GLvoid *);
    typedef void (APIENTRY *_glBindProgramARB) (GLenum, GLuint);
    typedef void (APIENTRY *_glDeleteProgramsARB) (GLsizei, const GLuint *);
    typedef void (APIENTRY *_glGenProgramsARB) (GLsizei, GLuint *);
    typedef void (APIENTRY *_glProgramLocalParameter4fARB) (
            GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
    typedef void (APIENTRY *_glActiveTexture) (GLenum);

    _glProgramStringARB glProgramStringARB;
    _glBindProgramARB glBindProgramARB;
    _glDeleteProgramsARB glDeleteProgramsARB;
    _glGenProgramsARB glGenProgramsARB;
    _glProgramLocalParameter4fARB glProgramLocalParameter4fARB;

    GLuint m_programId;
};

#endif

class GlslSurfacePainter : public OpenGLSurfacePainter
{
public:
    GlslSurfacePainter();

    virtual void init(const BufferFormat & format);
    virtual void cleanup();

protected:
    virtual void paintImpl(const QPainter *painter,
                           const GLfloat *vertexCoordArray,
                           const GLfloat *textureCoordArray);

private:
    QGLShaderProgram m_program;
};

#endif // GST_QT_VIDEO_SINK_NO_OPENGL

#endif // OPENGLSURFACEPAINTER_H
