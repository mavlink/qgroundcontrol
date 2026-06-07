// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDCLIENTBUFFERINTEGRATION_H
#define QWAYLANDCLIENTBUFFERINTEGRATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtWaylandClient/qtwaylandclientglobal.h>
#if QT_CONFIG(opengl)
#include <QtGui/private/qeglplatformcontext_p.h>
#endif

QT_BEGIN_NAMESPACE

class QWindow;
#if QT_CONFIG(opengl)
class QOpenGLContext;
#endif
class QPlatformOpenGLContext;
class QSurfaceFormat;

namespace QtWaylandClient {

class QWaylandWindow;
class QWaylandDisplay;

class Q_WAYLANDCLIENT_EXPORT QWaylandClientBufferIntegration
{
public:
    QWaylandClientBufferIntegration();
    virtual ~QWaylandClientBufferIntegration();

    virtual void initialize(QWaylandDisplay *display) = 0;

    virtual bool isValid() const { return true; }

    virtual bool supportsThreadedOpenGL() const { return false; }
    virtual bool supportsWindowDecoration() const { return false; }

    virtual QWaylandWindow *createEglWindow(QWindow *window) = 0;
    virtual QPlatformOpenGLContext *createPlatformOpenGLContext(const QSurfaceFormat &glFormat, QPlatformOpenGLContext *share) const = 0;
#if QT_CONFIG(opengl)
    virtual QOpenGLContext *createOpenGLContext(EGLContext context, EGLDisplay contextDisplay, QOpenGLContext *shareContext) const = 0;
#endif

    enum NativeResource {
        EglDisplay,
        EglConfig,
        EglContext
    };
    virtual void *nativeResource(NativeResource /*resource*/) { return nullptr; }
    virtual void *nativeResourceForContext(NativeResource /*resource*/, QPlatformOpenGLContext */*context*/) { return nullptr; }
};

}

QT_END_NAMESPACE

#endif // QWAYLANDCLIENTBUFFERINTEGRATION_H
