// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef Q_SPI_STRUCT_MARSHALLERS_H
#define Q_SPI_STRUCT_MARSHALLERS_H

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
#include <QtCore/qlist.h>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusObjectPath>

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

using QSpiIntList = QList<int>;
using QSpiUIntList = QList<uint>;

// FIXME: make this copy on write
struct QSpiObjectReference
{
    QString service;
    QDBusObjectPath path;

    QSpiObjectReference();
    QSpiObjectReference(const QDBusConnection& connection, const QDBusObjectPath& path)
        : service(connection.baseService()), path(path) {}
};
Q_DECLARE_TYPEINFO(QSpiObjectReference, Q_RELOCATABLE_TYPE); // QDBusObjectPath is movable, even though it
                                                         // cannot be marked that way until Qt 6

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiObjectReference &address);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiObjectReference &address);

typedef QList<QSpiObjectReference> QSpiObjectReferenceArray;

struct QSpiAccessibleCacheItem
{
    QSpiObjectReference         path;
    QSpiObjectReference         application;
    QSpiObjectReference         parent;
    QSpiObjectReferenceArray children;
    QStringList                 supportedInterfaces;
    QString                     name;
    uint                        role;
    QString                     description;
    QSpiUIntList                state;
};
Q_DECLARE_TYPEINFO(QSpiAccessibleCacheItem, Q_RELOCATABLE_TYPE);

typedef QList<QSpiAccessibleCacheItem> QSpiAccessibleCacheArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAccessibleCacheItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAccessibleCacheItem &item);

struct QSpiAction
{
    QString name;
    QString description;
    QString keyBinding;
};
Q_DECLARE_TYPEINFO(QSpiAction, Q_RELOCATABLE_TYPE);

typedef QList<QSpiAction> QSpiActionArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAction &action);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAction &action);

struct QSpiEventListener
{
    QString listenerAddress;
    QString eventName;
};
Q_DECLARE_TYPEINFO(QSpiEventListener, Q_RELOCATABLE_TYPE);

typedef QList<QSpiEventListener> QSpiEventListenerArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiEventListener &action);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiEventListener &action);

typedef std::pair<unsigned int, QSpiObjectReferenceArray> QSpiRelationArrayEntry;
typedef QList<QSpiRelationArrayEntry> QSpiRelationArray;

//a(iisv)
struct QSpiTextRange {
    int startOffset;
    int endOffset;
    QString contents;
    QVariant v;
};
Q_DECLARE_TYPEINFO(QSpiTextRange, Q_RELOCATABLE_TYPE);

typedef QList<QSpiTextRange> QSpiTextRangeList;
typedef QMap <QString, QString> QSpiAttributeSet;

enum QSpiAppUpdateType {
    QSPI_APP_UPDATE_ADDED = 0,
    QSPI_APP_UPDATE_REMOVED = 1
};
Q_DECLARE_TYPEINFO(QSpiAppUpdateType, Q_PRIMITIVE_TYPE);

struct QSpiAppUpdate {
    int type; /* Is an application added or removed */
    QString address; /* D-Bus address of application added or removed */
};
Q_DECLARE_TYPEINFO(QSpiAppUpdate, Q_RELOCATABLE_TYPE);

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAppUpdate &update);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAppUpdate &update);

struct QSpiDeviceEvent {
    unsigned int type;
    int id;
    int hardwareCode;
    int modifiers;
    int timestamp;
    QString text;
    bool isText;
};
Q_DECLARE_TYPEINFO(QSpiDeviceEvent, Q_RELOCATABLE_TYPE);

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiDeviceEvent &event);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiDeviceEvent &event);

void qSpiInitializeStructTypes();

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QSpiIntList, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiUIntList, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiObjectReference, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiObjectReferenceArray, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiAccessibleCacheItem, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiAccessibleCacheArray, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiAction, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiActionArray, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiEventListener, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiEventListenerArray, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiRelationArrayEntry, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiRelationArray, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiTextRange, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiTextRangeList, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiAttributeSet, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiAppUpdate, /* not exported */)
QT_DECL_METATYPE_EXTERN(QSpiDeviceEvent, /* not exported */)

// For qdbusxml2cpp-generated code
QT_USE_NAMESPACE

#endif /* Q_SPI_STRUCT_MARSHALLERS_H */
