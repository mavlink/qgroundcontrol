// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTESTOPTIONS_P_H
#define QTESTOPTIONS_P_H

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

# include <QtTest/qttestglobal.h>

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE


namespace QTest
{
    extern Q_TESTLIB_EXPORT bool printAvailableFunctions;
    extern Q_TESTLIB_EXPORT QStringList testFunctions;
    extern Q_TESTLIB_EXPORT QStringList testTags;
};

QT_END_NAMESPACE

#endif
