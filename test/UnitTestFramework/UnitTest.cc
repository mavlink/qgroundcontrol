#include "UnitTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEvent>
#include <QtCore/QHash>
#include <QtCore/QCoreApplication>
#include <QtCore/QStandardPaths>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <iterator>

#include "AppSettings.h"
#include "Fact.h"
#include "LinkManager.h"
#include "MissionItem.h"
#include "MultiVehicleManager.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(UnitTestLog, "Test.UnitTest")

// ============================================================================
// TestContext Implementation
// ============================================================================

QStringList& TestContext::stack()
{
    static thread_local QStringList s_stack;
    return s_stack;
}

TestContext::TestContext(const QString& context)
{
    push(context);
}

TestContext::~TestContext()
{
    pop();
}

QString TestContext::current()
{
    const QStringList& s = stack();
    if (s.isEmpty()) {
        return QString();
    }
    return s.join(QStringLiteral(" > "));
}

void TestContext::push(const QString& context)
{
    stack().append(context);
}

void TestContext::pop()
{
    if (!stack().isEmpty()) {
        stack().removeLast();
    }
}

// ============================================================================
// TestDebug Implementation
// ============================================================================

QStringList& TestDebug::messages()
{
    static thread_local QStringList s_messages;
    return s_messages;
}

bool& TestDebug::verbose()
{
    static bool s_verbose = qEnvironmentVariableIsSet("QGC_TEST_VERBOSE");
    return s_verbose;
}

void TestDebug::log(const QString& message)
{
    messages().append(message);
    while (messages().size() > kMaxBufferedMessages) {
        messages().removeFirst();
    }

    if (isVerbose()) {
        qCDebug(UnitTestLog) << message;
    }
}

void TestDebug::setVerbose(bool v)
{
    verbose() = v;
}

bool TestDebug::isVerbose()
{
    return verbose();
}

QStringList TestDebug::recentMessages()
{
    return messages();
}

void TestDebug::clearMessages()
{
    messages().clear();
}

namespace {
constexpr int kFileCompareBufferSize = 8192;
constexpr int kMaxTimeoutDebugMessages = 10;

// Single source of truth for label name/value mapping
struct LabelMapping
{
    TestLabel label;
    const char* name;
};

// clang-format off
constexpr LabelMapping kLabelMappings[] = {
    {TestLabel::Unit,           "unit"},
    {TestLabel::Integration,    "integration"},
    {TestLabel::Vehicle,        "vehicle"},
    {TestLabel::MissionManager, "missionmanager"},
    {TestLabel::Comms,          "comms"},
    {TestLabel::Utilities,      "utilities"},
    {TestLabel::Slow,           "slow"},
    {TestLabel::Network,        "network"},
    {TestLabel::Serial,         "serial"},
    {TestLabel::Joystick,       "joystick"},
    {TestLabel::AnalyzeView,    "analyzeview"},
    {TestLabel::Terrain,        "terrain"},
};
// clang-format on

// Build hash map lazily for O(1) string→label lookup
const QHash<QString, TestLabel>& labelMap()
{
    static const QHash<QString, TestLabel> map = []() {
        QHash<QString, TestLabel> m;
        for (const auto& mapping : kLabelMappings) {
            m.insert(QString::fromLatin1(mapping.name), mapping.label);
        }
        return m;
    }();
    return map;
}

QString currentTestName()
{
    const char* functionName = QTest::currentTestFunction();
    const char* dataTag = QTest::currentDataTag();

    QString testName = functionName ? QString::fromLatin1(functionName) : QStringLiteral("<unknown>");
    if (dataTag && dataTag[0] != '\0') {
        testName += QStringLiteral(" [") + QString::fromLatin1(dataTag) + QStringLiteral("]");
    }

    return testName;
}

void logRecentDebugMessages(const char* header)
{
    const QStringList debugMessages = TestDebug::recentMessages();
    if (debugMessages.isEmpty()) {
        return;
    }

    qCWarning(UnitTestLog) << header;

    const int startIndex = qMax(0, debugMessages.size() - kMaxTimeoutDebugMessages);
    for (int i = startIndex; i < debugMessages.size(); ++i) {
        qCWarning(UnitTestLog) << "  " << debugMessages.at(i);
    }
}

}  // anonymous namespace

