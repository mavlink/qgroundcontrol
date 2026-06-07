// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4INTERNALCLASS_H
#define QV4INTERNALCLASS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4global_p.h"

#include <QHash>
#include <QVarLengthArray>
#include <climits> // for UINT_MAX
#include <private/qv4propertykey_p.h>
#include <private/qv4heap_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct VTable;
struct MarkStack;

struct InternalClassEntry {
    uint index;
    uint setterIndex;
    PropertyAttributes attributes;
    bool isValid() const { return !attributes.isEmpty(); }
};

struct PropertyHashData;
struct PropertyHash
{
    struct Entry {
        PropertyKey identifier;
        uint index;
        uint setterIndex;
    };

    PropertyHashData *d;

    inline PropertyHash();
    inline PropertyHash(const PropertyHash &other);
    inline ~PropertyHash();
    PropertyHash &operator=(const PropertyHash &other);

    void addEntry(const Entry &entry, int classSize);
    Entry *lookup(PropertyKey identifier) const;
    void detach(bool grow, int classSize);
};

struct PropertyHashData
{
    PropertyHashData(int numBits);
    ~PropertyHashData() {
        free(entries);
    }

    int refCount;
    int alloc;
    int size;
    int numBits;
    PropertyHash::Entry *entries;
};

inline PropertyHash::PropertyHash()
{
    d = new PropertyHashData(3);
}

inline PropertyHash::PropertyHash(const PropertyHash &other)
{
    d = other.d;
    ++d->refCount;
}

inline PropertyHash::~PropertyHash()
{
    if (!--d->refCount)
        delete d;
}

inline PropertyHash &PropertyHash::operator=(const PropertyHash &other)
{
    ++other.d->refCount;
    if (!--d->refCount)
        delete d;
    d = other.d;
    return *this;
}



inline PropertyHash::Entry *PropertyHash::lookup(PropertyKey identifier) const
{
    Q_ASSERT(d->entries);

    uint idx = identifier.id() % d->alloc;
    while (1) {
        if (d->entries[idx].identifier == identifier)
            return d->entries + idx;
        if (!d->entries[idx].identifier.isValid())
            return nullptr;
        ++idx;
        idx %= d->alloc;
    }
}

template<class T>
struct SharedInternalClassDataPrivate {};

template<>
struct SharedInternalClassDataPrivate<PropertyAttributes> {
    SharedInternalClassDataPrivate(ExecutionEngine *engine)
        : refcount(1),
          m_alloc(0),
          m_size(0),
          m_data(nullptr),
          m_engine(engine)
    { }
    SharedInternalClassDataPrivate(const SharedInternalClassDataPrivate<PropertyAttributes> &other);
    SharedInternalClassDataPrivate(const SharedInternalClassDataPrivate<PropertyAttributes> &other,
                                   uint pos, PropertyAttributes value);
    ~SharedInternalClassDataPrivate();

    void grow();

    void markIfNecessary(const PropertyAttributes &) {}

    uint alloc() const { return m_alloc; }
    uint size() const { return m_size; }
    void setSize(uint s) { m_size = s; }

    PropertyAttributes at(uint i) const { Q_ASSERT(i < m_alloc); return data(i); }
    void set(uint i, PropertyAttributes t) { Q_ASSERT(i < m_alloc); setData(i, t); }

    void mark(MarkStack *) {}

    int refcount = 1;
private:
    uint m_alloc;
    uint m_size;

    enum {
        SizeOfAttributesPointer = sizeof(PropertyAttributes *),
        SizeOfAttributes = sizeof(PropertyAttributes),
        NumAttributesInPointer = SizeOfAttributesPointer / SizeOfAttributes,
    };

    static_assert(NumAttributesInPointer > 0);

    PropertyAttributes data(uint i) const {
        return m_alloc > NumAttributesInPointer ? m_data[i] : m_inlineData[i];
    }

    void setData(uint i, PropertyAttributes t) {
        if (m_alloc > NumAttributesInPointer)
            m_data[i] = t;
        else
            m_inlineData[i] = t;
    }

    union {
        PropertyAttributes *m_data;
        PropertyAttributes m_inlineData[NumAttributesInPointer];
    };
    ExecutionEngine *m_engine;
};

template<>
struct SharedInternalClassDataPrivate<PropertyKey> {
    SharedInternalClassDataPrivate(ExecutionEngine *e) : refcount(1), engine(e) {}
    SharedInternalClassDataPrivate(const SharedInternalClassDataPrivate &other);
    SharedInternalClassDataPrivate(const SharedInternalClassDataPrivate &other, uint pos, PropertyKey value);
    ~SharedInternalClassDataPrivate() {}

    template<typename StringOrSymbol = Heap::StringOrSymbol>
    void markIfNecessary(const PropertyKey &value);

    void grow();
    uint alloc() const;
    uint size() const;
    void setSize(uint s);

