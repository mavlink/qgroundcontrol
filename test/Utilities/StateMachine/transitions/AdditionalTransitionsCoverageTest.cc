#include "AdditionalTransitionsCoverageTest.h"
#include "TransitionTestCommon.h"

#include "QGCAbstractTransition.h"
#include "QGCEventTransition.h"

#include <QtCore/QTimerEvent>

namespace {

constexpr QEvent::Type CustomTransitionEventType = static_cast<QEvent::Type>(QEvent::User + 42);

class TestAbstractTransition : public QGCAbstractTransition
{
public:
    explicit TestAbstractTransition(QAbstractState* target)
        : QGCAbstractTransition(target)
    {
    }

protected:
    bool eventTest(QEvent* event) override
    {
        return event && (event->type() == CustomTransitionEventType);
    }
};

class OneShotTimerEmitter : public QObject
{
public:
    void trigger()
    {
        _timerId = startTimer(1);
    }

protected:
    void timerEvent(QTimerEvent* event) override
    {
        if ((_timerId != 0) && (event->timerId() == _timerId)) {
            killTimer(_timerId);
            _timerId = 0;
        }
        QObject::timerEvent(event);
    }

private:
    int _timerId = 0;
};

} // namespace

void AdditionalTransitionsCoverageTest::_testQGCEventTransitionMatchesEvent()
{
    QStateMachine machine;
    OneShotTimerEmitter watchedObject;
    bool targetReached = false;

    auto* sourceState = new QState(&machine);
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&targetReached]() {
        targetReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    sourceState->addTransition(new QGCEventTransition(&watchedObject, QEvent::Timer, targetState));
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(sourceState);

    QSignalSpy enteredSpy(sourceState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    watchedObject.trigger();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(targetReached);
}

void AdditionalTransitionsCoverageTest::_testQGCEventTransitionGuardBlocksThenFallbackTakesEvent()
{
    QStateMachine machine;
    OneShotTimerEmitter watchedObject;
    bool guardCalled = false;
    bool blockedStateReached = false;
    bool fallbackStateReached = false;

    auto* sourceState = new QState(&machine);
    auto* blockedState = new FunctionState(QStringLiteral("Blocked"), &machine, [&blockedStateReached]() {
        blockedStateReached = true;
    });
    auto* fallbackState = new FunctionState(QStringLiteral("Fallback"), &machine, [&fallbackStateReached]() {
        fallbackStateReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    sourceState->addTransition(new QGCEventTransition(
        &watchedObject, QEvent::Timer, blockedState,
        [&guardCalled](QEvent*) {
            guardCalled = true;
            return false;
        }));
    sourceState->addTransition(new QGCEventTransition(&watchedObject, QEvent::Timer, fallbackState));

    blockedState->addTransition(blockedState, &QGCState::advance, finalState);
    fallbackState->addTransition(fallbackState, &QGCState::advance, finalState);
    machine.setInitialState(sourceState);

    QSignalSpy enteredSpy(sourceState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    watchedObject.trigger();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(guardCalled);
    QVERIFY(!blockedStateReached);
    QVERIFY(fallbackStateReached);
}

void AdditionalTransitionsCoverageTest::_testQGCAbstractTransitionAccessorsAndCustomEventTransition()
{
    QGCStateMachine machine(QStringLiteral("AbstractTransitionCoverage"), nullptr);
    bool targetReached = false;

    auto* sourceState = new QState(&machine);
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&targetReached]() {
        targetReached = true;
    });
    auto* finalState = new QGCFinalState(QStringLiteral("Final"), &machine);

    auto* transition = new TestAbstractTransition(targetState);
    sourceState->addTransition(transition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(sourceState);

    QSignalSpy enteredSpy(sourceState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    QCOMPARE(transition->machine(), &machine);
    QCOMPARE(transition->vehicle(), nullptr);
    QCOMPARE(transition->targetState(), targetState);

    machine.QStateMachine::postEvent(new QEvent(CustomTransitionEventType));

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(targetReached);
}

UT_REGISTER_TEST(AdditionalTransitionsCoverageTest, TestLabel::Unit, TestLabel::Utilities)
