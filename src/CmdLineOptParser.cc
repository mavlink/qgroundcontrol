#include "CmdLineOptParser.h"

CommandLineParseResult parseCommandLine(QCommandLineParser &parser)
{
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    const QCommandLineOption versionOption = parser.addVersionOption();
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption runUnitTestsOption("unittest", "Run unit tests.");
    parser.addOption(runUnitTestsOption);
    const QCommandLineOption stressUnitTestsOption("unittest-stress", "Stress test unit tests.");
    parser.addOption(stressUnitTestsOption);
    const QCommandLineOption clearSettingsOption("clear-settings", "Clear stored settings.");
    parser.addOption(clearSettingsOption);
    const QCommandLineOption clearCacheOption("clear-cache", "Clear parameter/airframe caches.");
    parser.addOption(clearCacheOption);
    const QCommandLineOption loggingOption("logging", "Turn on logging.", "options");
    parser.addOption(loggingOption);
    const QCommandLineOption fakeMobileOption("fake-mobile", "Fake Mobile.");
    parser.addOption(fakeMobileOption);
    const QCommandLineOption logOutputOption("log-output", "Log Output.");
    parser.addOption(logOutputOption);
    const QCommandLineOption quietWindowsAssertsOption("no-windows-assert-ui", "Don't let asserts pop dialog boxes.");
    parser.addOption(quietWindowsAssertsOption);

    CommandLineParseResult result;

    using Status = CommandLineParseResult::Status;

    if (!parser.parse(QCoreApplication::arguments())) {
        result.statusCode = Status::Error;
        result.errorString = parser.errorText();
        return result;
    }

    if (parser.isSet(versionOption)) {
        result.statusCode = Status::VersionRequested;
        return result;
    }

    if (parser.isSet(helpOption)) {
        result.statusCode = Status::HelpRequested;
        return result;
    }

    result.runningUnitTests = parser.isSet(runUnitTestsOption);
    // result.unitTests = "";
    result.stressUnitTests = parser.isSet(stressUnitTestsOption);
    result.clearSettingsOptions = parser.isSet(clearSettingsOption);
    result.clearCache = parser.isSet(clearCacheOption);
    result.logging = parser.isSet(loggingOption);
    result.loggingOptions = parser.value(loggingOption);
    result.fakeMobile = parser.isSet(fakeMobileOption);
    result.logOutput = parser.isSet(logOutputOption);
    result.quietWindowsAsserts = parser.isSet(quietWindowsAssertsOption);
    #ifdef Q_OS_WIN
    #endif

    result.statusCode = Status::Ok;
    return result;
}
