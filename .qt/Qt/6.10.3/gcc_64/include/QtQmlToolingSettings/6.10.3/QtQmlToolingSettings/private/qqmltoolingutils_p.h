// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLTOOLINGUTILS_P_H
#define QQMLTOOLINGUTILS_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcommandlineoption.h>

QT_BEGIN_NAMESPACE

class QQmlToolingUtils
{
private:
    static void warnForInvalidDirs(const QStringList &dirs, const QString &origin);
public:
    static QStringList getAndWarnForInvalidDirsFromEnv(const QString &environmentVariableName);
    static QStringList getAndWarnForInvalidDirsFromOption(const QCommandLineParser &parser,
                                                          const QCommandLineOption &option);
};

QT_END_NAMESPACE

#endif // QQMLTOOLINGUTILS_P_H
