#include "StateMachineTest.h"

#include <QtCore/QRegularExpression>
#include <QtStateMachine/QStateMachine>
#include <QtTest/QSignalSpy>

void StateMachineTest::ignoreTimeoutWarnings()
{
    ignoreLogMessage("Utilities.QGCStateMachine", QtWarningMsg, QRegularExpression(QStringLiteral("^Timeout \"")));
    ignoreLogMessage("Utilities.StateMachine.RetryTransition", QtWarningMsg, QRegularExpression(QStringLiteral("timeout, retry")));
}

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
