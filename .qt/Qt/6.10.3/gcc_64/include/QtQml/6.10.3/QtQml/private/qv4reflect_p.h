// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4REFLECT_H
#define QV4REFLECT_H

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

namespace Heap {

struct Reflect : Object {
    void init();
};

}

struct Reflect : Object {
    V4_OBJECT2(Reflect, Object)

    static ReturnedValue method_apply(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_construct(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_defineProperty(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_deleteProperty(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getOwnPropertyDescriptor(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_getPrototypeOf(const FunctionObject *, const Value *, const Value *argv, int argc);
    static ReturnedValue method_has(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_isExtensible(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_ownKeys(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_preventExtensions(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_set(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_setPrototypeOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif
