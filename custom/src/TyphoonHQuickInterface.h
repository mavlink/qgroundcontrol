/*!
 * @file
 *   @brief ST16 QtQuick Interface
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "TyphoonHCommon.h"
#include "VideoReceiver.h"

class TyphoonHM4Interface;

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
// File Copy
class TyphoonHFileCopy : public QObject
{
    Q_OBJECT
public:
    TyphoonHFileCopy    (const QString& src, const QString& dst)
        : QObject(NULL)
        , _src(src)
        , _dst(dst)
    {
    }
signals:
    void    copyProgress    (int current);
    void    copyError       (QString errorMsg);
    void    copyDone        ();
public slots:
    void    startCopy       ();
private:
    QString _src;
    QString _dst;
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
        M4_STATE_FACTORY_CAL    = 7
    };

    enum CalibrationState {
        CalibrationStateNone = 0,
        CalibrationStateMin,
        CalibrationStateMid,
        CalibrationStateMax,
        CalibrationStateRag,
    };

    Q_ENUMS(M4State)
    Q_ENUMS(CalibrationState)

    Q_PROPERTY(M4State          m4State         READ    m4State             NOTIFY m4StateChanged)
    Q_PROPERTY(QString          m4StateStr      READ    m4StateStr          NOTIFY m4StateChanged)
    Q_PROPERTY(bool             hardwareGPS     READ    hardwareGPS         CONSTANT)
    Q_PROPERTY(double           latitude        READ    latitude            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           longitude       READ    longitude           NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           altitude        READ    altitude            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           speed           READ    altitude            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           gpsCount        READ    gpsCount            NOTIFY controllerLocationChanged)
    Q_PROPERTY(double           gpsAccuracy     READ    gpsAccuracy         NOTIFY controllerLocationChanged)
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
    Q_PROPERTY(bool             copyingFiles    READ    copyingFiles        NOTIFY copyingFilesChanged)
    Q_PROPERTY(int              copyResult      READ    copyResult          NOTIFY copyResultChanged)

    Q_PROPERTY(bool             wifiAlertEnabled    READ    wifiAlertEnabled    WRITE   setWifiAlertEnabled NOTIFY  wifiAlertEnabledChanged)

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

    Q_PROPERTY(int              J1Cal           READ    J1Cal               NOTIFY calibrationStateChanged)
    Q_PROPERTY(int              J2Cal           READ    J2Cal               NOTIFY calibrationStateChanged)
    Q_PROPERTY(int              J3Cal           READ    J3Cal               NOTIFY calibrationStateChanged)
    Q_PROPERTY(int              J4Cal           READ    J4Cal               NOTIFY calibrationStateChanged)
    Q_PROPERTY(int              K1Cal           READ    K1Cal               NOTIFY calibrationStateChanged)
    Q_PROPERTY(int              K2Cal           READ    K2Cal               NOTIFY calibrationStateChanged)
    Q_PROPERTY(int              K3Cal           READ    K3Cal               NOTIFY calibrationStateChanged)

    Q_PROPERTY(bool             calibrationComplete     READ    calibrationComplete NOTIFY calibrationCompleteChanged)
    Q_PROPERTY(bool             rcActive                READ    rcActive            NOTIFY rcActiveChanged)

    Q_PROPERTY(QString          updateError     READ    updateError         NOTIFY updateErrorChanged)
    Q_PROPERTY(int              updateProgress  READ    updateProgress      NOTIFY updateProgressChanged)
    Q_PROPERTY(bool             updateDone      READ    updateDone          NOTIFY updateDoneChanged)
    Q_PROPERTY(bool             updating        READ    updating            NOTIFY updatingChanged)

    Q_PROPERTY(VideoReceiver*   videoReceiver       READ    videoReceiver       CONSTANT)
    Q_PROPERTY(bool             thermalImagePresent READ    thermalImagePresent NOTIFY thermalImagePresentChanged)

    Q_INVOKABLE void enterBindMode      ();
    Q_INVOKABLE void initM4             ();
    Q_INVOKABLE void startScan          (int delay = 0);
    Q_INVOKABLE void stopScan           ();
    Q_INVOKABLE void bindWIFI           (QString ssid, QString password);
    Q_INVOKABLE void resetWifi          ();
    Q_INVOKABLE bool isWifiConfigured   (QString ssid);
    Q_INVOKABLE int  rawChannel         (int channel);
    Q_INVOKABLE int  calChannelState    (int channel);
    Q_INVOKABLE void exportData         ();
    Q_INVOKABLE void importMission      ();
    Q_INVOKABLE void manualBind         ();
    Q_INVOKABLE void startCalibration   ();
    Q_INVOKABLE void stopCalibration    ();
    Q_INVOKABLE void setWiFiPassword    (QString pwd);

    Q_INVOKABLE QStringList getMediaList();

    //-- Android image update
    Q_INVOKABLE bool checkForUpdate     ();
    Q_INVOKABLE void updateSystemImage  ();

    M4State     m4State             ();
    QString     m4StateStr          ();
    QString     connectedSSID       ();
    QString     connectedCamera     ();

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
    bool        copyingFiles        () { return _copyingFiles; }
    int         rssi                ();
    qreal       rcBattery           ();
    QString     flightTime          ();
    int         copyResult          () { return _copyResult; }
    bool        wifiAlertEnabled    () { return _wifiAlertEnabled; }
    bool        rcActive            ();

    void        init                (TyphoonHM4Interface* pHandler);
    void        setWifiAlertEnabled (bool enabled) { _wifiAlertEnabled = enabled; emit wifiAlertEnabledChanged(); }

    int         J1                  () { return rawChannel(0); }
    int         J2                  () { return rawChannel(1); }
    int         J3                  () { return rawChannel(2); }
    int         J4                  () { return rawChannel(3); }
    int         K1                  () { return rawChannel(4); }
    int         K2                  () { return rawChannel(5); }
    int         K3                  () { return rawChannel(6); }
    int         T12                 () { return rawChannel(7); }
    int         T34                 () { return rawChannel(8); }
    int         ASwitch             () { return rawChannel(9); }

    int         J1Cal               () { return calChannelState(0); }
    int         J2Cal               () { return calChannelState(1); }
    int         J3Cal               () { return calChannelState(2); }
    int         J4Cal               () { return calChannelState(3); }
    int         K1Cal               () { return calChannelState(4); }
    int         K2Cal               () { return calChannelState(5); }
    int         K3Cal               () { return calChannelState(6); }

    bool        calibrationComplete ();
    bool        thermalImagePresent ();

    QString     updateError         () { return _updateError; }
    int         updateProgress      () { return _updateProgress; }
    bool        updateDone          () { return _updateDone; }
    bool        updating            () { return _pFileCopy != NULL; }

    VideoReceiver*  videoReceiver   () { return _videoReceiver; }

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
    void    copyingFilesChanged         ();
    void    copyResultChanged           ();
    void    calibrationCompleteChanged  ();
    void    calibrationStateChanged     ();
    void    wifiAlertEnabledChanged     ();
    void    rcActiveChanged             ();
    void    updateErrorChanged          ();
    void    updateProgressChanged       ();
    void    updateDoneChanged           ();
    void    updatingChanged             ();
    void    thermalImagePresentChanged  ();
    void    updateAlert                 ();

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
    void    _exportData                 ();
    void    _importMissions             ();
    void    _calibrationCompleteChanged ();
    void    _calibrationStateChanged    ();
    void    _rcActiveChanged            ();
    void    _imageUpdateProgress        (int current);
    void    _imageUpdateError           (QString errorMsg);
    void    _imageUpdateDone            ();
    void    _videoRunningChanged        ();
    void    _checkUpdateStatus          ();
    void    _forgetSSID                 ();

private:
    void    _saveWifiConfigurations     ();
    void    _loadWifiConfigurations     ();
    int     _copyFilesInPath            (const QString src, const QString dst);
    void    _endCopyThread              ();
    void    _enableThermalVideo         ();

private:
    TyphoonSSIDItem*        _findSsid   (QString ssid, int rssi);
    void                    _clearSSids ();

private:
    TyphoonHM4Interface*    _pHandler;
    TyphoonHFileCopy*       _pFileCopy;
    VideoReceiver*          _videoReceiver;
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
    bool                    _copyingFiles;
    bool                    _wifiAlertEnabled;
    int                     _copyResult;
    QString                 _updateError;
    int                     _updateProgress;
    bool                    _updateDone;
};