    PropertyKey at(uint i) const;
    void set(uint i, PropertyKey t);

    void mark(MarkStack *s);

    int refcount = 1;
private:
    ExecutionEngine *engine;
    WriteBarrier::Pointer<Heap::MemberData> data;
};

template<typename StringOrSymbol>
void QV4::SharedInternalClassDataPrivate<PropertyKey>::markIfNecessary(const PropertyKey &value)
{
    QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *stack) {
        if constexpr (QV4::WriteBarrier::isInsertionBarrier) {
            if (auto s = value.asStringOrSymbol<StringOrSymbol>())
                s->mark(stack);
        }
    });
}

template <typename T>
struct SharedInternalClassData {
    using Private = SharedInternalClassDataPrivate<T>;
    Private *d;

    inline SharedInternalClassData(ExecutionEngine *e) {
        d = new Private(e);
    }

    inline SharedInternalClassData(const SharedInternalClassData &other)
        : d(other.d)
    {
        ++d->refcount;
    }
    inline ~SharedInternalClassData() {
        if (!--d->refcount)
            delete d;
    }
    SharedInternalClassData &operator=(const SharedInternalClassData &other) {
        ++other.d->refcount;
        if (!--d->refcount)
            delete d;
        d = other.d;
        return *this;
    }

    void add(uint pos, T value) {
        d->markIfNecessary(value);
        if (pos < d->size()) {
            Q_ASSERT(d->refcount > 1);
            // need to detach
            Private *dd = new Private(*d, pos, value);
            --d->refcount;
            d = dd;
            return;
        }
        Q_ASSERT(pos == d->size());
        if (pos == d->alloc())
            d->grow();
        if (pos >= d->alloc()) {
            qBadAlloc();
        } else {
            d->setSize(d->size() + 1);
            d->set(pos, value);
        }
    }

    void set(uint pos, T value) {
        Q_ASSERT(pos < d->size());
        d->markIfNecessary(value);
        if (d->refcount > 1) {
            // need to detach
            Private *dd = new Private(*d);
            --d->refcount;
            d = dd;
        }
        d->set(pos, value);
    }

    T at(uint i) const {
        Q_ASSERT(i < d->size());
        return d->at(i);
    }
    T operator[] (uint i) {
        Q_ASSERT(i < d->size());
        return d->at(i);
    }

    void mark(MarkStack *s) { d->mark(s); }
};

struct InternalClassTransition
{
    union {
        PropertyKey id;
        const VTable *vtable;
        Heap::Object *prototype;
    };
    Heap::InternalClass *lookup;
    int flags;
    enum {
        // range 0-0xff is reserved for attribute changes
        StructureChange = 0x100,
        NotExtensible   = StructureChange | (1 << 0),
        VTableChange    = StructureChange | (1 << 1),
        PrototypeChange = StructureChange | (1 << 2),
        ProtoClass      = StructureChange | (1 << 3),
        Sealed          = StructureChange | (1 << 4),
        Frozen          = StructureChange | (1 << 5),
        Locked          = StructureChange | (1 << 6),
    };

    bool operator==(const InternalClassTransition &other) const
    { return id == other.id && flags == other.flags; }

    bool operator<(const InternalClassTransition &other) const
    { return flags < other.flags || (flags == other.flags && id < other.id); }
};

namespace Heap {

struct InternalClass : Base {
    enum Flag {
        NotExtensible = 1 << 0,
        Sealed        = 1 << 1,
        Frozen        = 1 << 2,
        UsedAsProto   = 1 << 3,
        Locked        = 1 << 4,
    };
    enum { MaxRedundantTransitions = 255 };

    ExecutionEngine *engine;
    const VTable *vtable;
    quintptr protoId; // unique across the engine, gets changed whenever the proto chain changes
    Heap::Object *prototype;
    InternalClass *parent;

    PropertyHash propertyTable; // id to valueIndex
    SharedInternalClassData<PropertyKey> nameMap;
    SharedInternalClassData<PropertyAttributes> propertyData;

    typedef InternalClassTransition Transition;
    QVarLengthArray<Transition, 1> transitions;
    InternalClassTransition &lookupOrInsertTransition(const InternalClassTransition &t);

    uint size;
    quint8 numRedundantTransitions;
    quint8 flags;

    bool isExtensible() const { return !(flags & NotExtensible); }
    bool isSealed() const { return flags & Sealed; }
    bool isFrozen() const { return flags & Frozen; }
    bool isUsedAsProto() const { return flags & UsedAsProto; }
    bool isLocked() const { return flags & Locked; }

    void init(ExecutionEngine *engine);
    void init(InternalClass *other);
    void destroy();

    Q_QML_EXPORT ReturnedValue keyAt(uint index) const;
    Q_REQUIRED_RESULT InternalClass *nonExtensible();
    Q_REQUIRED_RESULT InternalClass *locked();

