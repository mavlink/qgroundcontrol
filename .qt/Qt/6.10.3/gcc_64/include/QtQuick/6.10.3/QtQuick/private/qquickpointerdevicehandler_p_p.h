// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOINTERDEVICEHANDLER_P_H
#define QQUICKPOINTERDEVICEHANDLER_P_H

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

#include "qquickpointerdevicehandler_p.h"
#include "qquickpointerhandler_p_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickPointerDeviceHandlerPrivate : public QQuickPointerHandlerPrivate
{
    Q_DECLARE_PUBLIC(QQuickPointerDeviceHandler)

public:
    static QQuickPointerDeviceHandlerPrivate* get(QQuickPointerDeviceHandler *q) { return q->d_func(); }
    static const QQuickPointerDeviceHandlerPrivate* get(const QQuickPointerDeviceHandler *q) { return q->d_func(); }

    QPointingDevice::DeviceTypes acceptedDevices = QPointingDevice::DeviceType::AllDevices;
    QPointingDevice::PointerTypes acceptedPointerTypes = QPointingDevice::PointerType::AllPointerTypes;
    Qt::MouseButtons acceptedButtons = Qt::LeftButton;
    Qt::KeyboardModifiers acceptedModifiers = Qt::KeyboardModifierMask;
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERDEVICEHANDLER_P_H
