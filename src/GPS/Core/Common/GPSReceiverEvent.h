#pragma once

#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include "GPSConnectionErrors.h"
#include "GPSDetectionHint.h"
#include "GPSReceiverReports.h"
#include "GPSType.h"

/// One RTCM frame the receiver sent.
struct GPSReceiverRTCM
{
    QByteArray frame;
    /// The steady clock when the frame was read.
    qint64 receivedAtMs = 0;
};

/// What the detection of an Automatic session looks for at the moment.
struct GPSDetectionProgress
{
    /// The family that is asked to identify itself; GPSType::Automatic while detection listens for the output of
    /// any family.
    GPSType family = GPSType::Automatic;
    /// The rate the link runs at meanwhile.
    unsigned baud = 0;
    /// The family is asked where the session's hint says it was, before detection scans for any.
    bool remembered = false;

    bool operator==(const GPSDetectionProgress&) const = default;
};

/// The family an Automatic session detected, before it configures the receiver, or the protocol a passive session's
/// input carries: the family whose native protocol it is, or GPSType::Passive for standard NMEA.
struct GPSReceiverDetected
{
    GPSType type = GPSType::Passive;
};

/// The receiver is configured, or a passive one is listened to.
struct GPSReceiverReady
{
    /// The receiver model and firmware, or empty when the family does not report them.
    QString identity;
    /// What an Automatic session found, as the hint for the next session on its link; any other session has none.
    GPSDetectionHint detected;
};

/// Why a session ended that was not stopped. Nothing follows it.
struct GPSReceiverEnded
{
    GPSConnectionError error = GPSConnectionError::None;
    /// The transport's or the protocol's diagnostic, or empty when it gave none. That of a configuration the receiver
    /// failed also says what the receiver identified itself as, unless the diagnostic names it.
    QString detail;
};

/// What a receiver session reports, one event at a time and in the order it happens. A GPSInputProblem tells why a
/// session that goes on delivers nothing useful, and GPSInputProblem::None that it does again.
using GPSReceiverEvent =
    std::variant<GPSPositionReport, GPSSatelliteReport, GPSBaseStatus, GPSReceiverRTCM, GPSDetectionProgress,
                 GPSReceiverDetected, GPSReceiverReady, GPSInputProblem, GPSReceiverEnded>;
