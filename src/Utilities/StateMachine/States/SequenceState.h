#pragma once

#include "QGCState.h"

#include <functional>

/// A composite state that executes a sequence of actions in order.
///
/// Each action is executed in turn. If any action fails (returns false),
/// the sequence stops and error() is emitted. When all actions complete
/// successfully, advance() is emitted.
///
/// Example usage:
/// @code
/// auto* seqState = new SequenceState("LoadSequence", &machine);
/// seqState->addStep("Validate", []() {
///     // Validation logic
///     return isValid;  // Return false to stop sequence
/// });
/// seqState->addStep("Load", []() {
///     // Load logic
///     return true;
/// });
/// seqState->addStep("Initialize", []() {
///     // Init logic
///     return true;
/// });
///
/// seqState->addTransition(seqState, &SequenceState::advance, successState);
/// seqState->addTransition(seqState, &SequenceState::error, errorState);
/// @endcode
class SequenceState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(SequenceState)

public:
    /// Step action - returns true to continue, false to stop with error
    using StepAction = std::function<bool()>;

    struct Step {
        QString name;
        StepAction action;
    };

    SequenceState(const QString& stateName, QState* parent);

    /// Add a step to the sequence
    /// @param name Step name for logging
    /// @param action Action to execute (returns true to continue, false to error)
    void addStep(const QString& name, StepAction action);

    /// Add multiple steps at once
    void addSteps(const QList<Step>& steps);

    /// Get the current step index
    int currentStepIndex() const { return _currentStep; }

    /// Get the current step name
    QString currentStepName() const;

    /// Get the total number of steps
    int stepCount() const { return _steps.size(); }

    /// Get the name of the step that failed (valid after error())
    QString failedStepName() const { return _failedStep; }

signals:
    /// Emitted after each step completes successfully
    void stepCompleted(const QString& stepName, int index);

protected:
    void onEnter() override;

private slots:
    void _executeNextStep();

private:
    QList<Step> _steps;
    int _currentStep = -1;
    QString _failedStep;
};
