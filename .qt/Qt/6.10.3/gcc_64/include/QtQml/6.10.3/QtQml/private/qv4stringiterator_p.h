// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4STRINGITERATOR_P_H
#define QV4STRINGITERATOR_P_H

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
#include "qv4string_p.h"

QT_BEGIN_NAMESPACE


namespace QV4 {

namespace Heap {

#define StringIteratorObjectMembers(class, Member) \
    Member(class, Pointer, String *, iteratedString) \
    Member(class, NoMark, quint32, nextIndex)

DECLARE_HEAP_OBJECT(StringIteratorObject, Object) {
    DECLARE_MARKOBJECTS(StringIteratorObject)
    void init(String *str, QV4::ExecutionEngine *engine)
    {
        Object::init();
        this->iteratedString.set(engine, str);
        this->nextIndex = 0;
    }
};

}

struct StringIteratorPrototype : Object
{
    V4_PROTOTYPE(iteratorPrototype)
    void init(ExecutionEngine *engine);

    static ReturnedValue method_next(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
};

struct StringIteratorObject : Object
{
    V4_OBJECT2(StringIteratorObject, Object)
    Q_MANAGED_TYPE(StringIteratorObject)
    V4_PROTOTYPE(stringIteratorPrototype)

    void init(ExecutionEngine *engine);
};


}

QT_END_NAMESPACE

#endif // QV4ARRAYITERATOR_P_H

