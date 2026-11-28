#pragma once

#ifdef QGC_PORTABLE_TEST
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtTest/QTest>

#include "UnitTestBase.h"

/// Harness of a standalone test executable: UnitTestBase without the application, capturing the log itself
class PortableTest : public UnitTestBase
{
    Q_OBJECT

protected slots:
    /// Turns off debug and info messages, and installs the capture of the log on top of QTest's message handler
    virtual void initTestCase();

    /// Restores QTest's message handler
    virtual void cleanupTestCase();

protected:
    void startLogCapture() override;
    void stopLogCapture() override;
    QList<TestLogMessage> capturedLogMessages() const override;
};

#define QGC_REGISTER_PORTABLE_TEST(className, ...) QTEST_GUILESS_MAIN(className)
#else
#include "UnitTest.h"

using PortableTest = UnitTest;
#define QGC_REGISTER_PORTABLE_TEST(className, ...) UT_REGISTER_TEST_LIGHTWEIGHT(className, __VA_ARGS__)
#endif
