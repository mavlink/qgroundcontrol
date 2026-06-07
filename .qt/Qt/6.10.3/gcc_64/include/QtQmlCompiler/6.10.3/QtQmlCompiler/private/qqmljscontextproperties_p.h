// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSCONTEXTPROPERTIES_P_H
#define QQMLJSCONTEXTPROPERTIES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>

#include <QtQml/private/qqmljssourcelocation_p.h>

#include "qqmljsloggingutils_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
class LoggerCategory;

struct ContextProperty;
using ContextProperties = QHash<QString, QList<ContextProperty>>;

struct Q_QMLCOMPILER_EXPORT ContextProperty
{
    QString filename;
    QQmlJS::SourceLocation location;

    static ContextProperties collectAllFrom(const QList<QString> &rootUrls);
    static bool isWarningEnabled(const QList<QQmlJS::LoggerCategory> &categories);
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSCONTEXTPROPERTIES_P_H
