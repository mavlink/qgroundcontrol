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

    static  int     byteArrayToInt  (QByteArray data, int offset, bool isBigEndian = false);
    static  short   byteArrayToShort(QByteArray data, int offset, bool isBigEndian = false);

    static TyphoonHM4Interface* pTyphoonHandler;

    //-- From QThread
    void        run     ();

public slots:
    void    softReboot                          ();

private slots:
    void    _bytesReady                         (QByteArray data);
    void    _stateManager                       ();
    void    _initSequence                       ();
    void    _vehicleAdded                       (Vehicle* vehicle);
    void    _vehicleRemoved                     (Vehicle* vehicle);
    void    _vehicleReady                       (bool ready);
    void    _mavlinkMessageReceived             (const mavlink_message_t& message);
    void    _initAndCheckBinding                ();
    void    _rcTimeout                          ();

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
    bool    _sendRxResInfo                      ();
    bool    _sendTableDeviceLocalInfo           (TableDeviceLocalInfo_t localInfo);
    bool    _sendTableDeviceChannelInfo         (TableDeviceChannelInfo_t channelInfo);
    void    _generateTableDeviceLocalInfo       (TableDeviceLocalInfo_t *localInfo);
    bool    _generateTableDeviceChannelInfo     (TableDeviceChannelInfo_t *channelInfo);
    bool    _sendTableDeviceChannelNumInfo      (ChannelNumType_t channelNumTpye);
    bool    _generateTableDeviceChannelNumInfo  (TableDeviceChannelNumInfo_t *channelNumInfo, ChannelNumType_t channelNumTpye, int& num);
    bool    _fillTableDeviceChannelNumMap       (TableDeviceChannelNumInfo_t *channelNumInfo, int num, QByteArray list);
    bool    _setPowerKey                        (int function);
    void    _handleBindResponse                 ();
    void    _handleQueryBindResponse            (QByteArray data);
    bool    _handleNonTypePacket                (m4Packet& packet);
    void    _handleRxBindInfo                   (m4Packet& packet);
    void    _handleChannel                      (m4Packet& packet);
    bool    _handleCommand                      (m4Packet& packet);
    void    _switchChanged                      (m4Packet& packet);
    void    _calibrationStateChanged            (m4Packet& packet);
    void    _handleMixedChannelData             (m4Packet& packet);
    void    _handleRawChannelData               (m4Packet& packet);
    void    _handleControllerFeedback           (m4Packet& packet);
    void    _handlePassThroughPacket            (m4Packet& packet);

signals:
    void    m4StateChanged                      ();
    void    switchStateChanged                  (int swId, int oldState, int newState);
    void    channelDataStatus                   (QByteArray channelData);
    void    controllerLocationChanged           ();
    void    destroyed                           ();
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
    enum {
        STATE_NONE,
        STATE_ENTER_BIND_ERROR,
        STATE_EXIT_RUN,
        STATE_ENTER_BIND,
        STATE_START_BIND,
        STATE_UNBIND,
        STATE_BIND,
        STATE_QUERY_BIND,
        STATE_EXIT_BIND,
        STATE_RECV_BOTH_CH,
        STATE_SET_CHANNEL_SETTINGS,
        STATE_MIX_CHANNEL_DELETE,
        STATE_MIX_CHANNEL_ADD,
        STATE_SEND_RX_INFO,
        STATE_ENTER_RUN,
        STATE_RUNNING
    };
    int                     _state;
    int                     _responseTryCount;
    int                     _currentChannelAdd;
    uint8_t                 _rxLocalIndex;
    uint8_t                 _rxchannelInfoIndex;
    uint8_t                 _channelNumIndex;
    bool                    _sendRxInfoEnd;
    RxBindInfo              _rxBindInfoFeedback;
    QTimer                  _timer;
    QTimer                  _rcTimer;
    ControllerLocation      _controllerLocation;
    bool                    _binding;
    Vehicle*                _vehicle;
    TyphoonHQuickInterface::M4State     _m4State;
    QString                 _currentConnection;
    QList<uint16_t>         _rawChannels;
    uint16_t                _rawChannelsCalibration[CalibrationHwIndexMax];
    uint32_t                _rcTime;
    bool                    _rcActive;
    bool                    _rcCalibrationComplete;
    bool                    _softReboot;
    bool                    _threadShouldStop;
};
