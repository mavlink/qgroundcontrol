// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOBJECT_P_P_H
#define QOBJECT_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qobject.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

// Even though this file is only used by qobject.cpp, the only reason this
// code lives here is that some special apps/libraries for e.g., QtJambi,
// Gammaray need access to the structs in this file.

#include <QtCore/qalloc.h>
#include <QtCore/qobject.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

// ConnectionList is a singly-linked list
struct QObjectPrivate::ConnectionList
{
    QAtomicPointer<Connection> first;
    QAtomicPointer<Connection> last;
};
static_assert(std::is_trivially_destructible_v<QObjectPrivate::ConnectionList>);
Q_DECLARE_TYPEINFO(QObjectPrivate::ConnectionList, Q_RELOCATABLE_TYPE);

struct QObjectPrivate::TaggedSignalVector
{
    quintptr c;

    TaggedSignalVector() = default;
    TaggedSignalVector(std::nullptr_t) noexcept : c(0) {}
    TaggedSignalVector(Connection *v) noexcept : c(reinterpret_cast<quintptr>(v)) { Q_ASSERT(v && (reinterpret_cast<quintptr>(v) & 0x1) == 0);   }
    TaggedSignalVector(SignalVector *v) noexcept : c(reinterpret_cast<quintptr>(v) | quintptr(1u)) { Q_ASSERT(v); }
    explicit operator SignalVector *() const noexcept
    {
        if (c & 0x1)
            return reinterpret_cast<SignalVector *>(c & ~quintptr(1u));
        return nullptr;
    }
    explicit operator Connection *() const noexcept
    {
        return reinterpret_cast<Connection *>(c);
    }
    operator uintptr_t() const noexcept { return c; }
};

struct QObjectPrivate::ConnectionOrSignalVector
{
    union {
        // linked list of orphaned connections that need cleaning up
        TaggedSignalVector nextInOrphanList;
        // linked list of connections connected to slots in this object
        Connection *next;
    };
};
static_assert(std::is_trivially_copyable_v<QObjectPrivate::ConnectionOrSignalVector>);

struct QObjectPrivate::Connection : public ConnectionOrSignalVector
{
    // linked list of connections connected to slots in this object, next is in base class
    Connection **prev;
    // linked list of connections connected to signals in this object
    QAtomicPointer<Connection> nextConnectionList;
    Connection *prevConnectionList;

    QObject *sender;
    QAtomicPointer<QObject> receiver;
    QAtomicPointer<QThreadData> receiverThreadData;
    union {
        StaticMetaCallFunction callFunction;
        QtPrivate::QSlotObjectBase *slotObj;
    };
    QAtomicPointer<const int> argumentTypes;
    QAtomicInt ref_{
        2
    }; // ref_ is 2 for the use in the internal lists, and for the use in QMetaObject::Connection
    uint id = 0;
    ushort method_offset;
    ushort method_relative;
    signed int signal_index : 27; // In signal range (see QObjectPrivate::signalIndex())
    ushort connectionType : 2; // 0 == auto, 1 == direct, 2 == queued, 3 == blocking
    ushort isSlotObject : 1;
    ushort ownArgumentTypes : 1;
    ushort isSingleShot : 1;
    Connection() : ownArgumentTypes(true) { }
    ~Connection();
    int method() const
    {
        Q_ASSERT(!isSlotObject);
        return method_offset + method_relative;
    }
    void ref() { ref_.ref(); }
    void freeSlotObject()
    {
        if (isSlotObject) {
            slotObj->destroyIfLastRef();
            isSlotObject = false;
        }
    }
    void deref()
    {
        if (!ref_.deref()) {
            Q_ASSERT(!receiver.loadRelaxed());
            Q_ASSERT(!isSlotObject);
            delete this;
        }
    }
};
Q_DECLARE_TYPEINFO(QObjectPrivate::Connection, Q_RELOCATABLE_TYPE);

struct QObjectPrivate::SignalVector : public ConnectionOrSignalVector
{
    quintptr allocated;
    // ConnectionList signals[]
    ConnectionList &at(int i) { return reinterpret_cast<ConnectionList *>(this + 1)[i + 1]; }
    const ConnectionList &at(int i) const
    {
        return reinterpret_cast<const ConnectionList *>(this + 1)[i + 1];
    }
    int count() const { return static_cast<int>(allocated); }
};
// it doesn't need to be, but it helps
static_assert(std::is_trivially_copyable_v<QObjectPrivate::SignalVector>);

