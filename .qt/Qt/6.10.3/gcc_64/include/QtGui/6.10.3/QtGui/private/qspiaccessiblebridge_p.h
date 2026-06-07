// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QSPIACCESSIBLEBRIDGE_H
#define QSPIACCESSIBLEBRIDGE_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtDBus/qdbusconnection.h>
#include <qpa/qplatformaccessibility.h>
#include <QtCore/qhash.h>

namespace QtGuiPrivate {
class DeviceEventControllerAdaptor;
} // namespace QtGuiPrivate

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

class QAtSpiDBusConnection;
class QSpiDBusCache;
class AtSpiAdaptor;
struct RoleNames;

class Q_GUI_EXPORT QSpiAccessibleBridge: public QObject, public QPlatformAccessibility
{
    Q_OBJECT
public:
    using SpiRoleMapping = QHash <QAccessible::Role, RoleNames>;

    QSpiAccessibleBridge();

    virtual ~QSpiAccessibleBridge();

    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
    QDBusConnection dBusConnection() const;

    const SpiRoleMapping &spiRoleNames() const { return m_spiRoleMapping; }

    static QSpiAccessibleBridge *instance();
    static RoleNames namesForRole(QAccessible::Role role);

public Q_SLOTS:
    void enabledChanged(bool enabled);

private:
    void initializeConstantMappings();
    void updateStatus();

    QSpiDBusCache *cache;
    QtGuiPrivate::DeviceEventControllerAdaptor *dec;
    AtSpiAdaptor *dbusAdaptor;
    QAtSpiDBusConnection* dbusConnection;
    SpiRoleMapping m_spiRoleMapping;
};

QT_END_NAMESPACE

#endif
