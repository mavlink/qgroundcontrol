#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoSatelliteInfo>

#include <memory>

#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"
#include "NMEASatelliteAdapter.h"

class QIODevice;
class QGeoPositionInfoSource;
class QNmeaSatelliteInfoSource;
class NMEAPositionSource;
class NMEAStreamSplitter;

/// Owns NMEA decoders and health independently of connection policy and device ownership.
class NMEADecoderSession : public QObject
{
    Q_OBJECT

    friend class NMEASourceManagerTest;

public:
    explicit NMEADecoderSession(QObject* parent = nullptr);
    ~NMEADecoderSession() override;

    bool start(QIODevice* device);
    void stop();
    QGeoPositionInfoSource* positionSource() const;

    GPSSourceHealth* health() { return &_health; }

    const GPSSourceHealth* health() const { return &_health; }

    GPSSatelliteObservation satelliteObservation() const { return _satellites.observation(); }

    quint64 sessionId() const { return _sessionId; }

signals:
    void satellitesChanged();
    void satellitesReceived(const GPSSatelliteObservation& observation);

private:
    void _updateSatellites();

    std::unique_ptr<NMEAStreamSplitter> _stream;
    std::unique_ptr<NMEAPositionSource> _positionSource;
    std::unique_ptr<NMEASatelliteAdapter> _satelliteAdapter;
    std::unique_ptr<QNmeaSatelliteInfoSource> _satelliteSource;
    QTimer _satellitePollTimer;
    GPSSourceHealth _health;
    NMEASatelliteAdapter::Snapshot _viewSnapshot;
    NMEASatelliteAdapter::Snapshot _useSnapshot;
    GPSSatelliteStore _satellites;
    quint64 _sessionId = 0;
};
