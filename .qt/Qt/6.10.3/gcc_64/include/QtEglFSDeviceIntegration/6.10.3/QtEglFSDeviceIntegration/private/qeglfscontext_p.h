// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSCONTEXT_H
#define QEGLFSCONTEXT_H

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

#include "qeglfsglobal_p.h"
#include "qeglfscursor_p.h"
#include <QtGui/private/qeglplatformcontext_p.h>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class Q_EGLFS_EXPORT QEglFSContext : public QEGLPlatformContext
{
public:
    using QEGLPlatformContext::QEGLPlatformContext;
    QEglFSContext() = default; // workaround for INTEGRITY compiler
    QEglFSContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                  EGLConfig *config);
    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) override;
    EGLSurface createTemporaryOffscreenSurface() override;
    void destroyTemporaryOffscreenSurface(EGLSurface surface) override;
    void runGLChecks() override;
    void swapBuffers(QPlatformSurface *surface) override;

    QEglFSCursorData cursorData;

private:
    EGLNativeWindowType m_tempWindow = 0;
};

QT_END_NAMESPACE

#endif // QEGLFSCONTEXT_H
