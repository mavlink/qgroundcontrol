#include "ShowAppMessageState.h"
#include "AppMessages.h"

ShowAppMessageState::ShowAppMessageState(QState* parentState, const QString& appMessage)
    : QGCState("ShowAppMessageState", parentState)
    , _appMessage(appMessage)
{
    connect(this, &QState::entered, this, [this] () {
        qCDebug(QGCStateMachineLog) << _appMessage << stateName();
        QGC::showAppMessage(_appMessage);
        emit advance();
    });
}
