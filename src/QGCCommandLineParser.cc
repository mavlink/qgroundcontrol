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
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>

namespace QGCCommandLineParser {

void parseCommandLine(CommandLineParseResult &result)
{
    QCommandLineParser &parser = result.parser;

    // Treat single-dash words as long options, and allow options after positionals
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);

    // Basic meta
    parser.setApplicationDescription(QCoreApplication::applicationName());
    const auto helpOption    = parser.addHelpOption();
    const auto versionOption = parser.addVersionOption();

    // Options
    static const QCommandLineOption runUnitTestsOption(
        QStringLiteral("unittest"),
        QStringLiteral("Run unit tests (optionally specify a filter)"),
        QStringLiteral("filter")
    );
    (void) parser.addOption(runUnitTestsOption);

    static const QCommandLineOption stressUnitTestsOption(
        QStringLiteral("unittest-stress"),
        QStringLiteral("Stress test unit tests (optionally specify a count)"),
        QStringLiteral("count")
    );
    (void) parser.addOption(stressUnitTestsOption);

#ifdef Q_OS_WIN
    static const QCommandLineOption quietWindowsAssertsOption(
        QStringLiteral("no-windows-assert-ui"),
        QStringLiteral("Disable Windows assert dialog boxes")
    );
    (void) parser.addOption(quietWindowsAssertsOption);
#endif

    static const QCommandLineOption clearSettingsOption(
        QStringLiteral("clear-settings"),
        QStringLiteral("Clear stored application settings")
    );
    (void) parser.addOption(clearSettingsOption);

    static const QCommandLineOption clearCacheOption(
        QStringLiteral("clear-cache"),
        QStringLiteral("Clear parameter and airframe caches")
    );
    (void) parser.addOption(clearCacheOption);

    static const QCommandLineOption loggingOption(
        QStringLiteral("logging"),
        QStringLiteral("Enable logging (specify options if needed)"),
        QStringLiteral("options")
    );
    (void) parser.addOption(loggingOption);

    static const QCommandLineOption fakeMobileOption(
        QStringLiteral("fake-mobile"),
        QStringLiteral("Run in fake-mobile mode")
    );
    (void) parser.addOption(fakeMobileOption);

    static const QCommandLineOption logOutputOption(
        QStringLiteral("log-output"),
        QStringLiteral("Redirect log output to console")
    );
    (void) parser.addOption(logOutputOption);

    // Simple‐boot‐test flag (release mode)
    static const QCommandLineOption simpleBootOption(
        QStringLiteral("simple-boot-test"),
        QStringLiteral("Perform only a simple boot test")
    );
    (void) parser.addOption(simpleBootOption);

    // Parse arguments
    parser.process(*QCoreApplication::instance());

    // Check for unknown or positional arguments
    static const QStringList unknown = parser.unknownOptionNames();
    if (!unknown.isEmpty()) {
        result.statusCode  = CommandLineParseResult::Status::Error;
        result.errorString = QStringLiteral("Unknown options: %1").arg(unknown.join(", "));
        return;
    }

    static const QStringList positional = parser.positionalArguments();
    if (!positional.isEmpty()) {
        result.statusCode  = CommandLineParseResult::Status::Error;
        result.errorString = QStringLiteral("Unexpected positional arguments: %1").arg(positional.join(", "));
        return;
    }

    // Help / version
    if (parser.isSet(helpOption)) {
        result.statusCode = CommandLineParseResult::Status::HelpRequested;
        return;
    }
    if (parser.isSet(versionOption)) {
        result.statusCode = CommandLineParseResult::Status::VersionRequested;
        return;
    }

    // Populate result flags
    if (parser.isSet(runUnitTestsOption)) {
        result.runningUnitTests     = true;
        result.unitTests            = parser.values(runUnitTestsOption);
        result.clearSettingsOptions = true;
    }
    if (parser.isSet(stressUnitTestsOption)) {
        result.runningUnitTests       = true;
        result.stressUnitTests        = true;
        bool ok = false;
        const QString val = parser.value(stressUnitTestsOption);
        result.stressUnitTestsCount = val.toUInt(&ok);
        if (!ok) {
            result.statusCode  = CommandLineParseResult::Status::Error;
            result.errorString = QStringLiteral("Invalid stress unit test count: %1").arg(val);
            return;
        }
    }
    if (parser.isSet(clearSettingsOption)) {
        result.clearSettingsOptions = true;
    }
    if (parser.isSet(clearCacheOption)) {
        result.clearCache = true;
    }
    if (parser.isSet(loggingOption)) {
        result.logging        = true;
        result.loggingOptions = parser.value(loggingOption);
    }
    if (parser.isSet(fakeMobileOption)) {
        result.fakeMobile = true;
    }
    if (parser.isSet(logOutputOption)) {
        result.logOutput = true;
    }
    if (parser.isSet(simpleBootOption)) {
       result.simpleBootTest = true;
    }

#ifdef Q_OS_WIN
    if (parser.isSet(quietWindowsAssertsOption)) {
        result.quietWindowsAsserts = true;
    }
#endif

    // Everything OK
    result.statusCode = CommandLineParseResult::Status::Ok;
}

} // namespace QGCCommandLineParser
