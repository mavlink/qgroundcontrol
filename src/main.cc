/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtCore/QCommandLineParser>
#ifdef Q_OS_MACOS
#include <QtCore/QProcessEnvironment>
#endif
#include <QtQuick/QQuickWindow>
#include <QtWidgets/QApplication>

#include "QGCApplication.h"
#include "QGCCommandLineParser.h"
#include "QGCLogging.h"
#ifdef QGC_UNITTEST_BUILD
#include "UnitTestList.h"
#endif

#include <QtWidgets/QMessageBox>
// #include <QtWidgets/QErrorMessage>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include "RunGuard.h"
#endif

#ifdef Q_OS_ANDROID
#include "AndroidInterface.h"
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#include "SignalHandler.h"
#endif

int main(int argc, char *argv[])
{
    QStringList rawArgs;
    for (int i = 1; i < argc; ++i) {
        rawArgs << QString::fromLocal8Bit(argv[i]);
    }

    QCommandLineParser preappParser;
    preappParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    (void) preappParser.addOption({QStringLiteral("desktop"), QStringLiteral("Use desktop OpenGL")});
    (void) preappParser.addOption({QStringLiteral("software"), QStringLiteral("Use software OpenGL")});
    (void) preappParser.addOption({QStringLiteral("gles"), QStringLiteral("Use OpenGLES")});
    (void) preappParser.addOption({QStringLiteral("shared"), QStringLiteral("Share OpenGL Contexts")});
    (void) preappParser.parse(rawArgs);

    if (preappParser.isSet("desktop")) {
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    } else if (preappParser.isSet("software")) {
        QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    } else if (preappParser.isSet("gles")) {
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    } else if (preappParser.isSet("shared")) {
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    const QApplication errorApp(argc, argv);
    const QString runGuardKey = QStringLiteral("%1 RunGuardKey").arg(QGC_APP_NAME);
    RunGuard guard(runGuardKey);
    if (!guard.tryToRun()) {
        // TODO: Qt6.9 QCommandLineParser::showMessageAndExit, QCommandLineParser::MessageType::Error
        (void) QMessageBox::critical(nullptr, QObject::tr("Error"),
                                     QObject::tr("A second instance of %1 is already running. Please close the other instance and try again.").arg(QGC_APP_NAME));
        return -1;
    }
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (getuid() == 0) {
        (void) QMessageBox::critical(nullptr, QObject::tr("Error"),
                                     QObject::tr("You are running %1 as root. This is not supported; %1 will now exit.").arg(QGC_APP_NAME));
        return -1;
    }
#endif

    QGCCommandLineParser::CommandLineParseResult parseResult{};
    parseCommandLine(parseResult);

    switch (parseResult.statusCode) {
    using Status = QGCCommandLineParser::CommandLineParseResult::Status;
    case Status::Error:
        (void) std::fputs(qPrintable(parseResult.errorString.value()), stderr);
        (void) std::fputs("\n\n", stderr);
        (void) std::fputs(qPrintable(parseResult.parser.helpText()), stderr);
        // QErrorMessage
        (void) QMessageBox::warning(nullptr, QGuiApplication::applicationDisplayName(),
                                    "<html><body><h2>" + parseResult.errorString.value() + "</h2><pre>" + parseResult.parser.helpText() + "</pre></body></html>");
        return -1;
    case Status::VersionRequested:
        parseResult.parser.showVersion();
        (void) QMessageBox::information(nullptr, QGuiApplication::applicationDisplayName(),
                                        QGuiApplication::applicationDisplayName() + ' ' + QCoreApplication::applicationVersion());
        return 0;
    case Status::HelpRequested:
        parseResult.parser.showHelp();
        (void) QMessageBox::warning(nullptr, QGuiApplication::applicationDisplayName(),
                                    "<html><body><pre>" + parseResult.parser.helpText() + "</pre></body></html>");
        return 0;
    case Status::Ok:
        break;
    }

    QGCLogging::installHandler(parseResult);

#ifdef Q_OS_MACOS
    QProcess::execute("defaults", {"write", "org.qgroundcontrol.qgroundcontrol", "NSAppSleepDisabled", "-bool", "YES"});
#endif

    QGCApplication app(argc, argv, parseResult);

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    (void) SignalHandler::instance()->setupSignalHandlers();
#endif

    app.init();
    int exitCode = 0;

#ifdef QGC_UNITTEST_BUILD
    if (parseResult.runningUnitTests) {
        exitCode = runTests(parseResult.stressUnitTests, parseResult.unitTests.join(","));
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
