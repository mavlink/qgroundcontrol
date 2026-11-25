#pragma once

#include "PortableTest.h"

/// Unit tests for QGCCommandLineParser.
///
/// parse() and parseCommandLine() read QCoreApplication::arguments(), which is
/// process-global and populated by the test runner itself, so this suite
/// covers what can be exercised inside a running QCoreApplication:
///   - CommandLineParseResult default-initialisation (ABI guard)
///   - parseArguments() — the effect of each option, the values, the options and
///     the empty argument list it rejects, and the status of a help or version request
///   - determineAppMode() — pure struct→enum mapping with #ifdef branches
///   - handleParseResult() — returns early-exit codes for Help/Version, nullopt for Ok
///   - parseSystemId() — --system-id range validation
///
/// Not covered, because they end the process: handleParseResult() on
/// Status::Error (QCommandLineParser::showMessageAndExit()) and on the help or
/// version request of a parsed command line (QCommandLineParser::showHelp(),
/// showVersion() and process()). Its Help/Version cases here have no parser.
/// The error for a unit-test option in a build without tests is not covered
/// either: this suite only exists in builds with them.
class QGCCommandLineParserTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _testDefaultResult();
    UT_PARAMETERIZED_TEST(_testParseArgumentsOptions);
    UT_PARAMETERIZED_TEST(_testParseArgumentsRejectedValues);
    void _testParseArgumentsEmptyList();
    UT_PARAMETERIZED_TEST(_testParseArgumentsHelpAndVersion);
    UT_PARAMETERIZED_TEST(_testDetermineAppMode);
    void _testHandleParseResult();
    UT_PARAMETERIZED_TEST(_testNormalizeArgs);
    UT_PARAMETERIZED_TEST(_testParseSystemId);
    UT_PARAMETERIZED_TEST(_testParseUnitTestShard);
};
