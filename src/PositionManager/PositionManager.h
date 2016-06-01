/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtPositioning/qgeopositioninfosource.h>

#include <QVariant>

#include "QGCToolbox.h"
#include "SimulatedPosition.h"

class QGCPositionManager : public QGCTool {
    Q_OBJECT

public:

    QGCPositionManager(QGCApplication* app);
    ~QGCPositionManager();

    enum QGCPositionSource {
        Simulated,
        GPS,
        Log
    };

    void setPositionSource(QGCPositionSource source);

    int updateInterval() const;

private slots:
    void positionUpdated(const QGeoPositionInfo &update);

signals:
    void lastPositionUpdated(bool valid, QVariant lastPosition);
    void positionInfoUpdated(QGeoPositionInfo update);

private:
    int _updateInterval;
    QGeoPositionInfoSource * _currentSource;
    QGeoPositionInfoSource * _defaultSource;
    QGeoPositionInfoSource * _simulatedSource;
};
