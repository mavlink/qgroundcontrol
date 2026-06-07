// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVDEVTABLETMANAGER_P_H
#define QEVDEVTABLETMANAGER_P_H

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

#include <QtInputSupport/private/devicehandlerlist_p.h>

#include <QObject>
#include <QHash>
#include <QSocketNotifier>

QT_BEGIN_NAMESPACE

class QDeviceDiscovery;
class QEvdevTabletHandlerThread;

class QEvdevTabletManager : public QObject
{
public:
    QEvdevTabletManager(const QString &key, const QString &spec, QObject *parent = nullptr);
    ~QEvdevTabletManager();

    void addDevice(const QString &deviceNode);
    void removeDevice(const QString &deviceNode);

private:
    void updateDeviceCount();

    QString m_spec;
    QtInputSupport::DeviceHandlerList<QEvdevTabletHandlerThread> m_activeDevices;
};

QT_END_NAMESPACE

#endif // QEVDEVTABLETMANAGER_P_H
