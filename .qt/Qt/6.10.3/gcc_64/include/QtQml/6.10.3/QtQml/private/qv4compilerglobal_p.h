// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4COMPILERGLOBAL_H
#define QV4COMPILERGLOBAL_H

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

#include <QtCore/qglobal.h>
#include <QString>

#include <private/qtqmlcompilerglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

enum class ObjectLiteralArgument {
    Value,
    Method,
    Getter,
    Setter
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4COMPILERGLOBAL_H
