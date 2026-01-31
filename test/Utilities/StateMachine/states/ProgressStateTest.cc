#include "ProgressStateTest.h"
#include "StateTestCommon.h"

#include "QGCStateMachine.h"

#include <QtTest/QSignalSpy>

void ProgressStateTest::_testFixedProgress()
{
    QStateMachine machine;
    float receivedProgress = -1.0f;

    auto* progressState = new ProgressState(QStringLiteral("Progress"), &machine, 0.5f);
    auto* finalState = new QFinalState(&machine);

    connect(progressState, &ProgressState::progressChanged, this, [&receivedProgress](float p) {
        receivedProgress = p;
    });

    progressState->addTransition(progressState, &QGCState::advance, finalState);
    machine.setInitialState(progressState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(receivedProgress, 0.5f);
    QCOMPARE(progressState->progress(), 0.5f);
}

void ProgressStateTest::_testProgressCallback()
{
    QStateMachine machine;
    float receivedProgress = -1.0f;
    int callbackCount = 0;

    auto* progressState = new ProgressState(QStringLiteral("Progress"), &machine, [&callbackCount]() {
        callbackCount++;
        return 0.75f;
    });
    auto* finalState = new QFinalState(&machine);

    connect(progressState, &ProgressState::progressChanged, this, [&receivedProgress](float p) {
        receivedProgress = p;
    });

    progressState->addTransition(progressState, &QGCState::advance, finalState);
    machine.setInitialState(progressState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(receivedProgress, 0.75f);
    QVERIFY(callbackCount >= 1);  // Callback was invoked
}

void ProgressStateTest::_testProgressClamping()
{
    QStateMachine machine;
    QList<float> receivedProgress;

    // Test values outside 0-1 range
    auto* state1 = new ProgressState(QStringLiteral("TooLow"), &machine, -0.5f);
    auto* state2 = new ProgressState(QStringLiteral("TooHigh"), &machine, 1.5f);
    auto* state3 = new ProgressState(QStringLiteral("Normal"), &machine, 0.5f);
    auto* finalState = new QFinalState(&machine);

    auto collectProgress = [&receivedProgress](float p) {
        receivedProgress.append(p);
    };

    connect(state1, &ProgressState::progressChanged, this, collectProgress);
    connect(state2, &ProgressState::progressChanged, this, collectProgress);
    connect(state3, &ProgressState::progressChanged, this, collectProgress);

    state1->addTransition(state1, &QGCState::advance, state2);
    state2->addTransition(state2, &QGCState::advance, state3);
    state3->addTransition(state3, &QGCState::advance, finalState);
    machine.setInitialState(state1);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(receivedProgress.size(), 3);
    QCOMPARE(receivedProgress[0], 0.0f);  // Clamped from -0.5
    QCOMPARE(receivedProgress[1], 1.0f);  // Clamped from 1.5
    QCOMPARE(receivedProgress[2], 0.5f);  // Normal
}

void ProgressStateTest::_testProgressSequence()
{
    QStateMachine machine;
    QList<float> progressValues;

    auto* state1 = new ProgressState(QStringLiteral("Step1"), &machine, 0.0f);
    auto* state2 = new ProgressState(QStringLiteral("Step2"), &machine, 0.33f);
    auto* state3 = new ProgressState(QStringLiteral("Step3"), &machine, 0.66f);
    auto* state4 = new ProgressState(QStringLiteral("Step4"), &machine, 1.0f);
    auto* finalState = new QFinalState(&machine);

    auto collectProgress = [&progressValues](float p) {
        progressValues.append(p);
    };

    connect(state1, &ProgressState::progressChanged, this, collectProgress);
    connect(state2, &ProgressState::progressChanged, this, collectProgress);
    connect(state3, &ProgressState::progressChanged, this, collectProgress);
    connect(state4, &ProgressState::progressChanged, this, collectProgress);

    state1->addTransition(state1, &QGCState::advance, state2);
    state2->addTransition(state2, &QGCState::advance, state3);
    state3->addTransition(state3, &QGCState::advance, state4);
    state4->addTransition(state4, &QGCState::advance, finalState);
    machine.setInitialState(state1);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(progressValues.size(), 4);

    // Verify progress increases
    for (int i = 1; i < progressValues.size(); i++) {
        QVERIFY(progressValues[i] >= progressValues[i-1]);
    }
}

void ProgressStateTest::_testActionExecuted()
{
    QStateMachine machine;
    bool actionCalled = false;
    float receivedProgress = -1.0f;

    auto* progressState = new ProgressState(QStringLiteral("Progress"), &machine, 0.5f, [&actionCalled]() {
        actionCalled = true;
    });
    auto* finalState = new QFinalState(&machine);

    connect(progressState, &ProgressState::progressChanged, this, [&receivedProgress](float p) {
        receivedProgress = p;
    });

    progressState->addTransition(progressState, &QGCState::advance, finalState);
    machine.setInitialState(progressState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(actionCalled);
    QCOMPARE(receivedProgress, 0.5f);
}
