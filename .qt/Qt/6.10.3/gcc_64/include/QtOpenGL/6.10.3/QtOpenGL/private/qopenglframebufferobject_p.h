// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLFRAMEBUFFEROBJECT_P_H
#define QOPENGLFRAMEBUFFEROBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qvarlengtharray.h>
#include <qopenglframebufferobject.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglextensions_p.h>

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObjectFormatPrivate
{
public:
    QOpenGLFramebufferObjectFormatPrivate()
        : ref(1),
          samples(0),
          attachment(QOpenGLFramebufferObject::NoAttachment),
          target(GL_TEXTURE_2D),
          mipmap(false)
    {
#if !QT_CONFIG(opengles2)
        // There is nothing that says QOpenGLFramebufferObjectFormat needs a current
        // context, so we need a fallback just to be safe, even though in practice there
        // will usually be a current context.
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        const bool isES = ctx ? ctx->isOpenGLES() : QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL;
        internal_format = isES ? GL_RGBA : GL_RGBA8;
#else
        internal_format = GL_RGBA;
#endif
    }
    QOpenGLFramebufferObjectFormatPrivate
            (const QOpenGLFramebufferObjectFormatPrivate *other)
        : ref(1),
          samples(other->samples),
          attachment(other->attachment),
          target(other->target),
          internal_format(other->internal_format),
          mipmap(other->mipmap)
    {
    }
    bool equals(const QOpenGLFramebufferObjectFormatPrivate *other)
    {
        return samples == other->samples &&
               attachment == other->attachment &&
               target == other->target &&
               internal_format == other->internal_format &&
               mipmap == other->mipmap;
    }

    QAtomicInt ref;
    int samples;
    QOpenGLFramebufferObject::Attachment attachment;
    GLenum target;
    GLenum internal_format;
    uint mipmap : 1;
};

class QOpenGLFramebufferObjectPrivate
{
public:
    QOpenGLFramebufferObjectPrivate() : fbo_guard(nullptr), depth_buffer_guard(nullptr)
                                  , stencil_buffer_guard(nullptr)
                                  , valid(false) {}
    ~QOpenGLFramebufferObjectPrivate() {}

    void init(QOpenGLFramebufferObject *q, const QSize &size,
              QOpenGLFramebufferObject::Attachment attachment,
              GLenum texture_target, GLenum internal_format,
              GLint samples = 0, bool mipmap = false);
    void initTexture(int idx);
    void initColorBuffer(int idx, GLint *samples);
    void initDepthStencilAttachments(QOpenGLContext *ctx, QOpenGLFramebufferObject::Attachment attachment);

    bool checkFramebufferStatus(QOpenGLContext *ctx) const;
    QOpenGLSharedResourceGuard *fbo_guard;
    QOpenGLSharedResourceGuard *depth_buffer_guard;
    QOpenGLSharedResourceGuard *stencil_buffer_guard;
    GLenum target;
    QSize dsSize;
    QOpenGLFramebufferObjectFormat format;
    int requestedSamples;
    uint valid : 1;
    QOpenGLFramebufferObject::Attachment fbo_attachment;
    QOpenGLExtensions funcs;

    struct ColorAttachment {
        ColorAttachment() : internalFormat(0), guard(nullptr) { }
        ColorAttachment(const QSize &size, GLenum internalFormat)
            : size(size), internalFormat(internalFormat), guard(nullptr) { }
        QSize size;
        GLenum internalFormat;
        QOpenGLSharedResourceGuard *guard;
    };
    QVarLengthArray<ColorAttachment, 8> colorAttachments;

    inline GLuint fbo() const { return fbo_guard ? fbo_guard->id() : 0; }
};

Q_OPENGL_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

QT_END_NAMESPACE

#endif // QOPENGLFRAMEBUFFEROBJECT_P_H
