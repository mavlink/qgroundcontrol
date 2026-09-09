#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoSatelliteInfo>

#include <memory>

#include "GPSSourceHealth.h"

class QIODevice;
class QGeoPositionInfoSource;
class QNmeaSatelliteInfoSource;
class NMEAPositionSource;
class NMEASatelliteAdapter;
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

signals:
    void satellitesChanged();

private:
    std::unique_ptr<NMEAStreamSplitter> _stream;
    std::unique_ptr<NMEAPositionSource> _positionSource;
    std::unique_ptr<NMEASatelliteAdapter> _satelliteAdapter;
    std::unique_ptr<QNmeaSatelliteInfoSource> _satelliteSource;
    QTimer _satellitePollTimer;
    GPSSourceHealth _health;
    QList<QGeoSatelliteInfo> _satellitesInView;
    QList<QGeoSatelliteInfo> _satellitesInUse;
};
