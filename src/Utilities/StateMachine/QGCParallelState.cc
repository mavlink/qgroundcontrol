/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCParallelStateMachine.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCParallelStateMachineLog, "Utilities.QGCParallelStateMachine")

QGCParallelState::QGCParallelState(const QString& stateName, QState* parentState)
    : QGCState(parentState)
{
    setObjectName(stateName);
    setChildMode(QState::ParallelStates);
}
