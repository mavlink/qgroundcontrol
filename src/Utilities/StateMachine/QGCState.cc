/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCStateMachineLog, "Utilities.QGCStateMachine")

QGCState::QGCState(const QString& stateName, QState* parentState) 
    : QState(QState::ExclusiveStates, parentState)
{
    setObjectName(stateName);

    connect(this, &QState::entered, this, [this] () {
        qCDebug(QGCStateMachineLog) << "Entered" << this->stateName();
    });
    connect(this, &QState::exited, this, [this] () {
        qCDebug(QGCStateMachineLog) << "Exited" << this->stateName();
    });
}

QGCStateMachine* QGCState::machine() const
{ 
    return qobject_cast<QGCStateMachine*>(QState::machine()); 
}

Vehicle *QGCState::vehicle() 
{ 
    return machine()->vehicle(); 
}

QString QGCState::stateName() const 
{
    if (machine()) {
        return QStringLiteral("%1:%2").arg(objectName(), machine()->machineName());
    } else {
        return objectName();
    }
}
