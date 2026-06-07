// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV8SQLERRORS_P_H
#define QV8SQLERRORS_P_H

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

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE
#define SQLEXCEPTION_UNKNOWN_ERR 1
#define SQLEXCEPTION_DATABASE_ERR 2
#define SQLEXCEPTION_VERSION_ERR 3
#define SQLEXCEPTION_TOO_LARGE_ERR 4
#define SQLEXCEPTION_QUOTA_ERR 5
#define SQLEXCEPTION_SYNTAX_ERR 6
#define SQLEXCEPTION_CONSTRAINT_ERR 7
#define SQLEXCEPTION_TIMEOUT_ERR 8

namespace QV4 {
struct ExecutionEngine;
}

void qt_add_sqlexceptions(QV4::ExecutionEngine *engine);

QT_END_NAMESPACE

#endif // QV8SQLERRORS_P_H
