#pragma once

#ifdef QGC_PORTABLE_TEST
#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtTest/QTest>

class PortableTest : public QObject
{
    Q_OBJECT

protected:
    /// Stand-in for UnitTest::expectLogMessage(): the next message that matches is expected, and does not fail the
    /// test. Follow the code that logs it with verifyExpectedLogMessage(). One message can be expected at a time.
    void expectLogMessage(const char* category, QtMsgType type, const QRegularExpression& pattern)
    {
        _expectedLog() = {category, type, pattern, false};
        if (!s_nextHandler) {
            s_nextHandler = qInstallMessageHandler(&PortableTest::_handleMessage);
        }
    }

    void verifyExpectedLogMessage()
    {
        _stopExpectingLogMessage();
        QVERIFY2(_expectedLog().seen, "The expected log message was not emitted");
    }

private slots:

    void init()
    {
        _stopExpectingLogMessage();
        QTest::failOnWarning();
    }

private:
    struct ExpectedLog
    {
        QByteArray category;
        QtMsgType type = QtDebugMsg;
        QRegularExpression pattern;
        bool seen = false;
    };

    static ExpectedLog& _expectedLog()
    {
        static ExpectedLog expectedLog;
        return expectedLog;
    }

    static void _stopExpectingLogMessage()
    {
        if (s_nextHandler) {
            (void) qInstallMessageHandler(s_nextHandler);
            s_nextHandler = nullptr;
        }
    }

    static void _handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
    {
        ExpectedLog& expectedLog = _expectedLog();
        if (!expectedLog.seen && (type == expectedLog.type) && (expectedLog.category == context.category) &&
            expectedLog.pattern.match(message).hasMatch()) {
            expectedLog.seen = true;
            return;
        }
        s_nextHandler(type, context, message);
    }

    /// Qt Test's handler, which fails the test on any other warning; set while a message is expected
    static inline QtMessageHandler s_nextHandler = nullptr;
};

#define QGC_REGISTER_PORTABLE_TEST(className, ...) QTEST_GUILESS_MAIN(className)
#else
#include "UnitTest.h"

using PortableTest = UnitTest;
#define QGC_REGISTER_PORTABLE_TEST(className, ...) UT_REGISTER_TEST_LIGHTWEIGHT(className, __VA_ARGS__)
#endif
