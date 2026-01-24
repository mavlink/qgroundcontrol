#include "UnitTest.h"
#include "TestHelpers.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "LinkManager.h"
#include "QGC.h"
#include "Fact.h"
#include "MissionItem.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

QGC_LOGGING_CATEGORY(UnitTestLog, "Test.UnitTest")

namespace {
    constexpr int kFileCompareBufferSize = 8192;
}

UnitTest::UnitTest(QObject *parent)
    : QObject(parent)
{
}

UnitTest::~UnitTest()
{
    // Only assert if actual test methods were run (init was called)
    // This handles the case where a base class is accidentally registered
    // or a test class has no test methods
    if (_unitTestRun && _initCalled) {
        Q_ASSERT(_cleanupCalled);
    }
}

void UnitTest::_addTest(UnitTest *test)
{
    QList<UnitTest*> &tests = _testList();
    Q_ASSERT(!tests.contains(test));
    tests.append(test);
}

QList<UnitTest*> &UnitTest::_testList()
{
    static QList<UnitTest*> tests;
    return tests;
}

QString &UnitTest::_outputFile()
{
    static QString outputFile;
    return outputFile;
}

void UnitTest::setOutputFile(const QString& outputFile)
{
    _outputFile() = outputFile;
}

QStringList UnitTest::registeredTests()
{
    QStringList names;
    for (const UnitTest *test : _testList()) {
        names.append(test->objectName());
    }
    return names;
}

int UnitTest::testCount()
{
    return _testList().size();
}

int UnitTest::run(QStringView singleTest, const QString& outputFile)
{
    int ret = 0;

    // Set output file if provided
    if (!outputFile.isEmpty()) {
        setOutputFile(outputFile);
    }

    for (UnitTest *test : _testList()) {
        if (singleTest.isEmpty() || singleTest == test->objectName()) {
            if (test->standalone() && singleTest.isEmpty()) {
                continue;
            }
            QStringList args;
            args << "*" << "-maxwarnings" << "0";

            // Add JUnit XML output if configured
            if (!_outputFile().isEmpty()) {
                // Create unique output file per test class to avoid overwriting
                QString testOutputFile = _outputFile();
                if (testOutputFile.endsWith(QStringLiteral(".xml"))) {
                    testOutputFile.insert(testOutputFile.length() - 4, QStringLiteral("-") + test->objectName());
                } else {
                    testOutputFile += QStringLiteral("-") + test->objectName() + QStringLiteral(".xml");
                }
                args << "-o" << (testOutputFile + QStringLiteral(",junitxml"));
            }

            ret += QTest::qExec(test, args);
        }
    }

    return ret;
}

void UnitTest::initTestCase()
{
    // Reset test tracking state at start of each test class
    _resetTestState();
}

void UnitTest::cleanupTestCase()
{
    // Override in derived classes for one-time teardown
}

void UnitTest::init()
{
    _initCalled = true;

    MultiVehicleManager::instance()->init();
    LinkManager::instance()->setConnectionsAllowed();

    // Force offline vehicle back to defaults
    AppSettings *const appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(appSettings->offlineEditingFirmwareClass()->rawDefaultValue());
    appSettings->offlineEditingVehicleClass()->setRawValue(appSettings->offlineEditingVehicleClass()->rawDefaultValue());

    MAVLinkProtocol::deleteTempLogFiles();
}

void UnitTest::cleanup()
{
    _cleanupCalled = true;

    _disconnectMockLink();
    _cleanupTempFiles();

    // Process any lingering events to prevent cross-test contamination
    QCoreApplication::processEvents();
}

void UnitTest::_cleanupTempFiles()
{
    qDeleteAll(_tempFiles);
    _tempFiles.clear();

    qDeleteAll(_tempDirs);
    _tempDirs.clear();
}

void UnitTest::_resetTestState()
{
    _unitTestRun = true;
    _initCalled = false;
    _cleanupCalled = false;
}

