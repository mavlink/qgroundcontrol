#include "SubMachineStateTest.h"
#include "StateTestCommon.h"


void SubMachineStateTest::_testSubMachineState()
{
    QGCStateMachine machine(QStringLiteral("ParentMachine"), nullptr);
    bool childMachineRan = false;

    auto* subMachineState = new SubMachineState(
        QStringLiteral("SubMachine"),
        &machine,
        [&childMachineRan](SubMachineState* /*parent*/) -> QGCStateMachine* {
            // Don't parent to SubMachineState - Qt might treat it as nested state machine
            auto* child = new QGCStateMachine(QStringLiteral("ChildMachine"), nullptr, nullptr);

            auto* state = new FunctionState(QStringLiteral("ChildState"), child, [&childMachineRan]() {
                childMachineRan = true;
            });
            auto* finalState = new QGCFinalState(QStringLiteral("ChildFinal"), child);

            state->addTransition(state, &QGCState::advance, finalState);
            child->setInitialState(state);

            return child;
        }
    );
    auto* finalState = new QGCFinalState(QStringLiteral("Final"), &machine);

    subMachineState->addTransition(subMachineState, &QGCState::advance, finalState);
    machine.setInitialState(subMachineState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(childMachineRan);
}

UT_REGISTER_TEST(SubMachineStateTest, TestLabel::Unit, TestLabel::Utilities)
