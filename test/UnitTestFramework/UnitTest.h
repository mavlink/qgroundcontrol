#pragma once

#include <QtCore/QFlags>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QStringView>
#include <QtTest/QTest>

class QRegularExpression;

#include <chrono>
#include <functional>
#include <initializer_list>
#include <memory>

// ============================================================================
// Test Labels - Categories for filtering and organizing tests
// ============================================================================

/// Test category labels for filtering and organization
enum class TestLabel
{
    None = 0,
    Unit = 1 << 0,            ///< Fast, isolated unit tests
    Integration = 1 << 1,     ///< Tests requiring multiple components
    Vehicle = 1 << 2,         ///< Tests requiring MockLink vehicle
    MissionManager = 1 << 3,  ///< Mission planning tests
    Comms = 1 << 4,           ///< Communication/link tests
    Utilities = 1 << 5,       ///< Utility class tests
    Slow = 1 << 6,            ///< Tests that take >5 seconds
    Network = 1 << 7,         ///< Tests requiring network access
    Serial = 1 << 8,          ///< Tests that must run serially (not parallel)
    Joystick = 1 << 9,        ///< Joystick/controller tests
    AnalyzeView = 1 << 10,    ///< Log analysis and geo-tagging tests
    Terrain = 1 << 11,        ///< Terrain query and tile tests
};
Q_DECLARE_FLAGS(TestLabels, TestLabel)
Q_DECLARE_OPERATORS_FOR_FLAGS(TestLabels)

// ============================================================================
// Label Conversion Utilities
// ============================================================================

/// Converts a label name string to TestLabel enum
/// @param name Label name (case-insensitive): "unit", "integration", "vehicle", etc.
/// @return Matching TestLabel, or TestLabel::None if not recognized
TestLabel labelFromString(const QString& name);

/// Converts TestLabel enum to string
/// @param label The label to convert
/// @return Label name string (lowercase)
QString labelToString(TestLabel label);

/// Parses a comma-separated list of label names
/// @param labelStr Comma-separated labels, e.g., "unit,missionmanager"
/// @return Combined TestLabels flags
TestLabels parseLabels(const QString& labelStr);

/// Converts TestLabels flags to a formatted string
/// @param labels The labels to format
/// @return Comma-separated label names in brackets, e.g., "[unit, vehicle]"
QString labelsToString(TestLabels labels);

/// Returns a list of available label names for display
QStringList availableLabelNames();

// ============================================================================
// Test Registration Macros
// ============================================================================

/// Register a test class with labels
/// Usage: UT_REGISTER_TEST(MyTest, TestLabel::Unit, TestLabel::MissionManager)
#define UT_REGISTER_TEST(className, ...) \
    static UnitTestWrapper<className> s_##className##_registration(#className, false, {__VA_ARGS__});

/// Register a standalone test (only runs when explicitly requested)
#define UT_REGISTER_TEST_STANDALONE(className, ...) \
    static UnitTestWrapper<className> s_##className##_registration(#className, true, {__VA_ARGS__});

/// Register a pure-logic test that needs no QGCApplication (no QML engine, no vehicle,
/// no plugin scan). Such a test opts in to the lightweight harness: when the test binary
/// is launched in lightweight mode it runs against a bare QCoreApplication, skipping the
/// expensive full-app startup. In the default (full-app) run it behaves like any other
/// non-standalone test, so this macro is always safe to use.
/// Usage: UT_REGISTER_TEST_LIGHTWEIGHT(MyPureLogicTest, TestLabel::Unit, TestLabel::Utilities)
#define UT_REGISTER_TEST_LIGHTWEIGHT(className, ...) \
    static UnitTestWrapper<className> s_##className##_registration( \
        UnitTestWrapper<className>::Lightweight, #className, false, {__VA_ARGS__});

// ============================================================================
// Test Assertion Macros
// ============================================================================

// Coordinate helpers live in UnitTestCoords.h.

