// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QMLJS_MATH_H
#define QMLJS_MATH_H

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

#include <qglobal.h>

#include <private/qv4staticvalue_p.h>
#include <QtCore/qnumeric.h>
#include <QtCore/private/qnumeric_p.h>
#include <cmath>

#if defined(Q_CC_GNU)
#define QMLJS_READONLY __attribute((const))
#else
#define QMLJS_READONLY
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

static inline QMLJS_READONLY ReturnedValue add_int32(int a, int b)
{
    int result;
    if (Q_UNLIKELY(qAddOverflow(a, b, &result)))
        return StaticValue::fromDouble(static_cast<double>(a) + b).asReturnedValue();
    return StaticValue::fromInt32(result).asReturnedValue();
}

static inline QMLJS_READONLY ReturnedValue sub_int32(int a, int b)
{
    int result;
    if (Q_UNLIKELY(qSubOverflow(a, b, &result)))
        return StaticValue::fromDouble(static_cast<double>(a) - b).asReturnedValue();
    return StaticValue::fromInt32(result).asReturnedValue();
}

static inline QMLJS_READONLY ReturnedValue mul_int32(int a, int b)
{
    int result;
    if (Q_UNLIKELY(qMulOverflow(a, b, &result)))
        return StaticValue::fromDouble(static_cast<double>(a) * b).asReturnedValue();
    // need to handle the case where one number is negative and the other 0 ==> -0
    if (((a < 0) xor (b < 0)) && (result == 0))
        return StaticValue::fromDouble(-0.0).asReturnedValue();
    return StaticValue::fromInt32(result).asReturnedValue();
}

}

QT_END_NAMESPACE

#ifdef QMLJS_READONLY
#undef QMLJS_READONLY
#endif

#endif // QMLJS_MATH_H
