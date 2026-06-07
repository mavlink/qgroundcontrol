// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4MAPOBJECT_P_H
#define QV4MAPOBJECT_P_H

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

class ESTable;

namespace Heap {

struct WeakMapCtor : FunctionObject {
    void init(ExecutionEngine *engine);
};

struct MapCtor : WeakMapCtor {
    void init(ExecutionEngine *engine);
};

struct MapObject : Object {
    static void markObjects(Heap::Base *that, MarkStack *markStack);
    void init();
    void destroy();
    void removeUnmarkedKeys();

    MapObject *nextWeakMap;
    ESTable *esTable;
    bool isWeakMap;
};

}

struct WeakMapCtor: FunctionObject
{
    V4_OBJECT2(WeakMapCtor, FunctionObject)

    static ReturnedValue construct(const FunctionObject *f, const Value *argv, int argc, const Value *, bool weakMap);

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct MapCtor : WeakMapCtor
{
    V4_OBJECT2(MapCtor, WeakMapCtor)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
};

struct MapObject : Object
{
    V4_OBJECT2(MapObject, Object)
    V4_PROTOTYPE(mapPrototype)
    V4_NEEDS_DESTROY
};

struct WeakMapPrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_delete(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_has(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    Q_AUTOTEST_EXPORT static ReturnedValue method_set(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct MapPrototype : WeakMapPrototype
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_clear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_delete(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_entries(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_forEach(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_has(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_keys(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    Q_AUTOTEST_EXPORT static ReturnedValue method_set(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_size(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_values(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4


QT_END_NAMESPACE

#endif // QV4MAPOBJECT_P_H

