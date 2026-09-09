#pragma once

#include "GPSObservation.h"

/// Receiver-reported diagnostics, distinct from correction transport delivery counters.
struct GPSIntegrityObservation
{
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    std::optional<quint32> systemErrors = std::nullopt;
    std::optional<int> spoofingState = std::nullopt;
    std::optional<int> jammingState = std::nullopt;
    std::optional<int> authenticationState = std::nullopt;
    std::optional<int> correctionsQuality = std::nullopt;
    std::optional<int> systemQuality = std::nullopt;
    std::optional<int> gnssSignalQuality = std::nullopt;
    std::optional<int> postProcessingQuality = std::nullopt;
    std::optional<int> correctionsProtocol = std::nullopt;
    std::optional<int> correctionsUsed = std::nullopt;

    static GPSIntegrityObservation fromPosition(const GPSObservation& observation)
    {
        GPSIntegrityObservation result;
        result.monotonicTimestampUs = observation.monotonicTimestampUs;
        result.sessionId = observation.sessionId;
        result.spoofingState = observation.spoofingState;
        result.jammingState = observation.jammingState;
        result.authenticationState = observation.authenticationState;
        result.correctionsProtocol = observation.correctionsProtocol;
        result.correctionsUsed = observation.correctionsUsed;
        return result;
    }
};
Q_DECLARE_METATYPE(GPSIntegrityObservation)
