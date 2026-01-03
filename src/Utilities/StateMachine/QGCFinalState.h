#pragma once

#include <QFinalState>

/// Final state for a QGCStateMachine
///     Same as QFinalState but with logging
class QGCFinalState : public QFinalState
{
    Q_OBJECT

public:
    QGCFinalState(QState* parent = nullptr);
};
