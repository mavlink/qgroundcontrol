#pragma once

#include <QtStateMachine/QFinalState>
#include <QtCore/QString>

class QGCStateMachine;
class Vehicle;

/// Final state for a QGCStateMachine with logging support
class QGCFinalState : public QFinalState
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCFinalState)

public:
    explicit QGCFinalState(const QString& stateName, QState* parent = nullptr);
    explicit QGCFinalState(QState* parent = nullptr);

    QString stateName() const;
    QGCStateMachine* machine() const;
    Vehicle* vehicle() const;
};
