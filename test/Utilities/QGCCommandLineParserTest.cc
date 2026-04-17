#include "QGCCommandLineParserTest.h"

#include "QGCCommandLineParser.h"

using QGCCommandLineParser::AppMode;
using QGCCommandLineParser::CommandLineParseResult;

void QGCCommandLineParserTest::_testDefaultResult()
{
    // The result struct is carried across the whole boot sequence, so a
    // drift in defaults would silently change app behaviour. Lock them down.
    CommandLineParseResult result;

    QCOMPARE(result.statusCode,          CommandLineParseResult::Status::Ok);
    QVERIFY(!result.systemId.has_value());
    QVERIFY(!result.errorString.has_value());
    QVERIFY(result.helpText.isEmpty());
    QVERIFY(result.versionText.isEmpty());
    QVERIFY(result.positional.isEmpty());
    QVERIFY(result.unknownOptions.isEmpty());

    QCOMPARE(result.clearSettingsOptions, false);
    QCOMPARE(result.clearCache,           false);
    QVERIFY(!result.loggingOptions.has_value());
    QCOMPARE(result.logOutput,            false);
    QCOMPARE(result.simpleBootTest,       false);

    QCOMPARE(result.runningUnitTests,     false);
    QVERIFY(result.unitTests.isEmpty());
    QCOMPARE(result.stressUnitTests,      false);
    QCOMPARE(result.stressUnitTestsCount, 0u);
    QVERIFY(!result.unitTestOutput.has_value());
    QVERIFY(!result.labelFilter.has_value());
    QCOMPARE(result.listTests,            false);

    QCOMPARE(result.fakeMobile,           false);
    QCOMPARE(result.allowMultiple,        false);

    QCOMPARE(result.useDesktopGL,         false);
    QCOMPARE(result.useSwRast,            false);
    QCOMPARE(result.quietWindowsAsserts,  false);
}

void QGCCommandLineParserTest::_testDetermineAppMode_Gui()
{
    // All flags default → GUI mode.
    CommandLineParseResult result;
    QCOMPARE(QGCCommandLineParser::determineAppMode(result), AppMode::Gui);
}

void QGCCommandLineParserTest::_testDetermineAppMode_BootTest()
{
    CommandLineParseResult result;
    result.simpleBootTest = true;
    QCOMPARE(QGCCommandLineParser::determineAppMode(result), AppMode::BootTest);
}

void QGCCommandLineParserTest::_testDetermineAppMode_Test()
{
#ifdef QGC_UNITTEST_BUILD
    // In the unit-test build both Test and ListTests short-circuit ahead of
    // simpleBootTest, so set simpleBootTest too to prove precedence.
    CommandLineParseResult result;
    result.runningUnitTests = true;
    result.simpleBootTest   = true;
    QCOMPARE(QGCCommandLineParser::determineAppMode(result), AppMode::Test);
#else
    QSKIP("Test mode only exists in QGC_UNITTEST_BUILD");
#endif
}

void QGCCommandLineParserTest::_testDetermineAppMode_ListTests()
{
#ifdef QGC_UNITTEST_BUILD
    // listTests must win over both runningUnitTests and simpleBootTest.
    CommandLineParseResult result;
    result.listTests        = true;
    result.runningUnitTests = true;
    result.simpleBootTest   = true;
    QCOMPARE(QGCCommandLineParser::determineAppMode(result), AppMode::ListTests);
#else
    QSKIP("ListTests mode only exists in QGC_UNITTEST_BUILD");
#endif
}

void QGCCommandLineParserTest::_testHandleParseResult_OkReturnsNullopt()
{
    CommandLineParseResult result;
    result.statusCode = CommandLineParseResult::Status::Ok;

    const std::optional<int> exitCode = QGCCommandLineParser::handleParseResult(result);
    QVERIFY(!exitCode.has_value());
}

void QGCCommandLineParserTest::_testHandleParseResult_HelpReturnsZero()
{
    CommandLineParseResult result;
    result.statusCode = CommandLineParseResult::Status::HelpRequested;

    const std::optional<int> exitCode = QGCCommandLineParser::handleParseResult(result);
    QVERIFY(exitCode.has_value());
    QCOMPARE(exitCode.value(), 0);
}

void QGCCommandLineParserTest::_testHandleParseResult_VersionReturnsZero()
{
    CommandLineParseResult result;
    result.statusCode = CommandLineParseResult::Status::VersionRequested;

    const std::optional<int> exitCode = QGCCommandLineParser::handleParseResult(result);
    QVERIFY(exitCode.has_value());
    QCOMPARE(exitCode.value(), 0);
}

UT_REGISTER_TEST(QGCCommandLineParserTest, TestLabel::Unit, TestLabel::Utilities)
