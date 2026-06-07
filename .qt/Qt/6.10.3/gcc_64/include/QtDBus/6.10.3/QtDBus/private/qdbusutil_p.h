// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDBUSUTIL_P_H
#define QDBUSUTIL_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include <QtDBus/qdbuserror.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

#include "qdbus_symbols_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

#define Q_DBUS_NO_EXPORT        // force syncqt looking into this namespace
namespace Q_DBUS_NO_EXPORT QDBusUtil
{
    Q_DBUS_EXPORT bool isValidInterfaceName(const QString &ifaceName);

    Q_DBUS_EXPORT bool isValidUniqueConnectionName(QStringView busName);

    Q_DBUS_EXPORT bool isValidBusName(const QString &busName);

    Q_DBUS_EXPORT bool isValidMemberName(QStringView memberName);

    Q_DBUS_EXPORT bool isValidErrorName(const QString &errorName);

    Q_DBUS_EXPORT bool isValidPartOfObjectPath(QStringView path);

    Q_DBUS_EXPORT bool isValidObjectPath(const QString &path);

    Q_DBUS_EXPORT bool isValidFixedType(int c);

    Q_DBUS_EXPORT bool isValidBasicType(int c);

    Q_DBUS_EXPORT bool isValidSignature(const QString &signature);

    Q_DBUS_EXPORT bool isValidSingleSignature(const QString &signature);

    Q_DBUS_EXPORT QString argumentToString(const QVariant &variant);

    enum AllowEmptyFlag {
        EmptyAllowed,
        EmptyNotAllowed
    };

    inline bool checkInterfaceName(const QString &name, AllowEmptyFlag empty, QDBusError *error)
    {
        if (name.isEmpty()) {
            if (empty == EmptyAllowed) return true;
            *error = QDBusError(QDBusError::InvalidInterface, QLatin1StringView("Interface name cannot be empty"));
            return false;
        }
        if (isValidInterfaceName(name)) return true;
        *error = QDBusError(QDBusError::InvalidInterface, QLatin1StringView("Invalid interface class: %1").arg(name));
        return false;
    }

    inline bool checkBusName(const QString &name, AllowEmptyFlag empty, QDBusError *error)
    {
        if (name.isEmpty()) {
            if (empty == EmptyAllowed) return true;
            *error = QDBusError(QDBusError::InvalidService, QLatin1StringView("Service name cannot be empty"));
            return false;
        }
        if (isValidBusName(name)) return true;
        *error = QDBusError(QDBusError::InvalidService, QLatin1StringView("Invalid service name: %1").arg(name));
        return false;
    }

    inline bool checkObjectPath(const QString &path, AllowEmptyFlag empty, QDBusError *error)
    {
        if (path.isEmpty()) {
            if (empty == EmptyAllowed) return true;
            *error = QDBusError(QDBusError::InvalidObjectPath, QLatin1StringView("Object path cannot be empty"));
            return false;
        }
        if (isValidObjectPath(path)) return true;
        *error = QDBusError(QDBusError::InvalidObjectPath, QLatin1StringView("Invalid object path: %1").arg(path));
        return false;
    }

    inline bool checkMemberName(const QString &name, AllowEmptyFlag empty, QDBusError *error, const char *nameType = nullptr)
    {
        if (!nameType) nameType = "member";
        if (name.isEmpty()) {
            if (empty == EmptyAllowed) return true;
            *error = QDBusError(QDBusError::InvalidMember, QLatin1StringView(nameType) + QLatin1StringView(" name cannot be empty"));
            return false;
        }
        if (isValidMemberName(name)) return true;
        *error = QDBusError(QDBusError::InvalidMember, QLatin1StringView("Invalid %1 name: %2")
                            .arg(QLatin1StringView(nameType), name));
        return false;
    }

    inline bool checkErrorName(const QString &name, AllowEmptyFlag empty, QDBusError *error)
    {
        if (name.isEmpty()) {
            if (empty == EmptyAllowed) return true;
            *error = QDBusError(QDBusError::InvalidInterface, QLatin1StringView("Error name cannot be empty"));
            return false;
        }
        if (isValidErrorName(name)) return true;
        *error = QDBusError(QDBusError::InvalidInterface, QLatin1StringView("Invalid error name: %1").arg(name));
        return false;
    }

    inline QString dbusService()
    { return QStringLiteral(DBUS_SERVICE_DBUS); }
    inline QString dbusPath()
    { return QStringLiteral(DBUS_PATH_DBUS); }
    inline QString dbusPathLocal()
    { return QStringLiteral(DBUS_PATH_LOCAL); }
    inline QString dbusInterface()
    {
        // it's the same string, but just be sure
        Q_ASSERT(dbusService() == QLatin1StringView(DBUS_INTERFACE_DBUS));
        return dbusService();
    }
    inline QString dbusInterfaceProperties()
    { return QStringLiteral(DBUS_INTERFACE_PROPERTIES); }
    inline QString dbusInterfaceIntrospectable()
    { return QStringLiteral(DBUS_INTERFACE_INTROSPECTABLE); }
    inline QString nameOwnerChanged()
    { return QStringLiteral("NameOwnerChanged"); }
    inline QString disconnectedErrorMessage()
    { return QStringLiteral("Not connected to D-Bus server"); }
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
