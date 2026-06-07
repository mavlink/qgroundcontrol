// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4ARRAYOBJECT_H
#define QV4ARRAYOBJECT_H

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
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

template<typename Number>
inline bool qIsAtMostUintLimit(Number length, uint limit = std::numeric_limits<uint>::max())
{
    // Use the type with the larger positive range to do the comparison.

    Q_ASSERT(length >= 0);
    if constexpr (sizeof(Number) > sizeof(uint)) {
        return length <= Number(limit);
    } else {
        return uint(length) <= limit;
    }
}

template<typename Number>
inline bool qIsAtMostSizetypeLimit(
        Number length, qsizetype limit = std::numeric_limits<qsizetype>::max())
{
    // Use the type with the larger positive range to do the comparison.

    Q_ASSERT(limit >= 0);
    if constexpr (sizeof(qsizetype) > sizeof(Number)) {
        return qsizetype(length) <= limit;
    } else {
        return length <= Number(limit);
    }
}

namespace QV4 {

namespace Heap {

struct ArrayCtor : FunctionObject {
    void init(QV4::ExecutionEngine *engine);
};

}

struct ArrayCtor: FunctionObject
{
    V4_OBJECT2(ArrayCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct ArrayPrototype: ArrayObject
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_isArray(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_from(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_of(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLocaleString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_concat(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_copyWithin(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_entries(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_find(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_findIndex(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_join(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_pop(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_push(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_reverse(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_shift(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sort(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_splice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_unshift(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_includes(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_indexOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_keys(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_lastIndexOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_every(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_fill(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_some(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_forEach(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_map(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_filter(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_reduce(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_reduceRight(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_values(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    // while this function is implemented here, it's the same for many other JS classes, so the corresponding JS function
    // is instantiated in the engine, and it can be added to any JS object through Object::addSymbolSpecies()
    static ReturnedValue method_get_species(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
