// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QUICKTEST_H
#define QUICKTEST_H

#include <QtQuickTest/quicktestglobal.h>
#include <QtTest/qtest.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickWindow;

Q_QMLTEST_EXPORT int quick_test_main(int argc, char **argv, const char *name, const char *sourceDir);
Q_QMLTEST_EXPORT int quick_test_main_with_setup(int argc, char **argv, const char *name, const char *sourceDir, QObject *setup);

#ifdef QUICK_TEST_SOURCE_DIR

#define QUICK_TEST_MAIN(name) \
    int main(int argc, char **argv) \
    { \
        QTEST_SET_MAIN_SOURCE_PATH \
        return quick_test_main(argc, argv, #name, QUICK_TEST_SOURCE_DIR); \
    }

#define QUICK_TEST_OPENGL_MAIN(name) \
    int main(int argc, char **argv) \
    { \
        QTEST_SET_MAIN_SOURCE_PATH \
        return quick_test_main(argc, argv, #name, QUICK_TEST_SOURCE_DIR); \
    }

#define QUICK_TEST_MAIN_WITH_SETUP(name, QuickTestSetupClass) \
    int main(int argc, char **argv) \
    { \
        QTEST_SET_MAIN_SOURCE_PATH \
        QuickTestSetupClass setup; \
        return quick_test_main_with_setup(argc, argv, #name, QUICK_TEST_SOURCE_DIR, &setup); \
    }

#else

#define QUICK_TEST_MAIN(name) \
    int main(int argc, char **argv) \
    { \
        QTEST_SET_MAIN_SOURCE_PATH \
        return quick_test_main(argc, argv, #name, nullptr); \
    }

#define QUICK_TEST_OPENGL_MAIN(name) \
    int main(int argc, char **argv) \
    { \
        QTEST_SET_MAIN_SOURCE_PATH \
        return quick_test_main(argc, argv, #name, nullptr); \
    }

#define QUICK_TEST_MAIN_WITH_SETUP(name, QuickTestSetupClass) \
    int main(int argc, char **argv) \
    { \
        QTEST_SET_MAIN_SOURCE_PATH \
        QuickTestSetupClass setup; \
        return quick_test_main_with_setup(argc, argv, #name, nullptr, &setup); \
    }

#endif

namespace QQuickTest {
static const int defaultTimeout = 5000;

Q_QMLTEST_EXPORT bool qIsPolishScheduled(const QQuickItem *item);
Q_QMLTEST_EXPORT bool qIsPolishScheduled(const QQuickWindow *window);

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
#if QT_DEPRECATED_SINCE(6, 4)
    QT_DEPRECATED_X("Use qWaitForPolish(QQuickItem *) instead")
    Q_QMLTEST_EXPORT bool qWaitForItemPolished(const QQuickItem *item, int timeout = defaultTimeout);
#endif
#endif
Q_QMLTEST_EXPORT bool qWaitForPolish(const QQuickItem *item, int timeout = defaultTimeout);
Q_QMLTEST_EXPORT bool qWaitForPolish(const QQuickWindow *window, int timeout = defaultTimeout);

namespace Private {
// These are "private" functions that we need in the public macros and hence
// can't live in private headers. This is the same as what qtestcase.h does with e.g. qCaught.
// controlstestutils_p.h has an overload for QQuickPopup.
[[nodiscard]] Q_QMLTEST_EXPORT QByteArray qActiveFocusFailureMessage(const QQuickItem *item);
} // namespace Private
} // namespace QQuickTest

#define QVERIFY_ACTIVE_FOCUS(item) \
    do { \
        QVERIFY2(item->hasActiveFocus(), QQuickTest::Private::qActiveFocusFailureMessage(item).constData()); \
    } while (false)

#define QTRY_VERIFY_ACTIVE_FOCUS(item) \
    do { \
        QTRY_VERIFY2(item->hasActiveFocus(), QQuickTest::Private::qActiveFocusFailureMessage(item).constData()); \
    } while (false)

QT_END_NAMESPACE

#endif
