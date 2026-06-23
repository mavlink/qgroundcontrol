#include "StateMachineTest.h"

#include <QtStateMachine/QStateMachine>
#include <QtTest/QSignalSpy>

bool StateMachineTest::startAndWaitForFinished(QStateMachine* machine, int timeoutMs)
{
    QSignalSpy finishedSpy(machine, &QStateMachine::finished);
    machine->start();
    return spyTriggered(finishedSpy, timeoutMs);
}

bool StateMachineTest::spyTriggered(QSignalSpy& spy, int timeoutMs)
{
    return (spy.count() > 0) || spy.wait(timeoutMs);
}
