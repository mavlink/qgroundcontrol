#pragma once

#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoPositionInfoSource>

#include "sensor_gps.h"

Q_DECLARE_LOGGING_CATEGORY(GPSPositionSourceLog)

/// Bridges sensor_gps_s updates from GPSProvider into Qt's QGeoPositionInfoSource API,
/// allowing a locally connected GPS receiver to serve as the GCS position source.
class GPSPositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT

public:
    explicit GPSPositionSource(QObject *parent = nullptr);
    ~GPSPositionSource() override;

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const override;
    PositioningMethods supportedPositioningMethods() const override;
    int minimumUpdateInterval() const override;
    Error error() const override;

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

    void updateFromSensorGps(const sensor_gps_s &gpsData);

private:
    QGeoPositionInfo _lastPosition;
    bool _running = false;

    static constexpr int kMinUpdateIntervalMs = 100;
};
