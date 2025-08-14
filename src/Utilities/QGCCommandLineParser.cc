/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCommandLineParser.h"
#include <QtCore/QCommandLineOption>
#include <QtCore/QCoreApplication>

namespace QGCCommandLineParser {

void parseCommandLine(const QStringList &args, CommandLineParseResult &out)
{
    QCommandLineParser &parser = out.parser;

    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);

    parser.setApplicationDescription(QCoreApplication::applicationName());
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    const QCommandLineOption unittestOpt(QStringLiteral("unittest"),
                                         QStringLiteral("Run unit tests (optional filter value)."),
                                         QStringLiteral("filter"));

    const QCommandLineOption unittestStressOpt(QStringLiteral("unittest-stress"),
                                               QStringLiteral("Stress unit tests (optional count, default 1)."),
                                               QStringLiteral("count"));

    const QCommandLineOption clearSettingsOpt(QStringLiteral("clear-settings"),
                                              QStringLiteral("Clear stored application settings."));

    const QCommandLineOption clearCacheOpt(QStringLiteral("clear-cache"),
                                           QStringLiteral("Clear parameter and airframe caches."));

    const QCommandLineOption loggingOpt(QStringLiteral("logging"),
                                        QStringLiteral("Enable logging with optional rules string, e.g. \"*.debug=false;qgc.*=true\"."),
                                        QStringLiteral("rules"));

    const QCommandLineOption fakeMobileOpt(QStringLiteral("fake-mobile"),
                                           QStringLiteral("Run with mobile-style UI."));

    const QCommandLineOption logOutputOpt(QStringLiteral("log-output"),
                                          QStringLiteral("Log to console."));

    const QCommandLineOption simpleBootOpt(QStringLiteral("simple-boot-test"),
                                           QStringLiteral("Initialize subsystems and exit."));

    const QCommandLineOption allowMultipleOpt(QStringLiteral("allow-multiple"),
                                              QStringLiteral("Bypass single-instance guard."));

    const QCommandLineOption systemIdOpt(QStringLiteral("system-id"),
                                         QStringLiteral("MAVLink GCS system id (1-255)."),
                                         QStringLiteral("id"));

#ifdef Q_OS_WIN
    const QCommandLineOption quietWinAssertOpt(QStringLiteral("no-windows-assert-ui"),
                                               QStringLiteral("Disable Windows assert dialog boxes (debug builds)."));

    const QCommandLineOption desktopOpt(QStringLiteral("desktop"),
                                        QStringLiteral("Force Desktop OpenGL."));

    const QCommandLineOption swrastOpt(QStringLiteral("swrast"),
                                       QStringLiteral("Force software OpenGL."));
#endif

    (void) parser.addOption(unittestOpt);
    (void) parser.addOption(unittestStressOpt);
    (void) parser.addOption(clearSettingsOpt);
    (void) parser.addOption(clearCacheOpt);
    (void) parser.addOption(loggingOpt);
    (void) parser.addOption(fakeMobileOpt);
    (void) parser.addOption(logOutputOpt);
    (void) parser.addOption(simpleBootOpt);
    (void) parser.addOption(allowMultipleOpt);
    (void) parser.addOption(systemIdOpt);
#ifdef Q_OS_WIN
    (void) parser.addOption(quietWinAssertOpt);
    (void) parser.addOption(desktopOpt);
    (void) parser.addOption(swrastOpt);
#endif

    parser.process(args);

    out.unknownOptions = parser.unknownOptionNames();
    if (!out.unknownOptions.isEmpty()) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QStringLiteral("Unknown options: %1").arg(out.unknownOptions.join(", "));
        return;
    }

    out.positional = parser.positionalArguments();
    if (!out.positional.isEmpty()) {
        out.statusCode = CommandLineParseResult::Status::Error;
        out.errorString = QStringLiteral("Unexpected positional arguments: %1").arg(out.positional.join(", "));
        return;
    }

    if (parser.isSet(helpOption)) {
        out.statusCode = CommandLineParseResult::Status::HelpRequested;
        out.helpText = parser.helpText();
        return;
    }

    if (parser.isSet(versionOption)) {
        out.statusCode = CommandLineParseResult::Status::VersionRequested;
        out.versionText = QCoreApplication::applicationVersion();
        return;
    }

    out.clearSettingsOptions = parser.isSet(clearSettingsOpt);
    out.clearCache = parser.isSet(clearCacheOpt);
    out.logging = parser.isSet(loggingOpt);
    out.loggingOptions = out.logging ? parser.value(loggingOpt) : QString();
    out.fakeMobile = parser.isSet(fakeMobileOpt);
    out.logOutput = parser.isSet(logOutputOpt);
    out.simpleBootTest = parser.isSet(simpleBootOpt);
    out.allowMultiple = parser.isSet(allowMultipleOpt);

    if (parser.isSet(systemIdOpt)) {
        out.hasSystemId = true;
        out.systemIdStr = parser.value(systemIdOpt);
    }

    if (parser.isSet(unittestOpt)) {
        out.runningUnitTests = true;
        out.unitTests = parser.values(unittestOpt);
    }

    if (parser.isSet(unittestStressOpt)) {
        out.runningUnitTests = true;
        out.stressUnitTests = true;
        const QString stress = parser.value(unittestStressOpt);
        if (!stress.isEmpty()) {
            bool ok = false;
            const uint count = stress.toUInt(&ok);
            if (!ok || (count == 0)) {
                out.statusCode = CommandLineParseResult::Status::Error;
                out.errorString = QStringLiteral("Invalid stress unit test count: %1").arg(stress);
                return;
            }
            out.stressUnitTestsCount = count;
        } // else default 1
    }

#ifdef Q_OS_WIN
    out.quietWindowsAsserts = parser.isSet(quietWinAssertOpt);
    out.useDesktopGL = parser.isSet(desktopOpt);
    out.useSwRast = parser.isSet(swrastOpt);
#endif

    out.statusCode = CommandLineParseResult::Status::Ok;
}

} // namespace QGCCommandLineParser
