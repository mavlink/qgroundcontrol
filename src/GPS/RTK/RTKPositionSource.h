#pragma once

#include <QtCore/QTimer>
#include <QtPositioning/QGeoPositionInfoSource>

#include <optional>

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

    void setUpdateInterval(int msec) override;
    void updatePosition(const sensor_gps_s& fix);
    void reset();

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

private:
    static QGeoPositionInfo _positionInfo(const sensor_gps_s& fix);
    void _emitPendingUpdate();
    void _reportUpdateTimeout();

    QGeoPositionInfo _lastPosition;
    std::optional<sensor_gps_s> _pendingFix;
    QTimer _requestTimer;
    QTimer _updateTimer;
    Error _error = NoError;
    bool _started = false;
    bool _updateTimeoutSent = false;
    bool _noUpdateLastInterval = false;
};
