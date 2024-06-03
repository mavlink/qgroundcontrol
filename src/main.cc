/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtCore/QProcessEnvironment>
#include <QtCore/QtPlugin>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtQuick/QQuickWindow>

#include "QGCApplication.h"
#include "QGC.h"
#include "AppMessages.h"

#ifdef Q_OS_ANDROID
    #include "AndroidInterface.h"
#endif

#ifdef QT_DEBUG

#include "CmdLineOptParser.h"

#ifdef UNITTEST_BUILD
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

// To shut down QGC on Ctrl+C on Linux
#ifdef Q_OS_LINUX

#include <csignal>

void sigHandler(int s)
{
    std::signal(s, SIG_DFL);
    if(qgcApp()) {
        qgcApp()->mainRootWindow()->close();
        QEvent event{QEvent::Quit};
        qgcApp()->event(&event);
    }
}

#endif /* Q_OS_LINUX */

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
#ifdef Q_OS_LINUX
#ifndef Q_OS_ANDROID
    if (getuid() == 0) {
        QApplication errorApp(argc, argv);
        QMessageBox::critical(nullptr, QObject::tr("Error"),
            QObject::tr("You are running %1 as root. "
                "You should not do this since it will cause other issues with %1."
                "%1 will now exit.<br/><br/>").arg(QGC_APP_NAME)
        );
        return -1;
    }
#endif
#endif

    //-- Record boot time
    QGC::initTimer();

#ifdef Q_OS_UNIX
    //Force writing to the console on UNIX/BSD devices
    if (!qEnvironmentVariableIsSet("QT_LOGGING_TO_CONSOLE")) {
        qputenv("QT_LOGGING_TO_CONSOLE", "1");
    }
#endif

    // install the message handler
    AppMessages::installHandler();

#ifdef Q_OS_MAC
#ifndef Q_OS_IOS
    // Prevent Apple's app nap from screwing us over
    // tip: the domain can be cross-checked on the command line with <defaults domains>
    QProcess::execute("defaults", {"write org.qgroundcontrol.qgroundcontrol NSAppSleepDisabled -bool YES"});
#endif
#endif

#ifdef Q_OS_WIN
    // Set our own OpenGL buglist
    // qputenv("QT_OPENGL_BUGLIST", ":/opengl/resources/opengl/buglist.json");

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

// In Windows, the compiler doesn't see the use of the class created by Q_IMPORT_PLUGIN
#pragma warning( disable : 4930 4101 )

#endif

    // We statically link our own QtLocation plugin
    Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryQGC)

    bool runUnitTests = false;          // Run unit tests

#ifdef QT_DEBUG
    // We parse a small set of command line options here prior to QGCApplication in order to handle the ones
    // which need to be handled before a QApplication object is started.

    bool stressUnitTests = false;       // Stress test unit tests
    bool quietWindowsAsserts = false;   // Don't let asserts pop dialog boxes

    QString unitTestOptions;
    CmdLineOpt_t rgCmdLineOptions[] = {
        { "--unittest",             &runUnitTests,          &unitTestOptions },
        { "--unittest-stress",      &stressUnitTests,       &unitTestOptions },
        { "--no-windows-assert-ui", &quietWindowsAsserts,   nullptr },
        // Add additional command line option flags here
    };

    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, sizeof(rgCmdLineOptions)/sizeof(rgCmdLineOptions[0]), false);
    if (stressUnitTests) {
        runUnitTests = true;
    }

#ifdef Q_OS_WIN
    if (quietWindowsAsserts) {
        _CrtSetReportHook(WindowsCrtReportHook);
    }

    if (runUnitTests) {
        // Don't pop up Windows Error Reporting dialog when app crashes. This prevents TeamCity from
        // hanging.
        const DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
        SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    }
#endif // Q_OS_WIN
#endif // QT_DEBUG

    QGCApplication app(argc, argv, runUnitTests);

    #ifdef Q_OS_LINUX
        std::signal(SIGINT, sigHandler);
        std::signal(SIGTERM, sigHandler);
    #endif

    app.init();

    int exitCode = 0;

#ifdef UNITTEST_BUILD
    if (runUnitTests) {
        exitCode = runTests(stressUnitTests, unitTestOptions);
    } else
#endif
    {
        #ifdef Q_OS_ANDROID
            AndroidInterface::checkStoragePermissions();
        #endif

        exitCode = app.exec();
    }

    app.shutdown();

    qDebug() << "Exiting main";

    return exitCode;
}
