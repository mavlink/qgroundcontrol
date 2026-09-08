#pragma once

#include <QtCore/QTimer>
#include <QtPositioning/QGeoPositionInfoSource>

#include "sensor_gps.h"

/// Adapts decoded receiver fixes without opening or configuring another connection.
class RTKPositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT

public:
    explicit RTKPositionSource(QObject* parent = nullptr);
    ~RTKPositionSource() override;

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const override;

    PositioningMethods supportedPositioningMethods() const override { return SatellitePositioningMethods; }

    int minimumUpdateInterval() const override { return 0; }

    Error error() const override { return _error; }

    void updatePosition(const sensor_gps_s& fix);
    void reset();

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

private:
    static QGeoPositionInfo _positionInfo(const sensor_gps_s& fix);

    QGeoPositionInfo _lastPosition;
    QTimer _requestTimer;
    Error _error = NoError;
    bool _started = false;
};
