#include "AsyncFunctionStateTest.h"
#include "StateTestCommon.h"


void AsyncFunctionStateTest::_testAsyncFunctionState()
{
    QStateMachine machine;
    bool setupCalled = false;
    AsyncFunctionState* capturedState = nullptr;

    auto* asyncState = new AsyncFunctionState(
        QStringLiteral("TestAsync"),
        &machine,
        [&setupCalled, &capturedState](AsyncFunctionState* state) {
            setupCalled = true;
            capturedState = state;
            QTimer::singleShot(50, state, [state]() {
                state->complete();
            });
        }
    );
    auto* finalState = new QFinalState(&machine);

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    machine.setInitialState(asyncState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(setupCalled);
    QVERIFY(capturedState != nullptr);
}

void AsyncFunctionStateTest::_testAsyncFunctionStateTimeout()
{
    QStateMachine machine;
    bool timeoutReached = false;
    const int timeoutMs = 100;

    auto* asyncState = new AsyncFunctionState(
        QStringLiteral("TestAsyncTimeout"),
        &machine,
        [](AsyncFunctionState* state) {
            Q_UNUSED(state);
            // Don't call complete() - let it timeout
        },
        timeoutMs
    );
    auto* timeoutState = new FunctionState(QStringLiteral("TimeoutHandler"), &machine, [&timeoutReached]() {
        timeoutReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    asyncState->addTransition(asyncState, &AsyncFunctionState::timeout, timeoutState);
    timeoutState->addTransition(timeoutState, &QGCState::advance, finalState);
    machine.setInitialState(asyncState);

    // Use MultiSignalSpyV2 to verify timeout() fired but advance() did not
    MultiSignalSpyV2 stateSpy;
    QVERIFY(stateSpy.init(asyncState));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(timeoutReached);
    // Verify timeout path taken, not success path
    QVERIFY(stateSpy.checkSignalByMask(stateSpy.signalNameToMask("timeout")));
    QVERIFY(stateSpy.checkNoSignalByMask(stateSpy.signalNameToMask("advance")));
}

void AsyncFunctionStateTest::_testErrorTransition()
{
    QStateMachine machine;
    bool errorHandled = false;

    auto* asyncState = new AsyncFunctionState(
        QStringLiteral("TestError"),
        &machine,
        [](AsyncFunctionState* state) {
            QTimer::singleShot(50, state, [state]() {
                state->fail();
            });
        }
    );
    auto* errorState = new FunctionState(QStringLiteral("ErrorHandler"), &machine, [&errorHandled]() {
        errorHandled = true;
    });
    auto* finalState = new QFinalState(&machine);

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    asyncState->addTransition(asyncState, &QGCState::error, errorState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);
    machine.setInitialState(asyncState);

    // Use MultiSignalSpyV2 to verify error() fired but advance() did not
    MultiSignalSpyV2 stateSpy;
    QVERIFY(stateSpy.init(asyncState));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(errorHandled);
    // Verify error path taken, not success path
    QVERIFY(stateSpy.checkSignalByMask(stateSpy.signalNameToMask("error")));
    QVERIFY(stateSpy.checkNoSignalByMask(stateSpy.signalNameToMask("advance")));
}
