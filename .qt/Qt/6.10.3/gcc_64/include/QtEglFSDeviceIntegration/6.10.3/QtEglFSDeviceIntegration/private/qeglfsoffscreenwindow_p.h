// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSOFFSCREENWINDOW_H
#define QEGLFSOFFSCREENWINDOW_H

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
#include <qpa/qplatformoffscreensurface.h>

QT_BEGIN_NAMESPACE

class Q_EGLFS_EXPORT QEglFSOffscreenWindow : public QPlatformOffscreenSurface
{
public:
    QEglFSOffscreenWindow(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface);
    ~QEglFSOffscreenWindow();

    QSurfaceFormat format() const override { return m_format; }
    bool isValid() const override { return m_surface != EGL_NO_SURFACE; }

private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLNativeWindowType m_window;
};

QT_END_NAMESPACE

#endif // QEGLFSOFFSCREENWINDOW_H
