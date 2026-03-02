#pragma once

#include "QGCState.h"

/// State that executes all child states in parallel
/// When entered, all child states are activated simultaneously.
/// The state completes when all child states have finished.
class ParallelState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(ParallelState)

public:
    /// Create a parallel state
    /// @param stateName Name for debugging
    /// @param parent Parent state or machine
    ParallelState(const QString& stateName, QState* parent);

    /// Add a child state to execute in parallel
    /// @param state The state to add as a parallel child
    void addParallelState(QAbstractState* state);

signals:
    /// Emitted when all parallel child states have completed
    void allComplete();

protected:
    void onEntry(QEvent* event) override;
    void onExit(QEvent* event) override;

private slots:
    void _onChildFinished();

private:
    void _checkAllComplete();

    int _activeChildren = 0;
    int _completedChildren = 0;
};
