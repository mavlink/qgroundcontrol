#include "GPSReceiverMessages.h"

#include <cmath>

#include "FactMetaData.h"
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

/// The status of an Automatic connection that has not identified its receiver yet: what its detection looks for.
QString identifyingStatus(const std::optional<GPSDetectionProgress>& detecting)
{
    const QString family = detecting ? gpsReceiverName(detecting->family) : QString();
    if (family.isEmpty()) {
        // Detection listens for any family, one rate after the other.
        return detecting && detecting->baud
                   //: %1 is a serial baud rate, such as 115200
                   ? GPSReceiver::tr("Identifying receiver at %1 baud...").arg(detecting->baud)
                   : GPSReceiver::tr("Identifying receiver...");
    }
    if (!detecting->remembered) {
        // Detection asks one family after the other at the rate.
        if (!detecting->baud) {
            //: %1 is a manufacturer whose receivers are asked to identify themselves, such as u-blox
            return GPSReceiver::tr("Identifying receiver: trying %1...").arg(family);
        }
        //: %1 is a manufacturer whose receivers are asked to identify themselves, such as u-blox, and %2 the serial
        //: baud rate they are asked at, such as 115200
        return GPSReceiver::tr("Identifying receiver: trying %1 at %2 baud...").arg(family).arg(detecting->baud);
    }
    if (!detecting->baud) {
        //: %1 is the manufacturer of the receiver that was connected here last time, such as u-blox
        return GPSReceiver::tr("Identifying receiver: trying %1, as last time...").arg(family);
    }
    //: %1 is the manufacturer of the receiver that was connected here last time, such as u-blox, and %2 its serial
    //: baud rate, such as 115200
    return GPSReceiver::tr("Identifying receiver: trying %1 at %2 baud, as last time...")
        .arg(family)
        .arg(detecting->baud);
}

/// @a meters in the application's horizontal distance unit, as the GPS panels show a distance (GPSUnits.qml): whole
/// meters or feet, and kilometers or miles from one of those on.
QString distanceText(double meters)
{
    const QString unit = FactMetaData::appSettingsHorizontalDistanceUnitsString();
    const double units = std::round(FactMetaData::metersToAppSettingsHorizontalDistanceUnits(meters).toDouble());
    const bool feet = unit == QLatin1String("ft");
    const double unitsPerLargeUnit = feet ? 5280 : 1000;
    if (units < unitsPerLargeUnit) {
        return QStringLiteral("%1 %2").arg(units, 0, 'f', 0).arg(unit);
    }
    const double largeUnits = units / unitsPerLargeUnit;
    const QString number = QString::number(largeUnits, 'f', largeUnits < 99.95 ? 1 : 0);
    //: %1 is a distance in miles or in kilometers, such as 12.3 or 1250
    return feet ? GPSReceiver::tr("%1 mi").arg(number) : GPSReceiver::tr("%1 km").arg(number);
}

}  // namespace

namespace GPSReceiverMessages {

QString sessionEnded(GPSReceiverSessionOutcome outcome, GPSConnectionError error, const QString& detail,
                     GPSReceiverSessionEnd end)
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
    if (end == GPSReceiverSessionEnd::Unplugged) {
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
            switch (detected.value_or(GPSType::Passive)) {
                case GPSType::UBlox:
                    return GPSReceiver::tr(
                        "Receiving u-blox UBX data without position messages. Enable NAV-PVT or NMEA output.");
                case GPSType::Septentrio:
                    return GPSReceiver::tr(
                        "Receiving Septentrio SBF data without position messages. Enable PVTGeodetic or NMEA "
                        "output.");
                case GPSType::Trimble:
                case GPSType::Femto:
                case GPSType::Unicore:
                case GPSType::Quectel:
                case GPSType::Passive:
                case GPSType::Automatic:
                    return GPSReceiver::tr("Receiving NMEA data without position messages. Enable GGA output.");
            }
            break;
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

QString fixedBaseFar(double distanceMeters)
{
    //: %1 is a distance with its unit, such as 12.4 km
    return GPSReceiver::tr(
               "The fixed base position is %1 from the position the receiver found on its own. Rovers cannot "
               "use corrections for a wrong base position: check its coordinates and their signs, and that "
               "the saved site is this one.")
        .arg(distanceText(distanceMeters));
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

QString constellationName(GPSConstellation constellation)
{
    switch (constellation) {
        case GPSConstellation::GPS:
            return GPSReceiver::tr("GPS");
        case GPSConstellation::GLONASS:
            return GPSReceiver::tr("GLONASS");
        case GPSConstellation::Galileo:
            return GPSReceiver::tr("Galileo");
        case GPSConstellation::BeiDou:
            return GPSReceiver::tr("BeiDou");
        case GPSConstellation::QZSS:
            return GPSReceiver::tr("QZSS");
        case GPSConstellation::SBAS:
            return GPSReceiver::tr("SBAS");
        case GPSConstellation::NavIC:
            return GPSReceiver::tr("NavIC");
        case GPSConstellation::Unknown:
            break;
    }
    return GPSReceiver::tr("Other", "Satellites of a constellation the receiver does not identify");
}

QString receiverStatus(const ReceiverStatus& status)
{
    using Mode = BaseModeDefinition::Mode;
    switch (status.state) {
        case GPSReceiver::State::Disconnected:
            return {};
        case GPSReceiver::State::Reconnecting:
            return GPSReceiver::tr("Connecting automatically...");
        case GPSReceiver::State::Connecting:
            if (status.awaitingOwnPosition) {
                return GPSReceiver::tr(
                    "Waiting for the receiver's own position, to check the fixed position against it...");
            }
            return status.identifying ? identifyingStatus(status.detecting)
                                      : GPSReceiver::tr("Connecting to receiver...");
        case GPSReceiver::State::Connected:
            break;
    }
    if (status.role == RTKSettings::Passive) {
        return status.forwardingCorrections ? GPSReceiver::tr("Passive receiver connected; forwarding its RTCM")
                                            : GPSReceiver::tr("Passive receiver connected; position only");
    }
    const bool determining = status.baseState == GPSBaseState::Active;
    if (status.correctionsWithheld && status.awaitingOwnPosition) {
        return GPSReceiver::tr(
            "Waiting for the receiver's own position, to check the fixed position against it; "
            "the base sends no corrections until then");
    }
    if (status.correctionsWithheld) {
        if (determining && status.baseMode == Mode::BaseSurveyIn) {
            return GPSReceiver::tr("Survey-In active; the base sends no corrections until it completes");
        }
        if (determining && status.baseMode == Mode::BaseReceiverAveraging) {
            return GPSReceiver::tr(
                "Receiver-managed averaging active; the base sends no corrections until it completes");
        }
        return GPSReceiver::tr("Waiting for a valid base position; the base sends no corrections until then");
    }
    if (status.baseMode == Mode::BaseReceiverAveraging) {
        return GPSReceiver::tr("Receiver-managed averaging — no accuracy guarantee");
    }
    if (status.baseMode == Mode::BaseFixed) {
        return GPSReceiver::tr("Fixed position");
    }
    return determining ? GPSReceiver::tr("Survey-In active") : GPSReceiver::tr("Receiver connected");
}

}  // namespace GPSReceiverMessages
