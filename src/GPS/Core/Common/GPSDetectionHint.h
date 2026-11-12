#pragma once

#include "GPSType.h"

/// Where a GPSType::Automatic connection looks for its receiver before it scans: the family the link's last connection
/// detected, and the rates that family may answer at. GPSDriver asks that family alone to identify itself, at each
/// rate in turn, and scans as without a hint when it does not answer.
struct GPSDetectionHint
{
    /// A family receiver detection probes; any other type is no hint.
    GPSType family = GPSType::Automatic;
    /// The rate the link ran at once the receiver was configured, where a receiver that kept running still answers.
    /// Zero when unknown.
    unsigned baud = 0;
    /// The rate the receiver answered at before its configuration moved the link to another one: where it answers
    /// again after a restart, unless it saved its configuration. Zero when unknown.
    unsigned foundBaud = 0;

    bool operator==(const GPSDetectionHint&) const = default;
};
