#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSSourceHealth.h"

class QGCCompass;

class QGCPositionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(GPSSourceHealth* sourceHealth READ sourceHealth NOTIFY sourceHealthChanged)
    Q_PROPERTY(QGeoCoordinate gcsPosition                   READ gcsPosition                    NOTIFY gcsPositionChanged)
    Q_PROPERTY(qreal          gcsHeading                    READ gcsHeading                     NOTIFY gcsHeadingChanged)
    Q_PROPERTY(qreal          gcsPositionHorizontalAccuracy READ gcsPositionHorizontalAccuracy  NOTIFY gcsPositionHorizontalAccuracyChanged)

    friend class PositionManagerTest;

public:
    explicit QGCPositionManager(QObject *parent = nullptr);
    ~QGCPositionManager();

    /// Gets the singleton instance of AudioOutput.
    ///     @return The singleton instance.
    static QGCPositionManager *instance();

    void init();

    GPSSourceHealth* sourceHealth() const { return _currentHealth; }
    QGeoCoordinate gcsPosition() const { return _gcsPosition; }
    qreal gcsHeading() const { return _gcsHeading; }
    qreal gcsPositionHorizontalAccuracy() const { return _gcsPositionHorizontalAccuracy; }
    QGeoPositionInfo geoPositionInfo() const { return _geoPositionInfo; }

    std::optional<GPSObservation> acceptedObservation(GPSObservation::PositionUse use) const;
    QGeoPositionInfoSource::Error gcsPositioningError() const { return _gcsPositioningError; }

    /// Local arrival time of the last position update which passed the accuracy gates and was
    /// copied into gcsPosition. Invalid until the first such update arrives. This is the local
    /// clock rather than the position source's own timestamp, which on some platforms (e.g.
    /// Android) is offset from the system clock.
    ///     @return Arrival time, in UTC, of the last position update applied to gcsPosition.
    QDateTime gcsPositionTimestamp() const { return _gcsPositionTimestamp; }

    int updateInterval() const { return _updateInterval; }

    /// Select a borrowed, connected receiver source ahead of NMEA and platform positioning.
    void setReceiverPositionSource(QGeoPositionInfoSource* source, GPSSourceHealth* health = nullptr);
    void clearReceiverPositionSource(QGeoPositionInfoSource* source);
    /// Borrow an NMEA position source; its owner manages the decoder and input device.
    void setNmeaPositionSource(QGeoPositionInfoSource* source, GPSSourceHealth* health = nullptr);
    void clearNmeaPositionSource(QGeoPositionInfoSource* source);

signals:
    void sourceHealthChanged();
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void positionInfoUpdated(QGeoPositionInfo update);
    void gcsPositionHorizontalAccuracyChanged(qreal gcsPositionHorizontalAccuracy);

private slots:
    void _positionUpdated(const QGeoPositionInfo &update);
    void _positionError(QGeoPositionInfoSource::Error gcsPositioningError);

private:
    enum QGCPositionSource {
        Simulated,
        InternalGPS,
        Log,
        NmeaGPS,
        ExternalGPS
    };

    void _setPositionSource(QGCPositionSource source);
    void _selectPositionSource();
    bool _isExternalSource() const;
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    void _clearPosition();
    void _externalPositionChanged();
    void _publishPosition(const std::optional<GPSObservation>& observation);

    bool _usingPluginSource = false;
    int _updateInterval = 0;
    GPSSourceHealth _externalHealth;
    QPointer<GPSSourceHealth> _receiverHealth;
    QPointer<GPSSourceHealth> _nmeaHealth;
    QPointer<GPSSourceHealth> _currentHealth;
    QMetaObject::Connection _healthConnection;
    QMetaObject::Connection _healthDestroyedConnection;

    QGeoPositionInfo _geoPositionInfo;
    QGeoPositionInfoSource::Error  _gcsPositioningError = QGeoPositionInfoSource::NoError;

    QGeoCoordinate _gcsPosition;
    QDateTime _gcsPositionTimestamp;
    qreal _gcsHeading = qQNaN();
    qreal _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsPositionVerticalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsPositionAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsDirectionAccuracy = std::numeric_limits<qreal>::infinity();

    QPointer<QGeoPositionInfoSource> _receiverSource;
    QMetaObject::Connection _receiverDestroyedConnection;
    quint64 _sourceGeneration = 0;
    quint64 _positionRevision = 0;
    QMetaObject::Connection _nmeaDestroyedConnection;
    QMetaObject::Connection _positionUpdateConnection;
    QMetaObject::Connection _positionErrorConnection;
    QGeoPositionInfoSource *_currentSource = nullptr;
    QGeoPositionInfoSource *_defaultSource = nullptr;
    QPointer<QGeoPositionInfoSource> _nmeaSource;
    QGeoPositionInfoSource *_simulatedSource = nullptr;

    QGCCompass *_compass = nullptr;

};
