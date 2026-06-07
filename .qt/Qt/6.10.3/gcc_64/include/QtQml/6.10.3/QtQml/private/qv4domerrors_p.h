// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV8DOMERRORS_P_H
#define QV8DOMERRORS_P_H

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
// From DOM-Level-3-Core spec
// http://www.w3.org/TR/DOM-Level-3-Core/core.html
#define DOMEXCEPTION_INDEX_SIZE_ERR 1
#define DOMEXCEPTION_DOMSTRING_SIZE_ERR 2
#define DOMEXCEPTION_HIERARCHY_REQUEST_ERR 3
#define DOMEXCEPTION_WRONG_DOCUMENT_ERR 4
#define DOMEXCEPTION_INVALID_CHARACTER_ERR 5
#define DOMEXCEPTION_NO_DATA_ALLOWED_ERR 6
#define DOMEXCEPTION_NO_MODIFICATION_ALLOWED_ERR 7
#define DOMEXCEPTION_NOT_FOUND_ERR 8
#define DOMEXCEPTION_NOT_SUPPORTED_ERR 9
#define DOMEXCEPTION_INUSE_ATTRIBUTE_ERR 10
#define DOMEXCEPTION_INVALID_STATE_ERR 11
#define DOMEXCEPTION_SYNTAX_ERR 12
#define DOMEXCEPTION_INVALID_MODIFICATION_ERR 13
#define DOMEXCEPTION_NAMESPACE_ERR 14
#define DOMEXCEPTION_INVALID_ACCESS_ERR 15
#define DOMEXCEPTION_VALIDATION_ERR 16
#define DOMEXCEPTION_TYPE_MISMATCH_ERR 17

#define THROW_DOM(error, string) { \
    QV4::ScopedValue v(scope, scope.engine->newString(QStringLiteral(string))); \
    QV4::ScopedObject ex(scope, scope.engine->newErrorObject(v)); \
    ex->put(QV4::ScopedString(scope, scope.engine->newIdentifier(QStringLiteral("code"))), QV4::ScopedValue(scope, QV4::Value::fromInt32(error))); \
    return scope.engine->throwError(ex); \
}

namespace QV4 {
struct ExecutionEngine;
}


void qt_add_domexceptions(QV4::ExecutionEngine *e);

QT_END_NAMESPACE

#endif // QV8DOMERRORS_P_H
