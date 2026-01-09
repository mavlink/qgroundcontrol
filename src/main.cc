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
#include <fstream>

#include "QGCApplication.h"
#include "QGCCommandLineParser.h"
#include "QGCLogging.h"
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

int main(int argc, char *argv[])
{
    std::ofstream log("C:\\IGCS\\boot_log.txt", std::ios::app);
    log << "MAIN: start" << std::endl;
#if 0
    // Useful for debugging specific unit tests
    char argument1[] = "--unittest:ParameterManagerTest";
    char argument2[] = "--logging:FactSystem.ParameterManager,Utilities.QGCStateMachine";
    char *newArgv[] = { argv[0], argument1, argument2 };
    argc = 3;
    argv = newArgv;
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (::getuid() == 0) {
        const QApplication errorApp(argc, argv);
        // QErrorMessage
        (void) QMessageBox::critical(nullptr,
                                     QCoreApplication::translate("main", "Error"),
                                     QCoreApplication::translate("main", "You are running %1 as root. "
                                                                         "You should not do this since it will cause other issues with %1. "
                                                                         "%1 will now exit.<br/><br/>").arg(QGC_APP_NAME));
        return -1;
    }
#endif

    QGCCommandLineParser::CommandLineParseResult args;
    {
        log << "MAIN: parsing args" << std::endl;
        const QCoreApplication pre(argc, argv);
        QCoreApplication::setApplicationName(QStringLiteral(QGC_APP_NAME));
        QCoreApplication::setApplicationVersion(QStringLiteral(QGC_APP_VERSION_STR));
        args = QGCCommandLineParser::parseCommandLine();
        if (args.statusCode == QGCCommandLineParser::CommandLineParseResult::Status::Error) {
            const QString errorMessage = args.errorString.value_or(QStringLiteral("Unknown error occurred"));
            qCritical() << qPrintable(errorMessage);
            // TODO: QCommandLineParser::showMessageAndExit(QCommandLineParser::MessageType::Error) - Qt6.9
            return 1;
        }
    }
    log << "MAIN: args parsed" << std::endl;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    const QString runguardString = QStringLiteral("%1 RunGuardKey").arg(QStringLiteral(QGC_APP_NAME));
    RunGuard guard(runguardString);
    if (!args.allowMultiple) {
        if (!guard.tryToRun()) {
            const QApplication errorApp(argc, argv);
            (void) QMessageBox::critical(nullptr,
                QCoreApplication::translate("main", "Error"),
                QCoreApplication::translate("main", "A second instance of %1 is already running. "
                                                    "Please close the other instance and try again.").arg(QStringLiteral(QGC_APP_NAME)));
            return -1;
        }
    }
#endif

    // Early platform setup before Qt app construction
    log << "MAIN: setupPreApp" << std::endl;
    Platform::setupPreApp(args);
    log << "MAIN: setupPreApp done" << std::endl;

    log << "MAIN: QGCApplication ctor" << std::endl;
    QGCApplication app(argc, argv, args);
    log << "MAIN: QGCApplication ctor done" << std::endl;

    log << "MAIN: installHandler" << std::endl;
    QGCLogging::installHandler();
    log << "MAIN: installHandler done" << std::endl;

    // Late platform setup after app and logging exist
    log << "MAIN: setupPostApp" << std::endl;
    Platform::setupPostApp();
    log << "MAIN: setupPostApp done" << std::endl;

    log << "MAIN: init starting" << std::endl;

    app.init();

    log << "MAIN: init done" << std::endl;

    int exitCode = 0;
    if (args.runningUnitTests) {
#ifdef QGC_UNITTEST_BUILD
        exitCode = QGCUnitTest::runTests(args.stressUnitTests, args.unitTests);
#endif
    } else if (!args.simpleBootTest) {
        log << "MAIN: starting exec" << std::endl;
        exitCode = app.exec();
    } else {
        log << "MAIN: simple boot test done" << std::endl;
    }

    log << "MAIN: shutting down" << std::endl;
    app.shutdown();

    qDebug() << "Exiting main";
    log << "MAIN: returning " << exitCode << std::endl;
    return exitCode;
}
