// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLIMPORTRESOLVER_P_H
#define QQMLIMPORTRESOLVER_P_H

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

#include <private/qtqmlcompilerglobal_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

Q_QML_COMPILER_EXPORT QStringList qQmlResolveImportPaths(QStringView uri, const QStringList &basePaths,
                                   QTypeRevision version);

QT_END_NAMESPACE

#endif // QQMLIMPORTRESOLVER_P_H
