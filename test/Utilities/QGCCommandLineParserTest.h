#pragma once

#include "PortableTest.h"

/// Unit tests for QGCCommandLineParser.
///
/// parse() and parseCommandLine() read QCoreApplication::arguments(), which is
/// process-global and populated by the test runner itself, so this suite
/// covers what can be exercised inside a running QCoreApplication:
///   - CommandLineParseResult default-initialisation (ABI guard)
///   - parseArguments() — the effect of each option, and the values it rejects
///   - determineAppMode() — pure struct→enum mapping with #ifdef branches
///   - handleParseResult() — returns early-exit codes for Help/Version, nullopt for Ok
///   - parseSystemId() — --system-id range validation
///
/// Not covered, because they end the process: handleParseResult() on
/// Status::Error (QCommandLineParser::showMessageAndExit()), and
/// parseArguments() on an unknown option, an option without its value, --help
/// or --version (QCommandLineParser::process()).
class QGCCommandLineParserTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _testDefaultResult();
    void _testParseArgumentsOptions_data();
    void _testParseArgumentsOptions();
    void _testParseArgumentsRejectedValues_data();
    void _testParseArgumentsRejectedValues();
    void _testDetermineAppMode_Gui();
    void _testDetermineAppMode_BootTest();
    void _testDetermineAppMode_Test();
    void _testDetermineAppMode_ListTests();
    void _testHandleParseResult_OkReturnsNullopt();
    void _testHandleParseResult_HelpReturnsZero();
    void _testHandleParseResult_VersionReturnsZero();
    void _testNormalizeArgs_data();
    void _testNormalizeArgs();
    void _testParseSystemId_data();
    void _testParseSystemId();
    void _testParseUnitTestShard_data();
    void _testParseUnitTestShard();
};
