// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:critical reason:code-generation

#ifndef QQMLJSLOADERGENERATOR_P_H
#define QQMLJSLOADERGENERATOR_P_H

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
#include <QtCore/qlist.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

bool Q_QMLCOMPILER_EXPORT qQmlJSGenerateLoader(const QStringList &compiledFiles,
                                               const QString &outputFileName,
                                               const QStringList &resourceFileMappings,
                                               QString *errorString);
QString Q_QMLCOMPILER_EXPORT qQmlJSSymbolNamespaceForPath(const QString &relativePath);

QT_END_NAMESPACE

#endif // QQMLJSLOADERGENERATOR_P_H
