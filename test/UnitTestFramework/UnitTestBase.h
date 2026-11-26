#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QStringView>
#include <QtTest/QTest>

#include "TestTimeout.h"

class QRegularExpression;
class QSignalSpy;

/// @file
/// @brief The part of the test harness that needs no application: waits with diagnostics and strict checking of the
/// log. UnitTest (the application's harness) and PortableTest (standalone test executables) build on it.

Q_DECLARE_LOGGING_CATEGORY(UnitTestLog)

// ============================================================================
// Test Assertion Macros
// ============================================================================

/// Wait for a signal with timeout, with better error message
/// @param spy QSignalSpy to wait on
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_SIGNAL_WAIT(spy, timeoutMs)                                    \
    QVERIFY2(                                                                  \
        UnitTestBase::waitForSignal((spy), (timeoutMs), QStringLiteral(#spy)), \
        qPrintable(QString("Timeout waiting for signal after %1ms: %2").arg(timeoutMs).arg(QStringLiteral(#spy))))

/// Verify that no signal is emitted within timeout, with better error message
/// @param spy QSignalSpy to monitor
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_NO_SIGNAL_WAIT(spy, timeoutMs)                                        \
    QVERIFY2(UnitTestBase::waitForNoSignal((spy), (timeoutMs), QStringLiteral(#spy)), \
             qPrintable(QString("Unexpected signal within %1ms: %2").arg(timeoutMs).arg(QStringLiteral(#spy))))

/// Wait for a signal count with timeout, with better error message
/// @param spy QSignalSpy to check
/// @param expectedCount Minimum signal count expected
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_SIGNAL_COUNT_WAIT(spy, expectedCount, timeoutMs)                                          \
    QVERIFY2(UnitTestBase::waitForSignalCount((spy), (expectedCount), (timeoutMs), QStringLiteral(#spy)), \
             qPrintable(QString("Timeout waiting for signal count %1 after %2ms: %3")                     \
                            .arg(expectedCount)                                                           \
                            .arg(timeoutMs)                                                               \
                            .arg(QStringLiteral(#spy))))

/// Wait for a condition with timeout.
/// Prefer QTRY_VERIFY_WITH_TIMEOUT for new code — it is the idiomatic Qt 6 equivalent.
/// @param condition Boolean expression to wait for
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_TRUE_WAIT(condition, timeoutMs) QTRY_VERIFY_WITH_TIMEOUT(condition, timeoutMs)

/// Compare with polling timeout. Retries until actual == expected or timeout.
/// Wrapper around QTRY_COMPARE_WITH_TIMEOUT for consistency with QVERIFY_TRUE_WAIT.
#define QCOMPARE_TRUE_WAIT(actual, expected, timeoutMs) QTRY_COMPARE_WITH_TIMEOUT(actual, expected, timeoutMs)

/// Declare a parameterized test function pair
/// Creates both the test slot and its _data() companion
/// Usage: UT_PARAMETERIZED_TEST(myTest) in class declaration
#define UT_PARAMETERIZED_TEST(testName) \
    void testName();                    \
    void testName##_data()

// ============================================================================
// Test Context - Improved failure diagnostics
// ============================================================================

/// RAII helper to add context to test failure messages
/// Usage: TEST_CONTEXT("Loading mission file: " + filename);
class TestContext
{
    Q_DISABLE_COPY_MOVE(TestContext)

public:
    explicit TestContext(const QString& context);
    ~TestContext();

    static QString current();
    static void push(const QString& context);
    static void pop();

private:
    static QStringList& stack();
};

/// Macro for easy context creation
#define _TEST_CONTEXT_CONCAT_(prefix, line) prefix##line
#define _TEST_CONTEXT_CONCAT(prefix, line) _TEST_CONTEXT_CONCAT_(prefix, line)
#define TEST_CONTEXT(msg) TestContext _TEST_CONTEXT_CONCAT(_testContext, __LINE__)(msg)

/// Print additional debug info (only shown on failure or in verbose mode)
#define TEST_DEBUG(msg) TestDebug::log(msg)

/// Utility class for conditional debug output
class TestDebug
{
public:
    static constexpr int kMaxBufferedMessages = 50;

    static void log(const QString& message);
    static void setVerbose(bool verbose);
    static bool isVerbose();
    static QStringList recentMessages();
    static void clearMessages();

private:
    static QStringList& messages();
    static bool& verbose();
};

// ============================================================================
// UnitTestBase
// ============================================================================

/// A log message that reached the message handler while a test function ran
struct TestLogMessage
{
    QtMsgType type = QtDebugMsg;
    QString category = {};
    QString message = {};
};

/// Base of both test harnesses. Each test function runs with strict log checking: cleanup() fails it on a log
/// message that was neither expected nor ignored. A derived harness supplies the captured messages.
class UnitTestBase : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(UnitTestBase)

public:
    explicit UnitTestBase(QObject* parent = nullptr);
    ~UnitTestBase() override;

    /// Wait for a signal with standardized timeout diagnostics.
    static bool waitForSignal(QSignalSpy& spy, std::chrono::milliseconds timeout, QStringView signalName = {});

    /// Wait to ensure no additional signal emissions occur during timeout.
    static bool waitForNoSignal(QSignalSpy& spy, std::chrono::milliseconds timeout, QStringView signalName = {});

    /// Wait until a signal spy reaches at least expectedCount emissions.
    static bool waitForSignalCount(QSignalSpy& spy, int expectedCount, std::chrono::milliseconds timeout,
                                   QStringView signalName = {});

    /// Wait for a condition with standardized timeout diagnostics.
    static bool waitForCondition(const std::function<bool()>& condition, std::chrono::milliseconds timeout,
                                 QStringView conditionName = {});

    /// Waits for a QObject to be deleted (QPointer becomes null) while draining deferred deletes.
    static bool waitForDeleted(const QPointer<QObject>& objectPtr, std::chrono::milliseconds timeout,
                               QStringView objectName = {});

    /// @name int millisecond overloads
    /// @{
    static bool waitForSignal(QSignalSpy& spy, int timeoutMs, QStringView signalName = {})
    {
        return waitForSignal(spy, std::chrono::milliseconds(timeoutMs), signalName);
    }

    static bool waitForNoSignal(QSignalSpy& spy, int timeoutMs, QStringView signalName = {})
    {
        return waitForNoSignal(spy, std::chrono::milliseconds(timeoutMs), signalName);
    }

    static bool waitForSignalCount(QSignalSpy& spy, int expectedCount, int timeoutMs, QStringView signalName = {})
    {
        return waitForSignalCount(spy, expectedCount, std::chrono::milliseconds(timeoutMs), signalName);
    }

    static bool waitForCondition(const std::function<bool()>& condition, int timeoutMs, QStringView conditionName = {})
    {
        return waitForCondition(condition, std::chrono::milliseconds(timeoutMs), conditionName);
    }

    static bool waitForDeleted(const QPointer<QObject>& objectPtr, int timeoutMs, QStringView objectName = {})
    {
        return waitForDeleted(objectPtr, std::chrono::milliseconds(timeoutMs), objectName);
    }

    /// @}

    /// Process queued events/deferred deletes to stabilize teardown between tests.
    /// If iterations <= 0, CI-aware defaults are used.
    /// If waitMs < 0, CI-aware defaults are used. If waitMs == 0, no sleep between iterations.
    static void settleEventLoopForCleanup(int iterations = 0, int waitMs = 0);

protected slots:
    /// Called before each test function: clears the expectations and starts capturing the log
    virtual void init();

    /// Called after each test function: fails it on an unexpected log message or an expectation it did not verify
    virtual void cleanup();

protected:
    /// Emits a one-time failure context dump for the currently running test function.
    void dumpFailureContextIfTestFailed(QStringView reason = {});

    /// Allows derived fixtures to append state to failure dumps.
    virtual QString failureContextSummary() const;

    /// Declare one required log message for the current test flow.
    /// Call this before the code that emits the message, then call
    /// verifyExpectedLogMessage() immediately after that code.
    ///
    /// @a category must be the exact Qt logging category string (e.g.
    /// "Utilities.QGCFileHelper"). An empty string is a precondition
    /// violation and will Q_ASSERT in debug builds.
    ///
    /// Contract:
    /// - Multiple pending expectations are allowed (FIFO verification order).
    /// - If verifyExpectedLogMessage() is never called, cleanup() fails.
    /// - After verifyExpectedLogMessage(), that expectation is consumed.
    void expectLogMessage(const char* category, QtMsgType type, const QRegularExpression& pattern);

    /// Verify and consume the next pending expectLogMessage() expectation.
    /// Fails the test if no matching message was captured after expectLogMessage().
    void verifyExpectedLogMessage();

    /// Permanently suppress all log messages matching @a pattern at level @a type
    /// in the given @a category for the duration of the test.
    ///
    /// @a category must be the exact Qt logging category string (e.g.
    /// "Utilities.QGCFileHelper"). An empty string is a precondition
    /// violation and will Q_ASSERT in debug builds.
    ///
    /// Use this sparingly and only as a last resort for non-deterministic or
    /// environment-dependent noise that cannot be tied to a precise call site.
    ///
    /// Prefer expectLogMessage(...) + verifyExpectedLogMessage() at specific
    /// points in the test whenever the log is part of the behavior under test.
    /// That pairing provides a stronger assertion and avoids masking regressions.
    void ignoreLogMessage(const char* category, QtMsgType type, const QRegularExpression& pattern);

    /// Clear the captured messages and start capturing
    virtual void startLogCapture() = 0;

    /// Stop capturing
    virtual void stopLogCapture() = 0;

    /// Messages captured since startLogCapture(), oldest first
    virtual QList<TestLogMessage> capturedLogMessages() const = 0;

private:
    // Defined in UnitTestBase.cc to keep QRegularExpression out of test TUs.
    struct ExpectedLogMessages;
    std::unique_ptr<ExpectedLogMessages> _expectedLogMessages;

    bool _failureContextDumped = false;
};
