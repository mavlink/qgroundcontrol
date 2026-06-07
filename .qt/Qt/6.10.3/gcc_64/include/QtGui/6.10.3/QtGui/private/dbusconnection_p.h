// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef DBUSCONNECTION_H
#define DBUSCONNECTION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusVariant>
#include <QtCore/private/qglobal_p.h>
Q_MOC_INCLUDE(<QtDBus/QDBusError>)

#include "bus_interface.h"
#include "properties_interface.h"

QT_BEGIN_NAMESPACE

class QDBusServiceWatcher;

class QAtSpiDBusConnection : public QObject
{
    Q_OBJECT

public:
    QAtSpiDBusConnection(QObject *parent = nullptr);
    QDBusConnection connection() const;
    bool isEnabled() const { return m_enabled; }

Q_SIGNALS:
    // Emitted when the global accessibility status changes to enabled
    void enabledChanged(bool enabled);

private Q_SLOTS:
    QString getAddressFromXCB();
    void checkEnabledState();
    void serviceUnregistered();
    void connectA11yBus(const QString &address);

    void dbusError(const QDBusError &error);

private:
    QString getAccessibilityBusAddress() const;

    QDBusServiceWatcher *dbusWatcher;
    QtGuiPrivate::OrgFreedesktopDBusPropertiesInterface *m_dbusProperties;
    QtGuiPrivate::OrgA11yStatusInterface *m_a11yStatus;
    QDBusConnection m_a11yConnection;
    bool m_enabled;
};

QT_END_NAMESPACE

#endif // DBUSCONNECTION_H
