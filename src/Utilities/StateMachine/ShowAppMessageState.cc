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
