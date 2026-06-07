// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDBUSMETAOBJECT_P_H
#define QDBUSMETAOBJECT_P_H

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

#include <QtDBus/private/qtdbusglobal_p.h>
#include <QtCore/qmetaobject.h>

#ifndef QT_NO_DBUS

#ifdef interface
#  undef interface
#endif

QT_BEGIN_NAMESPACE

class QDBusError;

struct QDBusMetaObjectPrivate;
struct Q_DBUS_EXPORT QDBusMetaObject: public QMetaObject
{
    bool cached;

    static QDBusMetaObject *createMetaObject(const QString &interface, const QString &xml,
                                             QHash<QString, QDBusMetaObject *> &map,
                                             QDBusError &error);
    ~QDBusMetaObject()
    {
        delete [] reinterpret_cast<const char *>(d.stringdata);
        delete [] d.data;
        delete [] reinterpret_cast<const QMetaType *>(d.metaTypes);
    }

    // methods (slots & signals):
    const int *inputTypesForMethod(int id) const;
    const int *outputTypesForMethod(int id) const;

    // properties:
    int propertyMetaType(int id) const;

private:
    QDBusMetaObject();
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
