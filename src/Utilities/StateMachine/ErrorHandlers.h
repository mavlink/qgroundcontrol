#pragma once

#include "FunctionState.h"

#include <QtCore/QTimer>

#include <functional>
#include <cmath>

class QGCStateMachine;
class QAbstractState;

/// Pre-built error handling utilities.
///
/// Provides common error handling patterns as ready-to-use functions and states.
///
/// Example usage:
/// @code
/// // Log and continue to next state
/// machine.setGlobalErrorState(
///     ErrorHandlers::logAndContinue(&machine, "ErrorLogged", nextState));
///
/// // Exponential backoff for retries
/// auto backoff = ErrorHandlers::exponentialBackoff(1000, 2.0, 30000);
/// int delay = backoff(attemptNumber);  // 1000, 2000, 4000, 8000, ...
/// @endcode
namespace ErrorHandlers
{

/// Create a state that logs the error and transitions to the next state
/// @param machine Parent state machine
/// @param stateName Name for the error logging state
/// @param nextState State to transition to after logging
/// @param message Optional custom message to log
FunctionState* logAndContinue(QGCStateMachine* machine,
                               const QString& stateName,
                               QAbstractState* nextState,
                               const QString& message = QString());

/// Create a state that logs the error and stops the machine
/// @param machine Parent state machine
/// @param stateName Name for the error state
/// @param message Optional custom message to log
FunctionState* logAndStop(QGCStateMachine* machine,
                           const QString& stateName,
                           const QString& message = QString());

/// Create an exponential backoff delay calculator
/// @param initialDelayMsecs Initial delay in milliseconds
/// @param multiplier Multiplier for each subsequent attempt (default 2.0)
/// @param maxDelayMsecs Maximum delay cap
/// @return Function that takes attempt number (1-based) and returns delay
std::function<int(int)> exponentialBackoff(int initialDelayMsecs,
                                           double multiplier = 2.0,
                                           int maxDelayMsecs = 60000);

/// Create a linear backoff delay calculator
/// @param initialDelayMsecs Initial delay in milliseconds
/// @param incrementMsecs Amount to add for each subsequent attempt
/// @param maxDelayMsecs Maximum delay cap
/// @return Function that takes attempt number (1-based) and returns delay
std::function<int(int)> linearBackoff(int initialDelayMsecs,
                                       int incrementMsecs,
                                       int maxDelayMsecs = 60000);

/// Create a constant delay calculator (same delay for all attempts)
/// @param delayMsecs Constant delay in milliseconds
/// @return Function that takes attempt number and returns constant delay
std::function<int(int)> constantDelay(int delayMsecs);

/// Create a jittered exponential backoff (adds randomness to prevent thundering herd)
/// @param initialDelayMsecs Initial delay in milliseconds
/// @param multiplier Multiplier for each subsequent attempt
/// @param maxDelayMsecs Maximum delay cap
/// @param jitterFraction Fraction of delay to add as random jitter (0.0-1.0)
/// @return Function that takes attempt number and returns jittered delay
std::function<int(int)> jitteredExponentialBackoff(int initialDelayMsecs,
                                                    double multiplier = 2.0,
                                                    int maxDelayMsecs = 60000,
                                                    double jitterFraction = 0.25);

/// Retry action wrapper with backoff
/// @param action The action to retry (returns true on success)
/// @param maxAttempts Maximum number of attempts
/// @param backoffFunc Function to calculate delay between attempts
/// @param onRetry Optional callback before each retry (attempt number)
/// @return Function that executes the action with retries, returns true if eventually successful
std::function<bool()> withRetry(std::function<bool()> action,
                                 int maxAttempts,
                                 std::function<int(int)> backoffFunc,
                                 std::function<void(int)> onRetry = nullptr);

} // namespace ErrorHandlers
