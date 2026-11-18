#include "RetryTransitionTest.h"

#include "StateTestCommon.h"

namespace {
class RetryInspectableWaitState : public WaitStateBase
{
public:
    RetryInspectableWaitState(const QString& name, QState* parent, int timeoutMsecs)
        : WaitStateBase(name, parent, timeoutMsecs) {}

    bool waitSignalConnected = false;

protected:
    void connectWaitSignal() override { waitSignalConnected = true; }
    void disconnectWaitSignal() override { waitSignalConnected = false; }
};
}

void RetryTransitionTest::_testRetriesThenAdvances_data()
{
    QTest::addColumn<int>("maxRetries");

    QTest::newRow("no retry") << 0;
    QTest::newRow("one retry") << 1;
    QTest::newRow("two retries") << 2;
    QTest::newRow("three retries") << 3;
}

void RetryTransitionTest::_testRetriesThenAdvances()
{
    QFETCH(int, maxRetries);

    ignoreTimeoutWarnings();
    QStateMachine machine;
    int retryCount = 0;
    bool targetReached = false;
    bool alternateReached = false;

    // Never completes, so every wait ends in a timeout
    auto* startState = new AsyncFunctionState(QStringLiteral("Start"), &machine, [](AsyncFunctionState*) {}, 20);
    auto* targetState =
        new FunctionState(QStringLiteral("Target"), &machine, [&targetReached]() { targetReached = true; });
    auto* alternateState =
        new FunctionState(QStringLiteral("Alternate"), &machine, [&alternateReached]() { alternateReached = true; });
    auto* finalState = addFinalState(&machine);

    auto* retryTransition = new RetryTransition(
        startState, &WaitStateBase::timeout, targetState, [&retryCount]() { retryCount++; }, maxRetries);
    startState->addTransition(retryTransition);
    // Must not be taken: the start state is left through the retry transition
    startState->addTransition(startState, &QGCState::advance, alternateState);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    alternateState->addTransition(alternateState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    expectLogMessage("Utilities.StateMachine.RetryTransition", QtWarningMsg,
                     QRegularExpression(R"("Start" timeout after \d+ retries, advancing)"));
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QCOMPARE(retryCount, maxRetries);
    QVERIFY(targetReached);
    QVERIFY(!alternateReached);
    // The count starts again for the next time the source state is entered
    QCOMPARE(retryTransition->retryCount(), 0);
}

void RetryTransitionTest::_testWaitRearmedBeforeRetryAction()
{
    ignoreTimeoutWarnings();
    QStateMachine machine;
    bool waitWasRearmedBeforeRetryAction = false;

    auto* startState = new RetryInspectableWaitState(QStringLiteral("Start"), &machine, 20);
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, []() {});
    auto* finalState = addFinalState(&machine);

    auto* retryTransition = new RetryTransition(
        startState, &WaitStateBase::timeout,
        targetState,
        [&waitWasRearmedBeforeRetryAction, startState]() {
            waitWasRearmedBeforeRetryAction = startState->waitSignalConnected;
        },
        1
    );
    startState->addTransition(retryTransition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    expectLogMessage("Utilities.StateMachine.RetryTransition", QtWarningMsg, QRegularExpression(R"("Start" timeout after \d+ retries, advancing)"));
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QVERIFY(waitWasRearmedBeforeRetryAction);
}

UT_REGISTER_TEST(RetryTransitionTest, TestLabel::Unit, TestLabel::Utilities)
