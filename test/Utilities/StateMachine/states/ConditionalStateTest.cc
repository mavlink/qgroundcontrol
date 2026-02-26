#include "ConditionalStateTest.h"
#include "StateTestCommon.h"


void ConditionalStateTest::_testConditionalStateExecute()
{
    QStateMachine machine;
    bool actionExecuted = false;
    bool predicateCalled = false;

    auto* conditionalState = new ConditionalState(
        QStringLiteral("ConditionalExecute"),
        &machine,
        [&predicateCalled]() {
            predicateCalled = true;
            return true;
        },
        [&actionExecuted]() {
            actionExecuted = true;
        }
    );
    auto* finalState = new QFinalState(&machine);

    conditionalState->addTransition(conditionalState, &QGCState::advance, finalState);
    machine.setInitialState(conditionalState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(predicateCalled);
    QVERIFY(actionExecuted);
}

void ConditionalStateTest::_testConditionalStateSkip()
{
    QStateMachine machine;
    bool actionExecuted = false;
    bool predicateCalled = false;
    bool skipHandled = false;

    auto* conditionalState = new ConditionalState(
        QStringLiteral("ConditionalSkip"),
        &machine,
        [&predicateCalled]() {
            predicateCalled = true;
            return false;
        },
        [&actionExecuted]() {
            actionExecuted = true;
        }
    );
    auto* skipState = new FunctionState(QStringLiteral("SkipHandler"), &machine, [&skipHandled]() {
        skipHandled = true;
    });
    auto* finalState = new QFinalState(&machine);

    conditionalState->addTransition(conditionalState, &QGCState::advance, finalState);
    conditionalState->addTransition(conditionalState, &ConditionalState::skipped, skipState);
    skipState->addTransition(skipState, &QGCState::advance, finalState);
    machine.setInitialState(conditionalState);

    // Use MultiSignalSpy to verify skipped() fired but advance() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(conditionalState));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(predicateCalled);
    QVERIFY(!actionExecuted);
    QVERIFY(skipHandled);
    // Verify skip path taken, not execute path
    QVERIFY(stateSpy.emittedByMask(stateSpy.mask("skipped")));
    QVERIFY(stateSpy.notEmittedByMask(stateSpy.mask("advance")));
}

UT_REGISTER_TEST(ConditionalStateTest, TestLabel::Unit, TestLabel::Utilities)
