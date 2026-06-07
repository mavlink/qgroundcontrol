// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4SETOBJECT_P_H
#define QV4SETOBJECT_P_H

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
#include "qv4objectproto_p.h"
#include "qv4functionobject_p.h"
#include "qv4string_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

class ESTable;

namespace Heap {

struct WeakSetCtor : FunctionObject {
    void init(ExecutionEngine *engine);
};


struct SetCtor : WeakSetCtor {
    void init(ExecutionEngine *engine);
};

struct SetObject : Object {
    static void markObjects(Heap::Base *that, MarkStack *markStack);
    void init();
    void destroy();
    void removeUnmarkedKeys();

    ESTable *esTable;
    SetObject *nextWeakSet;
    bool isWeakSet;
};

}


struct WeakSetCtor: FunctionObject
{
    V4_OBJECT2(WeakSetCtor, FunctionObject)

    static ReturnedValue construct(const FunctionObject *f, const Value *argv, int argc, const Value *, bool weakSet);

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct SetCtor : WeakSetCtor
{
    V4_OBJECT2(SetCtor, WeakSetCtor)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
};

struct SetObject : Object
{
    V4_OBJECT2(SetObject, Object)
    V4_PROTOTYPE(setPrototype)
    V4_NEEDS_DESTROY
};

struct WeakSetPrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    Q_AUTOTEST_EXPORT static ReturnedValue method_add(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_delete(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_has(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


struct SetPrototype : WeakSetPrototype
{
    void init(ExecutionEngine *engine, Object *ctor);

    Q_AUTOTEST_EXPORT static ReturnedValue method_add(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_clear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_delete(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_entries(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_forEach(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_has(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_size(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_values(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4


QT_END_NAMESPACE

#endif // QV4SETOBJECT_P_H
