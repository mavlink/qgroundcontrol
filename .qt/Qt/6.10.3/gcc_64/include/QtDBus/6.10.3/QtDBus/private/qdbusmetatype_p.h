// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDBUSMETATYPE_P_H
#define QDBUSMETATYPE_P_H

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

#include <QtDBus/private/qtdbusglobal_p.h>
#include <qdbusmetatype.h>

#include <qdbusmessage.h>
#include <qdbusargument.h>
#include <qdbusextratypes.h>
#include <qdbuserror.h>
#include <qdbusunixfiledescriptor.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

namespace QDBusMetaTypeId {
QMetaType message(); // QDBusMessage
QMetaType argument(); // QDBusArgument
QMetaType variant(); // QDBusVariant
QMetaType objectpath(); // QDBusObjectPath
QMetaType signature(); // QDBusSignature
QMetaType error(); // QDBusError
QMetaType unixfd(); // QDBusUnixFileDescriptor

Q_DBUS_EXPORT void init();
}; // namespace QDBusMetaTypeId

inline QMetaType QDBusMetaTypeId::message()
{ return QMetaType::fromType<QDBusMessage>(); }

inline QMetaType QDBusMetaTypeId::argument()
{ return QMetaType::fromType<QDBusArgument>(); }

inline QMetaType QDBusMetaTypeId::variant()
{ return QMetaType::fromType<QDBusVariant>(); }

inline QMetaType QDBusMetaTypeId::objectpath()
{ return QMetaType::fromType<QDBusObjectPath>(); }

inline QMetaType QDBusMetaTypeId::signature()
{ return QMetaType::fromType<QDBusSignature>(); }

inline QMetaType QDBusMetaTypeId::error()
{ return QMetaType::fromType<QDBusError>(); }

inline QMetaType QDBusMetaTypeId::unixfd()
{ return QMetaType::fromType<QDBusUnixFileDescriptor>(); }

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
