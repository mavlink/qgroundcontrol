#pragma once

#include <optional>

#include <QtCore/QString>

#include "GPSConnectionErrors.h"
#include "GPSReceiver.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverReports.h"
#include "GPSReceiverSession.h"
#include "GPSType.h"
#include "RTKSettings.h"

/// The wording of GPSReceiver's errors, warnings and labels, translated in the GPSReceiver context.
namespace GPSReceiverMessages {

/// The error a session leaves when it ends with @a error and the transport's or protocol's @a detail, after the
/// connection policy chose @a outcome.
[[nodiscard]] QString sessionEnded(GPSReceiverSessionOutcome outcome, GPSConnectionError error, const QString& detail,
                                   GPSReceiverSessionEnd end);

/// The error of an automatic connection that the @a family receiver refused for lack of flash-save consent.
[[nodiscard]] QString consentRefused(const QString& detail, GPSType family);

/// Why a passive receiver delivers no position, for the protocol @a detected in its output; empty when positions
/// arrive, or when forwarding is all a corrections-only receiver needs to do.
[[nodiscard]] QString inputProblem(GPSInputProblem problem, std::optional<GPSType> detected, const QString& endpoint,
                                   bool forwardingCorrections);

/// The warning that the receiver produces more output than its connection carries.
[[nodiscard]] QString outputOverflow(RTKSettings::ReceiverRole role, bool compactObservations);

/// The warning that the position a fixed base was given lies @a distanceMeters from the one its receiver computed
/// itself.
[[nodiscard]] QString fixedBaseFar(double distanceMeters);

/// The short label of a fix type; empty for an unknown one.
[[nodiscard]] QString fixLabel(GPSFixQuality fixType);

/// The name of a satellite constellation; "Other" for one the receiver does not identify.
[[nodiscard]] QString constellationName(GPSConstellation constellation);

/// What the receiver status sentence describes.
struct ReceiverStatus
{
    GPSReceiver::State state = GPSReceiver::State::Disconnected;
    /// A connecting receiver is one an Automatic connection has not identified yet.
    bool identifying = false;
    /// What the detection of such a connection looks for now; empty until it says. A rate of zero is not named, as
    /// on a network link.
    std::optional<GPSDetectionProgress> detecting;
    RTKSettings::ReceiverRole role = RTKSettings::ConfiguredBase;
    bool forwardingCorrections = false;
    /// Base mode of a configured base; empty for a passive receiver or without one.
    std::optional<BaseModeDefinition::Mode> baseMode;
    /// What a configured base reports of its base position.
    GPSBaseState baseState = GPSBaseState::Inactive;
    /// The base's corrections wait for a valid base position, so none are forwarded.
    bool correctionsWithheld = false;
    /// A fixed base waits for its receiver's own position, which the fixed one is checked against.
    bool awaitingOwnPosition = false;
};

/// The receiver status sentence; empty when no receiver is connected or being connected.
[[nodiscard]] QString receiverStatus(const ReceiverStatus& status);

}  // namespace GPSReceiverMessages
