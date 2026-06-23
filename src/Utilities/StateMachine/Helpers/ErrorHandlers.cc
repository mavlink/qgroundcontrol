#include "ErrorHandlers.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRandomGenerator>

namespace ErrorHandlers
{

FunctionState* logAndContinue(QGCStateMachine* machine,
                               const QString& stateName,
                               QAbstractState* nextState,
                               const QString& message)
{
    auto* state = new FunctionState(stateName, machine, [machine, message, stateName]() {
        if (message.isEmpty()) {
            qCWarning(QGCStateMachineLog) << machine->objectName() << "error handled in" << stateName;
        } else {
            qCWarning(QGCStateMachineLog) << machine->objectName() << message;
        }
    });

    state->addTransition(state, &QGCState::advance, nextState);

    return state;
}

FunctionState* logAndStop(QGCStateMachine* machine,
                           const QString& stateName,
                           const QString& message)
{
    auto* state = new FunctionState(stateName, machine, [machine, message, stateName]() {
        if (message.isEmpty()) {
            qCWarning(QGCStateMachineLog) << machine->objectName() << "stopping due to error in" << stateName;
        } else {
            qCWarning(QGCStateMachineLog) << machine->objectName() << message;
        }
        machine->stop();
    });

    return state;
}

std::function<int(int)> exponentialBackoff(int initialDelayMsecs,
                                           double multiplier,
                                           int maxDelayMsecs)
{
    return [=](int attempt) {
        if (attempt <= 1) {
            return initialDelayMsecs;
        }
        double delay = initialDelayMsecs * std::pow(multiplier, attempt - 1);
        return qMin(static_cast<int>(delay), maxDelayMsecs);
    };
}

std::function<int(int)> linearBackoff(int initialDelayMsecs,
                                       int incrementMsecs,
                                       int maxDelayMsecs)
{
    return [=](int attempt) {
        int delay = initialDelayMsecs + (attempt - 1) * incrementMsecs;
        return qMin(delay, maxDelayMsecs);
    };
}

std::function<int(int)> constantDelay(int delayMsecs)
{
    return [=](int) {
        return delayMsecs;
    };
}

std::function<int(int)> jitteredExponentialBackoff(int initialDelayMsecs,
                                                    double multiplier,
                                                    int maxDelayMsecs,
                                                    double jitterFraction)
{
    return [=](int attempt) {
        double baseDelay;
        if (attempt <= 1) {
            baseDelay = initialDelayMsecs;
        } else {
            baseDelay = initialDelayMsecs * std::pow(multiplier, attempt - 1);
        }

        baseDelay = qMin(baseDelay, static_cast<double>(maxDelayMsecs));

        // Add random jitter
        double jitter = baseDelay * jitterFraction;
        double randomJitter = QRandomGenerator::global()->generateDouble() * jitter * 2 - jitter;

        return qMax(1, static_cast<int>(baseDelay + randomJitter));
    };
}

} // namespace ErrorHandlers
