#include "AdditionalTransitionsCoverageTest.h"

#include <QtCore/QRegularExpression>

#include "StateTestCommon.h"

void AdditionalTransitionsCoverageTest::_testGuardedTransitionGuardDecides_data()
{
    QTest::addColumn<QString>("createdBy");
    QTest::addColumn<bool>("guardAllows");

    for (const char* createdBy : {"constructor", "addGuardedTransition"}) {
        QTest::addRow("%s, guard allows the transition", createdBy) << QString::fromLatin1(createdBy) << true;
        QTest::addRow("%s, guard blocks the transition", createdBy) << QString::fromLatin1(createdBy) << false;
    }
}

void AdditionalTransitionsCoverageTest::_testGuardedTransitionGuardDecides()
{
    QFETCH(QString, createdBy);
    QFETCH(bool, guardAllows);

    // A constructed transition runs in a plain QStateMachine; the factory is a member of QGCStateMachine
    QGCStateMachine qgcMachine(QStringLiteral("GuardedTransitionTest"), nullptr);
    QStateMachine plainMachine;
    QStateMachine& machine = (createdBy == QStringLiteral("constructor")) ? plainMachine : qgcMachine;
    bool guardCalled = false;
    int targetEntries = 0;
    int alternateEntries = 0;

    auto* startState = new FunctionState(QStringLiteral("Start"), &machine, []() {});
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&targetEntries]() { targetEntries++; });
    auto* alternateState =
        new FunctionState(QStringLiteral("Alternate"), &machine, [&alternateEntries]() { alternateEntries++; });
    auto* finalState = addFinalState(&machine);

    const GuardedTransition::Guard guard = [&guardCalled, guardAllows]() {
        guardCalled = true;
        return guardAllows;
    };
    GuardedTransition* guardedTransition = nullptr;
    // The second transition of the start state is taken when the guard blocks the first
    if (createdBy == QStringLiteral("addGuardedTransition")) {
        // On a signal that QState itself declares, where the constructor row takes one of a class derived from it
        guardedTransition = qgcMachine.addGuardedTransition(startState, &QState::entered, targetState, guard);
        startState->addTransition(startState, &QState::entered, alternateState);
    } else {
        guardedTransition = new GuardedTransition(startState, &QGCState::advance, targetState, guard);
        startState->addTransition(guardedTransition);
        startState->addTransition(startState, &QGCState::advance, alternateState);
    }
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    alternateState->addTransition(alternateState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QCOMPARE(guardedTransition->sourceState(), startState);
    QCOMPARE(guardedTransition->targetState(), targetState);

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(guardCalled);
    QCOMPARE(targetEntries, guardAllows ? 1 : 0);
    QCOMPARE(alternateEntries, guardAllows ? 0 : 1);
}

void AdditionalTransitionsCoverageTest::_testGuardedTransitionTargetSetLater()
{
    QStateMachine machine;
    bool guardCalled = false;

    auto* startState = new FunctionState(QStringLiteral("Start"), &machine, []() {});
    auto* finalState = addFinalState(&machine);

    auto* guardedTransition = new GuardedTransition(startState, &QGCState::advance, [&guardCalled]() {
        guardCalled = true;
        return true;
    });
    QCOMPARE(guardedTransition->targetState(), nullptr);
    guardedTransition->setTargetState(finalState);
    startState->addTransition(guardedTransition);
    machine.setInitialState(startState);

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(guardCalled);
}

void AdditionalTransitionsCoverageTest::_testRetryTransitionRetriesThenAdvances_data()
{
    QTest::addColumn<int>("maxRetries");
    QTest::addColumn<bool>("throughFactory");

    QTest::newRow("no retry") << 0 << false;
    QTest::newRow("one retry") << 1 << false;
    QTest::newRow("two retries") << 2 << false;
    QTest::newRow("three retries") << 3 << false;
    QTest::newRow("two retries, QGCStateMachine::addRetryTransition()") << 2 << true;
}

