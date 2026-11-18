#include "UnitTestLogExpectationTest.h"

#include <QtCore/QRegularExpression>

#include "MultiSignalSpy.h"

void UnitTestLogExpectationTest::_testExpectVerifySingleMessage()
{
    expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("single expected log")));

    qCWarning(UnitTestLog) << "single expected log";

    verifyExpectedLogMessage();
}

void UnitTestLogExpectationTest::_testExpectVerifyCategoryScopedMessage()
{
    const QRegularExpression pattern(QStringLiteral("category scoped expected log"));
    expectLogMessage("Test.UnitTest", QtWarningMsg, pattern);
    ignoreLogMessage("Test.MultiSignalSpy", QtWarningMsg, pattern);

    // The same text in another category, logged first: taken for the expected message, it would leave that
    // one as a message nothing expects
    qCWarning(MultiSignalSpyLog) << "category scoped expected log";
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
