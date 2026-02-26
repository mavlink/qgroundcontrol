#pragma once

#include <QtCore/QFlags>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QStringView>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

#include <functional>
#include <initializer_list>

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

// ============================================================================
// Test Assertion Macros
// ============================================================================

/// Compare two QGeoCoordinates with meter tolerance
/// @param actual The actual coordinate
/// @param expected The expected coordinate
/// @param toleranceMeters Maximum distance in meters (default 1.0)
#define QCOMPARE_COORDS(actual, expected, ...)                                                                 \
    do {                                                                                                       \
        const QGeoCoordinate _actual = (actual);                                                               \
        const QGeoCoordinate _expected = (expected);                                                           \
        const double _tolerance = (0.0 __VA_OPT__(+) __VA_ARGS__) > 0 ? (0.0 __VA_OPT__(+) __VA_ARGS__) : 1.0; \
        const double _distance = _actual.distanceTo(_expected);                                                \
        if (_distance > _tolerance) {                                                                          \
            const QString _msg = QString(                                                                      \
                                     "Coordinates differ by %1m (tolerance: %2m)\n"                            \
                                     "  Actual:   (%3, %4, %5)\n"                                              \
                                     "  Expected: (%6, %7, %8)")                                               \
                                     .arg(_distance, 0, 'f', 3)                                                \
                                     .arg(_tolerance, 0, 'f', 3)                                               \
                                     .arg(_actual.latitude(), 0, 'f', 7)                                       \
                                     .arg(_actual.longitude(), 0, 'f', 7)                                      \
                                     .arg(_actual.altitude(), 0, 'f', 2)                                       \
                                     .arg(_expected.latitude(), 0, 'f', 7)                                     \
                                     .arg(_expected.longitude(), 0, 'f', 7)                                    \
                                     .arg(_expected.altitude(), 0, 'f', 2);                                    \
            QFAIL(qPrintable(_msg));                                                                           \
        }                                                                                                      \
    } while (false)

/// Verify coordinates are equal within tolerance (less verbose than QCOMPARE_COORDS)
#define QVERIFY_COORDS_NEAR(actual, expected, toleranceMeters)   \
    QVERIFY2((actual).distanceTo(expected) <= (toleranceMeters), \
             qPrintable(QString("Coordinates differ by %1m").arg((actual).distanceTo(expected))))

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

/// Wait for a condition with timeout
/// @param condition Boolean expression to wait for
/// @param timeoutMs Timeout in milliseconds
#define QVERIFY_TRUE_WAIT(condition, timeoutMs)                           \
    QVERIFY2(UnitTest::waitForCondition([&]() { return (condition); }, (timeoutMs), QStringLiteral(#condition)), \
             qPrintable(QString("Condition not met within %1ms: " #condition).arg(timeoutMs)))

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
inline bool isCI()
{
    static const bool ci = qEnvironmentVariableIsSet("CI") || qEnvironmentVariableIsSet("GITHUB_ACTIONS");
    return ci;
}

/// Short timeout for quick operations (1 second, 2s on CI)
inline int shortMs()
{
    static const int timeout = isCI() ? 2000 : 1000;
    return timeout;
}

/// Medium timeout for normal async operations (5 seconds, 10s on CI)
inline int mediumMs()
{
    static const int timeout = isCI() ? 10000 : 5000;
    return timeout;
}

/// Long timeout for slow operations like vehicle connection (30 seconds, 60s on CI)
inline int longMs()
{
    static const int timeout = isCI() ? 60000 : 30000;
    return timeout;
}

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
class Vehicle;

// ============================================================================
// Test Context - Improved failure diagnostics
// ============================================================================

/// RAII helper to add context to test failure messages
/// Usage: TEST_CONTEXT("Loading mission file: " + filename);
class TestContext
{
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

    /// Enable verbose output for debugging
    static void setVerbose(bool verbose);

    /// Check if verbose mode is enabled
    static bool isVerbose();

    /// Wait for a signal with standardized timeout diagnostics.
    static bool waitForSignal(QSignalSpy& spy, int timeoutMs, QStringView signalName = {});

    /// Wait to ensure no additional signal emissions occur during timeout.
    static bool waitForNoSignal(QSignalSpy& spy, int timeoutMs, QStringView signalName = {});

    /// Wait until a signal spy reaches at least expectedCount emissions.
    static bool waitForSignalCount(QSignalSpy& spy, int expectedCount, int timeoutMs, QStringView signalName = {});

    /// Wait for a condition with standardized timeout diagnostics.
    static bool waitForCondition(const std::function<bool()>& condition, int timeoutMs,
                                 QStringView conditionName = {});

    /// Waits for a QObject to be deleted (QPointer becomes null) while draining deferred deletes.
    static bool waitForDeleted(const QPointer<QObject>& objectPtr, int timeoutMs,
                               QStringView objectName = {});

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

    /// Compares two MissionItems for equality using QCOMPARE/QVERIFY
    static void _missionItemsEqual(const MissionItem& actual, const MissionItem& expected);

    // ========================================================================
    // Fact/Value Manipulation
    // ========================================================================

    /// Changes a Fact's rawValue to trigger valueChanged signal
    /// @param fact The fact to modify
    /// @param increment For numeric facts, amount to add (0 = use default of 1)
    void changeFactValue(Fact* fact, double increment = 0);

    /// Returns a coordinate offset by 1 meter north
    QGeoCoordinate changeCoordinateValue(const QGeoCoordinate& coordinate);

    // ========================================================================
    // Temporary File/Directory Helpers
    // ========================================================================

    /// Creates a temporary file that is automatically deleted when test ends
    /// @param templateName Optional template (e.g., "test_XXXXXX.txt")
    /// @return Pointer to temporary file, or nullptr on failure
    QTemporaryFile* createTempFile(const QString& templateName = QString());

    /// Creates a temporary directory that is automatically deleted when test ends
    /// @return Pointer to temporary directory, or nullptr on failure
    QTemporaryDir* createTempDir();

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

private:
    void _cleanupTempFiles();
    void _resetTestState();

    static QList<UnitTest*>& _testList();
    static QString& _outputFile();

    QList<QTemporaryFile*> _tempFiles;
    QList<QTemporaryDir*> _tempDirs;

    TestLabels _labels;
    bool _unitTestRun = false;
    bool _initCalled = false;
    bool _cleanupCalled = false;
    bool _failureContextDumped = false;
    bool _standalone = false;
};

// ============================================================================
// UnitTestWrapper - Handles static registration
// ============================================================================

/// Template class for automatic test registration at static initialization time.
/// Test instances are owned by the static wrapper and live for the program duration.
template <class T>
class UnitTestWrapper
{
public:
    UnitTestWrapper(const QString& name, bool standalone, std::initializer_list<TestLabel> labels = {})
    {
        // Create test instance - lives for program duration (static storage)
        _unitTest = new T;
        _unitTest->setObjectName(name);
        _unitTest->setStandalone(standalone);

        TestLabels combinedLabels;
        for (TestLabel label : labels) {
            combinedLabels |= label;
        }
        _unitTest->setLabels(combinedLabels);

        UnitTest::_addTest(_unitTest);
    }

private:
    T* _unitTest = nullptr;  // Intentionally leaked - static lifetime
};
