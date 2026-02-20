#include "RetryableRequestMessageState.h"
#include "QGCStateMachine.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaObject>

Q_DECLARE_LOGGING_CATEGORY(QGCStateMachineLog)

RetryableRequestMessageState::RetryableRequestMessageState(
    const QString& stateName,
    QState* parent,
    uint32_t messageId,
    MessageHandler messageHandler,
    int maxRetries,
    int compId,
    int timeoutMsecs)
    : WaitStateBase(stateName, parent, timeoutMsecs)
    , _messageId(messageId)
    , _messageHandler(std::move(messageHandler))
    , _compId(compId)
    , _maxRetries(maxRetries)
{
}

void RetryableRequestMessageState::onWaitEntered()
{
    WaitStateBase::onWaitEntered();
    _retryCount = 0;

    // Check skip predicate
    if (_skipPredicate && _skipPredicate()) {
        qCDebug(QGCStateMachineLog) << stateName() << "Skipping request (skip predicate returned true)";
        waitComplete();
        return;
    }

    _sendRequest();
}

void RetryableRequestMessageState::onWaitExited()
{
    _requestActive = false;
    WaitStateBase::onWaitExited();
}

void RetryableRequestMessageState::_sendRequest()
{
    Vehicle* v = vehicle();
    if (!v) {
        qCWarning(QGCStateMachineLog) << stateName() << "No vehicle available";
        waitFailed();
        return;
    }

    qCDebug(QGCStateMachineLog) << stateName() << "Requesting message" << _messageId
                                 << "attempt" << (_retryCount + 1) << "of" << (_maxRetries + 1);

    _requestActive = true;
    v->requestMessage(
        [](void* resultHandlerData,
           MAV_RESULT result,
           Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
           const mavlink_message_t& message) {
            auto* self = static_cast<RetryableRequestMessageState*>(resultHandlerData);
            if (!self->_requestActive) {
                return;
            }
            self->_handleResult(result, failureCode, message);
        },
        this,
        _compId,
        _messageId
    );
}

void RetryableRequestMessageState::_queueRetry()
{
    // Defer retries so Vehicle can finish command-list cleanup from the prior callback.
    // Immediate resend from within the callback can hit duplicate-request rejection.
    QMetaObject::invokeMethod(this, [this]() {
        if (!_requestActive) {
            return;
        }
        _sendRequest();
    }, Qt::QueuedConnection);
}

void RetryableRequestMessageState::_handleResult(
    MAV_RESULT result,
    Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
    const mavlink_message_t& message)
{
    Vehicle::RequestMessageResultHandlerFailureCode_t effectiveFailureCode = failureCode;
    if (failureCode == Vehicle::RequestMessageFailureDuplicate) {
        // Retryable request flows can re-enter while Vehicle still tracks the prior
        // request as active. Treat duplicate as an in-flight timeout-equivalent.
        effectiveFailureCode = Vehicle::RequestMessageFailureMessageNotReceived;
    }

    _lastResult = result;
    _lastFailureCode = effectiveFailureCode;

    if (effectiveFailureCode == Vehicle::RequestMessageNoFailure) {
        // Success
        qCDebug(QGCStateMachineLog) << stateName() << "Message received successfully";

        if (_messageHandler) {
            _messageHandler(vehicle(), message);
        }
        emit messageReceived(message);
        waitComplete();
        return;
    }

    // Failure - check if we should retry
    qCDebug(QGCStateMachineLog) << stateName() << "Request failed, failureCode:" << effectiveFailureCode;

    if (_retryCount < _maxRetries) {
        _retryCount++;
        qCDebug(QGCStateMachineLog) << stateName() << "Retrying, attempt" << (_retryCount + 1);
        _queueRetry();
    } else {
        // Max retries exhausted
        qCWarning(QGCStateMachineLog) << stateName() << "Max retries exhausted";

        if (_failureHandler) {
            _failureHandler(effectiveFailureCode, result);
        }
        emit retriesExhausted();

        if (_failOnMaxRetries) {
            waitFailed();
        } else {
            waitComplete();  // Advance anyway (graceful degradation)
        }
    }
}

void RetryableRequestMessageState::onWaitTimeout()
{
    qCDebug(QGCStateMachineLog) << stateName() << "Timeout waiting for message";

    _lastFailureCode = Vehicle::RequestMessageFailureMessageNotReceived;

    if (_retryCount < _maxRetries) {
        _retryCount++;
        qCDebug(QGCStateMachineLog) << stateName() << "Retrying after timeout, attempt" << (_retryCount + 1);
        _queueRetry();
    } else {
        qCWarning(QGCStateMachineLog) << stateName() << "Max retries exhausted after timeout";

        if (_failureHandler) {
            _failureHandler(_lastFailureCode, _lastResult);
        }
        emit retriesExhausted();

        if (_failOnMaxRetries) {
            WaitStateBase::onWaitTimeout();  // Emits timeout() and error()
        } else {
            waitComplete();  // Advance anyway
        }
    }
}
