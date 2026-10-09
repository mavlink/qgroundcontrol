#include "GPSReceiverMessages.h"

#include "GPSReceiver.h"
#include "GPSReceiverDescriptor.h"

namespace {

/// The error of a session that ended with @a error while a reconnect is pending.
QString reconnectingMessage(GPSConnectionError error, const QString& detail)
{
    switch (error) {
        case GPSConnectionError::OpenFailed:
            return detail.isEmpty()
                       ? GPSReceiver::tr("Failed to open the receiver. Reconnecting automatically.")
                       : GPSReceiver::tr("Failed to open the receiver: %1. Reconnecting automatically.").arg(detail);
        case GPSConnectionError::ConfigFailed:
        case GPSConnectionError::ConsentRequired:
            return detail.isEmpty()
                       ? GPSReceiver::tr("Receiver configuration failed. Reconnecting automatically.")
                       : GPSReceiver::tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail);
        case GPSConnectionError::ProtocolError:
            return detail.isEmpty()
                       ? GPSReceiver::tr("Receiver protocol error. Reconnecting automatically.")
                       : GPSReceiver::tr("Receiver protocol error: %1. Reconnecting automatically.").arg(detail);
        case GPSConnectionError::DeviceError:
        case GPSConnectionError::None:
            break;
    }
    return detail.isEmpty() ? GPSReceiver::tr("Receiver connection lost. Reconnecting automatically.")
                            : GPSReceiver::tr("Receiver connection lost: %1. Reconnecting automatically.").arg(detail);
}

}  // namespace

