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
    QString         connectedVehicle            () { return _lastConnected; }
    int             nfcIndex                    () { return _nfcIndex; }
    int             microhardIndex              () { return _microhardIndex; }
    bool            firstBoot                   () { return _firstBoot; }
    bool            usePairing                  () { return _usePairing; }
    bool            videoCanRestart             () { return !_usePairing || !_connectedDevices.empty(); }
    bool            errorState                  () { return _status == PairingRejected || _status == PairingConnectionRejected || _status == PairingError; }
    void            setStatusMessage            (PairingStatus status, const QString& statusStr) { emit setPairingStatus(status, statusStr); }
    void            jsonReceived                (const QString& json) { emit parsePairingJson(json); }
    void            setFirstBoot                (bool set) { _firstBoot = set; emit firstBootChanged(); }
    void            setUsePairing               (bool set);
#ifdef __android__
    static void     setNativeMethods            (void);
#endif
    Q_INVOKABLE void connectToDevice            (const QString& name);
    Q_INVOKABLE void removePairedDevice         (const QString& name);
    Q_INVOKABLE void setConnectingChannel       (int channel);

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    Q_INVOKABLE void startNFCScan();
#endif    
#if QGC_GST_MICROHARD_ENABLED
    Q_INVOKABLE void startMicrohardPairing();
#endif
    Q_INVOKABLE void stopPairing();
    Q_INVOKABLE void disconnectDevice           (const QString& name);

    Q_PROPERTY(QString          pairingStatusStr        READ pairingStatusStr        NOTIFY pairingStatusChanged)
    Q_PROPERTY(PairingStatus    pairingStatus           READ pairingStatus           NOTIFY pairingStatusChanged)
    Q_PROPERTY(QStringList      connectedDeviceNameList READ connectedDeviceNameList NOTIFY connectedListChanged)
    Q_PROPERTY(QStringList      pairedDeviceNameList    READ pairedDeviceNameList    NOTIFY pairedListChanged)
    Q_PROPERTY(QStringList      pairingLinkTypeStrings  READ pairingLinkTypeStrings  CONSTANT)
    Q_PROPERTY(QString          connectedVehicle        READ connectedVehicle        NOTIFY connectedVehicleChanged)
    Q_PROPERTY(bool             errorState              READ errorState              NOTIFY pairingStatusChanged)
    Q_PROPERTY(int              nfcIndex                READ nfcIndex                CONSTANT)
    Q_PROPERTY(int              microhardIndex          READ microhardIndex          CONSTANT)
    Q_PROPERTY(bool             firstBoot               READ firstBoot               WRITE setFirstBoot  NOTIFY firstBootChanged)
    Q_PROPERTY(bool             usePairing              READ usePairing              WRITE setUsePairing NOTIFY usePairingChanged)

signals:
    void startUpload                            (const QString& name, const QString& pairURL, const QJsonDocument& jsonDoc, bool signAndEncrypt);
    void stopUpload                             ();
    void startCommand                           (const QString& name, const QString& url, const QString& content);
    void closeConnection                        ();
    void pairingConfigurationsChanged           ();
    void nameListChanged                        ();
    void pairingStatusChanged                   ();
    void parsePairingJson                       (const QString& json);
    void setPairingStatus                       (PairingStatus status, const QString& pairingStatus);
    void connectedListChanged                   ();
    void pairedListChanged                      ();
    void connectedVehicleChanged                ();
    void firstBootChanged                       ();
    void usePairingChanged                      ();
    void connectToPairedDevice                  (const QString& name);

private slots:
    void _startCommand                          (const QString& name, const QString& pairURL, const QString& content);
    void _startUpload                           (const QString& name, const QString& pairURL, const QJsonDocument& jsonDoc, bool signAndEncrypt);
    void _startUploadRequest                    (const QString& name, const QString& url, const QString& data);
    void _parsePairingJsonNFC                   (const QString& jsonEnc) { _parsePairingJson(jsonEnc, true); }
    void _parsePairingJson                      (const QString& jsonEnc, bool updateSettings);
    void _setPairingStatus                      (PairingStatus status, const QString& pairingStatus);
    void _connectToPairedDevice                 (const QString& name);
    void _setEnabled                            ();

private:
    QString                 _statusString;
    QString                 _lastConnected;
    QString                 _encryptionKey; // TODO get rid of this
    QString                 _publicKey; // TODO get rid of this
    int                     _nfcIndex = -1;
    int                     _microhardIndex = -1;
    int                     _pairRetryCount = 0;
    PairingStatus           _status = PairingIdle;
    OpenSSL_AES             _aes;
    OpenSSL_RSA             _rsa;
    OpenSSL_RSA             _device_rsa;
    QJsonDocument           _jsonDoc{}; // TODO get rid of this
    QNetworkAccessManager   _uploadManager;
    bool                    _firstBoot = true;
    bool                    _usePairing = false;
    QMap<QString, qint64>   _devicesToConnect{};
    QStringList             _deviceList;
    QTimer                  _reconnectTimer;

    QMap<QString, LinkInterface*> _connectedDevices;

    QJsonDocument           _createZeroTierConnectJson  (const QVariantMap& remotePairingMap);
    QJsonDocument           _createMicrohardConnectJson (const QVariantMap& remotePairingMap);
    QJsonDocument           _createZeroTierPairingJson  (const QVariantMap& remotePairingMap);
    QJsonDocument           _createMicrohardPairingJson (const QVariantMap& remotePairingMap);
    QString                 _assumeMicrohardPairingJson ();
    void                    _writeJson                  (const QJsonDocument &jsonDoc, const QString& fileName);
    QString                 _getLocalIPInNetwork        (const QString& remoteIP, int num);
    void                    _commandFinished            ();
    void                    _uploadFinished             ();
    void                    _uploadError                (QNetworkReply::NetworkError code);
    void                    _pairingCompleted           (const QString& name, const QString& devicePublicKey);
    bool                    _connectionCompleted        (const QString& response);
    QDir                    _pairingCacheDir            ();
    QString                 _pairingCacheFile           (const QString& uavName);
    void                    _updatePairedDeviceNameList ();
    QString                 _random_string              (uint length);
    void                    _readPairingConfig          ();
    void                    _resetPairingConfig         ();
    void                    _updateConnectedDevices     ();
    void                    _createUDPLink              (const QString& name, quint16 port);
    void                    _removeUDPLink              (const QString& name);
    void                    _linkActiveChanged          (LinkInterface* link, bool active, int vehicleID);
    void                    _autoConnect                ();
    QVariantMap             _getPairingMap              (const QString& name);
    void                    _setConnectingChannel       (const QString& name, int channel);

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    PairingNFC              pairingNFC;
#endif
};
