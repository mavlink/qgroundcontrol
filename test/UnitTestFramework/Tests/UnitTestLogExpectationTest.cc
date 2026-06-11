#include "UnitTestLogExpectationTest.h"

#include <QtCore/QRegularExpression>

void UnitTestLogExpectationTest::_testExpectVerifySingleMessage()
{
    expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("single expected log")));

    qCWarning(UnitTestLog) << "single expected log";

    verifyExpectedLogMessage();
}

void UnitTestLogExpectationTest::_testExpectVerifyCategoryScopedMessage()
{
    expectLogMessage("Test.UnitTest",
                     QtWarningMsg,
                     QRegularExpression(QStringLiteral("category scoped expected log")));

    qCWarning(UnitTestLog) << "category scoped expected log";

    verifyExpectedLogMessage();
}

void UnitTestLogExpectationTest::_testExpectVerifyMultipleMessagesInOrder()
{
    expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("first expected log")));
    expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("second expected log")));

    qCWarning(UnitTestLog) << "first expected log";
    qCWarning(UnitTestLog) << "second expected log";

    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void UnitTestLogExpectationTest::_testExpectVerifyDuplicatePatternMessages()
{
    expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("duplicate expected log")));
    expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("duplicate expected log")));

    qCWarning(UnitTestLog) << "duplicate expected log";
    qCWarning(UnitTestLog) << "duplicate expected log";

    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void UnitTestLogExpectationTest::_testIgnoreLogMessagePersistentSuppression()
{
    // ignoreLogMessage is intentionally non-consuming: same pattern may recur.
    ignoreLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("ignore expected log.*")));

    qCWarning(UnitTestLog) << "ignore expected log 1";
    qCWarning(UnitTestLog) << "ignore expected log 2";
}

UT_REGISTER_TEST(UnitTestLogExpectationTest, TestLabel::Unit)
