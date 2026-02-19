#include "PlatformTest.h"

#include <QtCore/QByteArray>

#include "Fixtures/RAIIFixtures.h"
#include "Platform.h"
#include "QGCCommandLineParser.h"
#include "UnitTest.h"
#include "qgc_version.h"

namespace {

class ScopedEnvVarRestore
{
public:
    explicit ScopedEnvVarRestore(const char *name)
        : _name(name)
        , _hadValue(qEnvironmentVariableIsSet(name))
    {
        if (_hadValue) {
            _value = qgetenv(name);
        }
    }

    ~ScopedEnvVarRestore()
    {
        if (_hadValue) {
            (void) qputenv(_name, _value);
        } else {
            (void) qunsetenv(_name);
        }
    }

private:
    const char *_name = nullptr;
    bool _hadValue = false;
    QByteArray _value;
};

} // namespace

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
void PlatformTest::_testCheckSingleInstanceAllowMultiple()
{
    // When allowMultiple is true, should always return true (allow running)
    QVERIFY(Platform::checkSingleInstance(true));
    // Call multiple times to ensure it's consistent
    QVERIFY(Platform::checkSingleInstance(true));
    QVERIFY(Platform::checkSingleInstance(true));
}

void PlatformTest::_testCheckSingleInstanceBlocks()
{
    const QString lockKey = QStringLiteral("%1 RunGuardKey").arg(QLatin1String(QGC_APP_NAME));
    TestFixtures::SingleInstanceLockFixture heldLock(lockKey);
    if (!heldLock.isLocked()) {
        QSKIP("Single-instance lock is already held by another process.");
    }

    // With the lock already held, Platform guard should refuse startup.
    QVERIFY(!Platform::checkSingleInstance(false));
}
#endif  // !Q_OS_ANDROID && !Q_OS_IOS
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
void PlatformTest::_testIsRunningAsRootNormal()
{
    // In normal test execution, we should NOT be running as root
    // If tests are run as root, this test will fail (which is intentional -
    // running tests as root is discouraged)
    QVERIFY2(!Platform::isRunningAsRoot(), "Tests should not be run as root. Running QGC as root is dangerous.");
}
#endif  // Q_OS_LINUX && !Q_OS_ANDROID

#if defined(QGC_UNITTEST_BUILD)
void PlatformTest::_testInitializeSetsUnitTestEnvironment()
{
    ScopedEnvVarRestore restoreConsole("QT_ASSUME_STDERR_HAS_CONSOLE");
    ScopedEnvVarRestore restoreLogging("QT_FORCE_STDERR_LOGGING");
    ScopedEnvVarRestore restoreQpa("QT_QPA_PLATFORM");

    (void) qunsetenv("QT_ASSUME_STDERR_HAS_CONSOLE");
    (void) qunsetenv("QT_FORCE_STDERR_LOGGING");
    (void) qunsetenv("QT_QPA_PLATFORM");

    QGCCommandLineParser::CommandLineParseResult args;
    args.runningUnitTests = true;
    args.allowMultiple = true;

    char appName[] = "qgc-unit-test";
    char *argv[] = { appName };
    const std::optional<int> initResult = Platform::initialize(1, argv, args);

    QVERIFY(!initResult.has_value());
    QCOMPARE(qgetenv("QT_ASSUME_STDERR_HAS_CONSOLE"), QByteArray("1"));
    QCOMPARE(qgetenv("QT_FORCE_STDERR_LOGGING"), QByteArray("1"));
    QCOMPARE(qgetenv("QT_QPA_PLATFORM"), QByteArray("offscreen"));
}
#endif

UT_REGISTER_TEST(PlatformTest, TestLabel::Unit, TestLabel::Utilities)
