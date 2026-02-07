#include "ShowAppMessageState.h"
#include "QGCApplication.h"

#include <QtCore/QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(QGCStateMachineLog, "Utilities.QGCStateMachine")

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
