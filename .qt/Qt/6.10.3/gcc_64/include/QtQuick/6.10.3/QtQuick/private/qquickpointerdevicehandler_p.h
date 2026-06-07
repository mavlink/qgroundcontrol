// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include "qquickpointerhandler_p.h"

#ifndef QQUICKPOINTERDEVICEHANDLER_H
#define QQUICKPOINTERDEVICEHANDLER_H

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

QT_BEGIN_NAMESPACE

class QQuickPointerDeviceHandlerPrivate;

class Q_QUICK_EXPORT QQuickPointerDeviceHandler : public QQuickPointerHandler
{
    Q_OBJECT
    Q_PROPERTY(QInputDevice::DeviceTypes acceptedDevices READ acceptedDevices WRITE
                       setAcceptedDevices NOTIFY acceptedDevicesChanged)
    Q_PROPERTY(QPointingDevice::PointerTypes acceptedPointerTypes READ acceptedPointerTypes WRITE setAcceptedPointerTypes NOTIFY acceptedPointerTypesChanged)
    Q_PROPERTY(Qt::MouseButtons acceptedButtons READ acceptedButtons WRITE setAcceptedButtons NOTIFY acceptedButtonsChanged)
    Q_PROPERTY(Qt::KeyboardModifiers acceptedModifiers READ acceptedModifiers WRITE setAcceptedModifiers NOTIFY acceptedModifiersChanged)

public:
    explicit QQuickPointerDeviceHandler(QQuickItem *parent = nullptr);

    QInputDevice::DeviceTypes acceptedDevices() const;
    QPointingDevice::PointerTypes acceptedPointerTypes() const;
    Qt::MouseButtons acceptedButtons() const;
    Qt::KeyboardModifiers acceptedModifiers() const;

public Q_SLOTS:
    void setAcceptedDevices(QInputDevice::DeviceTypes acceptedDevices);
    void setAcceptedPointerTypes(QPointingDevice::PointerTypes acceptedPointerTypes);
    void setAcceptedButtons(Qt::MouseButtons buttons);
    void setAcceptedModifiers(Qt::KeyboardModifiers acceptedModifiers);

Q_SIGNALS:
    void acceptedDevicesChanged();
    void acceptedPointerTypesChanged();
    void acceptedButtonsChanged();
    void acceptedModifiersChanged();

protected:
    QQuickPointerDeviceHandler(QQuickPointerDeviceHandlerPrivate &dd, QQuickItem *parent = nullptr);

    bool wantsPointerEvent(QPointerEvent *event) override;

    Q_DECLARE_PRIVATE(QQuickPointerDeviceHandler)
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERDEVICEHANDLER_H
