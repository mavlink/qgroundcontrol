// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDOMASTDUMPER_P_H
#define QQMLDOMASTDUMPER_P_H

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

#include "qqmldom_global.h"
#include "qqmldomconstants_p.h"
#include "qqmldomstringdumper_p.h"

#include <QtQml/private/qqmljsglobal_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QDebug;

namespace QQmlJS {
namespace Dom {

inline QStringView noStr(SourceLocation)
{
    return QStringView();
}

QMLDOM_EXPORT QString lineDiff(QString s1, QString s2, int nContext);
QMLDOM_EXPORT QString astNodeDiff(AST::Node *n1, AST::Node *n2, int nContext = 3,
                                  AstDumperOptions opt = AstDumperOption::None, int indent = 0,
                                  function_ref<QStringView(SourceLocation)> loc2str1 = noStr,
                                  function_ref<QStringView(SourceLocation)> loc2str2 = noStr);
QMLDOM_EXPORT void astNodeDumper(const Sink &s, AST::Node *n, AstDumperOptions opt = AstDumperOption::None,
                                 int indent = 1, int baseIndent = 0,
                                 function_ref<QStringView(SourceLocation)> loc2str = noStr);
QMLDOM_EXPORT QString astNodeDump(AST::Node *n, AstDumperOptions opt = AstDumperOption::None,
                                  int indent = 1, int baseIndent = 0,
                                  function_ref<QStringView(SourceLocation)> loc2str = noStr);

QMLDOM_EXPORT QDebug operator<<(QDebug d, AST::Node *n);

} // namespace Dom
} // namespace AST

QT_END_NAMESPACE

#endif // QQMLDOMASTDUMPER_P_H
