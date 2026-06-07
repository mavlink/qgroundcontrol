// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSSCREEN_H
#define QEGLFSSCREEN_H

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
#include <QtCore/QPointer>

#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class QEglFSWindow;
class QOpenGLContext;

class Q_EGLFS_EXPORT QEglFSScreen : public QPlatformScreen
{
public:
    QEglFSScreen(EGLDisplay display);
    ~QEglFSScreen();

    QRect geometry() const override;
    virtual QRect rawGeometry() const;
    int depth() const override;
    QImage::Format format() const override;

    QSizeF physicalSize() const override;
    QDpi logicalDpi() const override;
    QDpi logicalBaseDpi() const override;
    Qt::ScreenOrientation nativeOrientation() const override;
    Qt::ScreenOrientation orientation() const override;

    QPlatformCursor *cursor() const override;

    qreal refreshRate() const override;

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

    EGLSurface primarySurface() const { return m_surface; }

    EGLDisplay display() const { return m_dpy; }

    void handleCursorMove(const QPoint &pos);

    QWindow *topLevelAt(const QPoint &point) const override;

private:
    void setPrimarySurface(EGLSurface surface);

    EGLDisplay m_dpy;
    EGLSurface m_surface;
    QPlatformCursor *m_cursor;

    friend class QEglFSWindow;
};

QT_END_NAMESPACE

#endif // QEGLFSSCREEN_H
