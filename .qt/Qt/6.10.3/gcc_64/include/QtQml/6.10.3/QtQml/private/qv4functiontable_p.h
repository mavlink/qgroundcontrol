// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4FUNCTIONTABLE_P_H
#define QV4FUNCTIONTABLE_P_H

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

#include <QtQml/private/qqmlglobal_p.h>

namespace JSC {
class MacroAssemblerCodeRef;
}

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Function;

void generateFunctionTable(Function *function, JSC::MacroAssemblerCodeRef *codeRef);
void destroyFunctionTable(Function *function, JSC::MacroAssemblerCodeRef *codeRef);

size_t exceptionHandlerSize();

}

QT_END_NAMESPACE

#endif // QV4FUNCTIONTABLE_P_H
