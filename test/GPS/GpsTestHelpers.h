#pragma once

#include <QtCore/QByteArray>
#include <QtPositioning/QGeoPositionInfoSource>

#include <cstdint>
#include <functional>

#include "RTCMParser.h"

namespace GpsTestHelpers {

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

// Build a minimal RTCM3 frame with preamble, length, message ID, and CRC-24Q
inline QByteArray buildRtcmFrame(uint16_t messageId, int extraPayloadBytes = 0)
{
    const int payloadLen = 2 + extraPayloadBytes;
    QByteArray frame;

    frame.append(static_cast<char>(RTCMParser::kPreamble));
    frame.append(static_cast<char>((payloadLen >> 8) & 0x03));
    frame.append(static_cast<char>(payloadLen & 0xFF));

    frame.append(static_cast<char>((messageId >> 4) & 0xFF));
    frame.append(static_cast<char>((messageId & 0x0F) << 4));

    for (int i = 0; i < extraPayloadBytes; i++) {
        frame.append(static_cast<char>(i & 0xFF));
    }

    const uint32_t crc =
        RTCMParser::crc24q(reinterpret_cast<const uint8_t*>(frame.constData()), static_cast<size_t>(frame.size()));

    frame.append(static_cast<char>((crc >> 16) & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    frame.append(static_cast<char>(crc & 0xFF));

    return frame;
}

}  // namespace GpsTestHelpers
