// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMOPENGLCONTEXT_H
#define QPLATFORMOPENGLCONTEXT_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qnamespace.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qsurfaceformat.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopengl.h>
#include <QtGui/qopenglcontext.h>

#include <QtCore/qnativeinterface.h>

QT_BEGIN_NAMESPACE


class QPlatformOpenGLContextPrivate;

class Q_GUI_EXPORT QPlatformOpenGLContext
{
    Q_DECLARE_PRIVATE(QPlatformOpenGLContext)
public:
    QPlatformOpenGLContext();
    virtual ~QPlatformOpenGLContext();

    virtual void initialize();

    virtual QSurfaceFormat format() const = 0;

    virtual void swapBuffers(QPlatformSurface *surface) = 0;

    virtual GLuint defaultFramebufferObject(QPlatformSurface *surface) const;

    virtual bool makeCurrent(QPlatformSurface *surface) = 0;
    virtual void doneCurrent() = 0;

    virtual void beginFrame();
    virtual void endFrame();

    virtual bool isSharing() const { return false; }
    virtual bool isValid() const { return true; }

    virtual QFunctionPointer getProcAddress(const char *procName) = 0;

    QOpenGLContext *context() const;

    static bool parseOpenGLVersion(const QByteArray &versionString, int &major, int &minor);

private:
    friend class QOpenGLContext;
    friend class QOpenGLContextPrivate;

    QScopedPointer<QPlatformOpenGLContextPrivate> d_ptr;

    void setContext(QOpenGLContext *context);

    Q_DISABLE_COPY(QPlatformOpenGLContext)
};

namespace QNativeInterface::Private {

#if defined(Q_OS_MACOS)
struct Q_GUI_EXPORT QCocoaGLIntegration
{
    QT_DECLARE_NATIVE_INTERFACE(QCocoaGLIntegration)
    virtual QOpenGLContext *createOpenGLContext(NSOpenGLContext *, QOpenGLContext *shareContext) const = 0;
};
#endif

#if defined(Q_OS_WIN)
struct Q_GUI_EXPORT QWindowsGLIntegration
{
    QT_DECLARE_NATIVE_INTERFACE(QWindowsGLIntegration)
    virtual HMODULE openGLModuleHandle() const = 0;
    virtual QOpenGLContext *createOpenGLContext(HGLRC context, HWND window, QOpenGLContext *shareContext) const = 0;
};
#endif

#if QT_CONFIG(xcb_glx_plugin)
struct Q_GUI_EXPORT QGLXIntegration
{
    QT_DECLARE_NATIVE_INTERFACE(QGLXIntegration)
    virtual QOpenGLContext *createOpenGLContext(GLXContext context, void *visualInfo, QOpenGLContext *shareContext) const = 0;
};
#endif

#if QT_CONFIG(egl)
struct Q_GUI_EXPORT QEGLIntegration
{
    QT_DECLARE_NATIVE_INTERFACE(QEGLIntegration)
    virtual QOpenGLContext *createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const = 0;
};
#endif

} // QNativeInterface::Private

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QPLATFORMOPENGLCONTEXT_H