/// Wait for a signal with timeout, with better error message
/// @param spy QSignalSpy to wait on
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_SIGNAL_WAIT(spy, timeoutMs) \
    QVERIFY2(UnitTest::waitForSignal((spy), (timeoutMs), QStringLiteral(#spy)), \
             qPrintable(QString("Timeout waiting for signal after %1ms: %2").arg(timeoutMs).arg(QStringLiteral(#spy))))

/// Verify that no signal is emitted within timeout, with better error message
/// @param spy QSignalSpy to monitor
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_NO_SIGNAL_WAIT(spy, timeoutMs) \
    QVERIFY2(UnitTest::waitForNoSignal((spy), (timeoutMs), QStringLiteral(#spy)), \
             qPrintable(QString("Unexpected signal within %1ms: %2").arg(timeoutMs).arg(QStringLiteral(#spy))))

/// Wait for a signal count with timeout, with better error message
/// @param spy QSignalSpy to check
/// @param expectedCount Minimum signal count expected
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_SIGNAL_COUNT_WAIT(spy, expectedCount, timeoutMs) \
    QVERIFY2(UnitTest::waitForSignalCount((spy), (expectedCount), (timeoutMs), QStringLiteral(#spy)), \
             qPrintable(QString("Timeout waiting for signal count %1 after %2ms: %3") \
                            .arg(expectedCount) \
                            .arg(timeoutMs) \
                            .arg(QStringLiteral(#spy))))

/// Wait for a condition with timeout.
/// Prefer QTRY_VERIFY_WITH_TIMEOUT for new code — it is the idiomatic Qt 6 equivalent.
/// @param condition Boolean expression to wait for
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_TRUE_WAIT(condition, timeoutMs) QTRY_VERIFY_WITH_TIMEOUT(condition, timeoutMs)

/// Compare with polling timeout. Retries until actual == expected or timeout.
/// Wrapper around QTRY_COMPARE_WITH_TIMEOUT for consistency with QVERIFY_TRUE_WAIT.
#define QCOMPARE_TRUE_WAIT(actual, expected, timeoutMs) QTRY_COMPARE_WITH_TIMEOUT(actual, expected, timeoutMs)

/// Compare floating point values with configurable epsilon
#define QCOMPARE_FUZZY(actual, expected, epsilon)                                             \
    QVERIFY2(qAbs((actual) - (expected)) <= (epsilon),                                        \
             qPrintable(QString("Values differ: actual=%1, expected=%2, diff=%3, epsilon=%4") \
                            .arg(actual)                                                      \
                            .arg(expected)                                                    \
                            .arg(qAbs((actual) - (expected)))                                 \
                            .arg(epsilon)))

/// Declare a parameterized test function pair
/// Creates both the test slot and its _data() companion
/// Usage: UT_PARAMETERIZED_TEST(myTest) in class declaration
#define UT_PARAMETERIZED_TEST(testName) \
    void testName();                    \
    void testName##_data()

// ============================================================================
// Test Timeout Configuration
// ============================================================================

namespace TestTimeout {

/// Returns true when running under CI (GitHub Actions, etc.)
///
/// Evaluated on every call so tests may use EnvVarFixture to toggle the
/// CI/GITHUB_ACTIONS variables and observe the corresponding timeout changes
/// within the same process.
inline bool isCI()
{
    return qEnvironmentVariableIsSet("CI") || qEnvironmentVariableIsSet("GITHUB_ACTIONS");
}

/// Short timeout for quick operations (1s local, 2s CI)
inline std::chrono::milliseconds shortDuration()
{
    return std::chrono::milliseconds{isCI() ? 2000 : 1000};
}

/// Medium timeout for normal async operations (5s local, 10s CI)
inline std::chrono::milliseconds mediumDuration()
{
    return std::chrono::milliseconds{isCI() ? 10000 : 5000};
}

/// Long timeout for slow operations like vehicle connection (30s local, 60s CI)
inline std::chrono::milliseconds longDuration()
{
    return std::chrono::milliseconds{isCI() ? 60000 : 30000};
}

/// @name Legacy int-millisecond accessors (prefer the chrono versions above)
/// @{
inline int shortMs() { return static_cast<int>(shortDuration().count()); }
inline int mediumMs() { return static_cast<int>(mediumDuration().count()); }
inline int longMs() { return static_cast<int>(longDuration().count()); }
/// @}

/// Iteration count for stress tests.
/// Uses QGC_TEST_STRESS_ITERATIONS when set to a positive integer.
inline int stressIterations(int localDefault, int ciDefault)
{
    bool ok = false;
    const int configured = qEnvironmentVariableIntValue("QGC_TEST_STRESS_ITERATIONS", &ok);
    if (ok && configured > 0) {
        return configured;
    }
    return isCI() ? ciDefault : localDefault;
}
}  // namespace TestTimeout

// ============================================================================
// Forward Declarations
// ============================================================================

Q_DECLARE_LOGGING_CATEGORY(UnitTestLog)

class Fact;
class MissionItem;
class QSignalSpy;

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
// UnitTest Base Class
// ============================================================================

/// Base class for all QGC unit tests
/// Provides common test infrastructure including file comparison utilities,
/// temporary file management, and test lifecycle management.
/// For tests requiring a vehicle connection, use VehicleTest instead.
class UnitTest : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(UnitTest)

public:
    explicit UnitTest(QObject* parent = nullptr);
    ~UnitTest() override;

    // ========================================================================
    // Test Execution
    // ========================================================================

    /// Runs all registered unit tests matching the filter
    /// @param singleTest If non-empty, only run the test with this name
    /// @param outputFile Optional output file path for JUnit XML results
    /// @param labelFilter If set, only run tests with matching labels
    /// @return Number of test failures (0 = all passed)
    static int run(QStringView singleTest, const QString& outputFile = QString(),
                   TestLabels labelFilter = TestLabels());

    /// Sets the output file for JUnit XML test results
    static void setOutputFile(const QString& outputFile);

    /// Returns list of all registered test names
    static QStringList registeredTests();

    /// Returns list of registered test names matching label filter
    static QStringList registeredTests(TestLabels labelFilter);

    /// Returns total number of registered tests
    static int testCount();

    /// Returns names of registered tests that opted in to the lightweight harness
    /// (UT_REGISTER_TEST_LIGHTWEIGHT), optionally filtered by label. Used by the
    /// lightweight entry point to select the bare-QCoreApplication subset.
    static QStringList registeredLightweightTests(TestLabels labelFilter = TestLabels());

    /// True if @a testName was registered as lightweight.
    static bool isLightweightTest(QStringView testName);

    /// Enable verbose output for debugging
    static void setVerbose(bool verbose);

    /// Check if verbose mode is enabled
    static bool isVerbose();

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

    static bool waitForCondition(const std::function<bool()>& condition, int timeoutMs,
                                 QStringView conditionName = {})
    {
        return waitForCondition(condition, std::chrono::milliseconds(timeoutMs), conditionName);
    }

    static bool waitForDeleted(const QPointer<QObject>& objectPtr, int timeoutMs,
                               QStringView objectName = {})
    {
        return waitForDeleted(objectPtr, std::chrono::milliseconds(timeoutMs), objectName);
    }
    /// @}

    /// Process queued events/deferred deletes to stabilize teardown between tests.
    /// If iterations <= 0, CI-aware defaults are used.
    /// If waitMs < 0, CI-aware defaults are used. If waitMs == 0, no sleep between iterations.
    static void settleEventLoopForCleanup(int iterations = 0, int waitMs = 0);

    // ========================================================================
    // Test Properties
    // ========================================================================

    bool standalone() const
    {
        return _standalone;
    }

    void setStandalone(bool standalone)
    {
        _standalone = standalone;
    }

    /// True if this test opted in to the lightweight (bare QCoreApplication) harness.
    /// Pure-logic tests set this via UT_REGISTER_TEST_LIGHTWEIGHT; it has no effect on
    /// the default full-QGCApplication run, where lightweight tests run like any other.
    bool lightweight() const
    {
        return _lightweight;
    }

    void setLightweight(bool lightweight)
    {
        _lightweight = lightweight;
    }

    TestLabels labels() const
    {
        return _labels;
    }

    void setLabels(TestLabels labels)
    {
        _labels = labels;
    }

    bool hasLabel(TestLabel label) const
    {
        return _labels.testFlag(label);
    }

    bool hasAnyLabel(TestLabels labels) const
    {
        return (_labels & labels) != TestLabels();
    }

    /// Adds a unit test to the list. Should only be called by UnitTestWrapper.
    static void _addTest(UnitTest* test);

    // ========================================================================
    // File Comparison Utilities
    // ========================================================================

    /// Compares two files byte-by-byte
    /// @return true if files are identical, false otherwise
    static bool fileCompare(const QString& file1, const QString& file2);

    /// Compares file content against expected string
    /// @return true if file content matches expected string
    static bool fileContentsEqual(const QString& filePath, const QString& expectedContent);

    /// Compares file content against expected bytes
    /// @return true if file content matches expected bytes
    static bool fileContentsEqual(const QString& filePath, const QByteArray& expectedContent);

    /// Returns the path to test resource files
    /// @param relativePath Path relative to test/resources directory
    static QString testResourcePath(const QString& relativePath = QString());

protected slots:
    /// Called once before any test functions run (per test class)
    virtual void initTestCase();

    /// Called once after all test functions complete (per test class)
    virtual void cleanupTestCase();

    /// Called before each test function
    virtual void init();

    /// Called after each test function
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
    void expectLogMessage(const char *category, QtMsgType type, const QRegularExpression &pattern);

    /// Verify and consume the next pending expectLogMessage() expectation.
    /// Fails the test if no matching message was captured after expectLogMessage().
    void verifyExpectedLogMessage();

    /// Declare that a showAppMessage() call matching @a messagePattern is required.
    /// Call verifyExpectedLogMessage() after the code that should emit it.
    /// Convenience wrapper over expectLogMessage for the QGCAppMessageLog category.
    void expectAppMessage(const QRegularExpression &messagePattern);

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
    void ignoreLogMessage(const char *category, QtMsgType type, const QRegularExpression &pattern);

private:
    void _resetTestState();

    static QList<UnitTest*>& _testList();
    static QString& _outputFile();

    // Defined in UnitTest.cc to keep LogEntry.h out of test TUs.
    struct ExpectedLogMessages;
    std::unique_ptr<ExpectedLogMessages> _expectedLogMessages;

    TestLabels _labels;
    bool _unitTestRun = false;
    bool _initCalled = false;
    bool _cleanupCalled = false;
    bool _failureContextDumped = false;
    bool _standalone = false;
    bool _lightweight = false;
};

// ============================================================================
// UnitTestWrapper - Handles static registration
// ============================================================================

/// Template class for automatic test registration at static initialization time.
/// Test instances are owned by the wrapper via unique_ptr and destroyed at static
/// destruction time, avoiding leak-sanitizer reports.
template <class T>
class UnitTestWrapper
{
public:
    /// Tag type selecting the lightweight (bare QCoreApplication) registration overload.
    struct LightweightTag {};
    static constexpr LightweightTag Lightweight{};

    UnitTestWrapper(const QString& name, bool standalone, std::initializer_list<TestLabel> labels = {})
        : UnitTestWrapper(name, standalone, /*lightweight=*/false, labels)
    {
    }

    UnitTestWrapper(LightweightTag, const QString& name, bool standalone,
                    std::initializer_list<TestLabel> labels = {})
        : UnitTestWrapper(name, standalone, /*lightweight=*/true, labels)
    {
    }

private:
    UnitTestWrapper(const QString& name, bool standalone, bool lightweight,
                    std::initializer_list<TestLabel> labels)
    {
        _unitTest = std::make_unique<T>();
        _unitTest->setObjectName(name);
        _unitTest->setStandalone(standalone);
        _unitTest->setLightweight(lightweight);

        TestLabels combinedLabels;
        for (TestLabel label : labels) {
            combinedLabels |= label;
        }
        _unitTest->setLabels(combinedLabels);

        UnitTest::_addTest(_unitTest.get());
    }

    std::unique_ptr<T> _unitTest;
};
