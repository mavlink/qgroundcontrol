#pragma once

#include <QtCore/QObject>

#include <memory>

#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"
#include "NMEASatelliteAdapter.h"

class QIODevice;
class QGeoPositionInfoSource;
class NMEAPositionSource;
class NMEAStreamSplitter;

/// Owns NMEA decoders and health independently of connection policy and device ownership.
class NMEADecoderSession : public QObject
{
    Q_OBJECT

    friend class NMEASourceManagerTest;

public:
    explicit NMEADecoderSession(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NMEADecoderSession() override;

    bool start(QIODevice* device);
    void stop();
    void setFreshnessTimeoutMs(int timeoutMs);
    QGeoPositionInfoSource* positionSource() const;

    GPSSourceHealth* health() { return &_health; }

    const GPSSourceHealth* health() const { return &_health; }

    GPSSatelliteObservation satelliteObservation() const { return _satellites.observation(); }

    quint64 sessionId() const { return _sessionId; }

signals:
    void satellitesChanged();
    void satellitesReceived(const GPSSatelliteObservation& observation);

private:
    void _updateSatellites(const GPSSatelliteObservation& observation);

    QPointer<RuntimeScheduler> _scheduler;

    std::unique_ptr<NMEAStreamSplitter> _stream;
    std::unique_ptr<NMEAPositionSource> _positionSource;
    std::unique_ptr<NMEASatelliteAdapter> _satelliteAdapter;
    GPSSourceHealth _health;
    GPSSatelliteStore _satellites;
    quint64 _sessionId = 0;
    bool _active = false;
};
