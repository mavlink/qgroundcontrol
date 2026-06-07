// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4DATAVIEW_H
#define QV4DATAVIEW_H

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

struct DataViewCtor : FunctionObject {
    void init(ExecutionEngine *engine);
};

#define DataViewMembers(class, Member) \
    Member(class, Pointer, SharedArrayBuffer *, buffer) \
    Member(class, NoMark, uint, byteLength) \
    Member(class, NoMark, uint, byteOffset)

DECLARE_HEAP_OBJECT(DataView, Object) {
    DECLARE_MARKOBJECTS(DataView)
    void init() { Object::init(); }
};

}

struct DataViewCtor: FunctionObject
{
    V4_OBJECT2(DataViewCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct DataView : Object
{
    V4_OBJECT2(DataView, Object)
    V4_PROTOTYPE(dataViewPrototype)
};

struct DataViewPrototype: Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_get_buffer(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_byteLength(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_byteOffset(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    template <typename T>
    static ReturnedValue method_getChar(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    template <typename T>
    static ReturnedValue method_get(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    template <typename T>
    static ReturnedValue method_getFloat(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    template <typename T>
    static ReturnedValue method_setChar(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    template <typename T>
    static ReturnedValue method_set(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    template <typename T>
    static ReturnedValue method_setFloat(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4

QT_END_NAMESPACE

#endif
