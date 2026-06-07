// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLXCONVENIENCE_H
#define QGLXCONVENIENCE_H

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

#include <QtCore/qlist.h>
#include <QtGui/qsurfaceformat.h>
#include <QtCore/private/qglobal_p.h>

#include <X11/Xlib.h>
#include <GL/glx.h>

QT_BEGIN_NAMESPACE

enum QGlxFlags
{
    QGLX_SUPPORTS_SRGB = 0x01
};

Q_GUI_EXPORT QList<int> qglx_buildSpec(const QSurfaceFormat &format,
                                       int drawableBit = GLX_WINDOW_BIT,
                                       int flags = 0);

Q_GUI_EXPORT XVisualInfo *qglx_findVisualInfo(Display *display, int screen,
                                              QSurfaceFormat *format,
                                              int drawableBit = GLX_WINDOW_BIT,
                                              int flags = 0);

Q_GUI_EXPORT GLXFBConfig qglx_findConfig(Display *display, int screen,
                                         QSurfaceFormat format,
                                         bool highestPixelFormat = false,
                                         int drawableBit = GLX_WINDOW_BIT,
                                         int flags = 0);

Q_GUI_EXPORT void qglx_surfaceFormatFromGLXFBConfig(QSurfaceFormat *format, Display *display,
                                                    GLXFBConfig config, int flags = 0);

Q_GUI_EXPORT void qglx_surfaceFormatFromVisualInfo(QSurfaceFormat *format, Display *display,
                                                   XVisualInfo *visualInfo, int flags = 0);

Q_GUI_EXPORT bool qglx_reduceFormat(QSurfaceFormat *format);

QT_END_NAMESPACE

#endif // QGLXCONVENIENCE_H
