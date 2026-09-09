#pragma once

#include <QtCore/QTimer>
#include <QtPositioning/QGeoPositionInfoSource>

#include <optional>

#include "GPSObservation.h"

/// Deterministic Qt source fixture publishing the ground-station acceptance policy.
class TestGPSPositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT

public:
    explicit TestGPSPositionSource(QObject* parent = nullptr);
    ~TestGPSPositionSource() override;

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const override;

    PositioningMethods supportedPositioningMethods() const override { return SatellitePositioningMethods; }

    int minimumUpdateInterval() const override { return 0; }

    Error error() const override { return _error; }

    void setUpdateInterval(int msec) override;
    void updatePosition(const GPSObservation& fix);
    void reset();

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

private:
    static QGeoPositionInfo _positionInfo(const GPSObservation& fix);
    void _emitPendingUpdate();
    void _reportUpdateTimeout();

    QGeoPositionInfo _lastPosition;
    std::optional<GPSObservation> _pendingFix;
    QTimer _requestTimer;
    QTimer _updateTimer;
    Error _error = NoError;
    bool _started = false;
    bool _updateTimeoutSent = false;
    bool _noUpdateLastInterval = false;
};
