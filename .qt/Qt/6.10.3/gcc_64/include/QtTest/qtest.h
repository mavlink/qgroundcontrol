// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEST_H
#define QTEST_H

#if 0
#pragma qt_class(QTest)
#endif

#include <QtTest/qttestglobal.h>
#include <QtTest/qtestcase.h>
#include <QtTest/qtestdata.h>
#include <QtTest/qtesttostring.h>
#include <QtTest/qbenchmark.h>

#if defined(TESTCASE_LOWDPI)
#include <QtCore/qcoreapplication.h>
#endif

#include <cstdio>
#include <initializer_list>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QTest
{

template<>
inline bool qCompare(QString const &t1, QLatin1StringView const &t2, const char *actual,
                     const char *expected, const char *file, int line)
{
    return qCompare(t1, QString(t2), actual, expected, file, line);
}
template<>
inline bool qCompare(QLatin1StringView const &t1, QString const &t2, const char *actual,
                     const char *expected, const char *file, int line)
{
    return qCompare(QString(t1), t2, actual, expected, file, line);
}

// Compare sequences of equal size
template <typename ActualIterator, typename ExpectedIterator>
bool _q_compareSequence(ActualIterator actualIt, ActualIterator actualEnd,
                        ExpectedIterator expectedBegin, ExpectedIterator expectedEnd,
                        const char *actual, const char *expected,
                        const char *file, int line)
{
    char msg[1024];
    msg[0] = '\0';

    const qsizetype actualSize = actualEnd - actualIt;
    const qsizetype expectedSize = expectedEnd - expectedBegin;
    bool isOk = actualSize == expectedSize;

    if (!isOk) {
        std::snprintf(msg, sizeof(msg), "Compared lists have different sizes.\n"
                      "   Actual   (%s) size: %lld\n"
                      "   Expected (%s) size: %lld",
                      actual, qlonglong(actualSize),
                      expected, qlonglong(expectedSize));
    }

    for (auto expectedIt = expectedBegin; isOk && expectedIt < expectedEnd; ++actualIt, ++expectedIt) {
        if (!(*actualIt == *expectedIt)) {
            const qsizetype i = qsizetype(expectedIt - expectedBegin);
            char *val1 = toString(*actualIt);
            char *val2 = toString(*expectedIt);

            std::snprintf(msg, sizeof(msg), "Compared lists differ at index %lld.\n"
                          "   Actual   (%s): %s\n"
                          "   Expected (%s): %s",
                          qlonglong(i), actual, val1 ? val1 : "<null>",
                          expected, val2 ? val2 : "<null>");
            isOk = false;

            delete [] val1;
            delete [] val2;
        }
    }
    return compare_helper(isOk, msg, actual, expected, file, line);
}

namespace Internal {

#if defined(TESTCASE_LOWDPI)
void disableHighDpi()
{
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
}
Q_CONSTRUCTOR_FUNCTION(disableHighDpi);
#endif

} // namespace Internal

template <typename T>
inline bool qCompare(QList<T> const &t1, QList<T> const &t2, const char *actual, const char *expected,
                     const char *file, int line)
{
    return _q_compareSequence(t1.cbegin(), t1.cend(), t2.cbegin(), t2.cend(),
                                     actual, expected, file, line);
}

template <typename T, int N>
bool qCompare(QList<T> const &t1, std::initializer_list<T> t2,
              const char *actual, const char *expected,
              const char *file, int line)
{
    return _q_compareSequence(t1.cbegin(), t1.cend(), t2.cbegin(), t2.cend(),
                                     actual, expected, file, line);
}

// Compare QList against array
template <typename T, int N>
bool qCompare(QList<T> const &t1, const T (& t2)[N],
              const char *actual, const char *expected,
              const char *file, int line)
{
    return _q_compareSequence(t1.cbegin(), t1.cend(), t2, t2 + N,
                                     actual, expected, file, line);
}

template <typename T>
inline bool qCompare(QFlags<T> const &t1, T const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    using Int = typename QFlags<T>::Int;
    return qCompare(Int(t1), Int(t2), actual, expected, file, line);
}

template <typename T>
inline bool qCompare(QFlags<T> const &t1, int const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    using Int = typename QFlags<T>::Int;
    return qCompare(Int(t1), Int(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(qint64 const &t1, qint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<qint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(qint64 const &t1, quint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<qint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(quint64 const &t1, quint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<quint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(qint32 const &t1, qint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<qint64>(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(quint32 const &t1, qint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<qint64>(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(quint32 const &t1, quint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<quint64>(t1), t2, actual, expected, file, line);
}
namespace Internal {

template <typename T>
class HasInitMain // SFINAE test for the presence of initMain()
{
private:
    using YesType = char[1];
    using NoType = char[2];

    template <typename C> static YesType& test( decltype(&C::initMain) ) ;
    template <typename C> static NoType& test(...);

public:
    enum { value = sizeof(test<T>(nullptr)) == sizeof(YesType) };
};

template<typename T>
typename std::enable_if<HasInitMain<T>::value, void>::type callInitMain()
{
    T::initMain();
}

template<typename T>
typename std::enable_if<!HasInitMain<T>::value, void>::type callInitMain()
{
}

} // namespace Internal

} // namespace QTest
QT_END_NAMESPACE

#ifdef QT_TESTCASE_BUILDDIR
#  define QTEST_SET_MAIN_SOURCE_PATH  QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR);
#else
#  define QTEST_SET_MAIN_SOURCE_PATH  QTest::setMainSourcePath(__FILE__);
#endif

// Hooks for coverage-testing of QTestLib itself:
#if QT_CONFIG(testlib_selfcover) && defined(__COVERAGESCANNER__)
struct QtCoverageScanner
{
    QtCoverageScanner(const char *name)
    {
        __coveragescanner_clear();
        __coveragescanner_testname(name);
    }
    ~QtCoverageScanner()
    {
        __coveragescanner_save();
        __coveragescanner_testname("");
    }
};
#define TESTLIB_SELFCOVERAGE_START(name) QtCoverageScanner _qtCoverageScanner(name);
#else
#define TESTLIB_SELFCOVERAGE_START(name)
#endif

#if !defined(QTEST_BATCH_TESTS)
// Internal (but used by some testlib selftests to hack argc and argv).
// Tests should normally implement initMain() if they have set-up to do before
// instantiating the test class.
#define QTEST_MAIN_WRAPPER(TestObject, ...) \
int main(int argc, char *argv[]) \
{ \
    TESTLIB_SELFCOVERAGE_START(#TestObject) \
    QT_PREPEND_NAMESPACE(QTest::Internal::callInitMain)<TestObject>(); \
    __VA_ARGS__ \
    TestObject tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv); \
}
#else
// BATCHED_TEST_NAME is defined for each test in a batch in cmake. Some odd
// targets, like snippets, don't define it though. Play safe by providing a
// default value.
#if !defined(BATCHED_TEST_NAME)
#define BATCHED_TEST_NAME "other"
#endif
#define QTEST_MAIN_WRAPPER(TestObject, ...) \
\
void qRegister##TestObject() \
{ \
    auto runTest = [](int argc, char** argv) -> int { \
        TESTLIB_SELFCOVERAGE_START(TestObject) \
        QT_PREPEND_NAMESPACE(QTest::Internal::callInitMain)<TestObject>(); \
        __VA_ARGS__ \
        TestObject tc; \
        QTEST_SET_MAIN_SOURCE_PATH \
        return QTest::qExec(&tc, argc, argv); \
    }; \
    QTest::qRegisterTestCase(QStringLiteral(BATCHED_TEST_NAME), runTest); \
} \
\
Q_CONSTRUCTOR_FUNCTION(qRegister##TestObject)
#endif

// For when you don't even want a QApplication:
#define QTEST_APPLESS_MAIN(TestObject) QTEST_MAIN_WRAPPER(TestObject)

#include <QtTest/qtestsystem.h>

#if defined(QT_NETWORK_LIB)
#  include <QtTest/qtest_network.h>
#endif

// Internal
#define QTEST_QAPP_SETUP(klaz) \
    klaz app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true);

#if defined(QT_WIDGETS_LIB)
#  include <QtTest/qtest_widgets.h>
#  ifdef QT_KEYPAD_NAVIGATION
#    define QTEST_DISABLE_KEYPAD_NAVIGATION QApplication::setNavigationMode(Qt::NavigationModeNone);
#  else
#    define QTEST_DISABLE_KEYPAD_NAVIGATION
#  endif
// Internal
#  define QTEST_MAIN_SETUP() QTEST_QAPP_SETUP(QApplication) QTEST_DISABLE_KEYPAD_NAVIGATION
#elif defined(QT_GUI_LIB)
#  include <QtTest/qtest_gui.h>
// Internal
#  define QTEST_MAIN_SETUP() QTEST_QAPP_SETUP(QGuiApplication)
#else
// Internal
#  define QTEST_MAIN_SETUP() QTEST_QAPP_SETUP(QCoreApplication)
#endif // QT_GUI_LIB

// For most tests:
#define QTEST_MAIN(TestObject) QTEST_MAIN_WRAPPER(TestObject, QTEST_MAIN_SETUP())

// For command-line tests
#define QTEST_GUILESS_MAIN(TestObject) \
    QTEST_MAIN_WRAPPER(TestObject, QTEST_QAPP_SETUP(QCoreApplication))

#endif
