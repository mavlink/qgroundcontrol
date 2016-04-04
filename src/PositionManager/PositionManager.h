/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/
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