namespace GPSReceiverMessages {

QString sessionEnded(GPSReceiverSessionOutcome outcome, GPSConnectionError error, const QString& detail,
                     bool portRemoved)
{
    switch (outcome) {
        case GPSReceiverSessionOutcome::Unplugged:
            return GPSReceiver::tr("Receiver unplugged.");
        case GPSReceiverSessionOutcome::AutoRetrying:
        case GPSReceiverSessionOutcome::Retrying:
            return reconnectingMessage(error, detail);
        case GPSReceiverSessionOutcome::WaitingForPort:
            return GPSReceiver::tr("Receiver unplugged. Reconnecting when it is plugged back in.");
        case GPSReceiverSessionOutcome::None:
            break;
    }
    if (portRemoved) {
        return GPSReceiver::tr("Receiver unplugged. Select a device and reconnect.");
    }
    switch (error) {
        case GPSConnectionError::OpenFailed:
            return detail.isEmpty() ? GPSReceiver::tr(
                                          "Failed to open the receiver. Check the device or network "
                                          "address, permissions, and other connections.")
                                    : GPSReceiver::tr("Failed to open the receiver: %1.").arg(detail);
        case GPSConnectionError::ConfigFailed:
        case GPSConnectionError::ConsentRequired:
            return detail.isEmpty()
                       ? GPSReceiver::tr(
                             "Receiver configuration failed. Check the receiver type, baud rate, and base mode.")
                       : GPSReceiver::tr("Receiver configuration failed: %1").arg(detail);
        case GPSConnectionError::ProtocolError:
            //: %1 explains the protocol error
            return detail.isEmpty()
                       ? GPSReceiver::tr("Receiver protocol error. Check the receiver type and reconnect.")
                       : GPSReceiver::tr("Receiver protocol error: %1. Check the receiver type and reconnect.")
                             .arg(detail);
        case GPSConnectionError::DeviceError:
            //: %1 explains why the connection was lost
            return detail.isEmpty()
                       ? GPSReceiver::tr("Receiver connection lost. Check the device and reconnect.")
                       : GPSReceiver::tr("Receiver connection lost: %1. Check the device and reconnect.").arg(detail);
        case GPSConnectionError::None:
            break;
    }
    return {};
}

QString consentRefused(const QString& detail, GPSType family)
{
    //: %1 explains the failure; %2 is a receiver manufacturer, such as Quectel
    return GPSReceiver::tr(
               "Receiver configuration failed: %1. Automatic connections never save settings to "
               "the receiver's flash. To let QGroundControl save the %2 receiver's settings, "
               "connect manually and allow flash save and restart.")
        .arg(detail, gpsReceiverName(family));
}

QString inputProblem(GPSInputProblem problem, std::optional<GPSType> detected, const QString& endpoint,
                     bool forwardingCorrections)
{
    switch (problem) {
        case GPSInputProblem::None:
            break;
        case GPSInputProblem::NoData:
            //: %1 is the receiver's serial device or network address
            return GPSReceiver::tr(
                       "No data from the receiver on %1. Check the device and that no other program is using it.")
                .arg(endpoint);
        case GPSInputProblem::NotGNSS:
            return GPSReceiver::tr("Receiving data that isn't GNSS output. Check the device and baud rate.");
        case GPSInputProblem::NoPositions:
            switch (detected.value_or(GPSType::passive)) {
                case GPSType::ublox:
                    return GPSReceiver::tr(
                        "Receiving u-blox UBX data without position messages. Enable NAV-PVT or NMEA output.");
                case GPSType::septentrio:
                    return GPSReceiver::tr(
                        "Receiving Septentrio SBF data without position messages. Enable PVTGeodetic or NMEA "
                        "output.");
                default:
                    return GPSReceiver::tr("Receiving NMEA data without position messages. Enable GGA output.");
            }
        case GPSInputProblem::CorrectionsOnly:
            // Forwarding them is all a corrections-only receiver needs to do.
            return forwardingCorrections ? QString()
                                         : GPSReceiver::tr(
                                               "Receiving RTCM corrections without position messages. Turn on "
                                               "Forward receiver RTCM to send them to vehicles.");
    }
    return {};
}

QString outputOverflow(RTKSettings::ReceiverRole role, bool compactObservations)
{
    if (role == RTKSettings::Passive) {
        return GPSReceiver::tr(
            "The receiver produces more output than the connection carries, so data is being lost. Use a "
            "higher baud rate or reduce the receiver's output.");
    }
    if (compactObservations) {
        return GPSReceiver::tr(
            "The receiver produces more output than the connection carries, so corrections are being lost. "
            "Use a higher baud rate.");
    }
    //: "Compact RTCM corrections (MSM4)" is the label of the base receiver setting
    return GPSReceiver::tr(
        "The receiver produces more output than the connection carries, so corrections are being lost. "
        "Use a higher baud rate or turn on Compact RTCM corrections (MSM4).");
}

QString fixLabel(GPSFixQuality fixType)
{
    switch (fixType) {
        case GPSFixQuality::NoFix:
            return GPSReceiver::tr("No fix");
        case GPSFixQuality::Fix2D:
            return GPSReceiver::tr("2D");
        case GPSFixQuality::Fix3D:
            return GPSReceiver::tr("3D");
        case GPSFixQuality::Differential:
            return GPSReceiver::tr("DGPS");
        case GPSFixQuality::RTKFloat:
            return GPSReceiver::tr("Float", "RTK float fix");
        case GPSFixQuality::RTKFixed:
            return GPSReceiver::tr("Fixed", "RTK fixed fix");
        case GPSFixQuality::Extrapolated:
            return GPSReceiver::tr("DR", "Dead reckoning (extrapolated) fix");
        case GPSFixQuality::Unknown:
            break;
    }
    return {};
}

QString receiverStatus(const ReceiverStatus& status)
{
    using Mode = BaseModeDefinition::Mode;
    if (!status.configured) {
        if (status.hasReceiver) {
            return status.identifying ? GPSReceiver::tr("Identifying receiver...")
                                      : GPSReceiver::tr("Connecting to receiver...");
        }
        return status.reconnecting ? GPSReceiver::tr("Connecting automatically...") : QString();
    }
    if (status.role == RTKSettings::Passive) {
        return status.forwardingCorrections ? GPSReceiver::tr("Passive receiver connected; forwarding its RTCM")
                                            : GPSReceiver::tr("Passive receiver connected; position only");
    }
    if (status.baseMode == static_cast<int>(Mode::BaseReceiverAveraging)) {
        return GPSReceiver::tr("Receiver-managed averaging — no accuracy guarantee");
    }
    if (status.baseMode == static_cast<int>(Mode::BaseFixed)) {
        return GPSReceiver::tr("Fixed position");
    }
    return status.surveyActive ? GPSReceiver::tr("Survey-In active") : GPSReceiver::tr("Receiver connected");
}

}  // namespace GPSReceiverMessages
