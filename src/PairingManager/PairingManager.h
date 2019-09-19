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
#include <QMutex>
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
#if defined QGC_ENABLE_NFC
#include "PairingNFC.h"
#endif
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
        PairingSuccess,
        PairingConnecting,
        PairingConnected,
        PairingRejected,
        PairingConnectionRejected,
        PairingError
    };

    Q_ENUM(PairingStatus)

    QStringList     pairingLinkTypeStrings      ();
    QString         pairingStatusStr            () const;
    QStringList     connectedDeviceNameList     ();
    QStringList     pairedDeviceNameList        ();
    PairingStatus   pairingStatus               () { return _status; }
    QString         pairedVehicle               () { return _lastPaired; }
    int             nfcIndex                    () { return _nfcIndex; }
    int             microhardIndex              () { return _microhardIndex; }
    bool            firstBoot                   () { return _firstBoot; }
    bool            videoCanRestart             () { return !_connectedDeviceValid || !_connectedDevice.isEmpty(); }
    bool            errorState                  () { return _status == PairingRejected || _status == PairingConnectionRejected || _status == PairingError; }
    void            setStatusMessage            (PairingStatus status, QString statusStr) { emit setPairingStatus(status, statusStr); }
    void            jsonReceived                (QString json) { emit parsePairingJson(json); }
    void            setFirstBoot                (bool set) { _firstBoot = set; emit firstBootChanged(); }
    void            autoConnect                 ();
#ifdef __android__
    static void     setNativeMethods            (void);
#endif
    Q_INVOKABLE void connectToPairedDevice      (QString name);
    Q_INVOKABLE void removePairedDevice         (QString name);

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    Q_INVOKABLE void startNFCScan();
#endif    
#if QGC_GST_MICROHARD_ENABLED
    Q_INVOKABLE void startMicrohardPairing();
#endif
    Q_INVOKABLE void stopPairing();
    Q_INVOKABLE void disconnectDevice(const QString& name);

    Q_PROPERTY(QString          pairingStatusStr        READ pairingStatusStr        NOTIFY pairingStatusChanged)
    Q_PROPERTY(PairingStatus    pairingStatus           READ pairingStatus           NOTIFY pairingStatusChanged)
    Q_PROPERTY(QStringList      connectedDeviceNameList READ connectedDeviceNameList NOTIFY connectedListChanged)
    Q_PROPERTY(QStringList      pairedDeviceNameList    READ pairedDeviceNameList    NOTIFY pairedListChanged)
    Q_PROPERTY(QStringList      pairingLinkTypeStrings  READ pairingLinkTypeStrings  CONSTANT)
    Q_PROPERTY(QString          pairedVehicle           READ pairedVehicle           NOTIFY pairedVehicleChanged)
    Q_PROPERTY(bool             errorState              READ errorState              NOTIFY pairingStatusChanged)
    Q_PROPERTY(int              nfcIndex                READ nfcIndex                CONSTANT)
    Q_PROPERTY(int              microhardIndex          READ microhardIndex          CONSTANT)
    Q_PROPERTY(bool             firstBoot               READ firstBoot               WRITE setFirstBoot  NOTIFY firstBootChanged)

signals:
    void startUpload                            (QString pairURL, QJsonDocument, bool signAndEncrypt);
    void startCommand                           (QString pairURL);
    void closeConnection                        ();
    void pairingConfigurationsChanged           ();
    void nameListChanged                        ();
    void pairingStatusChanged                   ();
    void parsePairingJson                       (QString json);
    void setPairingStatus                       (PairingStatus status, QString pairingStatus);
    void connectedListChanged                   ();
    void pairedListChanged                      ();
    void pairedVehicleChanged                   ();
    void firstBootChanged                       ();

private slots:
    void _startCommand                          (QString pairURL);
    void _startUpload                           (QString pairURL, QJsonDocument, bool signAndEncrypt);
    void _stopUpload                            ();
    void _startUploadRequest                    ();
    void _parsePairingJsonNFC                   (QString jsonEnc) { _parsePairingJson(jsonEnc, true); }
    void _parsePairingJson                      (QString jsonEnc, bool updateSettings);
    void _setPairingStatus                      (PairingStatus status, QString pairingStatus);

private:
    QString                 _statusString;
    QString                 _jsonFileName;
    QString                 _lastPaired;
    QString                 _encryptionKey;
    QString                 _publicKey;
    QVariantMap             _remotePairingMap;
    int                     _nfcIndex = -1;
    int                     _microhardIndex = -1;
    int                     _pairRetryCount = 0;
    PairingStatus           _status = PairingIdle;
    OpenSSL_AES             _aes;
    OpenSSL_RSA             _rsa;
    OpenSSL_RSA             _device_rsa;
    QJsonDocument           _jsonDoc{};
    QMutex                  _uploadMutex{};
    QNetworkAccessManager*  _uploadManager = nullptr;
    QString                 _uploadURL{};
    QString                 _uploadData{};
    bool                    _firstBoot = true;
    bool                    _connectedDeviceValid = false;
    QString                 _connectedDevice{};
    QString                 _deviceToConnect{};
    QStringList             _deviceList;
    QTimer                  _reconnectTimer;
    QTimer                  _uploadTimer;
    bool                    _pairingActive = false;

    QMap<QString, LinkInterface*> _connectedDevices;

    void                    _parsePairingJsonFile       ();
    QJsonDocument           _createZeroTierConnectJson  ();
    QJsonDocument           _createMicrohardConnectJson ();
    QJsonDocument           _createZeroTierPairingJson  ();
    QJsonDocument           _createMicrohardPairingJson ();
    QString                 _assumeMicrohardPairingJson ();
    void                    _writeJson                  (QJsonDocument &jsonDoc, QString fileName);
    QString                 _getLocalIPInNetwork        (QString remoteIP, int num);
    void                    _uploadFinished             ();
    void                    _uploadError                (QNetworkReply::NetworkError code);
    void                    _pairingCompleted           (const QString name, const QString devicePublicKey);
    bool                    _connectionCompleted        (const QString response);
    QDir                    _pairingCacheDir            ();
    QString                 _pairingCacheFile           (QString uavName);
    void                    _updatePairedDeviceNameList ();
    QString                 _random_string              (uint length);
    void                    _readPairingConfig          ();
    void                    _resetPairingConfig         ();
    void                    _setLastConnectedDevice     (QString name);
    void                    _createUDPLink              (const QString name, quint16 port);
    void                    _removeUDPLink              (const QString name);
    void                    _linkActiveChanged          (LinkInterface* link, bool active, int vehicleID);

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    PairingNFC              pairingNFC;
#endif
};
