/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QGeoPositionInfoSource>
#include <QNmeaPositionInfoSource>

#include <QVariant>

#include "QGCToolbox.h"
#include "SimulatedPosition.h"

class QGCPositionManager : public QGCTool {
    Q_OBJECT

public:

    QGCPositionManager(QGCApplication* app, QGCToolbox* toolbox);
    ~QGCPositionManager();

    Q_PROPERTY(QGeoCoordinate gcsPosition  READ gcsPosition  NOTIFY gcsPositionChanged)
    Q_PROPERTY(qreal          gcsHeading   READ gcsHeading   NOTIFY gcsHeadingChanged)

    enum QGCPositionSource {
        Simulated,
        InternalGPS,
        Log,
        NmeaGPS
    };

    QGeoCoordinate      gcsPosition         (void) { return _gcsPosition; }
    qreal               gcsHeading          (void) { return _gcsHeading; }
    QGeoPositionInfo    geoPositionInfo     (void) const { return _geoPositionInfo; }
    void                setPositionSource   (QGCPositionSource source);
    int                 updateInterval      (void) const;
    void                setNmeaSourceDevice (QIODevice* device);

    // Overrides from QGCTool
    void setToolbox(QGCToolbox* toolbox) override;


private slots:
    void _positionUpdated(const QGeoPositionInfo &update);
    void _error(QGeoPositionInfoSource::Error positioningError);

signals:
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void positionInfoUpdated(QGeoPositionInfo update);

private:
    int                 _updateInterval =   0;
    QGeoPositionInfo    _geoPositionInfo;
    QGeoCoordinate      _gcsPosition;
    qreal               _gcsHeading =       qQNaN();

    QGeoPositionInfoSource*     _currentSource =        nullptr;
    QGeoPositionInfoSource*     _defaultSource =        nullptr;
    QNmeaPositionInfoSource*    _nmeaSource =           nullptr;
    QGeoPositionInfoSource*     _simulatedSource =      nullptr;
    bool                        _usingPluginSource =    false;
};
