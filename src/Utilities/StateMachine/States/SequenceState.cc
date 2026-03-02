#include "SequenceState.h"
#include "QGCLoggingCategory.h"

SequenceState::SequenceState(const QString& stateName, QState* parent)
    : QGCState(stateName, parent)
{
}

void SequenceState::addStep(const QString& name, StepAction action)
{
    _steps.append({name, std::move(action)});
}

void SequenceState::addSteps(const QList<Step>& steps)
{
    _steps.append(steps);
}

QString SequenceState::currentStepName() const
{
    if (_currentStep >= 0 && _currentStep < _steps.size()) {
        return _steps[_currentStep].name;
    }
    return QString();
}

void SequenceState::onEnter()
{
    _currentStep = -1;
    _failedStep.clear();

    // Start executing steps
    QMetaObject::invokeMethod(this, &SequenceState::_executeNextStep, Qt::QueuedConnection);
}

void SequenceState::_executeNextStep()
{
    _currentStep++;

    if (_currentStep >= _steps.size()) {
        // All steps completed successfully
        qCDebug(QGCStateMachineLog) << stateName() << "sequence completed successfully";
        emit advance();
        return;
    }

    const Step& step = _steps[_currentStep];
    qCDebug(QGCStateMachineLog) << stateName() << "executing step" << _currentStep << ":" << step.name;

    bool success = true;
    if (step.action) {
        success = step.action();
    }

    if (success) {
        emit stepCompleted(step.name, _currentStep);
        // Schedule next step
        QMetaObject::invokeMethod(this, &SequenceState::_executeNextStep, Qt::QueuedConnection);
    } else {
        _failedStep = step.name;
        qCDebug(QGCStateMachineLog) << stateName() << "step failed:" << step.name;
        emit error();
    }
}
