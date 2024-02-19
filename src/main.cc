/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtGlobal>
#include <QApplication>
#include <QIcon>
#include <QSslSocket>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QHostAddress>
#include <QUdpSocket>
#include <QtPlugin>
#include <QStringListModel>
#include <QQuickStyle>
#include <QQuickWindow>

#include "QGC.h"
#include "QGCApplication.h"
#include "AppMessages.h"
#ifdef __android__
    #include "AndroidInterface.h"
#endif

#include <iostream>

#ifndef __mobile__
    #include "QGCSerialPortInfo.h"
    #include "RunGuard.h"
#ifndef NO_SERIAL_LINK
    #include <QSerialPort>
#endif
#endif

#ifdef UNITTEST_BUILD
    #include "UnitTest.h"
#endif

#ifdef QT_DEBUG
    #include "CmdLineOptParser.h"
    #ifdef Q_OS_WIN
        #include <crtdbg.h>
    #endif
#endif

#ifdef QGC_ENABLE_BLUETOOTH
#include <QtBluetooth/QBluetoothSocket>
#endif

#include "QGCMapEngine.h"

/* SDL does ugly things to main() */
#ifdef main
#undef main
#endif

#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    Q_DECLARE_METATYPE(QGCSerialPortInfo)
#endif
#endif

#ifdef Q_OS_WIN

#include <windows.h>

/// @brief CRT Report Hook installed using _CrtSetReportHook. We install this hook when
/// we don't want asserts to pop a dialog on windows.
int WindowsCrtReportHook(int reportType, char* message, int* returnValue)
{
    Q_UNUSED(reportType);

    std::cerr << message << std::endl;  // Output message to stderr
    *returnValue = 0;                   // Don't break into debugger
    return true;                        // We handled this fully ourselves
}

#endif

<<<<<<< HEAD
#if defined(Q_OS_ANDROID)
#include <jni.h>
#include "AndroidInterface.h"
#include "JoystickAndroid.h"
#if !defined(NO_SERIAL_LINK)
#include "qserialport.h"
#endif
=======
>>>>>>> 178f241aa (separate android init code)


// To shut down QGC on Ctrl+C on Linux
#ifdef Q_OS_LINUX
#include <csignal>

void sigHandler(int s)
{
    std::signal(s, SIG_DFL);
    qgcApp()->mainRootWindow()->close();
    QEvent event{QEvent::Quit};
    qgcApp()->event(&event);
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
#ifndef __mobile__
    // We make the runguard key different for custom and non custom
    // builds, so they can be executed together in the same device.
    // Stable and Daily have same QGC_APPLICATION_NAME so they would
    // not be able to run at the same time
    QString runguardString(QGC_APPLICATION_NAME);
    runguardString.append("RunGuardKey");

    RunGuard guard(runguardString);
    if (!guard.tryToRun()) {
        // QApplication is necessary to use QMessageBox
        QApplication errorApp(argc, argv);
        QMessageBox::critical(nullptr, QObject::tr("Error"),
            QObject::tr("A second instance of %1 is already running. Please close the other instance and try again.").arg(QGC_APPLICATION_NAME)
        );
        return -1;
    }
#endif

    //-- Record boot time
    QGC::initTimer();

#ifdef Q_OS_UNIX
    //Force writing to the console on UNIX/BSD devices
    if (!qEnvironmentVariableIsSet("QT_LOGGING_TO_CONSOLE"))
        qputenv("QT_LOGGING_TO_CONSOLE", "1");
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
    qputenv("QT_OPENGL_BUGLIST", ":/opengl/resources/opengl/buglist.json");

    // Allow for command line override of renderer
    for (int i = 0; i < argc; i++) {
        const QString arg(argv[i]);
        if (arg == QStringLiteral("-angle")) {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
            break;
        } else if (arg == QStringLiteral("-swrast")) {
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
            break;
        }
    }
#endif

#ifdef Q_OS_LINUX
    std::signal(SIGINT, sigHandler);
    std::signal(SIGTERM, sigHandler);
#endif /* Q_OS_LINUX */

    // The following calls to qRegisterMetaType are done to silence debug output which warns
    // that we use these types in signals, and without calling qRegisterMetaType we can't queue
    // these signals. In general we don't queue these signals, but we do what the warning says
    // anyway to silence the debug output.
#ifndef NO_SERIAL_LINK
    qRegisterMetaType<QSerialPort::SerialPortError>();
#endif
#ifdef QGC_ENABLE_BLUETOOTH
    qRegisterMetaType<QBluetoothSocket::SocketError>();
    qRegisterMetaType<QBluetoothServiceInfo>();
#endif
    qRegisterMetaType<QAbstractSocket::SocketError>();
#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    qRegisterMetaType<QGCSerialPortInfo>();
#endif
#endif

    qRegisterMetaType<Vehicle::MavCmdResultFailureCode_t>("Vehicle::MavCmdResultFailureCode_t");

    // We statically link our own QtLocation plugin

#ifdef Q_OS_WIN
    // In Windows, the compiler doesn't see the use of the class created by Q_IMPORT_PLUGIN
#pragma warning( disable : 4930 4101 )
#endif

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

    if (quietWindowsAsserts) {
#ifdef Q_OS_WIN
        _CrtSetReportHook(WindowsCrtReportHook);
#endif
    }

#ifdef Q_OS_WIN
    if (runUnitTests) {
        // Don't pop up Windows Error Reporting dialog when app crashes. This prevents TeamCity from
        // hanging.
        DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
        SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    }
#endif
#endif // QT_DEBUG

#ifdef Q_OS_DARWIN
    // Gstreamer video playback requires OpenGL
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    QQuickStyle::setStyle("Basic");
    QGCApplication* app = new QGCApplication(argc, argv, runUnitTests);
    Q_CHECK_PTR(app);
    if(app->isErrorState()) {
        app->exec();
        return -1;
    }

#ifdef Q_OS_LINUX
    QApplication::setWindowIcon(QIcon(":/res/resources/icons/qgroundcontrol.ico"));
#endif /* Q_OS_LINUX */

    // There appears to be a threading issue in qRegisterMetaType which can cause it to throw a qWarning
    // about duplicate type converters. This is caused by a race condition in the Qt code. Still working
    // with them on tracking down the bug. For now we register the type which is giving us problems here
    // while we only have the main thread. That should prevent it from hitting the race condition later
    // on in the code.
    qRegisterMetaType<QList<QPair<QByteArray,QByteArray> > >();

    app->_initCommon();
    //-- Initialize Cache System
    getQGCMapEngine()->init();

    int exitCode = 0;

#ifdef UNITTEST_BUILD
    if (runUnitTests) {
        for (int i=0; i < (stressUnitTests ? 20 : 1); i++) {
            if (!app->_initForUnitTests()) {
                return -1;
            }

            // Run the test
            int failures = UnitTest::run(unitTestOptions);
            if (failures == 0) {
                qDebug() << "ALL TESTS PASSED";
                exitCode = 0;
            } else {
                qDebug() << failures << " TESTS FAILED!";
                exitCode = -failures;
                break;
            }
        }
    } else
#endif
    {

#ifdef Q_OS_ANDROID
        AndroidInterface::checkStoragePermissions();
#endif
        if (!app->_initForNormalAppBoot()) {
            return -1;
        }
        exitCode = app->exec();
    }

    app->_shutdown();
    delete app;
    //-- Shutdown Cache System
    destroyMapEngine();

    qDebug() << "After app delete";

    return exitCode;
}
