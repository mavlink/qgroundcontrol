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

#include "aes.h"
#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "Fact.h"
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
    QStringList     pairedDeviceNameList        () { return _deviceList; }
    PairingStatus   pairingStatus               () { return _status; }
    QString         pairedVehicle               () { return _lastPaired; }
    int             nfcIndex                    () { return _nfcIndex; }
    int             microhardIndex              () { return _microhardIndex; }
    bool            firstBoot                   () { return _firstBoot; }
    bool            errorState                  () { return _status == PairingRejected || _status == PairingConnectionRejected || _status == PairingError; }
    void            setStatusMessage            (PairingStatus status, QString statusStr) { emit setPairingStatus(status, statusStr); }
    void            jsonReceived                (QString json) { emit parsePairingJson(json); }
    void            setFirstBoot                (bool set) { _firstBoot = set; emit firstBootChanged(); }
#ifdef __android__
    static void     setNativeMethods            (void);
#endif
    Q_INVOKABLE void connectToPairedDevice      (QString name);
    Q_INVOKABLE void removePairedDevice         (QString name);

#if defined defined QGC_ENABLE_QTNFC
    Q_INVOKABLE void startNFCScan();
#endif    
#if QGC_GST_MICROHARD_ENABLED
    Q_INVOKABLE void startMicrohardPairing();
#endif
    Q_INVOKABLE void stopPairing();

    Q_PROPERTY(QString          pairingStatusStr        READ pairingStatusStr       NOTIFY pairingStatusChanged)
    Q_PROPERTY(PairingStatus    pairingStatus           READ pairingStatus          NOTIFY pairingStatusChanged)
    Q_PROPERTY(QStringList      pairedDeviceNameList    READ pairedDeviceNameList   NOTIFY pairedListChanged)
    Q_PROPERTY(QStringList      pairingLinkTypeStrings  READ pairingLinkTypeStrings CONSTANT)
    Q_PROPERTY(QString          pairedVehicle           READ pairedVehicle          NOTIFY pairedVehicleChanged)
    Q_PROPERTY(bool             errorState              READ errorState             NOTIFY pairingStatusChanged)
    Q_PROPERTY(int              nfcIndex                READ nfcIndex               CONSTANT)
    Q_PROPERTY(int              microhardIndex          READ microhardIndex         CONSTANT)
    Q_PROPERTY(bool             firstBoot               READ firstBoot              WRITE setFirstBoot  NOTIFY firstBootChanged)

signals:
    void startUpload                            (QString pairURL, QJsonDocument);
    void closeConnection                        ();
    void pairingConfigurationsChanged           ();
    void nameListChanged                        ();
    void pairingStatusChanged                   ();
    void parsePairingJson                       (QString json);
    void setPairingStatus                       (PairingStatus status, QString pairingStatus);
    void pairedListChanged                      ();
    void pairedVehicleChanged                   ();
    void firstBootChanged                       ();

private slots:
    void _startUpload                           (QString pairURL, QJsonDocument);
    void _stopUpload                            ();
    void _startUploadRequest                    ();
    void _parsePairingJson                      (QString jsonEnc);
    void _setPairingStatus                      (PairingStatus status, QString pairingStatus);

private:
    QString                 _statusString;
    QString                 _jsonFileName;
    QString                 _lastPaired;
    QVariantMap             _remotePairingMap;
    int                     _nfcIndex = -1;
    int                     _microhardIndex = -1;
    int                     _pairRetryCount = 0;
    PairingStatus           _status = PairingIdle;
    AES                     _aes;
    QJsonDocument           _jsonDoc{};
    QMutex                  _uploadMutex{};
    QNetworkAccessManager*  _uploadManager = nullptr;
    QString                 _uploadURL{};
    QString                 _uploadData{};
    bool                    _firstBoot = true;
    QStringList             _deviceList;

    void                    _parsePairingJsonFile       ();
    QJsonDocument           _createZeroTierConnectJson  (QString cert2);
    QJsonDocument           _createMicrohardConnectJson (QString cert2);
    QJsonDocument           _createZeroTierPairingJson  (QString cert1);
    QJsonDocument           _createMicrohardPairingJson (QString pwd, QString cert1);
    QString                 _assumeMicrohardPairingJson ();
    void                    _writeJson                  (QJsonDocument &jsonDoc, QString fileName);
    QString                 _getLocalIPInNetwork        (QString remoteIP, int num);
    void                    _uploadFinished             ();
    void                    _uploadError                (QNetworkReply::NetworkError code);
    void                    _pairingCompleted           (QString name);
    void                    _connectionCompleted        (QString name);
    QDir                    _pairingCacheDir            ();
    QString                 _pairingCacheFile           (QString uavName);
    void                    _updatePairedDeviceNameList ();

#if defined QGC_ENABLE_QTNFC
    PairingNFC              pairingNFC;
#endif
};
