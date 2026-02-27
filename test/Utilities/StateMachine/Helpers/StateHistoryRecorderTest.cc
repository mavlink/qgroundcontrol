#include "StateHistoryRecorderTest.h"

#include "QGCStateMachine.h"
#include "StateHistoryRecorder.h"

#include <QtTest/QSignalSpy>

namespace {
bool _spyTriggered(QSignalSpy& spy, int timeoutMsecs)
{
    return (spy.count() > 0) || spy.wait(timeoutMsecs);
}
}

void StateHistoryRecorderTest::_testManualEntriesRequireEnabledAndTrim()
{
    QGCStateMachine machine(QStringLiteral("RecorderManualTrim"), nullptr);
    StateHistoryRecorder recorder(&machine, 2);

    recorder.addEntry(QStringLiteral("A"), StateHistoryRecorder::Entered);
    QCOMPARE(recorder.count(), 0);

    recorder.setEnabled(true);
    recorder.addEntry(QStringLiteral("A"), StateHistoryRecorder::Entered);
    recorder.addEntry(QStringLiteral("B"), StateHistoryRecorder::Entered);
    recorder.addEntry(QStringLiteral("C"), StateHistoryRecorder::Entered);

    QCOMPARE(recorder.count(), 2);
    const auto history = recorder.history();
    QCOMPARE(history.at(0).stateName, QStringLiteral("B"));
    QCOMPARE(history.at(1).stateName, QStringLiteral("C"));

    recorder.setEnabled(false);
    recorder.addEntry(QStringLiteral("D"), StateHistoryRecorder::Entered);
    QCOMPARE(recorder.count(), 2);
}

void StateHistoryRecorderTest::_testLastEntriesAndEntriesForState()
{
    QGCStateMachine machine(QStringLiteral("RecorderEntries"), nullptr);
    StateHistoryRecorder recorder(&machine, 10);
    recorder.setEnabled(true);

    recorder.addEntry(QStringLiteral("A"), StateHistoryRecorder::Entered, QStringLiteral("start"));
    recorder.addEntry(QStringLiteral("B"), StateHistoryRecorder::Signal, QStringLiteral("sig"));
    recorder.addEntry(QStringLiteral("A"), StateHistoryRecorder::Exited, QStringLiteral("done"));
    recorder.addEntry(QStringLiteral("C"), StateHistoryRecorder::Error, QStringLiteral("failure"));

    QCOMPARE(recorder.count(), 4);

    const auto lastTwo = recorder.lastEntries(2);
    QCOMPARE(lastTwo.size(), 2);
    QCOMPARE(lastTwo.at(0).stateName, QStringLiteral("A"));
    QCOMPARE(lastTwo.at(1).stateName, QStringLiteral("C"));

    const auto stateAEntries = recorder.entriesForState(QStringLiteral("A"));
    QCOMPARE(stateAEntries.size(), 2);
    QCOMPARE(stateAEntries.at(0).reason, StateHistoryRecorder::Entered);
    QCOMPARE(stateAEntries.at(1).reason, StateHistoryRecorder::Exited);

    QCOMPARE(recorder.toJson().size(), 4);
    QVERIFY(!recorder.dumpHistory().isEmpty());
}

void StateHistoryRecorderTest::_testStateEntryExitRecording()
{
    QGCStateMachine machine(QStringLiteral("RecorderStateSignals"), nullptr);

    auto* state1 = new FunctionState(QStringLiteral("State1"), &machine, []() {});
    auto* state2 = new FunctionState(QStringLiteral("State2"), &machine, []() {});
    auto* finalState = machine.addFinalState();

    state1->addTransition(state1, &QGCState::advance, state2);
    state2->addTransition(state2, &QGCState::advance, finalState);
    machine.setInitialState(state1);

    StateHistoryRecorder recorder(&machine, 20);
    recorder.setEnabled(true);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();
    QVERIFY(_spyTriggered(finishedSpy, 500));

    const auto state1Entries = recorder.entriesForState(QStringLiteral("State1"));
    bool sawEntered = false;
    bool sawExited = false;
    for (const auto& entry : state1Entries) {
        if (entry.reason == StateHistoryRecorder::Entered) {
            sawEntered = true;
        } else if (entry.reason == StateHistoryRecorder::Exited) {
            sawExited = true;
        }
    }

    QVERIFY(sawEntered);
    QVERIFY(sawExited);
    QVERIFY(recorder.count() >= 4);

    recorder.clear();
    QCOMPARE(recorder.count(), 0);
}

UT_REGISTER_TEST(StateHistoryRecorderTest, TestLabel::Unit, TestLabel::Utilities)
