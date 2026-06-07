// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSMANAGEDVALUE_P_H
#define QJSMANAGEDVALUE_P_H

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

#include "qjsmanagedvalue.h"

#include <private/qv4managed_p.h>

QT_BEGIN_NAMESPACE

// ### Qt 7: Use this for proper PIMPL
class QJSManagedValuePrivate
{
public:
    static QV4::Value *member(const QJSManagedValue *jsmv) { return jsmv->d; }
    static QJSManagedValue create(QV4::ExecutionEngine *engine, const QV4::Value &value)
    {
        QJSManagedValue result(engine);
        *result.d = value;
        return result;
    }
};

QT_END_NAMESPACE

#endif // QJSMANAGEDVALUE_P_H