void UnitTest::_connectMockLink(MAV_AUTOPILOT autopilot, MockConfiguration::FailureMode_t failureMode)
{
    Q_ASSERT(!_mockLink);

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

    switch (autopilot) {
    case MAV_AUTOPILOT_PX4:
        _mockLink = MockLink::startPX4MockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_ARDUPILOTMEGA:
        _mockLink = MockLink::startAPMArduCopterMockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_GENERIC:
        _mockLink = MockLink::startGenericMockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_INVALID:
        _mockLink = MockLink::startNoInitialConnectMockLink(false);
        break;
    default:
        qCWarning(UnitTestLog) << "Unsupported autopilot type:" << autopilot;
        return;
    }

    // Connect to destroyed signal to prevent dangling pointer
    if (_mockLink) {
        (void) connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });
    }

    QVERIFY2(spyVehicle.wait(TestHelpers::kLongTimeoutMs), "Timeout waiting for vehicle connection");
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    VERIFY_NOT_NULL(_vehicle);

    if (autopilot != MAV_AUTOPILOT_INVALID) {
        QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
        QVERIFY2(spyConnect.isValid(), "Failed to create spy for initialConnectComplete");
        QVERIFY2(spyConnect.wait(TestHelpers::kLongTimeoutMs), "Timeout waiting for initial connect");
    }
}

void UnitTest::_disconnectMockLink()
{
    if (_mockLink) {
        QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

        _mockLink->disconnect();
        _mockLink = nullptr;

        QVERIFY2(spyVehicle.wait(TestHelpers::kLongTimeoutMs), "Timeout waiting for vehicle disconnection");
        _vehicle = MultiVehicleManager::instance()->activeVehicle();
        QVERIFY2(!_vehicle, "Vehicle should be null after disconnection");

        // Process ALL pending events including deleteLater() calls to ensure the old vehicle
        // and MockLink are fully destroyed before any subsequent test case creates new ones.
        // This prevents use-after-free issues when callbacks from old objects fire during
        // new object creation.
        //
        // We use a more aggressive approach on CI where system load can cause timer callbacks
        // to be delayed: process events, wait a bit for any pending timers to fire, then
        // process events again. This ensures cascading deletions complete fully.
        //
        // The DeferredDelete event type is specifically important for deleteLater() cleanup.
        static const bool isCi = qEnvironmentVariableIsSet("CI") ||
                                  qEnvironmentVariableIsSet("GITHUB_ACTIONS");
        const int iterations = isCi ? 10 : 5;
        const int waitMs = isCi ? 20 : 10;

        for (int i = 0; i < iterations; ++i) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            if (i < iterations - 1) {
                QTest::qWait(waitMs);  // Allow pending timer callbacks to fire
            }
        }
    }
}

void UnitTest::_linkDeleted(const LinkInterface *link)
{
    if (link == _mockLink) {
        _mockLink = nullptr;
    }
}

bool UnitTest::fileCompare(const QString &file1, const QString &file2)
{
    const QFileInfo info1(file1);
    const QFileInfo info2(file2);

    if (info1.size() != info2.size()) {
        qCWarning(UnitTestLog) << "fileCompare: sizes differ -" << info1.size() << "vs" << info2.size();
        return false;
    }

    QFile f1(file1);
    QFile f2(file2);

    if (!f1.open(QIODevice::ReadOnly)) {
        qCWarning(UnitTestLog) << "fileCompare: cannot open file1:" << f1.errorString();
        return false;
    }
    if (!f2.open(QIODevice::ReadOnly)) {
        qCWarning(UnitTestLog) << "fileCompare: cannot open file2:" << f2.errorString();
        return false;
    }

    // Use buffered comparison for efficiency
    qint64 offset = 0;
    while (!f1.atEnd()) {
        const QByteArray buf1 = f1.read(kFileCompareBufferSize);
        const QByteArray buf2 = f2.read(kFileCompareBufferSize);

        if (buf1 != buf2) {
            // Find exact mismatch position
            for (int i = 0; i < buf1.size() && i < buf2.size(); ++i) {
                if (buf1[i] != buf2[i]) {
                    qCWarning(UnitTestLog) << "fileCompare: mismatch at offset" << (offset + i)
                                           << "- got" << static_cast<int>(buf1[i])
                                           << "expected" << static_cast<int>(buf2[i]);
                    return false;
                }
            }
            return false;
        }
        offset += buf1.size();
    }

    return true;
}

