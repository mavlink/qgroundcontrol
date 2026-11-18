#include "DelayState.h"

#include <QtStateMachine/QStateMachine>

DelayState::DelayState(QState* parentState, int delayMsecs)
    : QGCState("DelayState", parentState)
{
    _delayTimer.setSingleShot(true);
    _delayTimer.setInterval(delayMsecs);

    connect(&_delayTimer, &QTimer::timeout, this, &DelayState::advance);

    connect(this, &QState::entered, this, [this, delayMsecs] ()
        {
            qCDebug(QGCStateMachineLog) << stateName() << QStringLiteral("Starting delay for %1 secs").arg(delayMsecs / 1000.0) << " - " << Q_FUNC_INFO;
            // Qt exits no state of a machine that stops. The machine is not known before the state is entered.
            if (const QStateMachine* const stateMachine = QAbstractState::machine()) {
                connect(stateMachine, &QStateMachine::stopped, &_delayTimer, &QTimer::stop, Qt::UniqueConnection);
            }
            _delayTimer.start();
        });

    connect(this, &QGCState::exited, this, [this] ()
        {
            _delayTimer.stop();
        });
}
