// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4VME_MOTH_P_H
#define QV4VME_MOTH_P_H

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

#include <private/qv4global_p.h>
#include <private/qv4staticvalue_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Moth {

class VME
{
public:
    struct ExecData {
        QV4::Function *function;
        const QV4::ExecutionContext *scope;
    };

    static void exec(MetaTypesStackFrame *frame, ExecutionEngine *engine);
    static QV4::ReturnedValue exec(JSTypesStackFrame *frame, ExecutionEngine *engine);
    static QV4::ReturnedValue interpret(JSTypesStackFrame *frame, ExecutionEngine *engine, const char *codeEntry);
};

} // namespace Moth
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4VME_MOTH_P_H
