/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtQuick/QQuickWindow>
#include <QtWidgets/QApplication>

#include "QGCApplication.h"
#include "QGCLogging.h"
#include "CmdLineOptParser.h"
#include "SettingsManager.h"
#include "MavlinkSettings.h"
#include "Platform.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    #include <QtWidgets/QMessageBox>
    #include "RunGuard.h"
#endif

#ifdef Q_OS_LINUX
#ifdef Q_OS_ANDROID
    #include "AndroidInterface.h"
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include "SignalHandler.h"
#endif
#endif

#ifdef QGC_UNITTEST_BUILD
    #include "UnitTestList.h"
#endif

//-----------------------------------------------------------------------------
/**
 * @brief Starts the application
 *
 * @param argc Number of commandline arguments
 * @param argv Commandline arguments
 * @return exit code, 0 for normal exit and !=0 for error cases
 */

int main(int argc, char *argv[])
{
    bool runUnitTests = false;
    bool simpleBootTest = false;
    QString systemIdStr = QString();
    bool hasSystemId = false;
    bool bypassRunGuard = false;

    bool stressUnitTests = false;       // Stress test unit tests
    bool quietWindowsAsserts = false;   // Don't let asserts pop dialog boxes
    QString unitTestOptions;

    CmdLineOpt_t rgCmdLineOptions[] = {
#ifdef QT_DEBUG
        { "--unittest",             &runUnitTests,          &unitTestOptions },
        { "--unittest-stress",      &stressUnitTests,       &unitTestOptions },
        { "--no-windows-assert-ui", &quietWindowsAsserts,   nullptr },
        { "--allow-multiple",       &bypassRunGuard,        nullptr },
#endif
        { "--system-id",            &hasSystemId,           &systemIdStr },
        { "--simple-boot-test",     &simpleBootTest,        nullptr },
        // Add additional command line option flags here
    };

    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, std::size(rgCmdLineOptions), false);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // We make the runguard key different for custom and non custom
    // builds, so they can be executed together in the same device.
    // Stable and Daily have same QGC_APP_NAME so they would
    // not be able to run at the same time
    const QString runguardString = QStringLiteral("%1 RunGuardKey").arg(QGC_APP_NAME);

    RunGuard guard(runguardString);
    if (!bypassRunGuard && !guard.tryToRun()) {
        const QApplication errorApp(argc, argv);
        (void) QMessageBox::critical(nullptr, QObject::tr("Error"),
            QObject::tr("A second instance of %1 is already running. Please close the other instance and try again.").arg(QGC_APP_NAME)
        );
        return -1;
    }
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (::getuid() == 0) {
        const QApplication errorApp(argc, argv);
        (void) QMessageBox::critical(nullptr, QObject::tr("Error"),
            QObject::tr("You are running %1 as root. "
                "You should not do this since it will cause other issues with %1."
                "%1 will now exit.<br/><br/>").arg(QGC_APP_NAME)
        );
        return -1;
    }
#endif

#ifdef Q_OS_UNIX
    if (!qEnvironmentVariableIsSet("QT_ASSUME_STDERR_HAS_CONSOLE")) {
        (void) qputenv("QT_ASSUME_STDERR_HAS_CONSOLE", "1");
    }

    if (!qEnvironmentVariableIsSet("QT_FORCE_STDERR_LOGGING")) {
        (void) qputenv("QT_FORCE_STDERR_LOGGING", "1");
    }
#endif

#ifdef Q_OS_WIN
    if (!qEnvironmentVariableIsSet("QT_WIN_DEBUG_CONSOLE")) {
        (void) qputenv("QT_WIN_DEBUG_CONSOLE", "attach"); // new
    }
#endif

    QGCLogging::installHandler(quietWindowsAsserts);

#ifdef QGC_UNITTEST_BUILD
    if (stressUnitTests) {
        runUnitTests = true;
    }
#endif

#ifdef Q_OS_MACOS
    Platform::disableAppNapViaInfoDict();
#endif

#ifdef Q_OS_WIN
    // Set our own OpenGL buglist
    // (void) qputenv("QT_OPENGL_BUGLIST", ":/opengl/resources/opengl/buglist.json");

    // Allow for command line override of renderer
    for (int i = 0; i < argc; i++) {
        const QString arg(argv[i]);
        if (arg == QStringLiteral("-desktop")) {
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
            break;
        } else if (arg == QStringLiteral("-swrast")) {
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
            break;
        }
    }

#ifdef QGC_UNITTEST_BUILD
    if (runUnitTests) {
        // Don't pop up Windows Error Reporting dialog when app crashes.
        const DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
        SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    }
#endif
#endif // Q_OS_WIN

    //my add, tell settings to read which file
    QCoreApplication::setOrganizationName("QGroundControl");
    QCoreApplication::setApplicationName("QGroundControl Daily");
    //end my add

    QGCApplication app(argc, argv, runUnitTests, simpleBootTest);

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    SignalHandler::instance();
    (void) SignalHandler::setupSignalHandlers();
#endif

    app.init();

    // Set system ID if specified via command line, for example --system-id:255
    if (hasSystemId) {
        bool ok;
        const int systemId = systemIdStr.toInt(&ok);
        if (ok && (systemId >= 1) && (systemId <= 255)) {  // MAVLink system IDs are 8-bit
            qDebug() << "Setting MAVLink System ID to:" << systemId;
            SettingsManager::instance()->mavlinkSettings()->gcsMavlinkSystemID()->setRawValue(systemId);
        } else {
            qDebug() << "Not setting MAVLink System ID. It must be between 0 and 255. Invalid system ID value:" << systemIdStr;
        }
    }

    int exitCode = 0;

#ifdef QGC_UNITTEST_BUILD
    if (runUnitTests) {
        exitCode = runTests(stressUnitTests, unitTestOptions);
    } else
#endif
    {
        #ifdef Q_OS_ANDROID
            AndroidInterface::checkStoragePermissions();
        #endif

        if (!simpleBootTest) {
            exitCode = app.exec();
        }
    }

    app.shutdown();

    qDebug() << "Exiting main";

    return exitCode;
}
