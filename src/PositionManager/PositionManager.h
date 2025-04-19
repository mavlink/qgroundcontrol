/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(QGCPositionManagerLog)

class QGeoPositionInfoSource;
class QNmeaPositionInfoSource;
class QGCCompass;

class QGCPositionManager : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")

    Q_PROPERTY(QGeoCoordinate gcsPosition                   READ gcsPosition                    NOTIFY gcsPositionChanged)
    Q_PROPERTY(qreal          gcsHeading                    READ gcsHeading                     NOTIFY gcsHeadingChanged)
    Q_PROPERTY(qreal          gcsPositionHorizontalAccuracy READ gcsPositionHorizontalAccuracy  NOTIFY gcsPositionHorizontalAccuracyChanged)

public:
    QGCPositionManager(QObject *parent = nullptr);
    ~QGCPositionManager();

    /// Gets the singleton instance of AudioOutput.
    ///     @return The singleton instance.
    static QGCPositionManager *instance();
    static void registerQmlTypes();

    void init();
    QGeoCoordinate gcsPosition() const { return _gcsPosition; }
    qreal gcsHeading() const { return _gcsHeading; }
    qreal gcsPositionHorizontalAccuracy() const { return _gcsPositionHorizontalAccuracy; }
    QGeoPositionInfo geoPositionInfo() const { return _geoPositionInfo; }
    int updateInterval() const { return _updateInterval; }

    void setNmeaSourceDevice(QIODevice *device);

signals:
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void positionInfoUpdated(QGeoPositionInfo update);
    void gcsPositionHorizontalAccuracyChanged(qreal gcsPositionHorizontalAccuracy);

private slots:
    void _positionUpdated(const QGeoPositionInfo &update);

private:
    enum QGCPositionSource {
        Simulated,
        InternalGPS,
        Log,
        NmeaGPS,
        ExternalGPS
    };

    void _setPositionSource(QGCPositionSource source);
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    void _setGCSHeading(qreal newGCSHeading);
    void _setGCSPosition(const QGeoCoordinate &newGCSPosition);

    bool _usingPluginSource = false;
    int _updateInterval = 0;

    QGeoPositionInfo _geoPositionInfo;
    QGeoCoordinate _gcsPosition;
    qreal _gcsHeading = qQNaN();
    qreal _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsPositionVerticalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsPositionAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsDirectionAccuracy = std::numeric_limits<qreal>::infinity();

    QGeoPositionInfoSource *_currentSource = nullptr;
    QGeoPositionInfoSource *_defaultSource = nullptr;
    QNmeaPositionInfoSource *_nmeaSource = nullptr;
    QGeoPositionInfoSource *_simulatedSource = nullptr;

    QGCCompass *_compass = nullptr;

    static constexpr qreal kMinHorizonalAccuracyMeters = 100.;
    static constexpr qreal kMinVerticalAccuracyMeters = 10.;
    static constexpr qreal kMinDirectionAccuracyDegrees = 30.;
};
