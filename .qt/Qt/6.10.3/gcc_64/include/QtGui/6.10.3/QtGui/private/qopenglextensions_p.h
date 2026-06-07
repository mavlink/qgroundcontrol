// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGL_EXTENSIONS_P_H
#define QOPENGL_EXTENSIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qopenglextrafunctions.h"

QT_BEGIN_NAMESPACE

class QOpenGLExtensionsPrivate;

class Q_GUI_EXPORT QOpenGLExtensions : public QOpenGLExtraFunctions
{
    Q_DECLARE_PRIVATE(QOpenGLExtensions)
public:
    QOpenGLExtensions();
    QOpenGLExtensions(QOpenGLContext *context);
    ~QOpenGLExtensions() {}

    enum OpenGLExtension {
        TextureRectangle        = 0x00000001,
        GenerateMipmap          = 0x00000002,
        TextureCompression      = 0x00000004,
        MirroredRepeat          = 0x00000008,
        FramebufferMultisample  = 0x00000010,
        StencilTwoSide          = 0x00000020,
        StencilWrap             = 0x00000040,
        PackedDepthStencil      = 0x00000080,
        NVFloatBuffer           = 0x00000100,
        PixelBufferObject       = 0x00000200,
        FramebufferBlit         = 0x00000400,
        BGRATextureFormat       = 0x00000800,
        DDSTextureCompression   = 0x00001000,
        ETC1TextureCompression  = 0x00002000,
        PVRTCTextureCompression = 0x00004000,
        ElementIndexUint        = 0x00008000,
        Depth24                 = 0x00010000,
        SRGBFrameBuffer         = 0x00020000,
        MapBuffer               = 0x00040000,
        GeometryShaders         = 0x00080000,
        MapBufferRange          = 0x00100000,
        Sized8Formats           = 0x00200000,
        DiscardFramebuffer      = 0x00400000,
        Sized16Formats          = 0x00800000,
        TextureSwizzle          = 0x01000000,
        StandardDerivatives     = 0x02000000,
        ASTCTextureCompression  = 0x04000000,
        ETC2TextureCompression  = 0x08000000,
        HalfFloatVertex         = 0x10000000,
        MultiView               = 0x20000000,
        MultiViewExtended       = 0x40000000
    };
    Q_DECLARE_FLAGS(OpenGLExtensions, OpenGLExtension)

    OpenGLExtensions openGLExtensions();
    bool hasOpenGLExtension(QOpenGLExtensions::OpenGLExtension extension) const;

    GLvoid *glMapBuffer(GLenum target, GLenum access);
    void glGetBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data);

    void flushShared();
    void discardFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments);

    QOpenGLExtensionsPrivate *d() const;

private:
    static bool isInitialized(const QOpenGLFunctionsPrivate *d) { return d != nullptr; }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QOpenGLExtensions::OpenGLExtensions)

class QOpenGLExtensionsPrivate : public QOpenGLExtraFunctionsPrivate
{
public:
    explicit QOpenGLExtensionsPrivate(QOpenGLContext *ctx);

    GLvoid* (QOPENGLF_APIENTRYP MapBuffer)(GLenum target, GLenum access);
    void (QOPENGLF_APIENTRYP GetBufferSubData)(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data);
    void (QOPENGLF_APIENTRYP DiscardFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments);

    bool flushVendorChecked;
    bool flushIsSufficientToSyncContexts;
};

inline QOpenGLExtensionsPrivate *QOpenGLExtensions::d() const
{
    return static_cast<QOpenGLExtensionsPrivate *>(d_ptr);
}

inline GLvoid *QOpenGLExtensions::glMapBuffer(GLenum target, GLenum access)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    GLvoid *result = d->MapBuffer(target, access);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtensions::glGetBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    d->GetBufferSubData(target, offset, size, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

QT_END_NAMESPACE

#endif // QOPENGL_EXTENSIONS_P_H
