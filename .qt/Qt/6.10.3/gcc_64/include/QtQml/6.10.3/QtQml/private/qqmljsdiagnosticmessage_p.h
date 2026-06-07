// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLJSDIAGNOSTICMESSAGE_P_H
#define QQMLJSDIAGNOSTICMESSAGE_P_H

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

#include <QtCore/qlogging.h>
#include <QtCore/qstring.h>

// Include the API version here, to avoid complications when querying it for the
// QQmlSourceLocation -> line/column change.

#include "qqmljssourcelocation_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
struct DiagnosticMessage
{
    QString message;
    QtMsgType type = QtCriticalMsg;
    SourceLocation loc;

    bool isError() const
    {
        return type == QtCriticalMsg;
    }

    bool isWarning() const
    {
        return type == QtWarningMsg;
    }
};
} // namespace QQmlJS

Q_DECLARE_TYPEINFO(QQmlJS::DiagnosticMessage, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QQMLJSDIAGNOSTICMESSAGE_P_H
