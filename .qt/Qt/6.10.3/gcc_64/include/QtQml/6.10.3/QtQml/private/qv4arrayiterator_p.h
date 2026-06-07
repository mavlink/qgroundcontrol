// Copyright (C) 2017 Crimson AS <info@crimson.no>
// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4ARRAYITERATOR_P_H
#define QV4ARRAYITERATOR_P_H

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

#define ArrayIteratorObjectMembers(class, Member) \
    Member(class, Pointer, Object *, iteratedObject) \
    Member(class, NoMark, IteratorKind, iterationKind) \
    Member(class, NoMark, quint32, nextIndex)

DECLARE_HEAP_OBJECT(ArrayIteratorObject, Object) {
    DECLARE_MARKOBJECTS(ArrayIteratorObject)
    void init(Object *obj, QV4::ExecutionEngine *engine)
    {
        Object::init();
        this->iteratedObject.set(engine, obj);
        this->nextIndex = 0;
    }
};

}

struct ArrayIteratorPrototype : Object
{
    V4_PROTOTYPE(iteratorPrototype)
    void init(ExecutionEngine *engine);

    static ReturnedValue method_next(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
};

struct ArrayIteratorObject : Object
{
    V4_OBJECT2(ArrayIteratorObject, Object)
    Q_MANAGED_TYPE(ArrayIteratorObject)
    V4_PROTOTYPE(arrayIteratorPrototype)

    void init(ExecutionEngine *engine);
};


}

QT_END_NAMESPACE

#endif // QV4ARRAYITERATOR_P_H

