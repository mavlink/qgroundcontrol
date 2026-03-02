#include "ParallelStateTest.h"
#include "StateTestCommon.h"


void ParallelStateTest::_testParallelState()
{
    QStateMachine machine;
    bool region1Executed = false;
    bool region2Executed = false;

    auto* parallelState = new ParallelState(QStringLiteral("Parallel"), &machine);

    // Create two parallel regions, each with its own internal state machine
    // Region 1: QState with initial + final
    auto* region1 = new QState(parallelState);
    auto* region1Initial = new QState(region1);
    auto* region1Final = new QFinalState(region1);
    region1->setInitialState(region1Initial);
    region1Initial->addTransition(region1Initial, &QState::entered, region1Final);
    connect(region1Initial, &QState::entered, this, [&region1Executed]() { region1Executed = true; });

    // Region 2: QState with initial + final
    auto* region2 = new QState(parallelState);
    auto* region2Initial = new QState(region2);
    auto* region2Final = new QFinalState(region2);
    region2->setInitialState(region2Initial);
    region2Initial->addTransition(region2Initial, &QState::entered, region2Final);
    connect(region2Initial, &QState::entered, this, [&region2Executed]() { region2Executed = true; });

    auto* finalState = new QFinalState(&machine);
    parallelState->addTransition(parallelState, &ParallelState::allComplete, finalState);

    machine.setInitialState(parallelState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(region1Executed);
    QVERIFY(region2Executed);
}

void ParallelStateTest::_testParallelStateEmpty()
{
    QStateMachine machine;

    auto* parallelState = new ParallelState(QStringLiteral("EmptyParallel"), &machine);
    auto* finalState = new QFinalState(&machine);

    parallelState->addTransition(parallelState, &ParallelState::allComplete, finalState);
    machine.setInitialState(parallelState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    // Empty parallel state should complete immediately
    QVERIFY(finishedSpy.wait(500));
}

UT_REGISTER_TEST(ParallelStateTest, TestLabel::Unit, TestLabel::Utilities)
