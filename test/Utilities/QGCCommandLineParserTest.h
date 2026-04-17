#pragma once

#include "UnitTest.h"

/// Unit tests for the pure-function layer of QGCCommandLineParser.
///
/// The actual parse() path reaches into QCoreApplication::arguments(), which
/// is process-global and populated by the test runner itself, so this suite
/// focuses on what we *can* exercise safely inside a running QCoreApplication:
///   - CommandLineParseResult default-initialisation (ABI guard)
///   - determineAppMode() — pure struct→enum mapping with #ifdef branches
///   - handleParseResult() — returns early-exit codes for Help/Version, nullopt for Ok
///
/// Error-path handling is not covered because handleParseResult() calls
/// QCommandLineParser::showMessageAndExit() on Status::Error which terminates
/// the process.
class QGCCommandLineParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDefaultResult();
    void _testDetermineAppMode_Gui();
    void _testDetermineAppMode_BootTest();
    void _testDetermineAppMode_Test();
    void _testDetermineAppMode_ListTests();
    void _testHandleParseResult_OkReturnsNullopt();
    void _testHandleParseResult_HelpReturnsZero();
    void _testHandleParseResult_VersionReturnsZero();
};
