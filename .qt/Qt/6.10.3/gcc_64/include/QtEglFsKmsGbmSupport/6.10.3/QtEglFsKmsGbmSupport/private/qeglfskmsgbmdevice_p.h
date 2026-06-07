// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSGBMDEVICE_H
#define QEGLFSKMSGBMDEVICE_H

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

#include "qeglfskmsgbmcursor_p.h"
#include <private/qeglfskmsdevice_p.h>
#include <private/qeglfskmseventreader_p.h>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsScreen;

class Q_EGLFS_EXPORT QEglFSKmsGbmDevice: public QEglFSKmsDevice
{
public:
    QEglFSKmsGbmDevice(QKmsScreenConfig *screenConfig, const QString &path);

    bool open() override;
    void close() override;

    void *nativeDisplay() const override;
    gbm_device *gbmDevice() const;

    QPlatformCursor *globalCursor() const;
    void destroyGlobalCursor();
    void createGlobalCursor(QEglFSKmsGbmScreen *screen);

    QPlatformScreen *createScreen(const QKmsOutput &output) override;
    QPlatformScreen *createHeadlessScreen() override;
    void registerScreenCloning(QPlatformScreen *screen,
                               QPlatformScreen *screenThisScreenClones,
                               const QList<QPlatformScreen *> &screensCloningThisScreen) override;
    void registerScreen(QPlatformScreen *screen,
                        bool isPrimary,
                        const QPoint &virtualPos,
                        const QList<QPlatformScreen *> &virtualSiblings) override;

    bool usesEventReader() const;
    QEglFSKmsEventReader *eventReader() { return &m_eventReader; }

private:
    Q_DISABLE_COPY(QEglFSKmsGbmDevice)

    gbm_device *m_gbm_device;
    QEglFSKmsEventReader m_eventReader;
    QEglFSKmsGbmCursor *m_globalCursor;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMDEVICE_H
