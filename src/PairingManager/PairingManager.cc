/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PairingManager.h"
#include "SettingsManager.h"
#include "MicrohardManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "VideoManager.h"

#include <QSettings>
#include <QJsonObject>
#include <QStandardPaths>
#include <QMutexLocker>

QGC_LOGGING_CATEGORY(PairingManagerLog, "PairingManagerLog")

static const char* jsonFileName = "pairing.json";

//-----------------------------------------------------------------------------
PairingManager::PairingManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _aes("J6+KuWh9K2!hG(F'", 0x368de30e8ec063ce)
{
    _jsonFileName = QDir::temp().filePath(jsonFileName);
    connect(this, &PairingManager::parsePairingJson, this, &PairingManager::_parsePairingJsonNFC);
    connect(this, &PairingManager::setPairingStatus, this, &PairingManager::_setPairingStatus);
    connect(this, &PairingManager::startUpload, this, &PairingManager::_startUpload);
    connect(this, &PairingManager::startCommand, this, &PairingManager::_startCommand);
    connect(&_uploadTimer, &QTimer::timeout, this, &PairingManager::_startUploadRequest);
    _uploadTimer.setSingleShot(true);
    connect(&_reconnectTimer, &QTimer::timeout, this, &PairingManager::autoConnect);
    _reconnectTimer.setSingleShot(true);
    _readPairingConfig();
}

//-----------------------------------------------------------------------------
PairingManager::~PairingManager()
{
}

//-----------------------------------------------------------------------------

void
PairingManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);
    _updatePairedDeviceNameList();
    emit pairedListChanged();
}

//-----------------------------------------------------------------------------
void
PairingManager::_pairingCompleted(const QString name, const QString devicePublicKey)
{
    QJsonObject jsonObj = _jsonDoc.object();
    jsonObj.insert("PublicKey", devicePublicKey);
    _jsonDoc.setObject(jsonObj);
    _writeJson(_jsonDoc, _pairingCacheFile(name));
    _remotePairingMap["NM"] = name;
    _lastPaired = name;
    _updatePairedDeviceNameList();
    emit pairedListChanged();
    emit pairedVehicleChanged();
    //_app->informationMessageBoxOnMainThread("", tr("Paired with %1").arg(name));
    setPairingStatus(PairingSuccess, tr("Pairing Successfull"));
    _toolbox->microhardManager()->switchToConnectionEncryptionKey(_encryptionKey);
}

//-----------------------------------------------------------------------------
QStringList
PairingManager::connectedDeviceNameList()
{
    QStringList list;

    QMapIterator<QString, LinkInterface*> i(_connectedDevices);
    while (i.hasNext()) {
        i.next();
        if (i.value() && i.value()->active()) {
            list.append(i.key());
        }
    }

    return list;
}

//-----------------------------------------------------------------------------
QStringList
PairingManager::pairedDeviceNameList()
{
    QStringList connectedDevs = connectedDeviceNameList();
    QStringList list;
    foreach (auto a, _deviceList) {
        if (!connectedDevs.contains(a)) {
            list.append(a);
        }
    }
    return list;
}

//-----------------------------------------------------------------------------
void
PairingManager::_linkActiveChanged(LinkInterface* link, bool active, int vehicleID)
{
    Q_UNUSED(vehicleID);
    if (!active) {
        QMapIterator<QString, LinkInterface*> i(_connectedDevices);
        while (i.hasNext()) {
            i.next();
            if (i.value() == link) {
                _deviceToConnect = i.key();
                _reconnectTimer.start(5000);
            }
        }
    } else {
        _reconnectTimer.stop();
        _deviceToConnect = "";
    }
    emit connectedListChanged();
    emit pairedListChanged();
}

//-----------------------------------------------------------------------------
void
PairingManager::_createUDPLink(const QString name, quint16 port)
{
    _removeUDPLink(name);
    UDPConfiguration* udpConfig = new UDPConfiguration(name);
    udpConfig->setLocalPort(port);
    udpConfig->setDynamic(true);
    SharedLinkConfigurationPointer linkConfig = _toolbox->linkManager()->addConfiguration(udpConfig);
    LinkInterface* link = _toolbox->linkManager()->createConnectedLink(linkConfig);
    connect(link, &LinkInterface::activeChanged, this, &PairingManager::_linkActiveChanged);
    _connectedDevices[name] = link;
    emit connectedListChanged();
    emit pairedListChanged();
}

