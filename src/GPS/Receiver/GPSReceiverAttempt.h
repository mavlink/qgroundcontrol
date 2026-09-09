#pragma once

#include <memory>

#include "GPSConnectionError.h"
#include "GPSReceiverProfile.h"

/// Immutable configuration and observed state for one generation; reconnect intent lives in the controller.
struct GPSReceiverAttempt
{
    enum class Phase
    {
        Idle,
        Connecting,
        Configuring,
        Ready,
        Failed,
        Cancelled
    };

    quint64 generation = 0;
    std::shared_ptr<const GPSReceiverProfile> profile;
    Phase phase = Phase::Idle;
    GPSConnectionError error = GPSConnectionError::None;
    QString errorDetail;

    bool terminal() const { return phase == Phase::Failed || phase == Phase::Cancelled; }

    bool ready() const { return phase == Phase::Ready; }
};
Q_DECLARE_METATYPE(GPSReceiverAttempt)
