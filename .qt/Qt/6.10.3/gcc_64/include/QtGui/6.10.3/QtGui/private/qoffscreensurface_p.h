// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOFFSCREENSURFACE_P_H
#define QOFFSCREENSURFACE_P_H

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

#include "qplatformoffscreensurface.h"

#include <private/qwindow_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QOffscreenSurfacePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOffscreenSurface)

public:
    QOffscreenSurfacePrivate()
        : QObjectPrivate()
          , surfaceType(QSurface::OpenGLSurface)
          , platformOffscreenSurface(nullptr)
          , offscreenWindow(nullptr)
          , requestedFormat(QSurfaceFormat::defaultFormat())
          , screen(nullptr)
          , size(1, 1)
    {
    }

    ~QOffscreenSurfacePrivate()
    {
    }

    static QOffscreenSurfacePrivate *get(QOffscreenSurface *surface)
    {
        return surface ? surface->d_func() : nullptr;
    }

    QSurface::SurfaceType surfaceType;
    QPlatformOffscreenSurface *platformOffscreenSurface;
    QWindow *offscreenWindow;
    QSurfaceFormat requestedFormat;
    QScreen *screen;
    QSize size;
};

QT_END_NAMESPACE

#endif // QOFFSCREENSURFACE_P_H
