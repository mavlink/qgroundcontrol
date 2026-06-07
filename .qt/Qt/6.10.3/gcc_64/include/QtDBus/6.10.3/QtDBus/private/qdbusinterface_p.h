// Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QDBUSINTERFACE_P_H
#define QDBUSINTERFACE_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include "qdbusabstractinterface_p.h"
#include "qdbusmetaobject_p.h"
#include <qdbusinterface.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusInterfacePrivate: public QDBusAbstractInterfacePrivate
{
public:
    Q_DECLARE_PUBLIC(QDBusInterface)

    QDBusMetaObject *metaObject;

    QDBusInterfacePrivate(const QString &serv, const QString &p, const QString &iface,
                          const QDBusConnection &con);
    ~QDBusInterfacePrivate();

    int metacall(QMetaObject::Call c, int id, void **argv);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
