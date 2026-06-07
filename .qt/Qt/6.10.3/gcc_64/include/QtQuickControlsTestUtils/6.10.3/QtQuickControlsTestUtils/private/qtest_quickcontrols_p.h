// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTEST_QUICKCONTROLS_P_H
#define QTEST_QUICKCONTROLS_P_H

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

#include <QtTest/qtest.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtGui/qguiapplication.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

inline QStringList testStyles()
{
    // It's not enough to check if the name is empty, because since Qt 6
    // we set an appropriate style for the platform if no style was specified.
    // Also, we need the name check to come first, as isUsingDefaultStyle() does not do any resolving,
    // and so its return value wouldn't be correct otherwise.
    if (QQuickStyle::name().isEmpty() || QQuickStylePrivate::isUsingDefaultStyle())
        return QQuickStylePrivate::builtInStyles();
    return QStringList(QQuickStyle::name());
}

inline int runTests(QObject *testObject, int argc, char *argv[])
{
    int res = 0;
    QTest::qInit(testObject, argc, argv);
    const QByteArray testObjectName = QTestResult::currentTestObjectName();
    // setCurrentTestObject() takes a C string, which means we must ensure
    // that the string we pass in lives long enough (i.e until the next call
    // to setCurrentTestObject()), so store the name outside of the loop.
    QByteArray testName;
    const QStringList styles = testStyles();
    for (const QString &style : styles) {
        qmlClearTypeRegistrations();
        QQuickStyle::setStyle(style);
        testName = testObjectName + "::" + style.toLocal8Bit();
        QTestResult::setCurrentTestObject(testName);
        res += QTest::qRun();
    }
    QTestResult::setCurrentTestObject(testObjectName);
    QTest::qCleanup();
    return res;
}

#define QTEST_QUICKCONTROLS_MAIN(TestCase) \
int main(int argc, char *argv[]) \
{ \
    qputenv("QML_NO_TOUCH_COMPRESSION", "1"); \
    QGuiApplication app(argc, argv); \
    TestCase tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return runTests(&tc, argc, argv); \
}

#endif // QTEST_QUICKCONTROLS_P_H