//-----------------------------------------------------------------------------
void
PairingManager::_removeUDPLink(const QString name)
{
    if (_connectedDevices.contains(name)) {
        if (_connectedDevice == name) {
            _setLastConnectedDevice("");
            _toolbox->videoManager()->stopVideo();
        }
        _toolbox->linkManager()->disconnectLink(_connectedDevices[name]);
        _connectedDevices.remove(name);
        if (_connectedDevice == name) {
            _setLastConnectedDevice("");
            _toolbox->videoManager()->stopVideo();
        }
        emit connectedListChanged();
        emit pairedListChanged();
    }
}

//-----------------------------------------------------------------------------
bool
PairingManager::_connectionCompleted(const QString response)
{
    QStringList a = QString::fromStdString(_rsa.decrypt(response.toStdString())).split(";");
    if (a.length() != 2 && !_device_rsa.verify(a[0].toStdString(), a[1].toStdString())) {
        return false;
    }

    _createUDPLink(a[0], 24550);
    _setLastConnectedDevice(a[0]);
    _toolbox->videoManager()->startVideo();
    setPairingStatus(PairingConnected, tr("Connection Successfull"));

    return true;
}

//-----------------------------------------------------------------------------
void
PairingManager::_startCommand(QString pairURL)
{
    QMutexLocker lock(&_uploadMutex);
    if (_uploadManager != nullptr) {
        return;
    }
    _uploadManager = new QNetworkAccessManager(this);

    qCDebug(PairingManagerLog) << "Starting command: " << pairURL;
    _uploadURL = pairURL;
    QNetworkRequest req;
    req.setUrl(QUrl(_uploadURL));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    _uploadManager->post(req, "");
}

//-----------------------------------------------------------------------------
void
PairingManager::_startUpload(QString pairURL, QJsonDocument jsonDoc, bool signAndEncrypt)
{
    QMutexLocker lock(&_uploadMutex);
    if (_uploadManager != nullptr) {
        return;
    }
    _uploadManager = new QNetworkAccessManager(this);

    QString str = jsonDoc.toJson(QJsonDocument::JsonFormat::Compact);
    qCDebug(PairingManagerLog) << "Starting upload to: " << pairURL << " " << str;

    std::string data = _aes.encrypt(str.toStdString());
    if (signAndEncrypt) {
        data = _device_rsa.encrypt(data + ";" + _rsa.sign(data));
    }
    _uploadData = QString::fromStdString(data);
    _uploadURL = pairURL;
    _startUploadRequest();
}

//-----------------------------------------------------------------------------
void
PairingManager::_startUploadRequest()
{
    if (_uploadURL.contains("pair")) {
        _pairingActive = true;
    }
    QNetworkRequest req;
    req.setUrl(QUrl(_uploadURL));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = _uploadManager->post(req, _uploadData.toUtf8());
    connect(reply, &QNetworkReply::finished, this, &PairingManager::_uploadFinished);
}

//-----------------------------------------------------------------------------
void
PairingManager::_stopUpload()
{
    QMutexLocker lock(&_uploadMutex);
    if (_uploadManager != nullptr) {
        delete _uploadManager;
        _uploadManager = nullptr;
        _pairingActive = false;
    }
}

