// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLTEXTUREBLITTER_H
#define QOPENGLTEXTUREBLITTER_H

#include <QtOpenGL/qtopenglglobal.h>

#include <QtGui/qopengl.h>
#include <QtGui/QMatrix3x3>
#include <QtGui/QMatrix4x4>

QT_BEGIN_NAMESPACE

class QOpenGLTextureBlitterPrivate;

class Q_OPENGL_EXPORT QOpenGLTextureBlitter
{
public:
    QOpenGLTextureBlitter();
    ~QOpenGLTextureBlitter();

    enum Origin {
        OriginBottomLeft,
        OriginTopLeft
    };

    bool create();
    bool isCreated() const;
    void destroy();

    bool supportsExternalOESTarget() const;
    bool supportsRectangleTarget() const;

    void bind(GLenum target = GL_TEXTURE_2D);
    void release();

    void setRedBlueSwizzle(bool swizzle);
    void setOpacity(float opacity);

    void blit(GLuint texture, const QMatrix4x4 &targetTransform, Origin sourceOrigin);
    void blit(GLuint texture, const QMatrix4x4 &targetTransform, const QMatrix3x3 &sourceTransform);

    static QMatrix4x4 targetTransform(const QRectF &target, const QRect &viewport);
    static QMatrix3x3 sourceTransform(const QRectF &subTexture, const QSize &textureSize, Origin origin);

private:
    Q_DISABLE_COPY(QOpenGLTextureBlitter)
    Q_DECLARE_PRIVATE(QOpenGLTextureBlitter)
    QScopedPointer<QOpenGLTextureBlitterPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif //QOPENGLTEXTUREBLITTER_H
