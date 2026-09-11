#include "GPSRecordingValidator.h"

#include <cmath>
#include <limits>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingValidatorLog, "GPS.Recording.GPSRecordingValidator")

GPSRecordingValidator::GPSRecordingValidator()
{
    qCDebug(GPSRecordingValidatorLog) << this;
}

GPSRecordingValidator::~GPSRecordingValidator()
{
    qCDebug(GPSRecordingValidatorLog) << this;
}

namespace {
using K = GPSRecordingEvent::Kind;
using T = GPSRecordingMetadata::Transport;
using W = GPSWriteStatus;

template <typename T>
bool inRange(T value, T minimum, T maximum)
{
    return value >= minimum && value <= maximum;
}
}  // namespace

bool GPSRecordingValidator::check(const GPSRecordingEvent& event, QString& error)
{
    constexpr quint64 maximum = 9000000000000000ULL;
    const auto fail = [&](const char* message) {
        error = QString::fromLatin1(message);
        return false;
    };
    if (!inRange(event.kind(), K::Session, K::BoundedWrite) || event.atUs > maximum || event.stream > maximum)
        return fail("Invalid recording event integer or kind");
    if (event.atUs < _previous || event.startedAtUs > event.atUs)
        return fail("Out-of-order event or invalid operation timing");
    _previous = event.atUs;
    if (event.receivedAtUs() && (event.kind() != K::Rx || *event.receivedAtUs() < -qint64(maximum) ||
                                 *event.receivedAtUs() > qint64(event.atUs)))
        return fail("Invalid producer receipt time");
    if (event.openStatus() && (!inRange(*event.openStatus(), GPSOpenStatus::Opened, GPSOpenStatus::Unsupported) ||
                               (event.kind() != K::Open && event.kind() != K::OpenError) ||
                               ((*event.openStatus() == GPSOpenStatus::Opened) != (event.kind() == K::Open))))
        return fail("Invalid open outcome");
    if (event.readStatus()) {
        const auto status = *event.readStatus();
        if (!inRange(status, GPSReadStatus::Data, GPSReadStatus::InvalidData) ||
            (status == GPSReadStatus::Data        ? event.kind() != K::Rx
             : status == GPSReadStatus::TimedOut  ? event.kind() != K::Timeout
             : status == GPSReadStatus::Cancelled ? event.kind() != K::Cancel
             : status == GPSReadStatus::Closed    ? event.kind() != K::Disconnect
                                                  : event.kind() != K::ReadError))
            return fail("Invalid read outcome");
    }
    if ((event.kind() == K::Rx || event.kind() == K::Tx) && event.bytes().isEmpty())
        return fail("Empty recording payload");
    if (event.resumed() && event.kind() != K::Open)
        return fail("Only open can be resumed");
    if (event.kind() == K::Session) {
        if (_sessions.contains(event.stream))
            return fail("Duplicate stream profile");
        const auto& m = event.metadata();
        if (!m.provenance.valid())
            return fail("Invalid recording provenance");
        const auto& r = m.receiver;
        const auto& b = r.base;
        if (!inRange(m.transport, T::Unknown, T::UdpPeer) || (m.driverType < -1 || m.driverType > 3) ||
            !inRange(r.role, GPSReceiverConfig::Role::RTKBase, GPSReceiverConfig::Role::Position) ||
            !inRange(r.outputProtocol, GPSReceiverConfig::OutputProtocol::Native,
                     GPSReceiverConfig::OutputProtocol::NMEA))
            return fail("Unknown profile enumeration");
        const auto validInteger = [](auto value) { return std::in_range<int>(value) && value >= 0; };
        if (!validInteger(m.initialBaud) || !validInteger(m.fixedBaud) || !validInteger(r.constellationMask) ||
            !validInteger(r.dynamicModel) || !validInteger(r.outputRateHz) || !validInteger(b.surveyInDurationSecs))
            return fail("Invalid profile integer");
        for (double number :
             {double(r.headingOffsetDeg), double(b.surveyInAccMeters), b.fixedBaseLatitude, b.fixedBaseLongitude,
              double(b.fixedBaseAltitudeMeters), double(b.fixedBaseAccuracyMeters)}) {
            if (!std::isfinite(number) || std::abs(number) > std::numeric_limits<float>::max())
                return fail("Invalid base coordinate or accuracy");
        }
        _sessions.insert(event.stream);
    }
    if (event.kind() == K::ConfigurationFinished && (event.value() < 0 || event.value() > 5))
        return fail("Unknown configuration status");
    if (const auto* write = std::get_if<GPSRecordingEvent::BoundedWrite>(&event.payload)) {
        const auto& w = write->result;
        if (!inRange(w.status, W::Completed, W::InvalidData) || w.acceptedBytes < 0 ||
            w.acceptedBytes > event.bytes().size() || w.writtenBytes < 0 || w.writtenBytes > w.acceptedBytes ||
            w.uncertainBytes < 0 || w.uncertainBytes > w.acceptedBytes - w.writtenBytes ||
            ((w.status == W::Unsupported || w.status == W::InvalidData) && w.acceptedBytes != 0) ||
            (w.status == W::Completed && (w.writtenBytes != event.bytes().size() || w.uncertainBytes)))
            return fail("Inconsistent bounded write counts");
    }
    return true;
}
