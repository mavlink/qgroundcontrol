/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCFinalState.h"
#include "QGCState.h"

QGCFinalState::QGCFinalState(QState* parent)
    : QFinalState(parent)
{
    connect(this, &QFinalState::entered, this, [this] () {
        qCDebug(QGCStateMachineLog) << "Entered Final State";
    });
    connect(this, &QState::exited, this, [this] () {
        qCDebug(QGCStateMachineLog) << "Exited Final State";
    });
}