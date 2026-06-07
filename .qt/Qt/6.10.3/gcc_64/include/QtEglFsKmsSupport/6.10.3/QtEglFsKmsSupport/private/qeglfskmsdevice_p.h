// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSDEVICE_H
#define QEGLFSKMSDEVICE_H

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

#include "private/qeglfsglobal_p.h"
#include <QtKmsSupport/private/qkmsdevice_p.h>

QT_BEGIN_NAMESPACE

class Q_EGLFS_EXPORT QEglFSKmsDevice : public QKmsDevice
{
public:
    QEglFSKmsDevice(QKmsScreenConfig *screenConfig, const QString &path);

    void registerScreen(QPlatformScreen *screen,
                        bool isPrimary,
                        const QPoint &virtualPos,
                        const QList<QPlatformScreen *> &virtualSiblings) override;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSDEVICE_H
