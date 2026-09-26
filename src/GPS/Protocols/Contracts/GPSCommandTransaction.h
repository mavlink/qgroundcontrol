#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "GPSConfigurationEvidence.h"
#include "GPSReceiverSettingId.h"

using GPSCommandOutcome = GPSConfigurationOutcome;

struct GPSConfigurationStep
{
    std::string command;
    std::chrono::milliseconds timeout;
    GPSReceiverSettingSet affectedSettings = {};
    bool required = true;
};

struct [[nodiscard]] GPSCommandResult
{
    GPSConfigurationEvidence evidence{};
    GPSReceiverSettingSet affectedSettings = {};

    [[nodiscard]] bool succeeded() const
    {
        return evidence.outcome == GPSCommandOutcome::Acknowledged ||
               evidence.outcome == GPSCommandOutcome::ReadbackVerified;
    }
};
