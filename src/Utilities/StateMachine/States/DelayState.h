#pragma once

#include "QGCState.h"

#include <QtCore/QTimer>

/// \brief Delays that state machine for the specified time in milliseconds
///
/// The delay starts when the state is entered and ends with advance(). It ends without it when the state is exited
/// before that, and when its machine stops.
///
class DelayState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(DelayState)

public:
    DelayState(QState* parentState, int delayMsecs);

private:
    QTimer _delayTimer;
};
