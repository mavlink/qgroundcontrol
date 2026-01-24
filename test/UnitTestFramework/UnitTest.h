#pragma once

#include "MockLink.h"
#include "MAVLinkLib.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtPositioning/QGeoCoordinate>

#include <memory>
#include <span>

/// Register a test class with the test framework
#define UT_REGISTER_TEST(className) static UnitTestWrapper<className> className(#className, false);

/// Register a standalone test that only runs when explicitly requested
#define UT_REGISTER_TEST_STANDALONE(className) static UnitTestWrapper<className> className(#className, true);

Q_DECLARE_LOGGING_CATEGORY(UnitTestLog)

class Vehicle;
class Fact;
class LinkInterface;
class MissionItem;

/// Base class for all QGC unit tests
/// Provides common test infrastructure including MockLink vehicle connections,
/// file comparison utilities, and test lifecycle management.
class UnitTest : public QObject
{
    Q_OBJECT

public:
    explicit UnitTest(QObject *parent = nullptr);
    ~UnitTest() override;

    /// Runs all registered unit tests
    /// @param singleTest If non-empty, only run the test with this name
    /// @param outputFile Optional output file path for JUnit XML results
    /// @return Number of test failures (0 = all passed)
    static int run(QStringView singleTest, const QString& outputFile = QString());

    /// Sets the output file for JUnit XML test results
    /// @param outputFile Path to output file (empty = console only)
    static void setOutputFile(const QString& outputFile);

    /// Returns list of all registered test names
    static QStringList registeredTests();

    /// Returns total number of registered tests
    static int testCount();

    bool standalone() const { return _standalone; }
    void setStandalone(bool standalone) { _standalone = standalone; }

    /// Adds a unit test to the list. Should only be called by UnitTestWrapper.
    static void _addTest(UnitTest *test);

    // ========================================================================
    // File Comparison Utilities
    // ========================================================================

    /// Compares two files byte-by-byte
    /// @return true if files are identical, false otherwise
    static bool fileCompare(const QString &file1, const QString &file2);

    /// Compares file content against expected string
    /// @return true if file content matches expected string
    static bool fileContentsEqual(const QString &filePath, const QString &expectedContent);

    /// Compares file content against expected bytes
    /// @return true if file content matches expected bytes
    static bool fileContentsEqual(const QString &filePath, const QByteArray &expectedContent);

    // ========================================================================
    // Fact/Value Manipulation
    // ========================================================================

    /// Changes a Fact's rawValue to trigger valueChanged signal
    /// @param fact The fact to modify
    /// @param increment For numeric facts, amount to add (0 = use default of 1)
    void changeFactValue(Fact *fact, double increment = 0);

    /// Returns a coordinate offset by 1 meter north
    QGeoCoordinate changeCoordinateValue(const QGeoCoordinate &coordinate);

    // Note: For coordinate comparison, use TestHelpers::coordsEqual() or
    // the VERIFY_COORDS_EQUAL macro from TestHelpers.h

    // ========================================================================
    // Temporary File/Directory Helpers
    // ========================================================================

    /// Creates a temporary file that is automatically deleted when test ends
    /// @param templateName Optional template (e.g., "test_XXXXXX.txt")
    /// @return Pointer to temporary file, or nullptr on failure
    QTemporaryFile *createTempFile(const QString &templateName = QString());

    /// Creates a temporary directory that is automatically deleted when test ends
    /// @return Pointer to temporary directory, or nullptr on failure
    QTemporaryDir *createTempDir();

    /// Returns the path to test resource files
    /// @param relativePath Path relative to test/resources directory
    static QString testResourcePath(const QString &relativePath = QString());

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
    // ========================================================================
    // MockLink Vehicle Helpers
    // ========================================================================

    /// Connects a MockLink vehicle and waits for initial connect sequence
    /// @param autopilot Autopilot type (PX4, ArduPilot, Generic)
    /// @param failureMode Optional failure mode for testing error handling
    void _connectMockLink(MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_PX4,
                          MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);

    /// Connects MockLink without waiting for initial connect sequence
    void _connectMockLinkNoInitialConnectSequence() { _connectMockLink(MAV_AUTOPILOT_INVALID); }

    /// Disconnects the current MockLink and waits for vehicle to be removed
    void _disconnectMockLink();

    /// Compares two MissionItems for equality using QCOMPARE/QVERIFY
    static void _missionItemsEqual(const MissionItem &actual, const MissionItem &expected);

    MockLink *_mockLink = nullptr;
    Vehicle *_vehicle = nullptr;

private slots:
    void _linkDeleted(const LinkInterface *link);

private:
    void _cleanupTempFiles();
    void _resetTestState();

    static QList<UnitTest*> &_testList();
    static QString &_outputFile();

    QList<QTemporaryFile*> _tempFiles;
    QList<QTemporaryDir*> _tempDirs;

    bool _unitTestRun = false;
    bool _initCalled = false;
    bool _cleanupCalled = false;
    bool _standalone = false;
};

template <class T>
class UnitTestWrapper
{
public:
    UnitTestWrapper(const QString &name, bool standalone)
        : _unitTest(new T)
    {
        _unitTest->setObjectName(name);
        _unitTest->setStandalone(standalone);
        UnitTest::_addTest(_unitTest.data());
    }

private:
    QSharedPointer<T> _unitTest;
};
