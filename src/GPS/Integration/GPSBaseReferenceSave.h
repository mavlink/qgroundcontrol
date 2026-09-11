#pragma once

#include <optional>

#include "GPSBaseReference.h"
#include "GPSReceiverConfig.h"

namespace GPSBaseReferenceSave {
struct Result
{
    std::optional<GPSReceiverConfig> configuration = std::nullopt;
    QString error;
};

/// Prepare a complete fixed-base configuration without mutating persistent settings.
Result prepare(const GPSBaseReference& reference, quint64 currentSession, const GPSReceiverConfig& settings,
               quint64 nowUs = GPSObservation::monotonicNowUs());
}  // namespace GPSBaseReferenceSave