struct QObjectPrivate::ConnectionData
{
    // the id below is used to avoid activating new connections. When the object gets
    // deleted it's set to 0, so that signal emission stops
    QAtomicInteger<uint> currentConnectionId;
    QAtomicInt ref;
    QAtomicPointer<SignalVector> signalVector;
    Connection *senders = nullptr;
    Sender *currentSender = nullptr; // object currently activating the object
    std::atomic<TaggedSignalVector> orphaned = {nullptr};

    ~ConnectionData()
    {
        Q_ASSERT(ref.loadRelaxed() == 0);
        TaggedSignalVector c = orphaned.exchange(nullptr, std::memory_order_relaxed);
        if (c)
            deleteOrphaned(c);
        SignalVector *v = signalVector.loadRelaxed();
        if (v) {
            const size_t allocSize = sizeof(SignalVector) + (v->allocated + 1) * sizeof(ConnectionList);
            v->~SignalVector();
            QtPrivate::sizedFree(v, allocSize);
        }
    }

    // must be called on the senders connection data
    // assumes the senders and receivers lock are held
    void removeConnection(Connection *c);
    enum LockPolicy {
        NeedToLock,
        // Beware that we need to temporarily release the lock
        // and thus calling code must carefully consider whether
        // invariants still hold.
        AlreadyLockedAndTemporarilyReleasingLock
    };
    void cleanOrphanedConnections(QObject *sender, LockPolicy lockPolicy = NeedToLock)
    {
        if (orphaned.load(std::memory_order_relaxed) && ref.loadAcquire() == 1)
            cleanOrphanedConnectionsImpl(sender, lockPolicy);
    }
    void cleanOrphanedConnectionsImpl(QObject *sender, LockPolicy lockPolicy);

    ConnectionList &connectionsForSignal(int signal)
    {
        return signalVector.loadRelaxed()->at(signal);
    }

    void resizeSignalVector(size_t size)
    {
        SignalVector *vector = this->signalVector.loadRelaxed();
        if (vector && vector->allocated > size)
            return;
        size = (size + 7) & ~7;
        void *ptr = QtPrivate::fittedMalloc(sizeof(SignalVector), &size, sizeof(ConnectionList), 1);
        auto newVector = new (ptr) SignalVector;

        int start = -1;
        if (vector) {
            // not (yet) existing trait:
            // static_assert(std::is_relocatable_v<SignalVector>);
            // static_assert(std::is_relocatable_v<ConnectionList>);
            memcpy(newVector, vector,
                   sizeof(SignalVector) + (vector->allocated + 1) * sizeof(ConnectionList));
            start = vector->count();
        }
        for (int i = start; i < int(size); ++i)
            new (&newVector->at(i)) ConnectionList();
        newVector->next = nullptr;
        newVector->allocated = size;

        signalVector.storeRelaxed(newVector);
        if (vector) {
            TaggedSignalVector o = nullptr;
            /* No ABA issue here: When adding a node, we only care about the list head, it doesn't
             * matter if the tail changes.
             */
            o = orphaned.load(std::memory_order_acquire);
            do {
                vector->nextInOrphanList = o;
            } while (!orphaned.compare_exchange_strong(o, TaggedSignalVector(vector), std::memory_order_release));
        }
    }
    int signalVectorCount() const
    {
        return signalVector.loadAcquire() ? signalVector.loadRelaxed()->count() : -1;
    }

    static void deleteOrphaned(TaggedSignalVector o);
};

struct QObjectPrivate::Sender
{
    Sender(QObject *receiver, QObject *sender, int signal, ConnectionData *receiverConnections)
        : receiver(receiver), sender(sender), signal(signal)
    {
        if (receiverConnections) {
            previous = receiverConnections->currentSender;
            receiverConnections->currentSender = this;
        }
    }
    ~Sender()
    {
        if (receiver)
            receiver->d_func()->connections.loadAcquire()->currentSender = previous;
    }
    void receiverDeleted()
    {
        Sender *s = this;
        while (s) {
            s->receiver = nullptr;
            s = s->previous;
        }
    }
    Sender *previous = nullptr;
    QObject *receiver;
    QObject *sender;
    int signal;
};
Q_DECLARE_TYPEINFO(QObjectPrivate::Sender, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