// ============================================================================
// Label Conversion Functions
// ============================================================================

TestLabel labelFromString(const QString& name)
{
    return labelMap().value(name.toLower().trimmed(), TestLabel::None);
}

QString labelToString(TestLabel label)
{
    if (label == TestLabel::None) {
        return QStringLiteral("none");
    }
    for (const auto& mapping : kLabelMappings) {
        if (mapping.label == label) {
            return QString::fromLatin1(mapping.name);
        }
    }
    return QStringLiteral("unknown");
}

TestLabels parseLabels(const QString& labelStr)
{
    TestLabels labels;
    const QStringList parts = labelStr.split(',', Qt::SkipEmptyParts);

    for (const QString& part : parts) {
        TestLabel label = labelFromString(part);
        if (label != TestLabel::None) {
            labels |= label;
        }
    }

    return labels;
}

QString labelsToString(TestLabels labels)
{
    if (labels == TestLabels()) {
        return QStringLiteral("[]");
    }

    QStringList names;
    for (const auto& mapping : kLabelMappings) {
        if (labels.testFlag(mapping.label)) {
            names.append(QString::fromLatin1(mapping.name));
        }
    }

    return QStringLiteral("[") + names.join(QStringLiteral(", ")) + QStringLiteral("]");
}

QStringList availableLabelNames()
{
    QStringList names;
    names.reserve(std::size(kLabelMappings));
    for (const auto& mapping : kLabelMappings) {
        names.append(QString::fromLatin1(mapping.name));
    }
    return names;
}

// ============================================================================
// UnitTest Implementation
// ============================================================================

UnitTest::UnitTest(QObject* parent) : QObject(parent)
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

void UnitTest::_addTest(UnitTest* test)
{
    QList<UnitTest*>& tests = _testList();
    Q_ASSERT(!tests.contains(test));
    tests.append(test);
}

QList<UnitTest*>& UnitTest::_testList()
{
    static QList<UnitTest*> tests;
    return tests;
}

QString& UnitTest::_outputFile()
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
    for (const UnitTest* test : _testList()) {
        names.append(test->objectName());
    }
    return names;
}

QStringList UnitTest::registeredTests(TestLabels labelFilter)
{
    QStringList names;
    for (const UnitTest* test : _testList()) {
        // If no filter specified, include all tests
        // If filter specified, test must have at least one matching label
        if (labelFilter == TestLabels() || test->hasAnyLabel(labelFilter)) {
            names.append(test->objectName());
        }
    }
    return names;
}

int UnitTest::testCount()
{
    return _testList().size();
}

void UnitTest::setVerbose(bool verbose)
{
    TestDebug::setVerbose(verbose);
}

bool UnitTest::isVerbose()
{
    return TestDebug::isVerbose();
}

bool UnitTest::waitForSignal(QSignalSpy& spy, int timeoutMs, QStringView signalName)
{
    QElapsedTimer waitTimer;
    waitTimer.start();

    if (spy.wait(timeoutMs)) {
        return true;
    }

    const QString displayName = signalName.isEmpty() ? QStringLiteral("<unnamed>") : signalName.toString();
    qCWarning(UnitTestLog) << "Timeout waiting for signal" << displayName << "in" << currentTestName() << "after"
                           << waitTimer.elapsed() << "ms (timeout:" << timeoutMs << "ms, count:" << spy.count()
                           << ")";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");

    return false;
}

bool UnitTest::waitForNoSignal(QSignalSpy& spy, int timeoutMs, QStringView signalName)
{
    const int initialCount = spy.count();
    QElapsedTimer waitTimer;
    waitTimer.start();

    const bool signalReceived = QTest::qWaitFor([&spy, initialCount]() { return spy.count() > initialCount; }, timeoutMs);
    if (!signalReceived) {
        return true;
    }

    const QString displayName = signalName.isEmpty() ? QStringLiteral("<unnamed>") : signalName.toString();
    qCWarning(UnitTestLog) << "Unexpected signal" << displayName << "in" << currentTestName() << "after"
                           << waitTimer.elapsed() << "ms (timeout:" << timeoutMs << "ms, initial:" << initialCount
                           << ", current:" << spy.count() << ")";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");
    return false;
}

