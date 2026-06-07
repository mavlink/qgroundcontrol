// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QINPUTDEVICE_P_H
#define QINPUTDEVICE_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qinputdevice.h>
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QInputDevicePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QInputDevice)
public:
    QInputDevicePrivate(const QString &name, qint64 winSysId, QInputDevice::DeviceType type,
                        QInputDevice::Capabilities caps = QInputDevice::Capability::None,
                        const QString &seatName = QString())
      : name(name), seatName(seatName), systemId(winSysId), capabilities(caps),
        deviceType(type)
    {
        // if the platform doesn't provide device IDs, make one up,
        // but try to avoid clashing with OS-provided 32-bit IDs
        static qint64 nextId = qint64(1) << 33;
        if (!systemId)
            systemId = nextId++;
    }
    ~QInputDevicePrivate() override;

    QString name;
    QString seatName;
    QString busId;
    QRect availableVirtualGeometry;
    void *qqExtra = nullptr;    // Qt Quick can store arbitrary device-specific data here
    qint64 systemId = 0;
    QInputDevice::Capabilities capabilities = QInputDevice::Capability::None;
    QInputDevice::DeviceType deviceType = QInputDevice::DeviceType::Unknown;
    bool pointingDeviceType = false;

    static void registerDevice(const QInputDevice *dev);
    static void unregisterDevice(const QInputDevice *dev);
    static bool isRegistered(const QInputDevice *dev);
    static const QInputDevice *fromId(qint64 systemId);

    void setAvailableVirtualGeometry(QRect a)
    {
        if (a == availableVirtualGeometry)
            return;

        availableVirtualGeometry = a;
        setCapabilities(capabilities | QInputDevice::Capability::NormalizedPosition);
        Q_Q(QInputDevice);
        Q_EMIT q->availableVirtualGeometryChanged(availableVirtualGeometry);
    }

    void setCapabilities(QInputDevice::Capabilities c)
    {
        if (c == capabilities)
            return;

        capabilities = c;
        Q_Q(QInputDevice);
        Q_EMIT q->capabilitiesChanged(capabilities);
    }

    inline static QInputDevicePrivate *get(QInputDevice *q)
    {
        return static_cast<QInputDevicePrivate *>(QObjectPrivate::get(q));
    }

    inline static const QInputDevicePrivate *get(const QInputDevice *q)
    {
        return static_cast<const QInputDevicePrivate *>(QObjectPrivate::get(q));
    }
};

QT_END_NAMESPACE

#endif // QINPUTDEVICE_P_H
