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

#ifndef QDBUSCONTEXT_P_H
#define QDBUSCONTEXT_P_H

#include <QtDBus/private/qtdbusglobal_p.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusMessage;
class QDBusConnection;

class QDBusContext;
class QDBusContextPrivate
{
public:
    inline QDBusContextPrivate(const QDBusConnection &conn, const QDBusMessage &msg)
        : connection(conn), message(msg) {}

    QDBusConnection connection;
    const QDBusMessage &message;

    static QDBusContextPrivate *set(QObject *obj, QDBusContextPrivate *newContext);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif

