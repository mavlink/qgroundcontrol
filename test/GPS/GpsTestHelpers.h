#pragma once

#include <functional>

#include <QtCore/QByteArray>
#include <QtPositioning/QGeoPositionInfoSource>

#include "RTCM/RTCMTestFixtures.h"

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

}  // namespace GpsTestHelpers
