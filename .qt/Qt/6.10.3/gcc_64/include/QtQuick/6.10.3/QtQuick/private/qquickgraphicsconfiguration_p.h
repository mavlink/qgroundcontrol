// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKGRAPHICSCONFIGURATION_P_H
#define QQUICKGRAPHICSCONFIGURATION_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QAtomicInt>
#include "qquickgraphicsconfiguration.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickGraphicsConfigurationPrivate
{
public:
    static QQuickGraphicsConfigurationPrivate *get(QQuickGraphicsConfiguration *p) { return p->d; }
    static const QQuickGraphicsConfigurationPrivate *get(const QQuickGraphicsConfiguration *p) { return p->d; }
    QQuickGraphicsConfigurationPrivate();
    QQuickGraphicsConfigurationPrivate(const QQuickGraphicsConfigurationPrivate &other);

    enum Flag {
        UseDepthBufferFor2D = 0x01,
        EnableDebugLayer = 0x02,
        EnableDebugMarkers = 0x04,
        PreferSoftwareDevice = 0x08,
        AutoPipelineCache = 0x10,
        EnableTimestamps = 0x20
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QAtomicInt ref;
    QByteArrayList deviceExtensions;
    Flags flags;
    QString pipelineCacheSaveFile;
    QString pipelineCacheLoadFile;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickGraphicsConfigurationPrivate::Flags)

QT_END_NAMESPACE

#endif // QQUICKGRAPHICSCONFIGURATION_P_H
