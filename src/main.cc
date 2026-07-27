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

#ifdef Q_OS_MACOS
    #include <QtCore/QProcessEnvironment>
#endif

#include "QGCApplication.h"
#include "QGCLogging.h"
#include "CmdLineOptParser.h"
#include "SettingsManager.h"
#include "MavlinkSettings.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    #include <QtWidgets/QMessageBox>
    #include "RunGuard.h"
#endif

#ifdef Q_OS_ANDROID
    #include "AndroidInterface.h"
#endif

#ifdef Q_OS_LINUX
#ifndef Q_OS_ANDROID
    #include "SignalHandler.h"
#endif
#endif

#ifdef QT_DEBUG
#ifdef QGC_UNITTEST_BUILD
    #include "UnitTestList.h"
#endif

#ifdef Q_OS_WIN

#include <crtdbg.h>
#include <windows.h>
#include <iostream>

/// @brief CRT Report Hook installed using _CrtSetReportHook. We install this hook when
/// we don't want asserts to pop a dialog on windows.
int WindowsCrtReportHook(int reportType, char* message, int* returnValue)
{
    Q_UNUSED(reportType);

    std::cerr << message << std::endl;  // Output message to stderr
    *returnValue = 0;                   // Don't break into debugger
    return true;                        // We handled this fully ourselves
}

#endif // Q_OS_WIN

#endif // QT_DEBUG

//-----------------------------------------------------------------------------
/**
 * @brief Starts the application
 *
 * @param argc Number of commandline arguments
 * @param argv Commandline arguments
 * @return exit code, 0 for normal exit and !=0 for error cases
 */

#include <fstream>

int main(int argc, char *argv[])
{
    std::ofstream log("C:\\projects\\qgroundcontrol\\startup.log", std::ios::out | std::ios::app);
    log << "main() started" << std::endl;

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
#endif
        { "--allow-multiple",       &bypassRunGuard,        nullptr },
        { "--system-id",            &hasSystemId,           &systemIdStr },
        { "--simple-boot-test",     &simpleBootTest,        nullptr },
        // Add additional command line option flags here
    };

    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, std::size(rgCmdLineOptions), false);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    const QString runguardString = QStringLiteral("%1 RunGuardKey").arg(QGC_APP_NAME);

    RunGuard guard(runguardString);
    if (!bypassRunGuard && !guard.tryToRun()) {
        log << "RunGuard failed, exiting" << std::endl;
        QApplication errorApp(argc, argv);
        QMessageBox::critical(nullptr, QObject::tr("Error"),
            QObject::tr("A second instance of %1 is already running. Please close the other instance and try again.").arg(QGC_APP_NAME)
        );
        return -1;
    }
    log << "RunGuard passed" << std::endl;
#endif

    log << "Creating QGCApplication" << std::endl;
    QGCApplication app(argc, argv, runUnitTests, simpleBootTest);

    log << "Initializing app" << std::endl;
    app.init();
    log << "app.init() completed" << std::endl;

    int exitCode = 0;

#ifdef QGC_UNITTEST_BUILD
    if (runUnitTests) {
        log << "Running unit tests" << std::endl;
        exitCode = runTests(stressUnitTests, unitTestOptions);
    } else
#endif
    {
        if (!simpleBootTest) {
            log << "Calling app.exec()" << std::endl;
            exitCode = app.exec();
            log << "app.exec() returned code " << exitCode << std::endl;
        }
    }

    log << "Calling app.shutdown()" << std::endl;
    app.shutdown();

    log << "Exiting main cleanly" << std::endl;

    return exitCode;
}
