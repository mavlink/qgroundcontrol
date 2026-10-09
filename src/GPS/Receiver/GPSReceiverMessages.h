#pragma once

#include <optional>

#include <QtCore/QString>

#include "GPSConnectionErrors.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverReports.h"
#include "GPSType.h"
#include "RTKSettings.h"

/// The wording of GPSReceiver's errors, warnings and labels, translated in the GPSReceiver context.
namespace GPSReceiverMessages {

/// The error a session leaves when it ends with @a error and the transport's or protocol's @a detail, after the
/// connection policy chose @a outcome.
[[nodiscard]] QString sessionEnded(GPSReceiverSessionOutcome outcome, GPSConnectionError error, const QString& detail,
                                   bool portRemoved);

/// The error of an automatic connection that the @a family receiver refused for lack of flash-save consent.
[[nodiscard]] QString consentRefused(const QString& detail, GPSType family);

/// Why a passive receiver delivers no position, for the protocol @a detected in its output; empty when positions
/// arrive, or when forwarding is all a corrections-only receiver needs to do.
[[nodiscard]] QString inputProblem(GPSInputProblem problem, std::optional<GPSType> detected, const QString& endpoint,
                                   bool forwardingCorrections);

/// The warning that the receiver produces more output than its connection carries.
[[nodiscard]] QString outputOverflow(RTKSettings::ReceiverRole role, bool compactObservations);

/// The short label of a fix type; empty for an unknown one.
[[nodiscard]] QString fixLabel(GPSFixQuality fixType);

/// What the receiver status sentence describes.
struct ReceiverStatus
{
    /// The receiver finished configuration.
    bool configured = false;
    bool hasReceiver = false;
    /// An Automatic connection that has not identified the receiver yet.
    bool identifying = false;
    bool reconnecting = false;
    RTKSettings::ReceiverRole role = RTKSettings::ConfiguredBase;
    bool forwardingCorrections = false;
    int baseMode = -1;
    bool surveyActive = false;
};

/// The receiver status sentence; empty when no receiver is connected or being connected.
[[nodiscard]] QString receiverStatus(const ReceiverStatus& status);

}  // namespace GPSReceiverMessages
