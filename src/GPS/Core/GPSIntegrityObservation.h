#pragma once

#include <QtCore/QMetaType>

#include <optional>

/// Original diagnostic receipts; zero means that diagnostic has never been reported.
struct GPSIntegrityProvenance
{
    quint64 jammingTimestampUs = 0;
    quint64 spoofingTimestampUs = 0;
    quint64 authenticationTimestampUs = 0;
    quint64 correctionsTimestampUs = 0;
    quint64 rfTimestampUs = 0;
};

/// Receiver-reported diagnostics, distinct from correction transport delivery counters.
struct GPSIntegrityObservation
{
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    // Absent for a single complete integrity message; present for independently updated native fields.
    std::optional<GPSIntegrityProvenance> provenance = std::nullopt;
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

    std::optional<int> noisePerMillisecond;
    std::optional<int> automaticGainControl;
    std::optional<int> jammingIndicator;
    std::optional<bool> correctionsCrcFailed;
};
Q_DECLARE_METATYPE(GPSIntegrityObservation)
