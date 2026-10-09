#pragma once

#include <QtCore/QObject>

namespace GPSConnectionErrors {
Q_NAMESPACE

enum class GPSConnectionError
{
    None = 0,
    OpenFailed = 1,
    ConfigFailed = 2,
    /// The connection to the receiver failed or went silent.
    DeviceError = 3,
    /// Configuration needs persistent receiver changes the connection did not allow.
    ConsentRequired = 4,
    /// The receiver's output broke its protocol, so the session cannot continue.
    ProtocolError = 5,
};
Q_ENUM_NS(GPSConnectionError)

/// Why a receiver that stays connected while silent reports no position.
enum class GPSInputProblem
{
    None = 0,
    /// No bytes arrive.
    NoData = 1,
    /// Bytes arrive, but no GNSS output.
    NotGNSS = 2,
    /// The protocol identified in the input (GPSReceiverWorker::receiverDetected()) carries no position messages.
    NoPositions = 3,
    /// Only RTCM corrections arrive.
    CorrectionsOnly = 4,
};
Q_ENUM_NS(GPSInputProblem)

}  // namespace GPSConnectionErrors

using GPSConnectionError = GPSConnectionErrors::GPSConnectionError;
using GPSInputProblem = GPSConnectionErrors::GPSInputProblem;
