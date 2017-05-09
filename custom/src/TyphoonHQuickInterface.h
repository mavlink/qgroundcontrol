/*!
 * @file
 *   @brief ST16 QtQuick Interface
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "TyphoonHCommon.h"

class TyphoonHM4Interface;
class CameraControl;

//-----------------------------------------------------------------------------
// Vehicle List
class TyphoonSSIDItem : public QObject
{
    Q_OBJECT
public:
    TyphoonSSIDItem(QString ssid, int rssi)
        : _ssid(ssid)
        , _rssi(rssi)
    {
    }

    Q_PROPERTY(QString  ssid    READ ssid                   CONSTANT)
    Q_PROPERTY(int      rssi    READ rssi   WRITE setRssi   NOTIFY rssiChanged)

    QString     ssid        () { return _ssid; }
    int         rssi        () { return _rssi; }

    void        setRssi     (int rssi) { _rssi = rssi; emit rssiChanged(); }

signals:
    void rssiChanged ();

protected:
    QString _ssid;
    int     _rssi;
};

//-----------------------------------------------------------------------------
// QtQuick Interface (UI)
class TyphoonHQuickInterface : public QObject
{
    Q_OBJECT
public:
    TyphoonHQuickInterface(QObject* parent = NULL);
    ~TyphoonHQuickInterface();

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

    Q_PROPERTY(M4State          m4State         READ    m4State             NOTIFY m4StateChanged)
    Q_PROPERTY(QString          m4StateStr      READ    m4StateStr          NOTIFY m4StateChanged)
    Q_PROPERTY(bool             hardwareGPS     READ    hardwareGPS         CONSTANT)
    Q_PROPERTY(double           latitude        READ    latitude            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           longitude       READ    longitude           NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           altitude        READ    altitude            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           speed           READ    altitude            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           gpsCount        READ    gpsCount            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           gpsAccuracy     READ    gpsAccuracy         NOTIFY controllerLocationChanged)
    Q_PROPERTY(CameraControl*   cameraControl   READ    cameraControl       CONSTANT)
    Q_PROPERTY(QVariantList     ssidList        READ    ssidList            NOTIFY ssidListChanged)
    Q_PROPERTY(bool             scanningWiFi    READ    scanningWiFi        NOTIFY scanningWiFiChanged)
    Q_PROPERTY(bool             bindingWiFi     READ    bindingWiFi         NOTIFY bindingWiFiChanged)
    Q_PROPERTY(bool             isTyphoon       READ    isTyphoon           NOTIFY wifiConnectedChanged)
    Q_PROPERTY(bool             connected       READ    connected           NOTIFY wifiConnectedChanged)
    Q_PROPERTY(QString          connectedSSID   READ    connectedSSID       NOTIFY connectedSSIDChanged)
    Q_PROPERTY(QString          connectedCamera READ    connectedCamera     NOTIFY connectedSSIDChanged)
    Q_PROPERTY(int              rssi            READ    rssi                NOTIFY rssiChanged)
    Q_PROPERTY(qreal            rcBattery       READ    rcBattery           NOTIFY rcBatteryChanged)
    Q_PROPERTY(QString          flightTime      READ    flightTime          NOTIFY flightTimeChanged)

    Q_PROPERTY(int              J1              READ    J1                  NOTIFY rawChannelChanged)
    Q_PROPERTY(int              J2              READ    J2                  NOTIFY rawChannelChanged)
    Q_PROPERTY(int              J3              READ    J3                  NOTIFY rawChannelChanged)
    Q_PROPERTY(int              J4              READ    J4                  NOTIFY rawChannelChanged)
    Q_PROPERTY(int              K1              READ    K1                  NOTIFY rawChannelChanged)
    Q_PROPERTY(int              K2              READ    K2                  NOTIFY rawChannelChanged)
    Q_PROPERTY(int              K3              READ    K3                  NOTIFY rawChannelChanged)
    Q_PROPERTY(int              T12             READ    T12                 NOTIFY rawChannelChanged)
    Q_PROPERTY(int              T34             READ    T34                 NOTIFY rawChannelChanged)
    Q_PROPERTY(int              ASwitch         READ    ASwitch             NOTIFY rawChannelChanged)

    Q_INVOKABLE void enterBindMode      ();
    Q_INVOKABLE void initM4             ();
    Q_INVOKABLE void startScan          (int delay = 0);
    Q_INVOKABLE void stopScan           ();
    Q_INVOKABLE void bindWIFI           (QString ssid, QString password);
    Q_INVOKABLE void resetWifi          ();
    Q_INVOKABLE bool isWifiConfigured   (QString ssid);
    Q_INVOKABLE void calibrateGimbalMV  ();
    Q_INVOKABLE int  rawChannels        (int channel);

    M4State     m4State             ();
    QString     m4StateStr          ();
    QString     connectedSSID       ();
    QString     connectedCamera     ();

    CameraControl* cameraControl    ();

#if defined(__androidx86__)
    bool        hardwareGPS         () { return true; }
#else
    bool        hardwareGPS         () { return false; }
#endif

    double      latitude            ();
    double      longitude           ();
    double      altitude            ();
    double      speed               ();
    double      gpsCount            ();
    double      gpsAccuracy         ();
    QVariantList& ssidList          () { return _ssidList; }
    bool        scanningWiFi        () { return _scanningWiFi; }
    bool        bindingWiFi         () { return _bindingWiFi; }
    bool        isTyphoon           ();
    bool        connected           ();
    int         rssi                ();
    qreal       rcBattery           ();
    QString     flightTime          ();

    void        init                (TyphoonHM4Interface* pHandler);

    int         J1                  () { return rawChannels(0); }
    int         J2                  () { return rawChannels(1); }
    int         J3                  () { return rawChannels(2); }
    int         J4                  () { return rawChannels(3); }
    int         K1                  () { return rawChannels(4); }
    int         K2                  () { return rawChannels(5); }
    int         K3                  () { return rawChannels(6); }
    int         T12                 () { return rawChannels(7); }
    int         T34                 () { return rawChannels(8); }
    int         ASwitch             () { return rawChannels(9); }

signals:
    void    m4StateChanged              ();
    void    controllerLocationChanged   ();
    void    ssidListChanged             ();
    void    scanningWiFiChanged         ();
    void    authenticationError         ();
    void    wifiConnectedChanged        ();
    void    connectedSSIDChanged        ();
    void    bindingWiFiChanged          ();
    void    rssiChanged                 ();
    void    bindTimeout                 ();
    void    rcBatteryChanged            ();
    void    flightTimeChanged           ();
    void    rawChannelChanged           ();
    void    powerHeld                   ();

private slots:
    void    _m4StateChanged             ();
    void    _destroyed                  ();
    void    _controllerLocationChanged  ();
    void    _newSSID                    (QString ssid, int rssi);
    void    _newRSSI                    ();
    void    _scanComplete               ();
    void    _authenticationError        ();
    void    _wifiConnected              ();
    void    _wifiDisconnected           ();
    void    _scanWifi                   ();
    void    _delayedBind                ();
    void    _bindTimeout                ();
    void    _batteryUpdate              ();
    void    _armedChanged               (bool armed);
    void    _flightUpdate               ();
    void    _powerTrigger               ();
    void    _rawChannelsChanged         ();
    void    _switchStateChanged         (int swId, int oldState, int newState);

private:
    void    _saveWifiConfigurations     ();
    void    _loadWifiConfigurations     ();

private:
    TyphoonSSIDItem*        _findSsid   (QString ssid, int rssi);
    void                    _clearSSids ();

private:
    TyphoonHM4Interface*    _pHandler;
    QMap<QString, QString>  _configurations;
    QVariantList            _ssidList;
    QString                 _ssid;
    QString                 _password;
    QTimer                  _scanTimer;
    QTimer                  _flightTimer;
    QTimer                  _powerTimer;
    QTime                   _flightTime;
    bool                    _scanEnabled;
    bool                    _scanningWiFi;
    bool                    _bindingWiFi;
};
