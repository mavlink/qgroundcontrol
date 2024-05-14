/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>

#include "QGCToolbox.h"

Q_DECLARE_LOGGING_CATEGORY(QGCPositionManagerLog)

class QGeoPositionInfoSource;
class QNmeaPositionInfoSource;
class QGCCompass;

class QGCPositionManager : public QGCTool
{
    Q_OBJECT

    Q_PROPERTY(QGeoCoordinate gcsPosition                   READ gcsPosition                    NOTIFY gcsPositionChanged)
    Q_PROPERTY(qreal          gcsHeading                    READ gcsHeading                     NOTIFY gcsHeadingChanged)
    Q_PROPERTY(qreal          gcsPositionHorizontalAccuracy READ gcsPositionHorizontalAccuracy  NOTIFY gcsPositionHorizontalAccuracyChanged)

public:
    QGCPositionManager(QGCApplication *app, QGCToolbox *toolbox);
    ~QGCPositionManager();

    void setToolbox(QGCToolbox *toolbox) final;

    enum QGCPositionSource {
        Simulated,
        InternalGPS,
        Log,
        NmeaGPS,
        ExternalGPS
    };

    QGeoCoordinate gcsPosition() const { return m_gcsPosition; }
    qreal gcsHeading() const { return m_gcsHeading; }
    qreal gcsPositionHorizontalAccuracy() const { return m_gcsPositionHorizontalAccuracy; }
    QGeoPositionInfo geoPositionInfo() const { return m_geoPositionInfo; }
    int updateInterval() const { return m_updateInterval; }

    void setNmeaSourceDevice(QIODevice *device);

signals:
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void positionInfoUpdated(QGeoPositionInfo update);
    void gcsPositionHorizontalAccuracyChanged(qreal gcsPositionHorizontalAccuracy);

private slots:
    void _positionUpdated(const QGeoPositionInfo &update);

private:
    void _setPositionSource(QGCPositionSource source);
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    void _setGCSHeading(qreal newGCSHeading);
    void _setGCSPosition(const QGeoCoordinate &newGCSPosition);

    bool m_usingPluginSource = false;
    int m_updateInterval = 0;

    QGeoPositionInfo m_geoPositionInfo;
    QGeoCoordinate m_gcsPosition;
    qreal m_gcsHeading = qQNaN();
    qreal m_gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal m_gcsPositionVerticalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal m_gcsPositionAccuracy = std::numeric_limits<qreal>::infinity();
    qreal m_gcsDirectionAccuracy = std::numeric_limits<qreal>::infinity();

    QGeoPositionInfoSource *m_currentSource = nullptr;
    QGeoPositionInfoSource *m_defaultSource = nullptr;
    QNmeaPositionInfoSource *m_nmeaSource = nullptr;
    QGeoPositionInfoSource *m_simulatedSource = nullptr;

    QGCCompass *m_compass = nullptr;

    static constexpr qreal s_minHorizonalAccuracyMeters = 100.;
    static constexpr qreal s_minVerticalAccuracyMeters = 10.;
    static constexpr qreal s_minDirectionAccuracyDegrees = 30.;
};
