#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "GPSIOStatus.h"
#include "GPSReceiverSettingId.h"

enum class GPSCommandOutcome
{
    Pending,
    Acknowledged,
    Rejected,
    TimedOut,
    Cancelled,
    TransportError,
};

struct GPSConfigurationStep
{
    std::string command;
    std::chrono::milliseconds timeout;
    GPSReceiverSettingSet affectedSettings = {};
    bool required = true;
};

struct GPSCommandResult
{
    std::string command;
    GPSCommandOutcome outcome = GPSCommandOutcome::Pending;
    uint64_t startedAtUs = 0;
    uint64_t finishedAtUs = 0;
    GPSReceiverSettingSet affectedSettings = {};
    int acceptedBytes = 0;
    int writtenBytes = 0;
    int uncertainBytes = 0;
    bool required = true;
};

/// One attempt under a single absolute deadline. Retry decisions belong to the configuration policy.
class GPSCommandTransaction
{
public:
    template <class Now, class Reply, class Pump, class Error>
    static GPSCommandOutcome await(uint64_t deadline, Now now, Reply reply, Pump pump, Error error)
    {
        for (;;) {
            if (const auto failure = error(); failure != GPSCommandOutcome::Pending)
                return failure;
            if (const auto response = reply(); response != GPSCommandOutcome::Pending)
                return response;
            if (now() >= deadline)
                return GPSCommandOutcome::TimedOut;
            pump();
        }
    }
};
