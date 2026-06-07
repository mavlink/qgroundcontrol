// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4OBJECTITERATOR_H
#define QV4OBJECTITERATOR_H

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
#include "qv4object_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Q_QML_EXPORT ObjectIterator
{
    enum Flags {
        NoFlags = 0,
        EnumerableOnly = 0x1,
        WithSymbols = 0x2
    };

    ExecutionEngine *engine;
    Object *object;
    OwnPropertyKeyIterator *iterator = nullptr;
    uint flags;

    ObjectIterator(Scope &scope, const Object *o, uint flags)
    {
        engine = scope.engine;
        object = static_cast<Object *>(scope.alloc());
        this->flags = flags;
        object->setM(o ? o->m() : nullptr);
        if (o)
            iterator = object->ownPropertyKeys(object);
    }
    ~ObjectIterator()
    {
        delete iterator;
    }

    PropertyKey next(Property *pd = nullptr, PropertyAttributes *attributes = nullptr);
    ReturnedValue nextPropertyName(Value *value);
    ReturnedValue nextPropertyNameAsString(Value *value);
    ReturnedValue nextPropertyNameAsString();
};

namespace Heap {

#define ForInIteratorObjectMembers(class, Member) \
    Member(class, Pointer, Object *, object) \
    Member(class, Pointer, Object *, current) \
    Member(class, Pointer, Object *, target) \
    Member(class, NoMark, OwnPropertyKeyIterator *, iterator)

DECLARE_HEAP_OBJECT(ForInIteratorObject, Object) {
    void init(QV4::Object *o);
    Value workArea[2];

    static void markObjects(Heap::Base *that, MarkStack *markStack);
    void destroy();
};

}

struct ForInIteratorPrototype : Object
{
    V4_PROTOTYPE(iteratorPrototype)
    void init(ExecutionEngine *engine);

    static ReturnedValue method_next(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
};

struct ForInIteratorObject: Object {
    V4_OBJECT2(ForInIteratorObject, Object)
    Q_MANAGED_TYPE(ForInIterator)
    V4_PROTOTYPE(forInIteratorPrototype)
    V4_NEEDS_DESTROY

    PropertyKey nextProperty() const;
};

inline
void Heap::ForInIteratorObject::init(QV4::Object *o)
{
    Object::init();
    if (!o)
        return;
    object.set(o->engine(), o->d());
    current.set(o->engine(), o->d());
    Scope scope(o);
    ScopedObject obj(scope);
    iterator = o->ownPropertyKeys(obj.getRef());
    target.set(o->engine(), obj->d());
}


}

QT_END_NAMESPACE

#endif
