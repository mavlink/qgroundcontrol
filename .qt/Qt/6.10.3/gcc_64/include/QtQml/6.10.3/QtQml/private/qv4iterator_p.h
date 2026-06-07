// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4ITERATOR_P_H
#define QV4ITERATOR_P_H

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

QT_BEGIN_NAMESPACE


namespace QV4 {

enum IteratorKind {
    KeyIteratorKind,
    ValueIteratorKind,
    KeyValueIteratorKind
};

struct IteratorPrototype : Object
{
    void init(ExecutionEngine *engine);

    static ReturnedValue method_iterator(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue createIterResultObject(ExecutionEngine *engine, const Value &value, bool done);
};

}

QT_END_NAMESPACE

#endif // QV4ARRAYITERATOR_P_H

