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
#include "RTCMMavlink.h"
#include "GPSRTKFactGroup.h"
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

    void setToolbox(QGCToolbox* toolbox) override;

    void connectGPS     (const QString& device, const QString& gps_type);
    void disconnectGPS  (void);
    bool connected      (void) const { return _gpsProvider && _gpsProvider->isRunning(); }
    FactGroup* gpsRtkFactGroup(void) { return _gpsRtkFactGroup; }

signals:
    void onConnect();
    void onDisconnect();
    void surveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active);
    void satelliteUpdate(int numSats);

private slots:
    void GPSPositionUpdate(GPSPositionMessage msg);
    void GPSSatelliteUpdate(GPSSatelliteMessage msg);
    void _onGPSConnect(void);
    void _onGPSDisconnect(void);
    void _gpsSurveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active);
    void _gpsNumSatellites(int numSatellites);

private:
    GPSProvider* _gpsProvider = nullptr;
    RTCMMavlink* _rtcmMavlink = nullptr;
    GPSRTKFactGroup* _gpsRtkFactGroup = nullptr;

    std::atomic_bool _requestGpsStop; ///< signals the thread to quit
};
