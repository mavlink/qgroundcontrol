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
        PairingConnectionRejected
    };

    Q_ENUM(PairingStatus)

    QStringList     pairingLinkTypeStrings(void);
    QString         pairingStatusStr(void) const;
    QStringList     pairedDeviceNameList(void);
    PairingStatus   pairingStatus() { return _status; }
    int             nfcIndex(void) { return _nfcIndex; }
    int             microhardIndex(void) { return _microhardIndex; }
    void            setStatusMessage(QString status) { emit setPairingStatus(status); }
    void            jsonReceived(QString json) { emit parsePairingJson(json); }
#ifdef __android__
    static void     setNativeMethods(void);
#endif
    Q_INVOKABLE void connectToPairedDevice(QString name);

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
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
    Q_PROPERTY(int              nfcIndex                READ nfcIndex               CONSTANT)
    Q_PROPERTY(int              microhardIndex          READ microhardIndex         CONSTANT)

signals:
    void startUpload(QString pairURL, QJsonDocument);
    void closeConnection();
    void pairingConfigurationsChanged();
    void nameListChanged();
    void pairingStatusChanged();
    void parsePairingJson(QString json);
    void setPairingStatus(QString pairingStatus);
    void pairedListChanged();

private slots:
    void _startUpload(QString pairURL, QJsonDocument);
    void _stopUpload();
    void _startUploadRequest();
    void _parsePairingJson(QString jsonEnc);
    void _setPairingStatus(QString pairingStatus);

private:
    QString                 _pairingStatus;
    QString                 _jsonFileName;
    QVariantMap             _remotePairingMap;
    int                     _nfcIndex = -1;
    int                     _microhardIndex = -1;
    PairingStatus           _status = PairingIdle;
    AES                     _aes;
    QJsonDocument           _jsonDoc{};
    QMutex                  _uploadMutex{};
    QNetworkAccessManager*  _uploadManager = nullptr;
    QString                 _uploadURL{};
    QString                 _uploadData{};

    void               _parsePairingJsonFile();
    QJsonDocument      _createZeroTierConnectJson(QString cert2);
    QJsonDocument      _createMicrohardConnectJson(QString cert2);
    QJsonDocument      _createZeroTierPairingJson(QString cert1);
    QJsonDocument      _createMicrohardPairingJson(QString pwd, QString cert1);
    QString            _assumeMicrohardPairingJson();
    void               _writeJson(QJsonDocument &jsonDoc, QString fileName);
    QString            _getLocalIPInNetwork(QString remoteIP, int num);
    void               _uploadFinished(void);
    void               _uploadError(QNetworkReply::NetworkError code);
    void               _pairingCompleted(QString name);
    void               _connectionCompleted(QString name);
    QDir               _pairingCacheDir();
    QString            _pairingCacheFile(QString uavName);

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    PairingNFC pairingNFC;
#endif
};
