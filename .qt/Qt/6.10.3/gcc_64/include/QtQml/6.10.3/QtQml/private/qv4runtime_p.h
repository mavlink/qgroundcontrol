// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

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

#include "qv4global_p.h"
#include "qv4value_p.h"
#include "qv4runtimeapi_p.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

#undef QV4_COUNT_RUNTIME_FUNCTIONS

namespace QV4 {

#ifdef QV4_COUNT_RUNTIME_FUNCTIONS
class RuntimeCounters
{
public:
    RuntimeCounters();
    ~RuntimeCounters();

    static RuntimeCounters *instance;

    void count(const char *func);
    void count(const char *func, uint tag);
    void count(const char *func, uint tag1, uint tag2);

private:
    struct Data;
    Data *d;
};

#  define TRACE0() RuntimeCounters::instance->count(Q_FUNC_INFO);
#  define TRACE1(x) RuntimeCounters::instance->count(Q_FUNC_INFO, x.type());
#  define TRACE2(x, y) RuntimeCounters::instance->count(Q_FUNC_INFO, x.type(), y.type());
#else
#  define TRACE0()
#  define TRACE1(x)
#  define TRACE2(x, y)
#endif // QV4_COUNT_RUNTIME_FUNCTIONS

enum TypeHint {
    PREFERREDTYPE_HINT,
    NUMBER_HINT,
    STRING_HINT
};

struct Q_QML_EXPORT RuntimeHelpers {
    static ReturnedValue objectDefaultValue(const Object *object, int typeHint);
    static ReturnedValue toPrimitive(const Value &value, TypeHint typeHint);
    static ReturnedValue ordinaryToPrimitive(ExecutionEngine *engine, const Object *object, String *typeHint);

    static double stringToNumber(const QString &s);
    static Heap::String *stringFromNumber(ExecutionEngine *engine, double number);
    static double toNumber(const Value &value);
    static void numberToString(QString *result, double num, int radix = 10);

    static Heap::String *convertToString(ExecutionEngine *engine, Value value, TypeHint = STRING_HINT);
    static Heap::Object *convertToObject(ExecutionEngine *engine, const Value &value);

    static Bool equalHelper(const Value &x, const Value &y);
    static Bool strictEqual(const Value &x, const Value &y);

    static ReturnedValue addHelper(ExecutionEngine *engine, const Value &left, const Value &right);
};


// type conversion and testing
inline ReturnedValue RuntimeHelpers::toPrimitive(const Value &value, TypeHint typeHint)
{
    if (!value.isObject())
        return value.asReturnedValue();
    return RuntimeHelpers::objectDefaultValue(&reinterpret_cast<const Object &>(value), typeHint);
}

inline double RuntimeHelpers::toNumber(const Value &value)
{
    return value.toNumber();
}
} // namespace QV4

QT_END_NAMESPACE

#endif // QMLJS_RUNTIME_H
