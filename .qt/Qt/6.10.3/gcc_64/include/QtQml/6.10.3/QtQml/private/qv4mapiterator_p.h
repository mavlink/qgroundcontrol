// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4MAPITERATOR_P_H
#define QV4MAPITERATOR_P_H

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

#define MapIteratorObjectMembers(class, Member) \
    Member(class, Pointer, Object *, iteratedMap) \
    Member(class, NoMark, IteratorKind, iterationKind) \
    Member(class, NoMark, quint32, mapNextIndex)

DECLARE_HEAP_OBJECT(MapIteratorObject, Object) {
    DECLARE_MARKOBJECTS(MapIteratorObject)
    void init(Object *obj, QV4::ExecutionEngine *engine)
    {
        Object::init();
        this->iteratedMap.set(engine, obj);
        this->mapNextIndex = 0;
    }
};

}

struct MapIteratorPrototype : Object
{
    V4_PROTOTYPE(iteratorPrototype)
    void init(ExecutionEngine *engine);

    static ReturnedValue method_next(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
};

struct MapIteratorObject : Object
{
    V4_OBJECT2(MapIteratorObject, Object)
    Q_MANAGED_TYPE(MapIteratorObject)
    V4_PROTOTYPE(mapIteratorPrototype)

    void init(ExecutionEngine *engine);
};


}

QT_END_NAMESPACE

#endif // QV4MAPITERATOR_P_H