//-----------------------------------------------------------------------------
void
PairingManager::_uploadFinished()
{
    _pairingActive = false;
    QMutexLocker lock(&_uploadMutex);
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (reply) {
        if (_uploadManager != nullptr) {
            if (reply->error() == QNetworkReply::NoError) {
                _uploadManager->deleteLater();
                _uploadManager = nullptr;
                _uploadMutex.unlock();
                qCDebug(PairingManagerLog) << "Upload finished.";
                QByteArray bytes = reply->readAll();
                QString str = QString::fromStdString(_aes.decrypt(QString::fromUtf8(bytes.data(), bytes.size()).toStdString()));
                qCDebug(PairingManagerLog) << "Reply: " << str;
                auto a = str.split(";");
                if (a[0] == "Accepted" && a.length() > 2) {
                    _pairingCompleted(a[1], a[2]);
                } else if (a[0] == "Connected" && a.length() > 1) {
                    if (!_connectionCompleted(a[1])) {
                        setPairingStatus(PairingConnectionRejected, tr("Connection verification failed."));
                        qCDebug(PairingManagerLog) << "Connection error: failed to verify remote vehicle ID";
                    }
                } else if (a[0] == "Connection" && a.length() > 1) {
                    setPairingStatus(PairingConnectionRejected, tr("Connection Rejected"));
                    qCDebug(PairingManagerLog) << "Connection error: " << str;
                } else {
                    setPairingStatus(PairingRejected, tr("Pairing Rejected"));
                    qCDebug(PairingManagerLog) << "Pairing error: " << str;
                }
            } else if(_uploadURL.contains("connect")) {
                qCDebug(PairingManagerLog) << "Connect error: " + reply->errorString();
                _uploadTimer.start(3000);
            } else {
                if(++_pairRetryCount > 3) {
                    qCDebug(PairingManagerLog) << "Giving up";
                    setPairingStatus(PairingError, tr("No Response From Vehicle"));
                    _uploadManager->deleteLater();
                    _uploadManager = nullptr;
                } else {
                    qCDebug(PairingManagerLog) << "Pairing error: " + reply->errorString();
                    _uploadTimer.start(3000);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
PairingManager::_parsePairingJsonFile()
{
    QFile file(_jsonFileName);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString json = file.readAll();
    file.remove();
    file.close();

    jsonReceived(json);
}

//-----------------------------------------------------------------------------
void
PairingManager::connectToPairedDevice(QString name)
{
    if (name.isEmpty()) {
        return;
    }
    setPairingStatus(PairingConnecting, tr("Connecting to %1").arg(name));
    QFile file(_pairingCacheFile(name));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString json = file.readAll();
    if (!json.isEmpty()) {
        jsonReceived(json);
    }
}

//-----------------------------------------------------------------------------
void
PairingManager::removePairedDevice(QString name)
{
    QFile file(_pairingCacheFile(name));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString json = QString::fromStdString(_aes.decrypt(file.readAll().toStdString()));
    _jsonDoc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject jsonObj = _jsonDoc.object();
    auto map = jsonObj.toVariantMap();
    QString pport = map["PP"].toString();
    if (pport.length() == 0) {
        pport = "29351";
    }
    file.close();
    file.remove();

    _removeUDPLink(name);

    QString pairURL = "http://" + map["IP"].toString() + ":" + pport + "/unpair";;
    emit startCommand(pairURL);
    _updatePairedDeviceNameList();
    emit pairedListChanged();
}

//-----------------------------------------------------------------------------
QString
PairingManager::_random_string(uint length)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const uint max_index = (sizeof(charset) - 1);
        return charset[static_cast<uint>(std::rand()) % max_index];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return QString::fromStdString(str);
}

//-----------------------------------------------------------------------------
void
PairingManager::_readPairingConfig()
{
    QFile file(_pairingCacheFile("gcs"));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString jsonEnc = file.readAll();
    QString json = QString::fromStdString(_aes.decrypt(jsonEnc.toStdString()));
    _jsonDoc = QJsonDocument::fromJson(json.toUtf8());

    if (_jsonDoc.isNull() || !_jsonDoc.isObject()) {
        _resetPairingConfig();
        return;
    }

    QJsonObject jsonObj = _jsonDoc.object();
    if (!jsonObj.contains("EK") || !jsonObj.contains("PublicKey") || !jsonObj.contains("PrivateKey")) {
        _resetPairingConfig();
        return;
    }

    if (jsonObj.contains("CONNECTED_DEVICE")) {
        _deviceToConnect = jsonObj.value("CONNECTED_DEVICE").toString();
    }
    _encryptionKey = jsonObj.value("EK").toString();
    _publicKey = jsonObj.value("PublicKey").toString();
    _rsa.generate_private(jsonObj.value("PrivateKey").toString().toStdString());
    _rsa.generate_public(_publicKey.toStdString());
}
//-----------------------------------------------------------------------------
void
PairingManager::autoConnect()
{
    if (_pairingActive) {
        return;
    }
    connectToPairedDevice(_deviceToConnect);
}

//-----------------------------------------------------------------------------
void
PairingManager::_setLastConnectedDevice(QString name)
{
    if (!name.isEmpty()) {
        _connectedDeviceValid = true;
    } else {
        _deviceToConnect = "";
    }
    _connectedDevice = name;

    QFile file(_pairingCacheFile("gcs"));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString jsonEnc = file.readAll();
    QString json = QString::fromStdString(_aes.decrypt(jsonEnc.toStdString()));
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json.toUtf8());
    file.close();

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    jsonObj.remove("CONNECTED_DEVICE");
    if (!_connectedDevice.isEmpty()) {
        jsonObj.insert("CONNECTED_DEVICE", _connectedDevice);
    }
    jsonDoc.setObject(jsonObj);

    _writeJson(jsonDoc, _pairingCacheFile("gcs"));
}

//-----------------------------------------------------------------------------
void
PairingManager::_resetPairingConfig()
{
    QJsonDocument json;
    QJsonObject   jsonObject;

    _rsa.generate();
    _publicKey = QString::fromStdString(_rsa.get_public_key());
    _encryptionKey = _random_string(8);
    jsonObject.insert("EK", _encryptionKey);
    jsonObject.insert("PublicKey", _publicKey);
    jsonObject.insert("PrivateKey", QString::fromStdString(_rsa.get_private_key()));
    json.setObject(jsonObject);

    _writeJson(json, _pairingCacheFile("gcs"));
}

//-----------------------------------------------------------------------------
void
PairingManager::_updatePairedDeviceNameList()
{
    _deviceList.clear();
    QDirIterator it(_pairingCacheDir().absolutePath(), QDir::Files);
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        if (fileInfo.fileName() != "gcs") {
            _deviceList.append(fileInfo.fileName());
        }
    }
    if (_deviceList.empty()) {
        _resetPairingConfig();
    }
}

//-----------------------------------------------------------------------------
QString
PairingManager::_assumeMicrohardPairingJson()
{
    QJsonDocument json;
    QJsonObject   jsonObject;

    jsonObject.insert("LT", "MH");

    QString remoteIPAddr = _toolbox->microhardManager()->remoteIPAddr();
    QString ipPrefix = remoteIPAddr.left(remoteIPAddr.lastIndexOf('.'));
    jsonObject.insert("IP", ipPrefix + ".10");
    jsonObject.insert("AIP", remoteIPAddr);
    jsonObject.insert("CU", _toolbox->microhardManager()->configUserName());
    jsonObject.insert("CP", _toolbox->microhardManager()->configPassword());
    jsonObject.insert("EK", _encryptionKey);
    json.setObject(jsonObject);

    return QString(json.toJson(QJsonDocument::Compact));
}

//-----------------------------------------------------------------------------
void
PairingManager::_parsePairingJson(QString jsonEnc, bool updateSettings)
{
    QString json = QString::fromStdString(_aes.decrypt(jsonEnc.toStdString()));
    if (json == "") {
        json = jsonEnc;
    }
    qCDebug(PairingManagerLog) << "Parsing JSON: " << json;

    _jsonDoc = QJsonDocument::fromJson(json.toUtf8());

    if (_jsonDoc.isNull()) {
        setPairingStatus(PairingError, tr("Invalid Pairing File"));
        qCDebug(PairingManagerLog) << "Failed to create Pairing JSON doc.";
        return;
    }
    if (!_jsonDoc.isObject()) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON is not an object.";
        return;
    }

    QJsonObject jsonObj = _jsonDoc.object();

    if (jsonObj.isEmpty()) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON object is empty.";
        return;
    }

    _remotePairingMap = jsonObj.toVariantMap();
    QString linkType  = _remotePairingMap["LT"].toString();
    QString pport     = _remotePairingMap["PP"].toString();
    if (pport.length() == 0) {
        pport = "29351";
    }

    if (_remotePairingMap.contains("PublicKey")) {
        _device_rsa.generate_public(_remotePairingMap["PublicKey"].toString().toStdString());
    }

    if (linkType.length() == 0) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON is malformed.";
        return;
    }

    QString pairURL = "http://" + _remotePairingMap["IP"].toString() + ":" + pport;
    bool connecting = jsonObj.contains("CONNECTING");
    QJsonDocument jsonDoc;

    if (!connecting) {
        pairURL +=  + "/pair";
        jsonObj.insert("CONNECTING", "true");
        _jsonDoc.setObject(jsonObj);
        if (linkType == "ZT") {
            jsonDoc = _createZeroTierPairingJson();
        } else if (linkType == "MH") {
            jsonDoc = _createMicrohardPairingJson();
        }
    } else {
        pairURL +=  + "/connect";
        if (linkType == "ZT") {
            jsonDoc = _createZeroTierConnectJson();
        } else if (linkType == "MH") {
            jsonDoc = _createMicrohardConnectJson();
        }
    }

    if (updateSettings) {
        if (linkType == "ZT") {
            _toolbox->settingsManager()->appSettings()->enableMicrohard()->setRawValue(false);
            _toolbox->settingsManager()->appSettings()->enableTaisync()->setRawValue(false);
        } else if (linkType == "MH") {
            _toolbox->settingsManager()->appSettings()->enableMicrohard()->setRawValue(true);
            _toolbox->settingsManager()->appSettings()->enableTaisync()->setRawValue(false);
            if (_remotePairingMap.contains("AIP")) {
                _toolbox->microhardManager()->setRemoteIPAddr(_remotePairingMap["AIP"].toString());
            }
            if (_remotePairingMap.contains("CU")) {
                _toolbox->microhardManager()->setConfigUserName(_remotePairingMap["CU"].toString());
            }
            if (_remotePairingMap.contains("CP")) {
                _toolbox->microhardManager()->setConfigPassword(_remotePairingMap["CP"].toString());
            }
        }
    }
    if (linkType == "MH") {
        if (!connecting) {
            _setLastConnectedDevice("");
            _toolbox->videoManager()->stopVideo();
            _toolbox->microhardManager()->switchToPairingEncryptionKey();
        } else if (_remotePairingMap.contains("EK")) {
            _toolbox->microhardManager()->switchToConnectionEncryptionKey(_remotePairingMap["EK"].toString());
        }
        _toolbox->microhardManager()->updateSettings();
    }
    emit startUpload(pairURL, jsonDoc, connecting);
}

