#pragma once

#include <cstdint>
#include <string>

enum class GPSConfigurationOutcome
{
    Pending,
    Written,
    Acknowledged,
    ReadbackVerified,
    Rejected,
    TimedOut,
    Cancelled,
    TransportError,
};

/// Evidence from one configuration command, not a claim of physical receiver operation.
struct GPSConfigurationEvidence
{
    std::string command;
    GPSConfigurationOutcome outcome = GPSConfigurationOutcome::Pending;
    uint64_t startedAtUs = 0;
    uint64_t finishedAtUs = 0;
    int acceptedBytes = 0;
    int writtenBytes = 0;
    int uncertainBytes = 0;
    bool required = true;
};
