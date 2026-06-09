#include "QGCFinalStateTest.h"
#include "StateTestCommon.h"


void QGCFinalStateTest::_testQGCFinalState()
{
    QGCStateMachine machine(QStringLiteral("FinalStateTest"), nullptr);

    auto* startState = new FunctionState(QStringLiteral("Start"), &machine, []() {});
    auto* finalState = new QGCFinalState(QStringLiteral("MyFinal"), &machine);

    startState->addTransition(startState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QCOMPARE(finalState->objectName(), QStringLiteral("MyFinal"));

    QVERIFY(startAndWaitForFinished(&machine));
}

UT_REGISTER_TEST(QGCFinalStateTest, TestLabel::Unit, TestLabel::Utilities)
