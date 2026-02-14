#include "InternalTransitionTest.h"
#include "TransitionTestCommon.h"


void InternalTransitionTest::_testInternalTransition()
{
    QStateMachine machine;
    int actionCount = 0;
    int entryCount = 0;
    int exitCount = 0;

    auto* state = new QGCState(QStringLiteral("TestState"), &machine);
    state->setOnEntry([&entryCount]() { entryCount++; });
    state->setOnExit([&exitCount]() { exitCount++; });

    QObject signalSource;
    auto* internalTransition = new InternalTransition(
        &signalSource, &QObject::objectNameChanged,
        [&actionCount]() { actionCount++; }
    );
    state->addTransition(internalTransition);

    auto* finalState = new QFinalState(&machine);
    // Add a way to exit after testing internal transitions
    QTimer exitTimer;
    exitTimer.setSingleShot(true);
    state->addTransition(&exitTimer, &QTimer::timeout, finalState);

    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QState::entered);
    machine.start();
    QVERIFY(enteredSpy.wait(500));

    // Trigger internal transitions - should NOT cause exit/re-entry
    signalSource.setObjectName(QStringLiteral("first"));
    signalSource.setObjectName(QStringLiteral("second"));
    signalSource.setObjectName(QStringLiteral("third"));

    QTest::qWait(50);

    QCOMPARE(actionCount, 3);
    QCOMPARE(entryCount, 1);  // Only entered once
    QCOMPARE(exitCount, 0);   // Never exited during internal transitions

    exitTimer.start(50);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(finishedSpy.wait(500));

    QCOMPARE(exitCount, 1);  // Now exited
}

UT_REGISTER_TEST(InternalTransitionTest, TestLabel::Unit, TestLabel::Utilities)
