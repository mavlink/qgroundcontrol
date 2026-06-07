// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTCASE_P_H
#define QTESTCASE_P_H

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

#include <QtTest/qtestcase.h>
#include <QtTest/qttestglobal.h>

#include <QtCore/qstring.h>
#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE

namespace QTest {
#if QT_CONFIG(batch_test_support)
    Q_TESTLIB_EXPORT QList<QString> qGetTestCaseNames();
    Q_TESTLIB_EXPORT TestEntryFunction qGetTestCaseEntryFunction(const QString &name);
#endif  // QT_CONFIG(batch_test_support)
} // namespace QTest

QT_END_NAMESPACE

#endif  // QTESTCASE_P_H
