/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <iterator>                 // std::size
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

#ifdef Q_OS_ANDROID
    #include "AndroidInterface.h"
#endif

#ifdef Q_OS_LINUX
    #include <unistd.h>
    #include <sys/types.h>
#endif

#ifdef QGC_UNITTEST_BUILD
    #include "UnitTestList.h"
#endif

int main(int argc, char *argv[])
{
    bool runUnitTests = false;
    bool simpleBootTest = false;
    QString systemIdStr;
    bool hasSystemId = false;
    bool bypassRunGuard = false;

    bool stressUnitTests = false;       // Stress test unit tests
    bool quietWindowsAsserts = false;   // Suppress Windows assert UI
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
    };

    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, std::size(rgCmdLineOptions), false);

#ifdef QGC_UNITTEST_BUILD
    if (stressUnitTests) {
        runUnitTests = true;
    }
#ifdef Q_OS_WIN
    if (runUnitTests) {
        quietWindowsAsserts = true;
    }
#endif
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // Single-instance guard for desktop
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
                        "You should not do this since it will cause other issues with %1. "
                        "%1 will now exit.<br/><br/>").arg(QGC_APP_NAME)
        );
        return -1;
    }
#endif

    // Early platform setup before Qt app construction
    Platform::setupPreApp(quietWindowsAsserts);

#ifdef Q_OS_WIN
    // Allow command-line override of renderer
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
#endif

    QGCApplication app(argc, argv, runUnitTests, simpleBootTest);

    QGCLogging::installHandler();

    // Late platform setup after app and logging exist
    Platform::setupPostApp();

    app.init();

    // Optional: set MAVLink System ID from CLI, e.g. --system-id:255
    if (hasSystemId) {
        bool ok = false;
        const int systemId = systemIdStr.toInt(&ok);
        if (ok && systemId >= 1 && systemId <= 255) { // MAVLink system IDs are 1..255 for GCS use
            qDebug() << "Setting MAVLink System ID to:" << systemId;
            SettingsManager::instance()->mavlinkSettings()->gcsMavlinkSystemID()->setRawValue(systemId);
        } else {
            qDebug() << "Not setting MAVLink System ID. It must be between 1 and 255. Invalid value:" << systemIdStr;
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
