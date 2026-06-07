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

#ifndef QDBUSINTEGRATOR_P_H
#define QDBUSINTEGRATOR_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include "qdbus_symbols_p.h"

#include "qcoreevent.h"
#include "qeventloop.h"
#include "qobject.h"
#include "private/qobject_p.h"
#include "qlist.h"
#include "qpointer.h"
#include "qsemaphore.h"

#include "qdbusconnection.h"
#include "qdbusmessage.h"
#include "qdbusconnection_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusConnectionPrivate;
class QDBusMessage;

// Really private structs used by qdbusintegrator.cpp
// Things that aren't used by any other file

struct QDBusSlotCache
{
    struct Data
    {
        int slotIdx;
        QList<QMetaType> metaTypes;

        void swap(Data &other) noexcept
        {
            std::swap(slotIdx, other.slotIdx);
            metaTypes.swap(other.metaTypes);
        }
    };

    struct Key
    {
        QString memberWithSignature;
        QDBusConnection::RegisterOptions flags;

        friend bool operator==(const Key &lhs, const Key &rhs) noexcept
        {
            return lhs.memberWithSignature == rhs.memberWithSignature && lhs.flags == rhs.flags;
        }

        friend size_t qHash(const QDBusSlotCache::Key &key, size_t seed = 0) noexcept
        {
            return qHashMulti(seed, key.memberWithSignature, key.flags);
        }
    };

    using Hash = QHash<Key, Data>;

    Hash hash;

    void swap(QDBusSlotCache &other) noexcept { hash.swap(other.hash); }
};
Q_DECLARE_SHARED(QDBusSlotCache::Data)
Q_DECLARE_SHARED(QDBusSlotCache)

class QDBusCallDeliveryEvent: public QAbstractMetaCallEvent
{
public:
    QDBusCallDeliveryEvent(const QDBusConnection &c, int id, QObject *sender,
                           const QDBusMessage &msg, const QList<QMetaType> &types)
        : QAbstractMetaCallEvent(sender, -1), connection(c), message(msg), metaTypes(types), id(id)
    {
    }

    void placeMetaCall(QObject *object) override
    {
        QDBusConnectionPrivate::d(connection)->deliverCall(object, message, metaTypes, id);
    }

private:
    QDBusConnection connection; // just for refcounting
    QDBusMessage message;
    QList<QMetaType> metaTypes;
    int id;
};

class QDBusActivateObjectEvent: public QAbstractMetaCallEvent
{
public:
    QDBusActivateObjectEvent(const QDBusConnection &c, QObject *sender,
                             const QDBusConnectionPrivate::ObjectTreeNode &n,
                             int p, const QDBusMessage &m, QSemaphore *s = nullptr)
        : QAbstractMetaCallEvent(sender, -1, s), connection(c), node(n),
          pathStartPos(p), message(m), handled(false)
        { }
    ~QDBusActivateObjectEvent() override;

    void placeMetaCall(QObject *) override;

private:
    QDBusConnection connection; // just for refcounting
    QDBusConnectionPrivate::ObjectTreeNode node;
    int pathStartPos;
    QDBusMessage message;
    bool handled;
};

class QDBusSpyCallEvent : public QAbstractMetaCallEvent
{
public:
    typedef void (*Hook)(const QDBusMessage&);
    QDBusSpyCallEvent(QDBusConnectionPrivate *cp, const QDBusConnection &c, const QDBusMessage &msg)
        : QAbstractMetaCallEvent(cp, 0), conn(c), msg(msg)
    {}
    ~QDBusSpyCallEvent() override;
    void placeMetaCall(QObject *) override;
    static inline void invokeSpyHooks(const QDBusMessage &msg);

    QDBusConnection conn;   // keeps the refcount in QDBusConnectionPrivate up
    QDBusMessage msg;
};

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QDBusSlotCache, Q_DBUS_EXPORT)

#endif // QT_NO_DBUS
#endif
