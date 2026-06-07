// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4_SYMBOL_H
#define QV4_SYMBOL_H

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

#include "qv4string_p.h"
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE


namespace QV4 {

namespace Heap {

struct SymbolCtor : FunctionObject {
    void init(ExecutionEngine *engine);
};

struct Symbol : StringOrSymbol {
    void init(const QString &s);
};

#define SymbolObjectMembers(class, Member) \
    Member(class, Pointer, Symbol *, symbol)

DECLARE_HEAP_OBJECT(SymbolObject, Object) {
    DECLARE_MARKOBJECTS(SymbolObject)
    void init(const QV4::Symbol *s);
};

}

struct SymbolCtor : FunctionObject
{
    V4_OBJECT2(SymbolCtor, FunctionObject)

    static ReturnedValue virtualCall(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *newTarget);
    static ReturnedValue method_for(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_keyFor(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct SymbolPrototype : Object
{
    V4_PROTOTYPE(objectPrototype)
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_symbolToPrimitive(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct Symbol : StringOrSymbol
{
    V4_MANAGED(Symbol, StringOrSymbol)
    Q_MANAGED_TYPE(Symbol)
    V4_INTERNALCLASS(Symbol)
    V4_NEEDS_DESTROY

    static Heap::Symbol *create(ExecutionEngine *e, const QString &s);

    QString descriptiveString() const;
};

struct SymbolObject : Object
{
    V4_OBJECT2(SymbolObject, Object)
    Q_MANAGED_TYPE(SymbolObject)
    V4_INTERNALCLASS(SymbolObject)
    V4_PROTOTYPE(symbolPrototype)
};

}

QT_END_NAMESPACE

#endif
