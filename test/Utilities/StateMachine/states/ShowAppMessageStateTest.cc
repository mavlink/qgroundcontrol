#include "ShowAppMessageStateTest.h"
#include "StateTestCommon.h"

#include <QtCore/QRegularExpression>


void ShowAppMessageStateTest::_testShowAppMessageStateCreation()
{
    QStateMachine machine;

    auto* state = new ShowAppMessageState(&machine, QStringLiteral("Test message"));

    QVERIFY(state != nullptr);
    // ShowAppMessageState is a QGCState, so it should have advance/error signals
}

void ShowAppMessageStateTest::_testShowAppMessageStateAdvance()
{
    QStateMachine machine;

    auto* state = new ShowAppMessageState(&machine, QStringLiteral("Test message"));

    // ShowAppMessageState should emit advance() after showing the message
    expectAppMessage(QRegularExpression("Test message"));
    QVERIFY(runStateToCompletion(state, &machine));
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(ShowAppMessageStateTest, TestLabel::Unit, TestLabel::Utilities)