bool UnitTest::waitForSignalCount(QSignalSpy& spy, int expectedCount, int timeoutMs, QStringView signalName)
{
    if (expectedCount <= 0 || spy.count() >= expectedCount) {
        return true;
    }

    QElapsedTimer waitTimer;
    waitTimer.start();

    if (QTest::qWaitFor([&spy, expectedCount]() { return spy.count() >= expectedCount; }, timeoutMs)) {
        return true;
    }

    const QString displayName = signalName.isEmpty() ? QStringLiteral("<unnamed>") : signalName.toString();
    qCWarning(UnitTestLog) << "Timeout waiting for signal count" << displayName << "in" << currentTestName()
                           << "after" << waitTimer.elapsed() << "ms (timeout:" << timeoutMs << "ms, expected:"
                           << expectedCount << ", actual:" << spy.count() << ")";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");

    return false;
}

bool UnitTest::waitForCondition(const std::function<bool()>& condition, int timeoutMs, QStringView conditionName)
{
    QElapsedTimer waitTimer;
    waitTimer.start();

    if (QTest::qWaitFor(condition, timeoutMs)) {
        return true;
    }

    const QString displayName = conditionName.isEmpty() ? QStringLiteral("<unnamed>") : conditionName.toString();
    qCWarning(UnitTestLog) << "Timeout waiting for condition" << displayName << "in" << currentTestName() << "after"
                           << waitTimer.elapsed() << "ms (timeout:" << timeoutMs << "ms)";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");

    return false;
}

bool UnitTest::waitForDeleted(const QPointer<QObject>& objectPtr, int timeoutMs, QStringView objectName)
{
    if (objectPtr.isNull()) {
        return true;
    }

    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::mediumMs();
    }

    QElapsedTimer waitTimer;
    waitTimer.start();

    const bool deleted = QTest::qWaitFor(
        [&objectPtr]() {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            return objectPtr.isNull();
        },
        timeoutMs);
    if (deleted) {
        return true;
    }

    const QString displayName = objectName.isEmpty() ? QStringLiteral("<unnamed>") : objectName.toString();
    qCWarning(UnitTestLog) << "Timeout waiting for QObject deletion" << displayName << "in" << currentTestName()
                           << "after" << waitTimer.elapsed() << "ms (timeout:" << timeoutMs
                           << "ms, ptr:" << objectPtr.data() << ")";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");

    return false;
}

void UnitTest::settleEventLoopForCleanup(int iterations, int waitMs)
{
    if (iterations <= 0) {
        iterations = TestTimeout::isCI() ? 10 : 5;
    }
    if (waitMs < 0) {
        waitMs = TestTimeout::isCI() ? 20 : 10;
    }

    for (int i = 0; i < iterations; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        if (i < iterations - 1 && waitMs > 0) {
            QTest::qWait(waitMs);
        }
    }
}

