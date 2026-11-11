#include "PlatformTest.h"

#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QScopeGuard>
#include <QtQuick/QQuickGraphicsConfiguration>
#include <QtQuick/QQuickWindow>

#include "Fixtures/RAIIFixtures.h"
#include "GraphicsSetup.h"
#include "Platform.h"
#include "QGCCommandLineParser.h"
#include "UnitTest.h"
#include "qgc_version.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
void PlatformTest::_testCheckSingleInstanceWhileLockIsHeld()
{
    const QString lockKey = QStringLiteral("%1 RunGuardKey").arg(QLatin1String(QGC_APP_NAME));
    TestFixtures::SingleInstanceLockFixture heldLock(lockKey);
    if (!heldLock.isLocked()) {
        QSKIP("Single-instance lock is already held by another process.");
    }

    // With the lock already held, Platform guard should refuse startup.
    QVERIFY(!Platform::checkSingleInstance(false));
    // Allowing multiple instances bypasses the guard
    QVERIFY(Platform::checkSingleInstance(true));
}
#endif  // !Q_OS_ANDROID && !Q_OS_IOS

#if defined(QGC_UNITTEST_BUILD)
void PlatformTest::_testInitializeSetsUnitTestEnvironment()
{
    TestFixtures::EnvVarFixture restoreConsole("QT_ASSUME_STDERR_HAS_CONSOLE");
    TestFixtures::EnvVarFixture restoreLogging("QT_FORCE_STDERR_LOGGING");
    TestFixtures::EnvVarFixture restoreQpa("QT_QPA_PLATFORM");

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

#if defined(QGC_UNITTEST_BUILD) && defined(Q_OS_WIN)
void PlatformTest::_testInitializeSwRastRequestsSoftwareDevice()
{
    TestFixtures::EnvVarFixture restoreSoftware("QSG_RHI_PREFER_SOFTWARE_RENDERER");
    (void) qunsetenv("QSG_RHI_PREFER_SOFTWARE_RENDERER");
    const bool hadSoftwareGl = QCoreApplication::testAttribute(Qt::AA_UseSoftwareOpenGL);
    // Qt warns when GL attributes change after app creation, so only restore on a real change.
    const auto restoreSoftwareGl = qScopeGuard([hadSoftwareGl]() {
        if (QCoreApplication::testAttribute(Qt::AA_UseSoftwareOpenGL) != hadSoftwareGl) {
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, hadSoftwareGl);
        }
    });

    QGCCommandLineParser::CommandLineParseResult args;
    args.allowMultiple = true;
    args.useSwRast = true;

    char appName[] = "qgc-unit-test";
    char* argv[] = {appName};
    const std::optional<int> initResult = Platform::initialize(1, argv, args);

    QVERIFY(!initResult.has_value());
    QCOMPARE(qgetenv("QSG_RHI_PREFER_SOFTWARE_RENDERER"), QByteArray("1"));
    // Qt 6 ships no opengl32sw.dll, so forcing software GL would silently land on the hardware GL driver.
    QVERIFY(!QCoreApplication::testAttribute(Qt::AA_UseSoftwareOpenGL));
}
#endif

void PlatformTest::_testGraphicsSetupConfiguresFirstWindowOnCreation()
{
    TestFixtures::EnvVarFixture restoreCache("QGC_RHI_PIPELINE_CACHE");
    qunsetenv("QGC_RHI_PIPELINE_CACHE");

    // Android shows the root window, initializing its scene graph, while the QML loads; the configuration must land
    // when the window's surface is created, and only on that first window.
    GraphicsSetup::configureMainWindowOnCreation();
    QQuickWindow first;
    QVERIFY(first.graphicsConfiguration().pipelineCacheSaveFile().isEmpty());
    first.create();
    QVERIFY(first.graphicsConfiguration().pipelineCacheSaveFile().endsWith(QStringLiteral("qgc_rhi_pipeline.cache")));

    QQuickWindow second;
    second.create();
    QVERIFY(second.graphicsConfiguration().pipelineCacheSaveFile().isEmpty());
}

UT_REGISTER_TEST(PlatformTest, TestLabel::Unit, TestLabel::Utilities)
