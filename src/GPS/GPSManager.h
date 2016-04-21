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

#include "GPSProvider.h"
#include "RTCM/RTCMMavlink.h"
#include <QGCToolbox.h>

#include <QString>
#include <QObject>

/**
 ** class GPSManager
 * handles a GPS provider and RTK
 */
class GPSManager : public QGCTool
{
    Q_OBJECT
public:
    GPSManager(QGCApplication* app);
    ~GPSManager();

    void setupGPS(const QString& device);

private slots:
    void GPSPositionUpdate(GPSPositionMessage msg);
    void GPSSatelliteUpdate(GPSSatelliteMessage msg);
private:
    void cleanup();

    GPSProvider* _gpsProvider = nullptr;
    RTCMMavlink* _rtcmMavlink = nullptr;

    std::atomic_bool _requestGpsStop; ///< signals the thread to quit
};
