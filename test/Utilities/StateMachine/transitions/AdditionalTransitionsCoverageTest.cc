#include "AdditionalTransitionsCoverageTest.h"
#include "TransitionTestCommon.h"

#include "QGCAbstractTransition.h"

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

} // namespace

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
    QVERIFY(enteredSpy.wait(TestTimeout::shortMs()));

    QCOMPARE(transition->machine(), &machine);
    QCOMPARE(transition->vehicle(), nullptr);
    QCOMPARE(transition->targetState(), targetState);

    machine.QStateMachine::postEvent(new QEvent(CustomTransitionEventType));

    QVERIFY(finishedSpy.wait(TestTimeout::shortMs()));
    QVERIFY(targetReached);
}

UT_REGISTER_TEST(AdditionalTransitionsCoverageTest, TestLabel::Unit, TestLabel::Utilities)
