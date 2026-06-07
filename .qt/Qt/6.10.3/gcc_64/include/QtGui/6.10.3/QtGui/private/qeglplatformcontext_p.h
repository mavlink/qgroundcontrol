// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLPLATFORMCONTEXT_H
#define QEGLPLATFORMCONTEXT_H

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

#include <QtCore/qtextstream.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformopenglcontext.h>
#include <QtCore/qvariant.h>
#include <QtGui/private/qt_egl_p.h>
#include <QtGui/private/qopenglcontext_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QEGLPlatformContext : public QPlatformOpenGLContext,
                                            public QNativeInterface::QEGLContext
{
public:
    enum Flag {
        NoSurfaceless = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                        EGLConfig *config = nullptr, Flags flags = { });

    template <typename T>
    static QOpenGLContext *createFrom(EGLContext context, EGLDisplay contextDisplay,
            EGLDisplay platformDisplay, QOpenGLContext *shareContext)
    {
        if (!context)
            return nullptr;

        // A context belonging to a given EGLDisplay cannot be used with another one
        if (contextDisplay != platformDisplay) {
            qWarning("QEGLPlatformContext: Cannot adopt context from different display");
            return nullptr;
        }

        QPlatformOpenGLContext *shareHandle = shareContext ? shareContext->handle() : nullptr;

        auto *resultingContext = new QOpenGLContext;
        auto *contextPrivate = QOpenGLContextPrivate::get(resultingContext);
        auto *platformContext = new T;
        platformContext->adopt(context, contextDisplay, shareHandle);
        contextPrivate->adopt(platformContext);
        return resultingContext;
    }

    ~QEGLPlatformContext();

    void initialize() override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    void swapBuffers(QPlatformSurface *surface) override;
    QFunctionPointer getProcAddress(const char *procName) override;

    QSurfaceFormat format() const override;
    bool isSharing() const override { return m_shareContext != EGL_NO_CONTEXT; }
    bool isValid() const override { return m_eglContext != EGL_NO_CONTEXT && !m_markedInvalid; }

    EGLContext nativeContext() const override { return eglContext(); }
    EGLConfig config() const override { return eglConfig(); }
    EGLDisplay display() const override { return eglDisplay(); }

    virtual void invalidateContext() override { m_markedInvalid = true; }

    EGLContext eglContext() const;
    EGLDisplay eglDisplay() const;
    EGLConfig eglConfig() const;

protected:
    QEGLPlatformContext() {} // For adoption
    virtual EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) = 0;
    virtual EGLSurface createTemporaryOffscreenSurface();
    virtual void destroyTemporaryOffscreenSurface(EGLSurface surface);
    virtual void runGLChecks();
    bool checkGraphicsReset();

private:
    QList<EGLint> buildContextAttributes(const QSurfaceFormat &format);
    void adopt(EGLContext context, EGLDisplay display, QPlatformOpenGLContext *shareContext);
    void updateFormatFromGL();

#ifndef QT_NO_OPENGL
    bool hasExtension(const char *name);
#endif

    EGLContext m_eglContext;
    EGLContext m_shareContext;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    QSurfaceFormat m_format;
    EGLenum m_api;
    int m_swapInterval = -1;
    bool m_swapIntervalEnvChecked = false;
    int m_swapIntervalFromEnv = -1;
    Flags m_flags;
    bool m_ownsContext = false;
    QList<EGLint> m_contextAttrs;

    bool m_markedInvalid = false;
    GLenum (QOPENGLF_APIENTRYP m_getGraphicsResetStatus)() = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QEGLPlatformContext::Flags)

QT_END_NAMESPACE

#endif //QEGLPLATFORMCONTEXT_H
