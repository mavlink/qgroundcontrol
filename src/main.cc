/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtQuick/QQuickWindow>
#include <QtWidgets/QApplication>

#include "QGCApplication.h"
#include "QGCCommandLineParser.h"
#include "QGCLogging.h"
#include "MavlinkSettings.h"
#ifdef Q_OS_MACOS
#include "Platform.h"
#endif
#include "SettingsManager.h"

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
    #ifdef Q_OS_WIN
        #include <windows.h>
    #endif
#endif

/**
 * @brief Starts the application
 *
 * @param argc Number of commandline arguments
 * @param argv Commandline arguments
 * @return exit code, 0 for normal exit and !=0 for error cases
 */
int main(int argc, char *argv[])
{
    QCoreApplication pre(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral(QGC_APP_NAME));
    QCoreApplication::setOrganizationName(QStringLiteral(QGC_ORG_NAME));
    QCoreApplication::setOrganizationDomain(QStringLiteral(QGC_ORG_DOMAIN));
    QCoreApplication::setApplicationVersion(QStringLiteral(QGC_APP_VERSION_STR));

    QGCCommandLineParser::CommandLineParseResult cli;
    QGCCommandLineParser::parseCommandLine(QCoreApplication::arguments(), cli);

    if (cli.statusCode == QGCCommandLineParser::CommandLineParseResult::Status::HelpRequested) {
        fputs(cli.helpText.toLocal8Bit().constData(), stdout);
        return 0;
    } else if (cli.statusCode == QGCCommandLineParser::CommandLineParseResult::Status::VersionRequested) {
        fputs((cli.versionText + "\n").toLocal8Bit().constData(), stdout);
        return 0;
    } else if (cli.statusCode == QGCCommandLineParser::CommandLineParseResult::Status::Error) {
        fputs((cli.errorString + "\n").toLocal8Bit().constData(), stderr);
        return 1;
    }

#ifdef Q_OS_UNIX
    if (!qEnvironmentVariableIsSet("QT_ASSUME_STDERR_HAS_CONSOLE")) {
        qputenv("QT_ASSUME_STDERR_HAS_CONSOLE", "1");
    }
    if (!qEnvironmentVariableIsSet("QT_FORCE_STDERR_LOGGING")) {
        qputenv("QT_FORCE_STDERR_LOGGING", "1");
    }
#endif
#ifdef Q_OS_WIN
    if (!qEnvironmentVariableIsSet("QT_WIN_DEBUG_CONSOLE")) {
        qputenv("QT_WIN_DEBUG_CONSOLE", "attach");
    }
    if (cli.useDesktopGL) {
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    }
    if (cli.useSwRast) {
        QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    }
    // (void) qputenv("QT_OPENGL_BUGLIST", ":/opengl/resources/opengl/buglist.json");
#endif

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    // QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);

    QGCLogging::installHandler(
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
        cli.quietWindowsAsserts
#else
        false
#endif
    );

#ifdef QGC_UNITTEST_BUILD
    if (cli.stressUnitTests) {
        cli.runningUnitTests = true;
    }
#ifdef Q_OS_WIN
    if (cli.runningUnitTests) {
        const DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
        SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    }
#endif
#endif

#ifdef Q_OS_MACOS
    Platform::disableAppNapViaInfoDict();
#endif

    QGCApplication app(argc, argv, cli);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    {
        const QString runguardString = QStringLiteral("%1 RunGuardKey").arg(QGC_APP_NAME);
        RunGuard guard(runguardString);
        if (!cli.allowMultiple && !guard.tryToRun()) {
            const QApplication errorApp(argc, argv);
            (void) QMessageBox::critical(nullptr, QObject::tr("Error"),
                QObject::tr("A second instance of %1 is already running. Please close the other instance and try again.").arg(QGC_APP_NAME));
            return -1;
        }
    }
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (::getuid() == 0) {
        const QApplication errorApp(argc, argv);
        (void) QMessageBox::critical(nullptr, QObject::tr("Error"),
            QObject::tr("You are running %1 as root. "
                        "You should not do this since it will cause other issues with %1."
                        "%1 will now exit.<br/><br/>").arg(QGC_APP_NAME));
        return -1;
    }
    SignalHandler::instance();
    (void) SignalHandler::setupSignalHandlers();
#endif

    app.init();

    if (cli.hasSystemId) {
        bool ok = false;
        const int systemId = cli.systemIdStr.toInt(&ok);
        if (ok && (systemId >= 1) && (systemId <= 255)) {
            qDebug() << "Setting MAVLink System ID to:" << systemId;
            SettingsManager::instance()->mavlinkSettings()->gcsMavlinkSystemID()->setRawValue(systemId);
        } else {
            qDebug() << "Not setting MAVLink System ID. It must be between 0 and 255. Invalid system ID value:" << cli.systemIdStr;
        }
    }

    int exitCode = 0;

#ifdef QGC_UNITTEST_BUILD
    if (cli.runningUnitTests) {
        exitCode = runTests(cli.stressUnitTests, cli.unitTests.join(' '));
    } else
#endif
    {
#ifdef Q_OS_ANDROID
        (void) AndroidInterface::checkStoragePermissions();
#endif
        if (!cli.simpleBootTest) {
            exitCode = app.exec();
        }
    }

    app.shutdown();

    qDebug() << "Exiting main";

    return exitCode;
}
