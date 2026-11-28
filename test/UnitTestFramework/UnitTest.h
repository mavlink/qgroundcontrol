#pragma once

#include <initializer_list>
#include <memory>

#include <QtCore/QFlags>
#include <QtCore/QObject>
#include <QtCore/QStringView>
#include <QtTest/QTest>

#include "UnitTestBase.h"

class QRegularExpression;

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

/// Selects every count-th test function of a test class, starting at the 1-based index, so one
/// class can be split across processes. A count of 0 selects all test functions.
struct TestShard
{
    uint index = 0;
    uint count = 0;
};

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

// Coordinate helpers live in UnitTestCoords.h, the wait macros in UnitTestBase.h.

#include "UnitTestMath.h"

// ============================================================================
// Forward Declarations
// ============================================================================

class Fact;
class MissionItem;
class Vehicle;
class VehicleComponent;

// ============================================================================
// UnitTest Base Class
// ============================================================================

/// Base class for all QGC unit tests
/// Provides common test infrastructure including file comparison utilities,
/// temporary file management, and test lifecycle management.
/// For tests requiring a vehicle connection, use VehicleTest instead.
class UnitTest : public UnitTestBase
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
    /// @param shard If set, only run this shard of each test's functions
    /// @return Number of test failures (0 = all passed)
    static int run(QStringView singleTest, const QString& outputFile = QString(), TestLabels labelFilter = TestLabels(),
                   TestShard shard = {});

    /// Returns the functions QTest would run for @a test, in declaration order, limited to @a shard when set
    static QStringList testFunctionNames(const QObject* test, TestShard shard = {});

    /// Returns the JUnit XML path for one test class: @a outputFile with the class name, and the shard
    /// index when sharded, inserted before ".xml" so classes and parallel shards never share a file
    static QString testOutputFileName(const QString& outputFile, const QString& testName, TestShard shard = {});

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

    /// Find a vehicle setup component (e.g. "Frame", "Sensors", "Radio") by display name.
    /// Returns nullptr if not found.
    static VehicleComponent *findVehicleComponent(Vehicle *vehicle, const QString &name);

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
    void init() override;

    /// Called after each test function
    void cleanup() override;

protected:
    /// True when an ArduPilot firmware plugin factory is registered. Single-firmware
    /// builds (e.g. custom) don't register one; APM-specific tests should QSKIP.
    static bool apmFirmwareSupported();

    /// Adds the state of the application to failure dumps.
    QString failureContextSummary() const override;

    /// Declare that a showAppMessage() call matching @a messagePattern is required.
    /// Call verifyExpectedLogMessage() after the code that should emit it.
    /// Convenience wrapper over expectLogMessage for the QGCAppMessageLog category.
    void expectAppMessage(const QRegularExpression &messagePattern);

    void startLogCapture() override;
    void stopLogCapture() override;
    QList<TestLogMessage> capturedLogMessages() const override;

private:
    void _resetTestState();

    static QList<UnitTest*>& _testList();
    static QString& _outputFile();

    TestLabels _labels;
    bool _unitTestRun = false;
    bool _initCalled = false;
    bool _cleanupCalled = false;
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
