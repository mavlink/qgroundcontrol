// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKGRAPHICSDEVICE_P_H
#define QQUICKGRAPHICSDEVICE_P_H

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
#include "qquickgraphicsdevice.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickGraphicsDevicePrivate
{
public:
    static QQuickGraphicsDevicePrivate *get(QQuickGraphicsDevice *p) { return p->d; }
    static const QQuickGraphicsDevicePrivate *get(const QQuickGraphicsDevice *p) { return p->d; }
    QQuickGraphicsDevicePrivate();
    QQuickGraphicsDevicePrivate(const QQuickGraphicsDevicePrivate &other);

    enum class Type {
        Null,
        OpenGLContext,
        Adapter,
        DeviceAndContext,
        DeviceAndCommandQueue,
        PhysicalDevice,
        DeviceObjects,
        Rhi,
        RhiAdapter
    };

    QAtomicInt ref;
    Type type = Type::Null;

    struct Adapter {
        quint32 luidLow;
        qint32 luidHigh;
        int featureLevel;
    };

    struct DeviceAndContext {
        void *device;
        void *context;
    };

    struct DeviceAndCommandQueue {
        void *device;
        void *cmdQueue;
    };

    struct PhysicalDevice {
        void *physicalDevice;
    };

    struct DeviceObjects {
        void *physicalDevice;
        void *device;
        int queueFamilyIndex;
        int queueIndex;
    };

    union {
        QOpenGLContext *context;
        Adapter adapter;
        DeviceAndContext deviceAndContext;
        DeviceAndCommandQueue deviceAndCommandQueue;
        PhysicalDevice physicalDevice;
        DeviceObjects deviceObjects;
        QRhi *rhi;
        QRhiAdapter *rhiAdapter;
    } u;
};

QT_END_NAMESPACE

#endif // QQUICKGRAPHICSDEVICE_P_H
