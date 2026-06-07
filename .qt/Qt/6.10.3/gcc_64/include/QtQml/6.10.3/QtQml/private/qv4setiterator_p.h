// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4SETITERATOR_P_H
#define QV4SETITERATOR_P_H

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

#include "qv4object_p.h"
#include "qv4iterator_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

#define SetIteratorObjectMembers(class, Member) \
    Member(class, Pointer, Object *, iteratedSet) \
    Member(class, NoMark, IteratorKind, iterationKind) \
    Member(class, NoMark, quint32, setNextIndex)

DECLARE_HEAP_OBJECT(SetIteratorObject, Object) {
    DECLARE_MARKOBJECTS(SetIteratorObject)
    void init(Object *obj, QV4::ExecutionEngine *engine)
    {
        Object::init();
        this->iteratedSet.set(engine, obj);
        this->setNextIndex = 0;
    }
};

}

struct SetIteratorPrototype : Object
{
    V4_PROTOTYPE(iteratorPrototype)
    void init(ExecutionEngine *engine);

    static ReturnedValue method_next(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
};

struct SetIteratorObject : Object
{
    V4_OBJECT2(SetIteratorObject, Object)
    Q_MANAGED_TYPE(SetIteratorObject)
    V4_PROTOTYPE(setIteratorPrototype)

    void init(ExecutionEngine *engine);
};


}

QT_END_NAMESPACE

#endif // QV4SETITERATOR_P_H

