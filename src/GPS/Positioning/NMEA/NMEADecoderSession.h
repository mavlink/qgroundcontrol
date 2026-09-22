#pragma once

#include <memory>

#include <QtCore/QObject>

#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentenceEnvelope.h"

class QIODevice;
class QGeoPositionInfoSource;
class NMEAPositionSource;
class NMEAStreamSplitter;

/// Owns NMEA decoders and health independently of connection policy and device ownership.
class NMEADecoderSession : public QObject
{
    Q_OBJECT
    friend class NMEASatelliteAdapterTest;

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

    bool receiving() const { return _receiving; }

    bool hasReceivedData() const { return _lastDataTimestampUs != 0; }

    quint64 sessionId() const { return _sessionId; }

signals:
    void activityChanged();
    void satellitesReceived(const GPSSatelliteObservation& observation);

private:
    void _receivedData(quint64 receivedAtUs);
    void _updateSatellites(const GPSSatelliteObservation& observation);
    void _ingestSatellites(const NMEASentenceEnvelope& sentence);
    void _closeSatellites();
    void _flushSatellites();
    void _scheduleSatelliteFlush();
    void _queueSatellites(NMEA::SatelliteEpoch epoch);
    void _deliverSatellites();

    QPointer<RuntimeScheduler> _scheduler;

    struct Decoders
    {
        std::unique_ptr<NMEAStreamSplitter> stream;
        std::unique_ptr<NMEAPositionSource> position;
    } _decoders;

    NMEA::SatelliteAssembler _satelliteAssembler;
    QList<GPSSatelliteObservation> _pendingSatellites;
    ScheduledTask _satelliteFlushTask;
    ScheduledTask _satelliteDeliveryTask;
    bool _satellitesOpen = false;
    GPSSourceHealth _health;
    GPSSatelliteStore _satellites;
    ScheduledTask _activityTask;
    quint64 _lastDataTimestampUs = 0;
    bool _receiving = false;
    quint64 _sessionId = 0;
    bool _active = false;
};