int UnitTest::run(QStringView singleTest, const QString& outputFile, TestLabels labelFilter)
{
    int ret = 0;
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    QElapsedTimer totalTimer;
    totalTimer.start();

    // Set output file if provided
    if (!outputFile.isEmpty()) {
        setOutputFile(outputFile);
    }

    // Print header
    qCInfo(UnitTestLog) << "═══════════════════════════════════════════════════════════════";
    qCInfo(UnitTestLog) << " QGroundControl Unit Tests";
    qCInfo(UnitTestLog) << "═══════════════════════════════════════════════════════════════";

    if (isVerbose()) {
        qCInfo(UnitTestLog) << "Verbose mode enabled (QGC_TEST_VERBOSE=1)";
    }

    QStringList failedTests;

    for (UnitTest* test : _testList()) {
        // Check if this test should run
        bool shouldRun = false;

        if (!singleTest.isEmpty()) {
            // Specific test requested - check name match
            shouldRun = (singleTest == test->objectName());
        } else {
            // No specific test - check labels and standalone
            if (test->standalone()) {
                skipped++;
                continue;  // Skip standalone tests unless explicitly requested
            }

            // If label filter specified, check if test has matching labels
            if (labelFilter != TestLabels()) {
                shouldRun = test->hasAnyLabel(labelFilter);
            } else {
                shouldRun = true;  // No filter, run all non-standalone tests
            }
        }

        if (!shouldRun) {
            skipped++;
            continue;
        }

        // Clear debug messages before each test class
        TestDebug::clearMessages();

        bool maxWarningsOk = false;
        const int configuredMaxWarnings = qEnvironmentVariableIntValue("QGC_TEST_MAXWARNINGS", &maxWarningsOk);
        const int maxWarnings = (maxWarningsOk && configuredMaxWarnings >= 0) ? configuredMaxWarnings : 100;

        QStringList args;
        args << "*" << "-maxwarnings" << QString::number(maxWarnings);

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

        QElapsedTimer testTimer;
        testTimer.start();

        const int testResult = QTest::qExec(test, args);

        const qint64 elapsed = testTimer.elapsed();

        if (testResult != 0) {
            failed++;
            failedTests.append(test->objectName());

            // Print failure details
            qCWarning(UnitTestLog) << "╭─────────────────────────────────────────────────────────────╮";
            qCWarning(UnitTestLog) << "│ FAILED:" << test->objectName();
            qCWarning(UnitTestLog) << "╰─────────────────────────────────────────────────────────────╯";

            // Print test context if available
            const QString context = TestContext::current();
            if (!context.isEmpty()) {
                qCWarning(UnitTestLog) << "Context:" << context;
            }

            // Print recent debug messages
            const QStringList& debugMsgs = TestDebug::recentMessages();
            if (!debugMsgs.isEmpty()) {
                qCWarning(UnitTestLog) << "Recent debug output:";
                for (const QString& msg : debugMsgs) {
                    qCWarning(UnitTestLog) << "  " << msg;
                }
            }
        } else {
            passed++;
            if (isVerbose()) {
                qCInfo(UnitTestLog) << "✓" << test->objectName() << QString("(%1ms)").arg(elapsed);
            }
        }

        ret += testResult;
    }

    // Print summary
    const qint64 totalElapsed = totalTimer.elapsed();
    qCInfo(UnitTestLog) << "═══════════════════════════════════════════════════════════════";
    qCInfo(UnitTestLog) << "Test Summary";
    qCInfo(UnitTestLog) << "───────────────────────────────────────────────────────────────";
    qCInfo(UnitTestLog) << "  Passed: " << passed;
    if (failed > 0) {
        qCWarning(UnitTestLog) << "  Failed: " << failed;
    }
    if (skipped > 0) {
        qCInfo(UnitTestLog) << "  Skipped:" << skipped;
    }
    qCInfo(UnitTestLog) << "  Total:  " << (passed + failed);
    qCInfo(UnitTestLog) << "  Time:   " << QString("%1s").arg(totalElapsed / 1000.0, 0, 'f', 2);

    if (!failedTests.isEmpty()) {
        qCWarning(UnitTestLog) << "Failed tests:";
        for (const QString& name : failedTests) {
            qCWarning(UnitTestLog) << "  •" << name;
        }
    }

    qCInfo(UnitTestLog) << "═══════════════════════════════════════════════════════════════";

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
    _failureContextDumped = false;

    // Force offline vehicle back to defaults
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(
        appSettings->offlineEditingFirmwareClass()->rawDefaultValue());
    appSettings->offlineEditingVehicleClass()->setRawValue(
        appSettings->offlineEditingVehicleClass()->rawDefaultValue());
}

void UnitTest::cleanup()
{
    _cleanupCalled = true;
    dumpFailureContextIfTestFailed(QStringLiteral("cleanup"));

    _cleanupTempFiles();

    // Process any lingering events to prevent cross-test contamination
    settleEventLoopForCleanup(3, 0);
}

