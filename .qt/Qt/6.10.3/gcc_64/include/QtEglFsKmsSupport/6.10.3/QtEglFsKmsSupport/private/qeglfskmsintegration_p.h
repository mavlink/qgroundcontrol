// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSINTEGRATION_H
#define QEGLFSKMSINTEGRATION_H

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

#include "private/qeglfsdeviceintegration_p.h"
#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QKmsDevice;
class QKmsScreenConfig;

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(qLcEglfsKmsDebug, Q_EGLFS_EXPORT)

class Q_EGLFS_EXPORT QEglFSKmsIntegration : public QEglFSDeviceIntegration
{
public:
    QEglFSKmsIntegration();
    ~QEglFSKmsIntegration();

    void platformInit() override;
    void platformDestroy() override;
    EGLNativeDisplayType platformDisplay() const override;
    bool usesDefaultScreen() override;
    void screenInit() override;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const override;
    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    void waitForVSync(QPlatformSurface *surface) const override;
    bool supportsPBuffers() const override;
    void *nativeResourceForIntegration(const QByteArray &name) override;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;

    QKmsDevice *device() const;
    QKmsScreenConfig *screenConfig() const;

protected:
    virtual QKmsDevice *createDevice() = 0;
    virtual QKmsScreenConfig *createScreenConfig();

    QKmsDevice *m_device;
    QKmsScreenConfig *m_screenConfig = nullptr;
};

QT_END_NAMESPACE

#endif
