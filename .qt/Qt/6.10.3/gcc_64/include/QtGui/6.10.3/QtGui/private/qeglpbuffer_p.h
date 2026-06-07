// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLPBUFFER_H
#define QEGLPBUFFER_H

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

#include <qpa/qplatformoffscreensurface.h>
#include <QtGui/private/qeglplatformcontext_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QEGLPbuffer : public QPlatformOffscreenSurface
{
public:
    QEGLPbuffer(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface,
                QEGLPlatformContext::Flags flags = { });
    ~QEGLPbuffer();

    QSurfaceFormat format() const override { return m_format; }
    bool isValid() const override;

    EGLSurface pbuffer() const { return m_pbuffer; }

private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_pbuffer;
    bool m_hasSurfaceless;
};

QT_END_NAMESPACE

#endif // QEGLPBUFFER_H
