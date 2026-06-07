// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLSASOURCELOCATION_P_H
#define QQMLSASOURCELOCATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmlsasourcelocation.h"

#include <QtQml/private/qqmljssourcelocation_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlSA {

class SourceLocationPrivate
{
public:
    static const QQmlJS::SourceLocation &
    sourceLocation(const QQmlSA::SourceLocation &sourceLocation)
    {
        return reinterpret_cast<const QQmlJS::SourceLocation &>(sourceLocation.m_data);
    }

    static QQmlSA::SourceLocation
    createQQmlSASourceLocation(const QQmlJS::SourceLocation &jsLocation)
    {
        QQmlSA::SourceLocation saLocation;
        auto &internal = reinterpret_cast<QQmlJS::SourceLocation &>(saLocation.m_data);
        internal = jsLocation;
        return saLocation;
    }

    static constexpr qsizetype sizeOfSourceLocation()
    {
        return SourceLocation::sizeofSourceLocation;
    }
};

} // namespace QQmlSA

QT_END_NAMESPACE

#endif // QQMLSASOURCELOCATION_P_H
