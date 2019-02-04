/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    QGeoCoordinate gcsPosition(void) { return _gcsPosition; }

    qreal gcsHeading() { return _gcsHeading; }

    void setPositionSource(QGCPositionSource source);

    int updateInterval() const;

    void setToolbox(QGCToolbox* toolbox);

    void setNmeaSourceDevice(QIODevice* device);

private slots:
    void _positionUpdated(const QGeoPositionInfo &update);
    void _error(QGeoPositionInfoSource::Error positioningError);

signals:
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void positionInfoUpdated(QGeoPositionInfo update);

private:
    int             _updateInterval;
    QGeoCoordinate  _gcsPosition;
    qreal           _gcsHeading;

    QGeoPositionInfoSource*     _currentSource;
    QGeoPositionInfoSource*     _defaultSource;
    QNmeaPositionInfoSource*    _nmeaSource;
    QGeoPositionInfoSource*     _simulatedSource;
};
