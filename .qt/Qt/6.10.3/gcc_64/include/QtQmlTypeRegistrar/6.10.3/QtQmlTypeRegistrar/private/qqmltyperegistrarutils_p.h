// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLTYPEREGISTRAR_UTILS_P_H
#define QQMLTYPEREGISTRAR_UTILS_P_H

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

#include "qmetatypesjsonprocessor_p.h"

#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

QTypeRevision handleInMinorVersion(QTypeRevision revision, int majorVersion);
QAnyStringView interfaceName(const Interface &iface);

QDebug warning(const MetaType &classDef);
QDebug warning(QAnyStringView fileName, int lineNumber = 0);

QDebug error(QAnyStringView fileName, int lineNumber = 0);

int mergeQtConfFiles(const QString &pathToList, const QString &pathToMergedQtConfsList);

QT_END_NAMESPACE

#endif // QQMLTYPEREGISTRAR_UTILS_P_H
