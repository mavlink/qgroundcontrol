#include "DelayState.h"

DelayState::DelayState(QState* parentState, int delayMsecs)
    : QGCState("DelayState", parentState)
{
    _delayTimer.setSingleShot(true);
    _delayTimer.setInterval(delayMsecs);

    connect(&_delayTimer, &QTimer::timeout, this, &DelayState::delayComplete);
    connect(&_delayTimer, &QTimer::timeout, this, &DelayState::advance);

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