void AdditionalTransitionsCoverageTest::_testRetryTransitionRetriesThenAdvances()
{
    QFETCH(int, maxRetries);
    QFETCH(bool, throughFactory);

    ignoreTimeoutWarnings();
    // A constructed transition runs in a plain QStateMachine; the factory is a member of QGCStateMachine
    QGCStateMachine qgcMachine(QStringLiteral("RetryTest"), nullptr);
    QStateMachine plainMachine;
    QStateMachine& machine = throughFactory ? qgcMachine : plainMachine;
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

    const auto retryAction = [&retryCount]() { retryCount++; };
    RetryTransition* retryTransition = nullptr;
    if (throughFactory) {
        retryTransition =
            qgcMachine.addRetryTransition(startState, &WaitStateBase::timedOut, targetState, retryAction, maxRetries);
    } else {
        retryTransition =
            new RetryTransition(startState, &WaitStateBase::timedOut, targetState, retryAction, maxRetries);
        startState->addTransition(retryTransition);
    }
    // Must not be taken: the start state is left through the retry transition
    startState->addTransition(startState, &QGCState::advance, alternateState);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    alternateState->addTransition(alternateState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QVERIFY(retryTransition != nullptr);
    QCOMPARE(retryTransition->maxRetries(), maxRetries);

    // The warning names the state with its machine when that is a QGCStateMachine
    const QString stateName = throughFactory ? QStringLiteral("RetryTest:Start") : QStringLiteral("Start");
    expectLogMessage("Utilities.StateMachine.RetryTransition", QtWarningMsg,
                     QRegularExpression(
                         QStringLiteral("\"%1\" timeout after %2 retries, advancing").arg(stateName).arg(maxRetries)));
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QCOMPARE(retryCount, maxRetries);
    QVERIFY(targetReached);
    QVERIFY(!alternateReached);
    // The count starts again for the next time the source state is entered
    QCOMPARE(retryTransition->retryCount(), 0);
}

void AdditionalTransitionsCoverageTest::_testRetryTransitionWaitRearmedBeforeRetryAction()
{
    ignoreTimeoutWarnings();
    QStateMachine machine;
    bool waitWasRearmedBeforeRetryAction = false;

    auto* startState = new TestWaitState(QStringLiteral("Start"), &machine, 20);
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, []() {});
    auto* finalState = addFinalState(&machine);

    auto* retryTransition = new RetryTransition(
        startState, &WaitStateBase::timedOut, targetState,
        [&waitWasRearmedBeforeRetryAction, startState]() {
            waitWasRearmedBeforeRetryAction = startState->waitSignalConnected;
        },
        1);
    startState->addTransition(retryTransition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    expectLogMessage("Utilities.StateMachine.RetryTransition", QtWarningMsg,
                     QRegularExpression(R"("Start" timeout after \d+ retries, advancing)"));
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QVERIFY(waitWasRearmedBeforeRetryAction);
}

void AdditionalTransitionsCoverageTest::_testRetrySetsAsyncStateUpAgain_data()
{
    QTest::addColumn<bool>("skippable");

    QTest::newRow("AsyncFunctionState") << false;
    QTest::newRow("SkippableAsyncState") << true;
}

void AdditionalTransitionsCoverageTest::_testRetrySetsAsyncStateUpAgain()
{
    QFETCH(bool, skippable);

    ignoreTimeoutWarnings();
    QGCStateMachine machine(QStringLiteral("RetryAsyncTest"), nullptr);
    // Its advance() stands for the answer that the state waits for
    TestAbstractState replier(QStringLiteral("Replier"), nullptr);
    int setupCalls = 0;
    bool gaveUp = false;

    const auto setup = [&replier, &setupCalls](CompletionWaitState* state) {
        setupCalls++;
        state->connectToCompletion(&replier, &QGCAbstractState::advance);
    };
    CompletionWaitState* waitState = nullptr;
    if (skippable) {
        waitState =
            new SkippableAsyncState(QStringLiteral("Wait"), &machine, []() { return false; }, setup, nullptr, 20);
    } else {
        waitState = new AsyncFunctionState(QStringLiteral("Wait"), &machine, setup, 20);
    }
    auto* gaveUpState = new FunctionState(QStringLiteral("GaveUp"), &machine, [&gaveUp]() { gaveUp = true; });
    auto* finalState = machine.addFinalState();

    // The answer comes while the retry is made. The timeout has ended the connection of the first attempt, so the
    // state hears it only if the retry has set it up again.
    machine.addRetryTransition(
        waitState, &WaitStateBase::timedOut, gaveUpState, [&replier]() { replier.emitAdvance(); }, 1);
    waitState->addTransition(waitState, &QGCState::advance, finalState);
    gaveUpState->addTransition(gaveUpState, &QGCState::advance, finalState);
    machine.setInitialState(waitState);

    QSignalSpy advanceSpy(waitState, &QGCState::advance);
    QVERIFY(startAndWaitForFinished(&machine));
    QCOMPARE(setupCalls, 2);
    QCOMPARE(advanceSpy.count(), 1);
    QVERIFY(!gaveUp);
}

void AdditionalTransitionsCoverageTest::_testRetryTransitionCountStartsAgainOnEntry()
{
    ignoreTimeoutWarnings();
    QGCStateMachine machine(QStringLiteral("RetryReentryTest"), nullptr);
    int retryActionCalls = 0;

    // Never completes, so every wait ends in a timeout
    auto* waitState = new AsyncFunctionState(QStringLiteral("Wait"), &machine, [](AsyncFunctionState*) {}, 20);
    auto* detourState = new FunctionState(QStringLiteral("Detour"), &machine, []() {});
    machine.addRetryTransition(
        waitState, &WaitStateBase::timedOut, machine.addFinalState(),
        [&machine, &retryActionCalls]() {
            // After its first retry the state is left through another transition, and entered again
            if (++retryActionCalls == 1) {
                machine.postEvent(QStringLiteral("detour"));
            }
        },
        1);
    machine.addEventTransition(waitState, QStringLiteral("detour"), detourState);
    detourState->addTransition(detourState, &QGCState::advance, waitState);
    machine.setInitialState(waitState);

    QSignalSpy waitEnteredSpy(waitState, &QState::entered);
    expectLogMessage("Utilities.StateMachine.RetryTransition", QtWarningMsg,
                     QRegularExpression("timeout after 1 retries, advancing"));
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QCOMPARE(waitEnteredSpy.count(), 2);
    // The state has its one retry each time it is entered
    QCOMPARE(retryActionCalls, 2);
}

void AdditionalTransitionsCoverageTest::_testRetryTransitionKeepsSignalWhileRetrying_data()
{
    QTest::addColumn<bool>("otherOnParentState");

    QTest::newRow("another transition of the state") << false;
    QTest::newRow("a transition of the parent state") << true;
}

void AdditionalTransitionsCoverageTest::_testRetryTransitionKeepsSignalWhileRetrying()
{
    QFETCH(bool, otherOnParentState);

    ignoreTimeoutWarnings();
    QGCStateMachine machine(QStringLiteral("RetrySignalTest"), nullptr);
    int retries = 0;
    bool otherReached = false;

    auto* parentState = new QState(&machine);
    // Never completes, so every wait ends in a timeout
    auto* waitState = new AsyncFunctionState(QStringLiteral("Wait"), parentState, [](AsyncFunctionState*) {}, 20);
    parentState->setInitialState(waitState);
    auto* otherState = new FunctionState(QStringLiteral("Other"), &machine, [&otherReached]() { otherReached = true; });
    auto* finalState = machine.addFinalState();

    machine.addRetryTransition(waitState, &WaitStateBase::timedOut, finalState, [&retries]() { retries++; }, 2);
    // For the same signal and after the retry transition: Qt asks it only if the retry transition is not taken
    QState* const otherSource = otherOnParentState ? parentState : waitState;
    otherSource->addTransition(waitState, &WaitStateBase::timedOut, otherState);
    otherState->addTransition(otherState, &QGCState::advance, finalState);
    machine.setInitialState(parentState);

    expectLogMessage("Utilities.StateMachine.RetryTransition", QtWarningMsg,
                     QRegularExpression("timeout after 2 retries, advancing"));
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QCOMPARE(retries, 2);
    QVERIFY(!otherReached);
}

UT_REGISTER_TEST(AdditionalTransitionsCoverageTest, TestLabel::Unit, TestLabel::Utilities)