    static void addMember(QV4::Object *object, PropertyKey id, PropertyAttributes data, InternalClassEntry *entry);
    Q_REQUIRED_RESULT InternalClass *addMember(PropertyKey identifier, PropertyAttributes data, InternalClassEntry *entry = nullptr);
    Q_REQUIRED_RESULT InternalClass *changeMember(PropertyKey identifier, PropertyAttributes data, InternalClassEntry *entry = nullptr);
    static void changeMember(QV4::Object *object, PropertyKey id, PropertyAttributes data, InternalClassEntry *entry = nullptr);
    static void removeMember(QV4::Object *object, PropertyKey identifier);
    PropertyHash::Entry *findEntry(const PropertyKey id)
    {
        Q_ASSERT(id.isStringOrSymbol());

        PropertyHash::Entry *e = propertyTable.lookup(id);
        if (e && e->index < size)
            return e;

        return nullptr;
    }

    InternalClassEntry find(const PropertyKey id)
    {
        Q_ASSERT(id.isStringOrSymbol());

        PropertyHash::Entry *e = propertyTable.lookup(id);
        if (e && e->index < size) {
            PropertyAttributes a = propertyData.at(e->index);
            if (!a.isEmpty())
                return { e->index, e->setterIndex, a };
        }

        return { UINT_MAX, UINT_MAX, Attr_Invalid };
    }

    struct IndexAndAttribute {
        uint index;
        PropertyAttributes attrs;
        bool isValid() const { return index != UINT_MAX; }
    };

    IndexAndAttribute findValueOrGetter(const PropertyKey id)
    {
        Q_ASSERT(id.isStringOrSymbol());

        PropertyHash::Entry *e = propertyTable.lookup(id);
        if (e && e->index < size) {
            PropertyAttributes a = propertyData.at(e->index);
            if (!a.isEmpty())
                return { e->index, a };
        }

        return { UINT_MAX, Attr_Invalid };
    }

    IndexAndAttribute findValueOrSetter(const PropertyKey id)
    {
        Q_ASSERT(id.isStringOrSymbol());

        PropertyHash::Entry *e = propertyTable.lookup(id);
        if (e && e->index < size) {
            PropertyAttributes a = propertyData.at(e->index);
            if (!a.isEmpty()) {
                if (a.isAccessor()) {
                    Q_ASSERT(e->setterIndex != UINT_MAX);
                    return { e->setterIndex, a };
                }
                return { e->index, a };
            }
        }

        return { UINT_MAX, Attr_Invalid };
    }

    uint indexOfValueOrGetter(const PropertyKey id)
    {
        Q_ASSERT(id.isStringOrSymbol());

        PropertyHash::Entry *e = propertyTable.lookup(id);
        if (e && e->index < size) {
            Q_ASSERT(!propertyData.at(e->index).isEmpty());
            return e->index;
        }

        return UINT_MAX;
    }

    bool verifyIndex(const PropertyKey id, uint index)
    {
        Q_ASSERT(id.isStringOrSymbol());

        PropertyHash::Entry *e = propertyTable.lookup(id);
        if (e && e->index < size) {
            Q_ASSERT(!propertyData.at(e->index).isEmpty());
            return e->index == index;
        }

        return false;
    }

    Q_REQUIRED_RESULT InternalClass *sealed();
    Q_REQUIRED_RESULT InternalClass *frozen();
    Q_REQUIRED_RESULT InternalClass *canned();        // sealed + nonExtensible
    Q_REQUIRED_RESULT InternalClass *cryopreserved(); // frozen + sealed + nonExtensible
    bool isImplicitlyFrozen() const;

    Q_REQUIRED_RESULT InternalClass *asProtoClass();

    Q_REQUIRED_RESULT InternalClass *changeVTable(const VTable *vt) {
        if (vtable == vt)
            return this;
        return changeVTableImpl(vt);
    }
    Q_REQUIRED_RESULT InternalClass *changePrototype(Heap::Object *proto) {
        if (prototype == proto)
            return this;
        return changePrototypeImpl(proto);
    }

    void updateProtoUsage(Heap::Object *o);

    static void markObjects(Heap::Base *ic, MarkStack *stack);

private:
    Q_QML_EXPORT InternalClass *changeVTableImpl(const VTable *vt);
    Q_QML_EXPORT InternalClass *changePrototypeImpl(Heap::Object *proto);
    InternalClass *addMemberImpl(PropertyKey identifier, PropertyAttributes data, InternalClassEntry *entry);

    void removeChildEntry(InternalClass *child);
    friend struct ::QV4::ExecutionEngine;
};

inline
void Base::markObjects(Base *b, MarkStack *stack)
{
    b->internalClass->mark(stack);
}

}

}

QT_END_NAMESPACE

#endif
