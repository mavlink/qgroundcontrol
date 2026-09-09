#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoSatelliteInfo>

#include <memory>

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

    QList<QGeoSatelliteInfo> satellitesInView() const { return _satellitesInView; }

    QList<QGeoSatelliteInfo> satellitesInUse() const { return _satellitesInUse; }

    quint64 sessionId() const { return _sessionId; }

    quint64 satellitesReceivedAtUs() const { return _satellitesReceivedAtUs; }

    bool satellitesUsedKnown() const { return _satellitesUsedKnown; }

    QSet<int> satelliteUseSystems() const { return _satelliteUseSystems; }

signals:
    void satellitesChanged();

private:
    void _expireSatellites();

    std::unique_ptr<NMEAStreamSplitter> _stream;
    std::unique_ptr<NMEAPositionSource> _positionSource;
    std::unique_ptr<NMEASatelliteAdapter> _satelliteAdapter;
    std::unique_ptr<QNmeaSatelliteInfoSource> _satelliteSource;
    QTimer _satellitePollTimer;
    GPSSourceHealth _health;
    NMEASatelliteAdapter::Snapshot _viewSnapshot;
    NMEASatelliteAdapter::Snapshot _useSnapshot;
    quint64 _sessionId = 0;
    quint64 _satellitesReceivedAtUs = 0;
    quint64 _viewRejectedThroughUs = 0;
    quint64 _useRejectedThroughUs = 0;
    bool _satellitesUsedKnown = false;
    QSet<int> _satelliteUseSystems;
    bool _refreshingSatellites = false;
    QList<QGeoSatelliteInfo> _satellitesInView;
    QList<QGeoSatelliteInfo> _satellitesInUse;
};