void UnitTest::dumpFailureContextIfTestFailed(QStringView reason)
{
    if (_failureContextDumped || !QTest::currentTestFailed()) {
        return;
    }

    _failureContextDumped = true;

    const QString testFunction = currentTestName();
    if (reason.isEmpty()) {
        qCWarning(UnitTestLog) << "Failure context dump for" << objectName() << "::" << testFunction;
    } else {
        qCWarning(UnitTestLog) << "Failure context dump for" << objectName() << "::" << testFunction << "("
                               << reason.toString() << ")";
    }

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    const QString fixtureSummary = failureContextSummary();
    if (!fixtureSummary.isEmpty()) {
        qCWarning(UnitTestLog) << "Fixture state:";
        const QStringList lines = fixtureSummary.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            qCWarning(UnitTestLog).noquote() << "  " << line;
        }
    }

    logRecentDebugMessages("Recent test debug messages:");
}

QString UnitTest::failureContextSummary() const
{
    QStringList lines;

    if (!qgcApp()) {
        lines.append(QStringLiteral("QGC application instance is not available"));
        return lines.join('\n');
    }

    MultiVehicleManager* vehicleManager = MultiVehicleManager::instance();
    QmlObjectListModel* vehicles = vehicleManager ? vehicleManager->vehicles() : nullptr;
    const int vehicleCount = vehicles ? vehicles->count() : 0;
    Vehicle* activeVehicle = vehicleManager ? vehicleManager->activeVehicle() : nullptr;

    lines.append(QStringLiteral("MultiVehicleManager: vehicles=%1 activeVehicle=%2")
                     .arg(vehicleCount)
                     .arg(activeVehicle ? QStringLiteral("present") : QStringLiteral("null")));
    if (activeVehicle) {
        lines.append(QStringLiteral("Active vehicle: id=%1 initialConnectComplete=%2")
                         .arg(activeVehicle->id())
                         .arg(activeVehicle->isInitialConnectComplete()));
    }

    LinkManager* linkManager = LinkManager::instance();
    const int linkCount = linkManager ? linkManager->links().count() : 0;
    lines.append(QStringLiteral("LinkManager: links=%1").arg(linkCount));

    return lines.join('\n');
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

bool UnitTest::fileCompare(const QString& file1, const QString& file2)
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
                    qCWarning(UnitTestLog) << "fileCompare: mismatch at offset" << (offset + i) << "- got"
                                           << static_cast<int>(buf1[i]) << "expected" << static_cast<int>(buf2[i]);
                    return false;
                }
            }
            return false;
        }
        offset += buf1.size();
    }

    return true;
}

bool UnitTest::fileContentsEqual(const QString& filePath, const QString& expectedContent)
{
    return fileContentsEqual(filePath, expectedContent.toUtf8());
}

bool UnitTest::fileContentsEqual(const QString& filePath, const QByteArray& expectedContent)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(UnitTestLog) << "fileContentsEqual: cannot open file:" << file.errorString();
        return false;
    }

    const QByteArray actualContent = file.readAll();
    if (actualContent != expectedContent) {
        qCWarning(UnitTestLog) << "fileContentsEqual: content mismatch" << "- actual size:" << actualContent.size()
                               << "expected size:" << expectedContent.size();
        return false;
    }

    return true;
}

void UnitTest::_missionItemsEqual(const MissionItem& actual, const MissionItem& expected)
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

void UnitTest::changeFactValue(Fact* fact, double increment)
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

QGeoCoordinate UnitTest::changeCoordinateValue(const QGeoCoordinate& coordinate)
{
    return coordinate.atDistanceAndAzimuth(1, 0);
}

QTemporaryFile* UnitTest::createTempFile(const QString& templateName)
{
    auto* tempFile = templateName.isEmpty() ? new QTemporaryFile(this)
                                            : new QTemporaryFile(QDir::tempPath() + "/" + templateName, this);

    if (!tempFile->open()) {
        qCWarning(UnitTestLog) << "createTempFile: failed to create temp file:" << tempFile->errorString();
        delete tempFile;
        return nullptr;
    }

    _tempFiles.append(tempFile);
    return tempFile;
}

QTemporaryDir* UnitTest::createTempDir()
{
    auto* tempDir = new QTemporaryDir();

    if (!tempDir->isValid()) {
        qCWarning(UnitTestLog) << "createTempDir: failed to create temp directory";
        delete tempDir;
        return nullptr;
    }

    _tempDirs.append(tempDir);
    return tempDir;
}

QString UnitTest::testResourcePath(const QString& relativePath)
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

        for (const QString& candidate : candidates) {
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
