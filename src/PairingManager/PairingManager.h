/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QJsonDocument>
#include <QNetworkReply>
#include <QTimer>
#include <QTime>
#include <QVariantMap>

#include "openssl_aes.h"
#include "openssl_rsa.h"
#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "Fact.h"
#include "UDPLink.h"
#include "Vehicle.h"
#if defined QGC_ENABLE_QTNFC
#include "QtNFC.h"
#endif
#ifdef __android__
#include <jni.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#endif

Q_DECLARE_LOGGING_CATEGORY(PairingManagerLog)

class AppSettings;
class QGCApplication;

//-----------------------------------------------------------------------------
class PairingManager : public QGCTool
{
    Q_OBJECT
public:
    explicit PairingManager (QGCApplication* app, QGCToolbox* toolbox);
    ~PairingManager         () override;

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox) override;

    enum PairingStatus {
        PairingIdle,
        PairingActive,
        PairingError,
        Success,
        Error,
        Connecting,
        Connected,
        Disconnecting,
        Disconnected,
        Unpairing,
        ConfiguringModem
    };

    Q_ENUM(PairingStatus)

    QStringList     pairingLinkTypeStrings      ();
    QString         pairingStatusStr            () const;
    QStringList     connectedDeviceNameList     ();
    QStringList     pairedDeviceNameList        ();
    PairingStatus   pairingStatus               () { return _status; }
    QString         connectedVehicle            () { return _lastConnected; }
    int             nfcIndex                    () { return _nfcIndex; }
    int             microhardIndex              () { return _microhardIndex; }
    bool            firstBoot                   () { return _firstBoot; }
    bool            usePairing                  () { return _usePairing; }
    bool            confirmHighPowerMode        () { return _confirmHighPowerMode; }
    QString         nidPrefix                   () { return _nidPrefix; }
    void            setStatusMessage            (PairingStatus status, const QString& statusStr) { emit setPairingStatus(status, statusStr); }
    void            setFirstBoot                (bool set) { _firstBoot = set; emit firstBootChanged(); }
    void            setUsePairing               (bool set);
    void            setNidPrefix                (QString nidPrefix) { _nidPrefix = nidPrefix; emit nidPrefixChanged(); }
    void            setConfirmHPM               (bool set) { _confirmHighPowerMode = set; emit confirmHighPowerModeChanged(); }
    void            jsonReceivedStartPairing    (const QString& jsonEnc);
    QString         pairingKey                  ();
    QString         networkId                   ();
    int             pairingChannel              ();
    int             connectingChannel           ();
#ifdef __android__
    static void     setNativeMethods            (void);
#endif
    Q_INVOKABLE void    connectToDevice         (const QString& deviceName, bool confirm = false);
    Q_INVOKABLE void    unpairDevice            (const QString& name);
    Q_INVOKABLE void    stopConnectingDevice    (const QString& name);
    Q_INVOKABLE bool    isDeviceConnecting      (const QString& name);
    Q_INVOKABLE void    setModemParameters      (int channel, int power, int bandwidth);
    Q_INVOKABLE QString extractName             (const QString& name);
    Q_INVOKABLE QString extractChannel          (const QString& name);

#if defined QGC_ENABLE_QTNFC
    Q_INVOKABLE void    startNFCScan            ();
#endif    
#if QGC_GST_MICROHARD_ENABLED
    Q_INVOKABLE void    startMicrohardPairing   (const QString& pairingKey, const QString& networkId, int pairingChannel, int connectingChannel);
#endif
    Q_INVOKABLE void    stopPairing             ();
    Q_INVOKABLE void    disconnectDevice        (const QString& name);

    Q_PROPERTY(QString          pairingStatusStr        READ pairingStatusStr                            NOTIFY pairingStatusChanged)
    Q_PROPERTY(PairingStatus    pairingStatus           READ pairingStatus                               NOTIFY pairingStatusChanged)
    Q_PROPERTY(QStringList      connectedDeviceNameList READ connectedDeviceNameList                     NOTIFY deviceListChanged)
    Q_PROPERTY(QStringList      pairedDeviceNameList    READ pairedDeviceNameList                        NOTIFY deviceListChanged)
    Q_PROPERTY(QStringList      pairingLinkTypeStrings  READ pairingLinkTypeStrings  CONSTANT)
    Q_PROPERTY(QString          connectedVehicle        READ connectedVehicle                            NOTIFY connectedVehicleChanged)
    Q_PROPERTY(QString          pairingKey              READ pairingKey                                  NOTIFY pairingKeyChanged)
    Q_PROPERTY(QString          networkId               READ networkId                                   NOTIFY networkIdChanged)
    Q_PROPERTY(QString          nidPrefix               READ nidPrefix               WRITE setNidPrefix  NOTIFY nidPrefixChanged)
    Q_PROPERTY(int              pairingChannel          READ pairingChannel                              NOTIFY pairingChannelChanged)
    Q_PROPERTY(int              connectingChannel       READ connectingChannel                           NOTIFY connectingChannelChanged)
    Q_PROPERTY(bool             confirmHighPowerMode    READ confirmHighPowerMode    WRITE setConfirmHPM NOTIFY confirmHighPowerModeChanged)
    Q_PROPERTY(int              nfcIndex                READ nfcIndex                CONSTANT)
    Q_PROPERTY(int              microhardIndex          READ microhardIndex          CONSTANT)
    Q_PROPERTY(bool             firstBoot               READ firstBoot               WRITE setFirstBoot  NOTIFY firstBootChanged)
    Q_PROPERTY(bool             usePairing              READ usePairing              WRITE setUsePairing NOTIFY usePairingChanged)

