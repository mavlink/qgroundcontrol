// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4MATHOBJECT_H
#define QV4MATHOBJECT_H

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

struct MathObject : Object {
    void init();
};

}

struct MathObject: Object
{
    V4_OBJECT2(MathObject, Object)
    Q_MANAGED_TYPE(MathObject)

    static ReturnedValue method_abs(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_acos(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_acosh(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_asin(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_asinh(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_atan(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_atanh(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_atan2(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_cbrt(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_ceil(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_clz32(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_cos(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_cosh(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_exp(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_expm1(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_floor(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_fround(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_hypot(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_imul(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_log(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_log10(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_log1p(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_log2(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_max(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_min(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_pow(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_random(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_round(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sign(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sin(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sinh(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sqrt(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_tan(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_tanh(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_trunc(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
