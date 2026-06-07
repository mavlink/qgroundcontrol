// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QELFPARSER_P_H
#define QELFPARSER_P_H

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

#include <qendian.h>
#include "qlibrary_p.h"

QT_REQUIRE_CONFIG(library);

#ifdef Q_OF_ELF

QT_BEGIN_NAMESPACE

struct QElfParser
{
    static QLibraryScanResult parse(QByteArrayView data, QString *errMsg);
};

QT_END_NAMESPACE

#endif // Q_OF_ELF

#endif // QELFPARSER_P_H
