// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSGBMINTEGRATION_H
#define QEGLFSKMSGBMINTEGRATION_H

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

#include <private/qeglfskmsintegration_p.h>
#include <QtCore/QMap>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;

class Q_EGLFS_EXPORT QEglFSKmsGbmIntegration : public QEglFSKmsIntegration
{
public:
    QEglFSKmsGbmIntegration();

    EGLDisplay createDisplay(EGLNativeDisplayType nativeDisplay) override;
    EGLNativeWindowType createNativeOffscreenWindow(const QSurfaceFormat &format) override;
    void destroyNativeWindow(EGLNativeWindowType window) override;

    QPlatformCursor *createCursor(QPlatformScreen *screen) const override;
    void presentBuffer(QPlatformSurface *surface) override;
    QEglFSWindow *createWindow(QWindow *window) const override;

protected:
    QKmsDevice *createDevice() override;

private:
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMINTEGRATION_H
