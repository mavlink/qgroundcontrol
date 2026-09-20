#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "GPSConfigurationEvidence.h"
#include "GPSIOStatus.h"
#include "GPSReceiverSettingId.h"

using GPSCommandOutcome = GPSConfigurationOutcome;

struct GPSConfigurationStep
{
    std::string command;
    std::chrono::milliseconds timeout;
    GPSReceiverSettingSet affectedSettings = {};
    bool required = true;
};

struct GPSCommandResult
{
    GPSConfigurationEvidence evidence{};
    GPSReceiverSettingSet affectedSettings = {};
};

/// One attempt under a single absolute deadline. Retry decisions belong to the configuration policy.
class GPSCommandTransaction
{
public:
    template <class Now, class Reply, class Pump, class Error>
    static GPSCommandOutcome await(uint64_t deadline, Now now, Reply reply, Pump pump, Error error)
    {
        for (;;) {
            if (const auto failure = error(); failure != GPSCommandOutcome::Pending) {
                return failure;
            }
            if (const auto response = reply(); response != GPSCommandOutcome::Pending) {
                return response;
            }
            if (now() >= deadline) {
                return GPSCommandOutcome::TimedOut;
            }
            pump();
        }
    }
};
