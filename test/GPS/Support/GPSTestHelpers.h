#pragma once

#include <cstdint>
#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtPositioning/QGeoPositionInfoSource>

#include "GPSCorrectionManager.h"
#include "LogManager.h"
#include "MAVLinkLib.h"
#include "MonotonicClock.h"

/// Fakes and helpers the GPS suites, and the vehicle, FollowMe and RemoteID suites, share.
namespace GPSTest {

/// The default scheduler's clock in milliseconds, for stamps that code without an injected scheduler compares.
inline qint64 nowMs()
{
    return static_cast<qint64>(MonotonicClock::nowUs() / 1000);
}

/// A platform position source whose updates and errors the test publishes.
class PositionSource : public QGeoPositionInfoSource
{
public:
    PositionSource()
        : QGeoPositionInfoSource(nullptr)
    {}

    QGeoPositionInfo lastKnownPosition(bool = false) const override { return {}; }

    PositioningMethods supportedPositioningMethods() const override { return SatellitePositioningMethods; }

    int minimumUpdateInterval() const override { return 100; }

    Error error() const override { return NoError; }

    void startUpdates() override { active = true; }

    void stopUpdates() override
    {
        active = false;
        if (onStop) {
            const auto callback = onStop;
            callback();
        }
    }

    void requestUpdate(int = 0) override {}

    void publish(const QGeoPositionInfo& position) { emit positionUpdated(position); }

    void fail(Error error) { emit errorOccurred(error); }

    bool active = false;
    std::function<void()> onStop;
};

/// Delivers the queued signals and invocations already posted on this thread, without running the event loop.
inline void deliverQueuedCalls()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
}

/// The debug messages @a category logged during the current test, in order. Enable the category with
/// TestFixtures::LoggingCategoryFixture first.
inline QStringList debugMessages(const QString& category)
{
    QStringList messages;
    for (const LogEntry& entry : LogManager::capturedMessages(category)) {
        if (entry.level == LogEntry::Debug) {
            messages.append(entry.message);
        }
    }
    return messages;
}

/// GPS_RAW_INT and GPS2_RAW fields; unset fields are zero, as in a default-initialized message.
struct GPSRawFields
{
    int32_t latitudeE7 = 0;
    int32_t longitudeE7 = 0;
    int32_t altitudeMm = 0;
    uint8_t fixType = GPS_FIX_TYPE_NO_FIX;
    uint16_t eph = 0;
    uint16_t epv = 0;
    uint16_t cog = 0;
    uint8_t satellitesVisible = 0;
    uint16_t yaw = 0;
    uint32_t horizontalAccuracyMm = 0;
    uint32_t verticalAccuracyMm = 0;
    uint64_t timeUsec = 0;
};

enum class GPSReceiverIndex
{
    Primary,
    Secondary,
};

inline mavlink_message_t gpsRawMessage(const GPSRawFields& fields,
                                       GPSReceiverIndex receiver = GPSReceiverIndex::Primary, uint8_t systemId = 1,
                                       uint8_t componentId = 1)
{
    const auto fill = [&fields](auto& raw) {
        raw.time_usec = fields.timeUsec;
        raw.lat = fields.latitudeE7;
        raw.lon = fields.longitudeE7;
        raw.alt = fields.altitudeMm;
        raw.fix_type = fields.fixType;
        raw.eph = fields.eph;
        raw.epv = fields.epv;
        raw.cog = fields.cog;
        raw.satellites_visible = fields.satellitesVisible;
        raw.yaw = fields.yaw;
        raw.h_acc = fields.horizontalAccuracyMm;
        raw.v_acc = fields.verticalAccuracyMm;
    };
    mavlink_message_t message{};
    if (receiver == GPSReceiverIndex::Secondary) {
        mavlink_gps2_raw_t raw{};
        fill(raw);
        mavlink_msg_gps2_raw_encode(systemId, componentId, &message, &raw);
    } else {
        mavlink_gps_raw_int_t raw{};
        fill(raw);
        mavlink_msg_gps_raw_int_encode(systemId, componentId, &message, &raw);
    }
    return message;
}

/// Collects the frames @a corrections sends to vehicles; test frames fit one GPS_RTCM_DATA packet.
inline void captureVehicleFrames(GPSCorrectionManager& corrections, QList<QByteArray>& frames)
{
    corrections.rtcmMavlink()->setOutputProvider([&frames]() {
        return QList<RTCMMAVLink::Output>{[&frames](const GPSRTCMPacket& packet) {
            frames.append(packet.data);
            return true;
        }};
    });
}

}  // namespace GPSTest