bool UnitTest::fileContentsEqual(const QString &filePath, const QString &expectedContent)
{
    return fileContentsEqual(filePath, expectedContent.toUtf8());
}

bool UnitTest::fileContentsEqual(const QString &filePath, const QByteArray &expectedContent)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(UnitTestLog) << "fileContentsEqual: cannot open file:" << file.errorString();
        return false;
    }

    const QByteArray actualContent = file.readAll();
    if (actualContent != expectedContent) {
        qCWarning(UnitTestLog) << "fileContentsEqual: content mismatch"
                               << "- actual size:" << actualContent.size()
                               << "expected size:" << expectedContent.size();
        return false;
    }

    return true;
}

void UnitTest::changeFactValue(Fact *fact, double increment)
{
    if (fact->typeIsBool()) {
        fact->setRawValue(!fact->rawValue().toBool());
    } else {
        if (qFuzzyIsNull(increment)) {
            increment = 1.0;
        }
        fact->setRawValue(fact->rawValue().toDouble() + increment);
    }
}

void UnitTest::_missionItemsEqual(const MissionItem &actual, const MissionItem &expected)
{
    QCOMPARE(static_cast<int>(actual.command()), static_cast<int>(expected.command()));
    QCOMPARE(static_cast<int>(actual.frame()), static_cast<int>(expected.frame()));
    QCOMPARE(actual.autoContinue(), expected.autoContinue());

    QVERIFY(QGC::fuzzyCompare(actual.param1(), expected.param1()));
    QVERIFY(QGC::fuzzyCompare(actual.param2(), expected.param2()));
    QVERIFY(QGC::fuzzyCompare(actual.param3(), expected.param3()));
    QVERIFY(QGC::fuzzyCompare(actual.param4(), expected.param4()));
    QVERIFY(QGC::fuzzyCompare(actual.param5(), expected.param5()));
    QVERIFY(QGC::fuzzyCompare(actual.param6(), expected.param6()));
    QVERIFY(QGC::fuzzyCompare(actual.param7(), expected.param7()));
}

QGeoCoordinate UnitTest::changeCoordinateValue(const QGeoCoordinate &coordinate)
{
    return coordinate.atDistanceAndAzimuth(1, 0);
}

QTemporaryFile *UnitTest::createTempFile(const QString &templateName)
{
    auto *tempFile = templateName.isEmpty()
        ? new QTemporaryFile(this)
        : new QTemporaryFile(QDir::tempPath() + "/" + templateName, this);

    if (!tempFile->open()) {
        qCWarning(UnitTestLog) << "createTempFile: failed to create temp file:" << tempFile->errorString();
        delete tempFile;
        return nullptr;
    }

    _tempFiles.append(tempFile);
    return tempFile;
}

QTemporaryDir *UnitTest::createTempDir()
{
    auto *tempDir = new QTemporaryDir();

    if (!tempDir->isValid()) {
        qCWarning(UnitTestLog) << "createTempDir: failed to create temp directory";
        delete tempDir;
        return nullptr;
    }

    _tempDirs.append(tempDir);
    return tempDir;
}

QString UnitTest::testResourcePath(const QString &relativePath)
{
    // Try to find test resources relative to the source directory
    // basePath is computed once and cached
    static const QString basePath = []() {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QStringList candidates = {
            appDir + QStringLiteral("/../../test/resources"),
            appDir + QStringLiteral("/../test/resources"),
            appDir + QStringLiteral("/test/resources"),
            QStringLiteral(QT_TESTCASE_SOURCEDIR "/resources"),
        };

        for (const QString &candidate : candidates) {
            if (QDir(candidate).exists()) {
                return QDir(candidate).absolutePath();
            }
        }

        qCWarning(UnitTestLog) << "testResourcePath: could not find test resources directory";
        return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }();

    if (relativePath.isEmpty()) {
        return basePath;
    }

    return basePath + QStringLiteral("/") + relativePath;
}
