/*!
 * @file
 *   @brief ST16 Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "m4common.h"
#include "m4def.h"
#include "m4util.h"
#include "camera.h"

#include "Vehicle.h"

class QGCToolbox;
class TyphoonM4Handler;
class CameraControl;

//-----------------------------------------------------------------------------
// QtQuick Interface (UI)
class TyphoonHQuickInterface : public QObject
{
    Q_OBJECT
public:
    TyphoonHQuickInterface(QObject* parent = NULL);
    ~TyphoonHQuickInterface() {}

    //-- QtQuick Interface
    enum M4State {
        M4_STATE_NONE           = 0,
        M4_STATE_AWAIT          = 1,
        M4_STATE_BIND           = 2,
        M4_STATE_CALIBRATION    = 3,
        M4_STATE_SETUP          = 4,
        M4_STATE_RUN            = 5,
        M4_STATE_SIM            = 6,
        M4_STATE_FACTORY_CALI   = 7
    };

    Q_ENUMS(M4State)

    Q_PROPERTY(M4State          m4State         READ    m4State                             NOTIFY m4StateChanged)
    Q_PROPERTY(QString          m4StateStr      READ    m4StateStr                          NOTIFY m4StateChanged)
    Q_PROPERTY(bool             hardwareGPS     READ    hardwareGPS                         CONSTANT)
    Q_PROPERTY(double           latitude        READ    latitude                            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           longitude       READ    longitude                           NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           altitude        READ    altitude                            NOTIFY controllerLocationChanged)
    Q_PROPERTY(CameraControl*   cameraControl   READ    cameraControl                       CONSTANT)
    Q_PROPERTY(QStringList      ssidList        READ    ssidList                            NOTIFY ssidListChanged)
    Q_PROPERTY(bool             scanningWiFi    READ    scanningWiFi                        NOTIFY scanningWiFiChanged)

    Q_INVOKABLE void enterBindMode  ();
    Q_INVOKABLE void initM4         ();
    Q_INVOKABLE void startScan      ();
    Q_INVOKABLE void stopScan       ();
    Q_INVOKABLE void bindWIFI       (QString ssid);
    Q_INVOKABLE bool isWIFIConnected();

    M4State     m4State             ();
    QString     m4StateStr          ();

    CameraControl* cameraControl    ();

#if defined(__androidx86__)
    bool        hardwareGPS         () { return true; }
#else
    bool        hardwareGPS         () { return false; }
#endif

    double      latitude            ();
    double      longitude           ();
    double      altitude            ();
    QStringList ssidList            () { return _ssidList; }
    bool        scanningWiFi        () { return _scanningWiFi; }

    void    init                    (TyphoonM4Handler* pHandler);

signals:
    void    m4StateChanged              ();
    void    controllerLocationChanged   ();
    void    ssidListChanged             ();
    void    scanningWiFiChanged         ();

private slots:
    void    _m4StateChanged             ();
    void    _destroyed                  ();
    void    _controllerLocationChanged  ();
    void    _newSSID                    (QString ssid);

private:
    TyphoonM4Handler*   _pHandler;
    QStringList         _ssidList;
    bool                _scanningWiFi;
};

//-----------------------------------------------------------------------------
// M4 Handler
class TyphoonM4Handler : public QObject
{
    Q_OBJECT
public:
    TyphoonM4Handler(QObject* parent = NULL);
    ~TyphoonM4Handler();

    void    init                    ();
    bool    vehicleReady            ();
    void    enterBindMode           ();
    void    initM4                  ();

    CameraControl* cameraControl    () { return _cameraControl; }

    TyphoonHQuickInterface::M4State     m4State             () { return _m4State; }
    const ControllerLocation&           controllerLocation  () { return _controllerLocation; }

    static  int     byteArrayToInt  (QByteArray data, int offset, bool isBigEndian = false);
    static  short   byteArrayToShort(QByteArray data, int offset, bool isBigEndian = false);

public slots:
    void    softReboot                          ();

private slots:
    void    _bytesReady                         (QByteArray data);
    void    _stateManager                       ();
    void    _initSequence                       ();
    void    _vehicleAdded                       (Vehicle* vehicle);
    void    _vehicleRemoved                     (Vehicle* vehicle);
    void    _vehicleReady                       (bool ready);
    void    _httpFinished                       ();

private:
    bool    _enterRun                           ();
    bool    _exitRun                            ();
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
    void    _handleMixedChannelData             (m4Packet& packet);
    void    _handControllerFeedback             (m4Packet& packet);
    void    _handleInitialState                 ();
    void    _initStreaming                      ();

signals:
    void    m4StateChanged                      ();
    void    switchStateChanged                  (int swId, int oldState, int newState);
    void    channelDataStatus                   (QByteArray channelData);
    void    controllerLocationChanged           ();
    void    destroyed                           ();
    void    newWifiSSID                         (QString ssid);

private:
    M4SerialComm* _commPort;
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
    ControllerLocation      _controllerLocation;
    bool                    _binding;
    Vehicle*                _vehicle;
    QNetworkAccessManager*  _networkManager;
    CameraControl*          _cameraControl;
    TyphoonHQuickInterface::M4State     _m4State;
};