signals:
    void startUpload                            (const QString& name, const QString& pairURL, const QJsonDocument& jsonDoc, bool signAndEncrypt);
    void stopUpload                             ();
    void closeConnection                        ();
    void pairingConfigurationsChanged           ();
    void nameListChanged                        ();
    void pairingStatusChanged                   ();
    void setPairingStatus                       (PairingStatus status, const QString& pairingStatus);
    void deviceListChanged                      ();
    void connectedVehicleChanged                ();
    void firstBootChanged                       ();
    void usePairingChanged                      ();
    void connectToPairedDevice                  (const QString& deviceName);
    void pairingKeyChanged                      ();
    void confirmHighPowerModeChanged            ();
    void networkIdChanged                       ();
    void pairingChannelChanged                  ();
    void connectingChannelChanged               ();
    void nidPrefixChanged                       ();

private slots:
    void _startUpload                           (const QString& name, const QString& pairURL, const QJsonDocument& jsonDoc, bool signAndEncrypt);
    void _startUploadRequest                    (const QString& name, const QString& url, const QString& data);
    void _setPairingStatus                      (PairingStatus status, const QString& pairingStatus);
    void _connectToPairedDevice                 (const QString& deviceName);
    void _setEnabled                            ();

private:
    int                           _nfcIndex = -1;
    int                           _microhardIndex = -1;
    int                           _mavlink_router_port = -1;
    PairingStatus                 _status = PairingIdle;
    QString                       _statusString;
    QString                       _lastConnected;
    QString                       _encryptionKey;
    QString                       _publicKey;
    OpenSSL_AES                   _aes;
    OpenSSL_AES                   _aes_config;
    OpenSSL_RSA                   _rsa;
    OpenSSL_RSA                   _device_rsa;
    QJsonDocument                 _gcsJsonDoc{};
    QMap<QString, QJsonDocument>  _devices{};
    QNetworkAccessManager         _uploadManager;
    bool                          _firstBoot = true;
    bool                          _usePairing = false;
    bool                          _usePairingSet = false;
    bool                          _confirmHighPowerMode = false;
    QMap<QString, qint64>         _devicesToConnect{};
    QTimer                        _reconnectTimer;
    QMap<QString, LinkInterface*> _connectedDevices;
    QMap<QString, QNetworkReply*> _connectRequests;
    QString                       _lastDeviceNameToConnect = "";
    QString                       _nidPrefix = "SRR_";
    std::function<void(void)>     _disconnect_callback;
    int                           _resetMavlinkMessagesTimersCounter;

    QJsonDocument           _createZeroTierConnectJson  (const QVariantMap& remotePairingMap);
    QJsonDocument           _createMicrohardConnectJson (const QVariantMap& remotePairingMap);
    QJsonDocument           _createZeroTierPairingJson  (const QVariantMap& remotePairingMap);
    QJsonDocument           _createMicrohardPairingJson (const QVariantMap& remotePairingMap);
    void                    _writeJson                  (const QJsonDocument &jsonDoc, const QString& name);
    QString                 _getLocalIPInNetwork        (const QString& remoteIP, int num);
    void                    _uploadFinished             ();
    void                    _uploadError                (QNetworkReply::NetworkError code);
    void                    _pairingCompleted           (const QString& tempName, const QString& newName, const QString& ip, const QString& devicePublicKey, const int channel);
    void                    _connectionCompleted        (const QString& name, const int channel);
    void                    _disconnectCompleted        (const QString& name);
    void                    _requestedParameters        (int channel, int power, int bandwidth);
    void                    _modemParametersCompleted   (const QString& name, const QString& nid, int channel, int power, int bandwidth);
    QDir                    _pairingCacheDir            ();
    QDir                    _pairingCacheTempDir        ();
    QString                 _pairingCacheFile           (const QString& uavName);
    void                    _updatePairedDeviceNameList ();
    QString                 _random_string              (uint length);
    void                    _readPairingConfig          ();
    void                    _resetPairingConfig         ();
    void                    _resetMicrohardModem        ();
    void                    _updateConnectedDevices     ();
    void                    _createUDPLink              (const QString& name, quint16 port);
    void                    _removeUDPLink              (const QString& name);
    void                    _linkActiveChanged          (LinkInterface* link, bool active, int vehicleID);
    void                    _vehicleAuxiliaryLinkAdded  (Vehicle *vehicle, LinkInterface* link);
    void                    _linkInactiveOrDeleted      (LinkInterface* link);
    void                    _autoConnect                ();
    QJsonDocument           _getPairingJsonDoc          (const QString& name, bool remove = false);
    QVariantMap             _getPairingMap              (const QString& name);
    void                    _setModemParameters         (const QString& name, int channel, int power, int bandwidth);
    void                    _resetMavlinkMessagesTimers (const QString& name);
    QString                 _removeRSAkey               (const QString& s);
    int                     _getDeviceChannel           (const QString& name);
    QDateTime               _getDeviceConnectTime       (const QString& name);
    QString                 _getDeviceIP                (const QString& name);
    bool                    _getFreeDeviceAndMicrohardIP(QString& ip, QString& mhip);
    QString                 _getDeviceConnectNid        (int channel);

#if defined QGC_ENABLE_QTNFC
    PairingNFC              pairingNFC;
#endif
};
