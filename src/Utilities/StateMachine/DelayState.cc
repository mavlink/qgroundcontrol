/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "DelayState.h"

DelayState::DelayState(QState* parentState, int delayMsecs)
    : QGCState("DelayState", parentState)
{
    _delayTimer.setSingleShot(true);
    _delayTimer.setInterval(delayMsecs);

    connect(&_delayTimer, &QTimer::timeout, this, &DelayState::delayComplete);

    connect(this, &QState::entered, this, [this, delayMsecs] () 
        { 
            qCDebug(QGCStateMachineLog) << stateName() << QStringLiteral("Starting delay for %1 secs").arg(delayMsecs / 1000.0) << " - " << Q_FUNC_INFO;
            _delayTimer.start();
        });

    connect(this, &QGCState::exited, this, [this] () 
        { 
            _delayTimer.stop();
        });
}
