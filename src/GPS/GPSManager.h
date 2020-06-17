/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
    GPSManager(QGCApplication* app, QGCToolbox* toolbox);
    ~GPSManager();

    void connectGPS     (const QString& device, const QString& gps_type);
    void disconnectGPS  (void);
    bool connected      (void) const { return _gpsProvider && _gpsProvider->isRunning(); }

signals:
    void onConnect();
    void onDisconnect();
    void surveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active);
    void satelliteUpdate(int numSats);

private slots:
    void GPSPositionUpdate(GPSPositionMessage msg);
    void GPSSatelliteUpdate(GPSSatelliteMessage msg);

private:
    GPSProvider* _gpsProvider = nullptr;
    RTCMMavlink* _rtcmMavlink = nullptr;

    std::atomic_bool _requestGpsStop; ///< signals the thread to quit
};