//-----------------------------------------------------------------------------
QString
PairingManager::_getLocalIPInNetwork(QString remoteIP, int num)
{
    QStringList pieces = remoteIP.split(".");
    QString ipPrefix = "";
    for (int i = 0; i<num && i<pieces.length(); i++) {
        ipPrefix += pieces[i] + ".";
    }

    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
            if (address.toString().startsWith(ipPrefix)) {
                return address.toString();
            }
        }
    }

    return "";
}

//-----------------------------------------------------------------------------
QDir
PairingManager::_pairingCacheDir()
{
    const QString spath(QFileInfo(QSettings().fileName()).dir().absolutePath());
    QDir dir = spath + QDir::separator() + "PairingCache";
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return dir;
}

//-----------------------------------------------------------------------------
QString
PairingManager::_pairingCacheFile(QString uavName)
{
    return _pairingCacheDir().filePath(uavName);
}

//-----------------------------------------------------------------------------
void
PairingManager::_writeJson(QJsonDocument &jsonDoc, QString fileName)
{
    QString val = jsonDoc.toJson(QJsonDocument::JsonFormat::Compact);
    qCDebug(PairingManagerLog) << "Write json " << val;
    QString enc = QString::fromStdString(_aes.encrypt(val.toStdString()));

    QFile file(fileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    file.write(enc.toUtf8());
    file.close();
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_createZeroTierPairingJson()
{
    QString localIP = _getLocalIPInNetwork(_remotePairingMap["IP"].toString(), 2);

    QJsonObject jsonObj;
    jsonObj.insert("LT", "ZT");
    jsonObj.insert("IP", localIP);
    jsonObj.insert("P", 14550);
    jsonObj.insert("PublicKey", _publicKey);
    return QJsonDocument(jsonObj);
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_createMicrohardPairingJson()
{
    QString localIP = _getLocalIPInNetwork(_remotePairingMap["IP"].toString(), 3);

    QJsonObject jsonObj;
    jsonObj.insert("LT", "MH");
    jsonObj.insert("IP", localIP);
    jsonObj.insert("EK", _encryptionKey);
    jsonObj.insert("PublicKey", _publicKey);
    return QJsonDocument(jsonObj);
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_createZeroTierConnectJson()
{
    QString localIP = _getLocalIPInNetwork(_remotePairingMap["IP"].toString(), 2);

    QJsonObject jsonObj;
    jsonObj.insert("LT", "ZT");
    jsonObj.insert("IP", localIP);
    jsonObj.insert("P", 14550);
    return QJsonDocument(jsonObj);
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_createMicrohardConnectJson()
{
    QString localIP = _getLocalIPInNetwork(_remotePairingMap["IP"].toString(), 3);

    QJsonObject jsonObj;
    jsonObj.insert("LT", "MH");
    jsonObj.insert("IP", localIP);
    jsonObj.insert("P", 24550);
    return QJsonDocument(jsonObj);
}

//-----------------------------------------------------------------------------

QStringList
PairingManager::pairingLinkTypeStrings()
{
    //-- Must follow same order as enum LinkType in LinkConfiguration.h
    static QStringList list;
    int i = 0;
    if (!list.size()) {
#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
        list += tr("NFC");
        _nfcIndex = i++;
#endif
#if defined QGC_GST_MICROHARD_ENABLED
        list += tr("Microhard");
        _microhardIndex = i++;
#endif
    }
    return list;
}

//-----------------------------------------------------------------------------
void
PairingManager::_setPairingStatus(PairingStatus status, QString statusStr)
{
    _status = status;
    _statusString = statusStr;
    emit pairingStatusChanged();
}

//-----------------------------------------------------------------------------
QString
PairingManager::pairingStatusStr() const
{
    return _statusString;
}

#if QGC_GST_MICROHARD_ENABLED
//-----------------------------------------------------------------------------
void
PairingManager::startMicrohardPairing()
{
    stopPairing();
    _pairRetryCount = 0;
    setPairingStatus(PairingActive, tr("Pairing..."));
    _parsePairingJson(_assumeMicrohardPairingJson(), false);
}
#endif

//-----------------------------------------------------------------------------
void
PairingManager::stopPairing()
{
#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    pairingNFC.stop();
#endif
    _stopUpload();
    setPairingStatus(PairingIdle, "");
}

//-----------------------------------------------------------------------------
void
PairingManager::disconnectDevice(const QString& name)
{
    _removeUDPLink(name);
}

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
//-----------------------------------------------------------------------------
void
PairingManager::startNFCScan()
{
    stopPairing();
    setPairingStatus(PairingActive, tr("Pairing..."));
    pairingNFC.start();
}

#endif

//-----------------------------------------------------------------------------
#ifdef __android__
static const char kJniClassName[] {"org/mavlink/qgroundcontrol/QGCActivity"};

//-----------------------------------------------------------------------------
static void jniNFCTagReceived(JNIEnv *envA, jobject thizA, jstring messageA)
{
    Q_UNUSED(thizA);

    const char *stringL = envA->GetStringUTFChars(messageA, nullptr);
    QString ndef = QString::fromUtf8(stringL);
    envA->ReleaseStringUTFChars(messageA, stringL);
    if (envA->ExceptionCheck())
        envA->ExceptionClear();
    qCDebug(PairingManagerLog) << "NDEF Tag Received: " << ndef;
    qgcApp()->toolbox()->pairingManager()->jsonReceived(ndef);
}

//-----------------------------------------------------------------------------
void PairingManager::setNativeMethods(void)
{
    //  REGISTER THE C++ FUNCTION WITH JNI
    JNINativeMethod javaMethods[] {
        {"nativeNFCTagReceived", "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniNFCTagReceived)}
    };

    QAndroidJniEnvironment jniEnv;
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    jclass objectClass = jniEnv->FindClass(kJniClassName);
    if(!objectClass) {
        qWarning() << "Couldn't find class:" << kJniClassName;
        return;
    }

    jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));

    if (val < 0) {
        qWarning() << "Error registering methods: " << val;
    } else {
        qCDebug(PairingManagerLog) << "Native Functions Registered";
    }

    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }
}
#endif
//-----------------------------------------------------------------------------
