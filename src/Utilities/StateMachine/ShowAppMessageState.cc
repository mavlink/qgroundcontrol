/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ShowAppMessageState.h"
#include "QGCApplication.h"

ShowAppMessageState::ShowAppMessageState(QState* parentState, const QString& appMessage)
    : QGCState("ShowAppMessageState", parentState)
    , _appMessage(appMessage)
{
    connect(this, &QState::entered, this, [this] () {
        qCDebug(QGCStateMachineLog) << _appMessage << stateName();
        qgcApp()->showAppMessage(_appMessage);
        emit advance();
    });
}
