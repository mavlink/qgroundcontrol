#pragma once

#include "WaitStateBase.h"
#include "Vehicle.h"

#include <functional>

/// Requests a MAVLink message with built-in retry logic.
/// On failure, retries up to maxRetries times before giving up.
///
/// Emits advance() on success or after all retries exhausted.
/// Emits error() only if explicitly requested via failOnMaxRetries.
///
/// Example usage:
/// @code
/// auto* state = new RetryableRequestMessageState(
///     "GetVersion", &machine,
///     MAVLINK_MSG_ID_AUTOPILOT_VERSION,
///     [this](Vehicle* v, const mavlink_message_t& msg) {
///         // Handle successful message
///     },
///     2  // max retries
/// );
/// @endcode
class RetryableRequestMessageState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(RetryableRequestMessageState)

public:
    using MessageHandler = std::function<void(Vehicle* vehicle, const mavlink_message_t& message)>;
    using FailureHandler = std::function<void(Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, MAV_RESULT result)>;
    using SkipPredicate = std::function<bool()>;

    /// Create a retryable request message state
    /// @param stateName Name for this state
    /// @param parent Parent state
    /// @param messageId MAVLink message ID to request
    /// @param messageHandler Handler called on successful message receipt
    /// @param maxRetries Maximum retry attempts (default: 1)
    /// @param compId Component ID to request from (default: MAV_COMP_ID_AUTOPILOT1)
    /// @param timeoutMsecs Timeout per attempt in milliseconds (default: 5000)
    RetryableRequestMessageState(const QString& stateName,
                                  QState* parent,
                                  uint32_t messageId,
                                  MessageHandler messageHandler = nullptr,
                                  int maxRetries = 1,
                                  int compId = MAV_COMP_ID_AUTOPILOT1,
                                  int timeoutMsecs = 5000);

    /// Set handler called when all retries fail
    void setFailureHandler(FailureHandler handler) { _failureHandler = std::move(handler); }

    /// Set predicate to check if request should be skipped
    /// If predicate returns true, state completes immediately without sending request
    void setSkipPredicate(SkipPredicate predicate) { _skipPredicate = std::move(predicate); }

    /// If true, emit error() after max retries instead of advance()
    void setFailOnMaxRetries(bool fail) { _failOnMaxRetries = fail; }

    /// Get the failure code from the last attempt
    Vehicle::RequestMessageResultHandlerFailureCode_t lastFailureCode() const { return _lastFailureCode; }

    /// Get the MAV_RESULT from the last attempt
    MAV_RESULT lastResult() const { return _lastResult; }

    /// Get current retry count
    int retryCount() const { return _retryCount; }

    /// Get max retries setting
    int maxRetries() const { return _maxRetries; }

signals:
    /// Emitted when message is successfully received
    void messageReceived(const mavlink_message_t& message);

    /// Emitted when all retries are exhausted
    void retriesExhausted();

protected:
    void connectWaitSignal() override {}
    void disconnectWaitSignal() override {}
    void onWaitEntered() override;
    void onWaitExited() override;
    void onWaitTimeout() override;

private:
    void _sendRequest();
    void _handleResult(MAV_RESULT result,
                       Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                       const mavlink_message_t& message);

    uint32_t _messageId;
    MessageHandler _messageHandler;
    FailureHandler _failureHandler;
    SkipPredicate _skipPredicate;
    int _compId;
    int _maxRetries;
    int _retryCount = 0;
    bool _failOnMaxRetries = false;

    bool _requestActive = false;

    Vehicle::RequestMessageResultHandlerFailureCode_t _lastFailureCode = Vehicle::RequestMessageNoFailure;
    MAV_RESULT _lastResult = MAV_RESULT_ACCEPTED;
};
