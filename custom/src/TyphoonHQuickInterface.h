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
    Q_PROPERTY(QStringList      ssidList        READ    ssidList            NOTIFY ssidListChanged)
    Q_PROPERTY(bool             scanningWiFi    READ    scanningWiFi        NOTIFY scanningWiFiChanged)
    Q_PROPERTY(bool             bindingWiFi     READ    bindingWiFi         NOTIFY bindingWiFiChanged)
    Q_PROPERTY(QString          connectedSSID   READ    connectedSSID       NOTIFY connectedSSIDChanged)
    Q_PROPERTY(QString          connectedCamera READ    connectedCamera     NOTIFY connectedSSIDChanged)
    Q_PROPERTY(int              rssi            READ    rssi                NOTIFY rssiChanged)
    Q_PROPERTY(qreal            rcBattery       READ    rcBattery           NOTIFY rcBatteryChanged)
    Q_PROPERTY(QString          flightTime      READ    flightTime          NOTIFY flightTimeChanged)

    Q_INVOKABLE void enterBindMode  ();
    Q_INVOKABLE void initM4         ();
    Q_INVOKABLE void startScan      (int delay = 0);
    Q_INVOKABLE void stopScan       ();
    Q_INVOKABLE void bindWIFI       (QString ssid, QString password);
    Q_INVOKABLE bool isWIFIConnected();
    Q_INVOKABLE void resetWifi      ();

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
    QStringList ssidList            () { return _ssidList; }
    bool        scanningWiFi        () { return _scanningWiFi; }
    bool        bindingWiFi         () { return _bindingWiFi; }
    int         rssi                ();
    qreal       rcBattery           ();
    QString     flightTime          ();

    void        init                (TyphoonHM4Interface* pHandler);

signals:
    void    m4StateChanged              ();
    void    controllerLocationChanged   ();
    void    ssidListChanged             ();
    void    scanningWiFiChanged         ();
    void    authenticationError         ();
    void    wifiConnected               ();
    void    connectedSSIDChanged        ();
    void    bindingWiFiChanged          ();
    void    rssiChanged                 ();
    void    bindTimeout                 ();
    void    rcBatteryChanged            ();
    void    flightTimeChanged           ();

private slots:
    void    _m4StateChanged             ();
    void    _destroyed                  ();
    void    _controllerLocationChanged  ();
    void    _newSSID                    (QString ssid, int rssi);
    void    _newRSSI                    ();
    void    _scanComplete               ();
    void    _authenticationError        ();
    void    _wifiConnected              ();
    void    _scanWifi                   ();
    void    _delayedBind                ();
    void    _bindTimeout                ();
    void    _batteryUpdate              ();
    void    _armedChanged               (bool armed);
    void    _flightUpdate               ();

private:
    TyphoonHM4Interface*    _pHandler;
    QStringList             _ssidList;
    QString                 _ssid;
    QString                 _password;
    QTimer                  _scanTimer;
    QTimer                  _flightTimer;
    QTime                   _flightTime;
    bool                    _scanEnabled;
    bool                    _scanningWiFi;
    bool                    _bindingWiFi;
};
