#pragma once

#include "StateMachineTest.h"

#include "MultiSignalSpy.h"
#include "QGCStateMachine.h"

#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

/// Start the machine and wait until @a state is entered.
/// @return true if the state was entered within the timeout
inline bool startAndWaitForEntered(QStateMachine* machine, QAbstractState* state,
                                   int timeoutMs = TestTimeout::shortMs())
{
    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    if (auto* qgcMachine = qobject_cast<QGCStateMachine*>(machine)) {
        qgcMachine->start();
    } else {
        machine->start();
    }
    return (enteredSpy.count() > 0) || enteredSpy.wait(timeoutMs);
}
