// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

/*
    This file was originally created by qdbusxml2cpp version 0.8
    Command line was:
    qdbusxml2cpp -p qxdgnotificationproxy ../../3rdparty/dbus-ifaces/org.freedesktop.Notifications.xml

    However it is maintained manually.

    It is also not part of the public API. This header file may change from
    version to version without notice, or even be removed.
*/

#ifndef QXDGNOTIFICATIONPROXY_P_H
#define QXDGNOTIFICATIONPROXY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QLoggingCategory>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QDBusAbstractInterface>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcTray)

/*
 * Proxy class for interface org.freedesktop.Notifications
 */
class QXdgNotificationInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.Notifications"; }

public:
    QXdgNotificationInterface(const QString &service, const QString &path,
                              const QDBusConnection &connection, QObject *parent = nullptr);

    ~QXdgNotificationInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> closeNotification(uint id)
    {
        return asyncCall(QStringLiteral("CloseNotification"), id);
    }

    inline QDBusPendingReply<QStringList> getCapabilities()
    {
        return asyncCall(QStringLiteral("GetCapabilities"));
    }

    inline QDBusPendingReply<QString, QString, QString, QString> getServerInformation()
    {
        return asyncCall(QStringLiteral("GetServerInformation"));
    }
    inline QDBusReply<QString> getServerInformation(QString &vendor, QString &version, QString &specVersion)
    {
        QDBusMessage reply = call(QDBus::Block, QStringLiteral("GetServerInformation"));
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() == 4) {
            vendor = qdbus_cast<QString>(reply.arguments().at(1));
            version = qdbus_cast<QString>(reply.arguments().at(2));
            specVersion = qdbus_cast<QString>(reply.arguments().at(3));
        }
        return reply;
    }

    // see https://developer.gnome.org/notification-spec/#basic-design
    inline QDBusPendingReply<uint> notify(const QString &appName, uint replacesId, const QString &appIcon,
                                          const QString &summary, const QString &body, const QStringList &actions,
                                          const QVariantMap &hints, int timeout)
    {
        qCDebug(qLcTray) << appName << replacesId << appIcon << summary << body << actions << hints << timeout;
        return asyncCall(QStringLiteral("Notify"), appName, replacesId, appIcon, summary, body, actions, hints, timeout);
    }

Q_SIGNALS:
    void ActionInvoked(uint id, const QString &action_key);
    void NotificationClosed(uint id, uint reason);
};

QT_END_NAMESPACE

namespace org {
  namespace freedesktop {
    using Notifications = QT_PREPEND_NAMESPACE(QXdgNotificationInterface);
  }
}

#endif
