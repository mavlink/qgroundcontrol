// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//

#ifndef QDBUSABSTRACTINTERFACE_P_H
#define QDBUSABSTRACTINTERFACE_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include <qdbusabstractinterface.h>
#include <qdbusconnection.h>
#include <qdbuserror.h>
#include "qdbusconnection_p.h"
#include "private/qobject_p.h"

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusAbstractInterfacePrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QDBusAbstractInterface)

    mutable QDBusConnection connection; // mutable because we want to make calls from const functions
    QString service;
    QString currentOwner;
    QString path;
    QString interface;
    mutable QDBusError lastError;
    int timeout;
    bool interactiveAuthorizationAllowed;

    // this is set during creation and never changed
    // it can't be const because QDBusInterfacePrivate has one more check
    bool isValid;

    QDBusAbstractInterfacePrivate(const QString &serv, const QString &p,
                                  const QString &iface, const QDBusConnection& con, bool dynamic);
    virtual ~QDBusAbstractInterfacePrivate() { }
    void initOwnerTracking();
    bool canMakeCalls() const;

    // these functions do not check if the property is valid
    bool property(const QMetaProperty &mp, void *returnValuePtr) const;
    bool setProperty(const QMetaProperty &mp, const QVariant &value);

    // return conn's d pointer
    inline QDBusConnectionPrivate *connectionPrivate() const
    { return QDBusConnectionPrivate::d(connection); }

    void _q_serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

    static void finishDisconnectNotify(QDBusAbstractInterface *iface, int signalId);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
