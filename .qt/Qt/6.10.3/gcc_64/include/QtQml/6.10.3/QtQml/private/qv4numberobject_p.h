// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4NUMBEROBJECT_H
#define QV4NUMBEROBJECT_H

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

namespace QV4 {

namespace Heap {

struct NumberCtor : FunctionObject {
    void init(ExecutionEngine *engine);
};

}

class NumberLocale : public QLocale
{
public:
    static const NumberLocale *instance();
    const int defaultDoublePrecision;
protected:
    NumberLocale();
};

struct NumberCtor: FunctionObject
{
    V4_OBJECT2(NumberCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct NumberPrototype: NumberObject
{
    V4_PROTOTYPE(objectPrototype)
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_isFinite(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_isInteger(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_isSafeInteger(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_isNaN(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLocaleString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toFixed(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toExponential(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toPrecision(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
