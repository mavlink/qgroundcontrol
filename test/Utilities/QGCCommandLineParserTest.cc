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
    QCOMPARE(result.unitTestShardIndex, 0u);
    QCOMPARE(result.unitTestShardCount, 0u);
    QVERIFY(!result.labelFilter.has_value());
    QCOMPARE(result.listTests,            false);

    QCOMPARE(result.fakeMobile,           false);
    QCOMPARE(result.allowMultiple,        false);

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

void QGCCommandLineParserTest::_testNormalizeArgs_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QStringList>("expected");

    const QString windowsPath = QStringLiteral("C:\\results.xml");

    QTest::newRow("unittest-space-separated-value")
        << QStringList{QStringLiteral("qgc"), QStringLiteral("--unittest"), QStringLiteral("FooTest")}
        << QStringList{QStringLiteral("qgc"), QStringLiteral("--unittest"), QStringLiteral("FooTest")};
    QTest::newRow("unittest-colon-value") << QStringList{QStringLiteral("--unittest:FooTest")}
                                          << QStringList{QStringLiteral("--unittest"), QStringLiteral("FooTest")};
    QTest::newRow("unittest-bare") << QStringList{QStringLiteral("--unittest")}
                                   << QStringList{QStringLiteral("--unittest"), QString()};
    QTest::newRow("unittest-bare-followed-by-option")
        << QStringList{QStringLiteral("--unittest"), QStringLiteral("--allow-multiple")}
        << QStringList{QStringLiteral("--unittest"), QString(), QStringLiteral("--allow-multiple")};
    QTest::newRow("unittest-colon-without-value")
        << QStringList{QStringLiteral("--unittest:")} << QStringList{QStringLiteral("--unittest"), QString()};
    QTest::newRow("colon-value") << QStringList{QStringLiteral("--logging:Vehicle.FTPManager")}
                                 << QStringList{QStringLiteral("--logging"), QStringLiteral("Vehicle.FTPManager")};

    // In the --option:value form the first colon ends the option name; later colons and '=' belong to the value.
    QTest::newRow("colon-value-with-colon") << QStringList{QStringLiteral("--unittest-output:") + windowsPath}
                                            << QStringList{QStringLiteral("--unittest-output"), windowsPath};
    QTest::newRow("colon-value-with-equals") << QStringList{QStringLiteral("--logging:a=b")}
                                             << QStringList{QStringLiteral("--logging"), QStringLiteral("a=b")};

    // --option=value is passed on unchanged whatever the value contains.
    QTest::newRow("equals-value") << QStringList{QStringLiteral("--system-id=7")}
                                  << QStringList{QStringLiteral("--system-id=7")};
    QTest::newRow("equals-windows-path") << QStringList{QStringLiteral("--unittest-output=") + windowsPath}
                                         << QStringList{QStringLiteral("--unittest-output=") + windowsPath};
    QTest::newRow("equals-value-with-colon")
        << QStringList{QStringLiteral("--logging=a:b")} << QStringList{QStringLiteral("--logging=a:b")};
    QTest::newRow("equals-url-value") << QStringList{QStringLiteral("--opt=udp://host:14550")}
                                      << QStringList{QStringLiteral("--opt=udp://host:14550")};

    QTest::newRow("separate-value-with-colon") << QStringList{QStringLiteral("--unittest-output"), windowsPath}
                                               << QStringList{QStringLiteral("--unittest-output"), windowsPath};
}

void QGCCommandLineParserTest::_testNormalizeArgs()
{
    QFETCH(QStringList, args);
    QFETCH(QStringList, expected);

    QCOMPARE(QGCCommandLineParser::normalizeArgs(args), expected);
}

void QGCCommandLineParserTest::_testParseSystemId_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<quint32>("expected");

    QTest::newRow("minimum") << QStringLiteral("1") << true << quint32(1);
    QTest::newRow("legacy-maximum") << QStringLiteral("255") << true << quint32(255);
    QTest::newRow("first-wide") << QStringLiteral("256") << true << quint32(256);
    QTest::newRow("above-int-max") << QStringLiteral("2147483648") << true << quint32(0x80000000U);
    QTest::newRow("maximum") << QStringLiteral("4294967295") << true << quint32(0xffffffffU);
    QTest::newRow("zero") << QStringLiteral("0") << false << quint32(0);
    QTest::newRow("overflow") << QStringLiteral("4294967296") << false << quint32(0);
    QTest::newRow("negative") << QStringLiteral("-1") << false << quint32(0);
    QTest::newRow("not-a-number") << QStringLiteral("abc") << false << quint32(0);
    QTest::newRow("empty") << QString() << false << quint32(0);
}

void QGCCommandLineParserTest::_testParseSystemId()
{
    QFETCH(QString, value);
    QFETCH(bool, valid);
    QFETCH(quint32, expected);

    const std::optional<quint32> systemId = QGCCommandLineParser::parseSystemId(value);
    QCOMPARE(systemId.has_value(), valid);
    if (valid) {
        QCOMPARE(*systemId, expected);
    }
}

void QGCCommandLineParserTest::_testParseUnitTestShard_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<uint>("index");
    QTest::addColumn<uint>("count");

    QTest::newRow("first") << QStringLiteral("1/3") << true << 1u << 3u;
    QTest::newRow("last") << QStringLiteral("3/3") << true << 3u << 3u;
    QTest::newRow("single") << QStringLiteral("1/1") << true << 1u << 1u;
    QTest::newRow("zero-index") << QStringLiteral("0/3") << false << 0u << 0u;
    QTest::newRow("index-past-count") << QStringLiteral("4/3") << false << 0u << 0u;
    QTest::newRow("missing-count") << QStringLiteral("1") << false << 0u << 0u;
    QTest::newRow("extra-part") << QStringLiteral("1/2/3") << false << 0u << 0u;
    QTest::newRow("negative") << QStringLiteral("-1/3") << false << 0u << 0u;
    QTest::newRow("not-a-number") << QStringLiteral("a/b") << false << 0u << 0u;
    QTest::newRow("empty") << QString() << false << 0u << 0u;
}

void QGCCommandLineParserTest::_testParseUnitTestShard()
{
    QFETCH(QString, value);
    QFETCH(bool, valid);
    QFETCH(uint, index);
    QFETCH(uint, count);

    const std::optional<std::pair<uint, uint>> shard = QGCCommandLineParser::parseUnitTestShard(value);
    QCOMPARE(shard.has_value(), valid);
    if (valid) {
        QCOMPARE(shard->first, index);
        QCOMPARE(shard->second, count);
    }
}

QGC_REGISTER_PORTABLE_TEST(QGCCommandLineParserTest, TestLabel::Unit, TestLabel::Utilities)
