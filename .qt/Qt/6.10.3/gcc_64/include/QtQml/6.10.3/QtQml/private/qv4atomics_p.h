// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4ATOMICS_H
#define QV4ATOMICS_H

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
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct Atomics : Object {
    void init();
};

}

struct Atomics : Object
{
    V4_OBJECT2(Atomics, Object)

    static ReturnedValue method_add(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_and(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_compareExchange(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_exchange(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_isLockFree(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_load(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_or(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_store(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sub(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_wait(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_wake(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_xor(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4

QT_END_NAMESPACE

#endif
