#include <QtQuick/QQuickWindow>
#include <QtWidgets/QApplication>

#include "QGCApplication.h"
#include "QGCCommandLineParser.h"
#include "QGCLogging.h"
#include "QGCLoggingCategory.h"
#include "Platform.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    #include <QtWidgets/QMessageBox>
    #include "RunGuard.h"
#endif

#ifdef Q_OS_LINUX
    #include <unistd.h>
    #include <sys/types.h>
#endif

#ifdef QGC_UNITTEST_BUILD
    #include "UnitTestList.h"
#endif

QGC_LOGGING_CATEGORY(MainLog, "Main")

// ============================================================================
// Platform-Specific Checks
// ============================================================================

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
/// @brief Check if running as root (dangerous on Linux)
/// @return true if running as root
static bool isRunningAsRoot()
{
    return ::getuid() == 0;
}

/// @brief Show error dialog for root execution
/// @return Exit code (always -1)
static int showRootError(int argc, char *argv[])
{
    const QApplication errorApp(argc, argv);
    (void) QMessageBox::critical(nullptr,
        QCoreApplication::translate("main", "Error"),
        QCoreApplication::translate("main",
            "You are running %1 as root. "
            "You should not do this since it will cause other issues with %1. "
            "%1 will now exit.<br/><br/>").arg(QLatin1String(QGC_APP_NAME)));
    return -1;
}
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
/// @brief Show error dialog for multiple instances
/// @return Exit code (always -1)
static int showMultipleInstanceError(int argc, char *argv[])
{
    const QApplication errorApp(argc, argv);
    (void) QMessageBox::critical(nullptr,
        QCoreApplication::translate("main", "Error"),
        QCoreApplication::translate("main",
            "A second instance of %1 is already running. "
            "Please close the other instance and try again.").arg(QLatin1String(QGC_APP_NAME)));
    return -1;
}
#endif

// ============================================================================
// Application Entry Point
// ============================================================================

int main(int argc, char *argv[])
{
#if defined(QGC_UNITTEST_BUILD) && 0
    // Debugging helper: Override command line for specific test debugging
    // Uncomment the #if 0 above to enable
    char argument1[] = "--unittest:FTPManagerTest";
    char argument2[] = "--logging:Vehicle.FTPManager";
    char *newArgv[] = { argv[0], argument1, argument2 };
    argc = 3;
    argv = newArgv;
#endif

    // --- Platform safety checks ---
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (isRunningAsRoot()) {
        return showRootError(argc, argv);
    }
#endif

    // --- Parse command line arguments ---
    QGCCommandLineParser::CommandLineParseResult args;
    {
        const QCoreApplication pre(argc, argv);
        QCoreApplication::setApplicationName(QLatin1String(QGC_APP_NAME));
        QCoreApplication::setApplicationVersion(QLatin1String(QGC_APP_VERSION_STR));
        args = QGCCommandLineParser::parseCommandLine();

        if (args.statusCode == QGCCommandLineParser::CommandLineParseResult::Status::Error) {
            const QString errorMessage = args.errorString.value_or(QStringLiteral("Unknown error occurred"));
            qCritical() << qPrintable(errorMessage);
            return 1;
        }

        if (args.statusCode == QGCCommandLineParser::CommandLineParseResult::Status::HelpRequested) {
            // Help is printed by QCommandLineParser
            return 0;
        }

        if (args.statusCode == QGCCommandLineParser::CommandLineParseResult::Status::VersionRequested) {
            // Version is printed by QCommandLineParser
            return 0;
        }
    }

    // --- Single instance check (desktop only) ---
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    const QString runguardString = QStringLiteral("%1 RunGuardKey").arg(QLatin1String(QGC_APP_NAME));
    RunGuard guard(runguardString);
    if (!args.allowMultiple && !guard.tryToRun()) {
        return showMultipleInstanceError(argc, argv);
    }
#endif

    // --- Application initialization ---
    Platform::setupPreApp(args);

    QGCApplication app(argc, argv, args);

    QGCLogging::installHandler();

    Platform::setupPostApp();

    app.init();

    // --- Run application or tests ---
    int exitCode = 0;

#ifdef QGC_UNITTEST_BUILD
    if (args.runningUnitTests) {
        qCInfo(MainLog) << "Running unit tests";
        exitCode = QGCUnitTest::runTests(
            args.stressUnitTests,
            args.unitTests,
            args.unitTestOutput.value_or(QString())
        );
    } else
#endif
    if (!args.simpleBootTest) {
        qCInfo(MainLog) << "Starting application event loop";
        exitCode = app.exec();
    } else {
        qCInfo(MainLog) << "Simple boot test completed";
    }

    // --- Cleanup ---
    app.shutdown();

    qCInfo(MainLog) << "Exiting main";
    return exitCode;
}
