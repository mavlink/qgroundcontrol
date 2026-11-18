#include "RetryableRequestMessageState.h"
#include "QGCStateMachine.h"
#include "Vehicle.h"

#include "QGCLoggingCategory.h"
#include <QtCore/QMetaObject>

QGC_LOGGING_CATEGORY(RetryableRequestMessageStateLog, "Utilities.StateMachine.RetryableRequestMessageState")

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

RetryableRequestMessageState::~RetryableRequestMessageState()
{
    _cancelRequest();
}

void RetryableRequestMessageState::onWaitEntered()
{
    WaitStateBase::onWaitEntered();
    _retryCount = 0;
    _retryWhenRequestFails = false;

    if (_skipPredicate && _skipPredicate()) {
        qCDebug(RetryableRequestMessageStateLog) << stateName() << "Skipping request (skip predicate returned true)";
        waitComplete();
        return;
    }

    _sendRequest();
}

void RetryableRequestMessageState::onWaitExited()
{
    _requestActive = false;
    _cancelRequest();
    WaitStateBase::onWaitExited();
}

void RetryableRequestMessageState::_cancelRequest()
{
    // Vehicle would otherwise call back a state that is no longer waiting, or one that has been destroyed
    if (_requestVehicle) {
        _requestVehicle->cancelRequestMessage(this);
        _requestVehicle.clear();
    }
}

void RetryableRequestMessageState::_vehicleDestroyed()
{
    if (!_requestActive) {
        return;
    }

    // Nothing answers the outstanding request any more, and no timeout runs while a retry waits for it
    qCWarning(RetryableRequestMessageStateLog)
        << stateName() << "Vehicle destroyed while requesting message" << _messageId;
    _requestActive = false;
    waitFailed();
}

void RetryableRequestMessageState::_sendRequest()
{
    Vehicle* v = vehicleOrWarn(RetryableRequestMessageStateLog);
    if (!v) {
        waitFailed();
        return;
    }

    qCDebug(RetryableRequestMessageStateLog) << stateName() << "Requesting message" << _messageId
                                 << "attempt" << (_retryCount + 1) << "of" << (_maxRetries + 1);

    _requestActive = true;
    _requestVehicle = v;
    // Queued, so that a state deleted along with its vehicle is gone before this is delivered
    connect(v, &QObject::destroyed, this, &RetryableRequestMessageState::_vehicleDestroyed,
            static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
    v->requestMessage(
        [](void* resultHandlerData,
           MAV_RESULT result,
           VehicleTypes::RequestMessageResultHandlerFailureCode_t failureCode,
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
    // Each attempt gets the full timeout
    restartWait();

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
    VehicleTypes::RequestMessageResultHandlerFailureCode_t failureCode,
    const mavlink_message_t& message)
{
    VehicleTypes::RequestMessageResultHandlerFailureCode_t effectiveFailureCode = failureCode;
    if (failureCode == VehicleTypes::RequestMessageFailureDuplicate) {
        // Retryable request flows can re-enter while Vehicle still tracks the prior
        // request as active. Treat duplicate as an in-flight timeout-equivalent.
        effectiveFailureCode = VehicleTypes::RequestMessageFailureMessageNotReceived;
    }

    _lastResult = result;
    _lastFailureCode = effectiveFailureCode;

    if (effectiveFailureCode == VehicleTypes::RequestMessageNoFailure) {
        qCDebug(RetryableRequestMessageStateLog) << stateName() << "Message received successfully";

        if (_messageHandler) {
            _messageHandler(vehicle(), message);
        }
        emit messageReceived(message);
        waitComplete();
        return;
    }

    qCDebug(RetryableRequestMessageStateLog) << stateName() << "Request failed, failureCode:" << effectiveFailureCode;

    if (_retryWhenRequestFails) {
        // This attempt was counted when it timed out
        _retryWhenRequestFails = false;
        qCDebug(RetryableRequestMessageStateLog) << stateName() << "Sending retry, attempt" << (_retryCount + 1);
        _queueRetry();
    } else if (_retryCount < _maxRetries) {
        _retryCount++;
        qCDebug(RetryableRequestMessageStateLog) << stateName() << "Retrying, attempt" << (_retryCount + 1);
        _queueRetry();
    } else {
        qCWarning(RetryableRequestMessageStateLog) << stateName() << "Max retries exhausted";

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
    qCDebug(RetryableRequestMessageStateLog) << stateName() << "Timeout waiting for message";

    _lastFailureCode = VehicleTypes::RequestMessageFailureMessageNotReceived;

    if (_retryCount < _maxRetries) {
        _retryCount++;
        qCDebug(RetryableRequestMessageStateLog) << stateName() << "Retrying after timeout, attempt" << (_retryCount + 1);
        // Vehicle still tracks the request that timed out and rejects a second one for the same message as a
        // duplicate. The retry is sent once that request fails; if it is answered after all, the answer is used.
        // No timeout runs until then: one that fired first would use up a retry that could not be sent.
        _retryWhenRequestFails = true;
    } else {
        qCWarning(RetryableRequestMessageStateLog) << stateName() << "Max retries exhausted after timeout";

        // The outcome is reported now, so whatever the outstanding request still returns is ignored
        _requestActive = false;

        if (_failureHandler) {
            _failureHandler(_lastFailureCode, _lastResult);
        }
        emit retriesExhausted();

        if (_failOnMaxRetries) {
            WaitStateBase::onWaitTimeout();  // Emits timeout()
            waitFailed();                    // Emits error()
        } else {
            waitComplete();  // Advance anyway
        }
    }
}
