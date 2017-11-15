/*!
 * @file
 *   @brief ST16 Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "TyphoonHCommon.h"
#include "TyphoonHQuickInterface.h"

#include "m4lib/m4lib.h"

#include "m4def.h"
#include "m4util.h"

#include "Vehicle.h"

//-----------------------------------------------------------------------------
// M4 Handler
class TyphoonHM4Interface : public QThread
{
    Q_OBJECT
public:
    TyphoonHM4Interface(QObject* parent = NULL);
    ~TyphoonHM4Interface();

    void    init                    (bool skipConnections = false);
    bool    vehicleReady            ();
    void    enterBindMode           (bool skipPairCommand = false);
    void    initM4                  ();
    QString m4StateStr              ();
    void    resetBind               ();
    bool    rcActive                ();
    bool    rcCalibrationComplete   ();
    void    startCalibration        ();
    void    stopCalibration         ();
    bool    sendPassThroughMessage  (QByteArray message);


    QList<uint16_t>     rawChannels     ();
    int                 calChannel      (int index);

    TyphoonHQuickInterface::M4State     m4State             ();

    const ControllerLocation&           controllerLocation  ();

    static TyphoonHM4Interface* pTyphoonHandler;

    //-- From QThread
    void        run     ();

public slots:
    void    softReboot                          ();

private slots:
    void    _vehicleAdded                       (Vehicle* vehicle);
    void    _vehicleRemoved                     (Vehicle* vehicle);
    void    _vehicleReady                       (bool ready);
    void    _mavlinkMessageReceived             (const mavlink_message_t& message);
    void    _rcTimeout                          ();
    void    _rcActiveChanged                    ();
    void    _enterBindMode                      ();
    void    _saveSettings                       (const RxBindInfo& rxBindInfo);

private:
    bool    _exitToAwait                        ();
    bool    _enterRun                           ();
    bool    _exitRun                            ();
    bool    _enterFactoryCalibration            ();
    bool    _exitFactoryCalibration             ();
    bool    _startBind                          ();
    bool    _enterBind                          ();
    bool    _exitBind                           ();
    bool    _bind                               (int rxAddr);
    bool    _unbind                             ();
    void    _checkExitRun                       ();
    bool    _queryBindState                     ();
    bool    _sendRecvBothCh                     ();
    bool    _setChannelSetting                  ();
    bool    _syncMixingDataDeleteAll            ();
    bool    _syncMixingDataAdd                  ();
    bool    _sendTableDeviceLocalInfo           (TableDeviceLocalInfo_t localInfo);
    bool    _sendTableDeviceChannelInfo         (TableDeviceChannelInfo_t channelInfo);
    bool    _sendTableDeviceChannelNumInfo      (ChannelNumType_t channelNumTpye);
    bool    _setPowerKey                        (int function);

signals:
    void    m4StateChanged                      ();
    void    switchStateChanged                  (int swId, int oldState, int newState);
    void    channelDataStatus                   (QByteArray channelData);
    void    destroyed                           ();
    void    controllerLocationChanged           ();
    void    armedChanged                        (bool armed);
    void    rawChannelsChanged                  ();
    void    calibrationCompleteChanged          ();
    void    calibrationStateChanged             ();
    void    rcActiveChanged                     ();
    void    distanceSensor                      (int minDist, int maxDist, int curDist);
    //-- WIFI
    void    newWifiSSID                         (QString ssid, int rssi);
    void    newWifiRSSI                         ();
    void    scanComplete                        ();
    void    authenticationError                 ();
    void    wifiConnected                       ();
    void    wifiDisconnected                    ();
    void    batteryUpdate                       ();

private:
    M4Lib* _m4Lib;
    Vehicle*                _vehicle;
    QString                 _currentConnection;
    uint32_t                _rcTime;
    bool                    _threadShouldStop;
    QTimer                  _rcTimer;
};
