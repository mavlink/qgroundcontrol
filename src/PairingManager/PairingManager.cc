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

static const qint64  min_time_between_connects = 5000;
static const int     uploadRetries = 5;
static const int     pairRetryWait = 3000;
static const QString tempPrefix = "temp";
static const QString chSeparator = "\t";
static const QString timeFormat = "yyyy-MM-dd HH:mm:ss";

//-----------------------------------------------------------------------------
PairingManager::PairingManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _aes("J6+KuWh9K2!hG(F'", 0x368de30e8ec063ce)
    , _uploadManager(this)
{
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
    _setEnabled();

    connect(this, &PairingManager::setPairingStatus, this, &PairingManager::_setPairingStatus, Qt::QueuedConnection);
    connect(this, &PairingManager::startUpload, this, &PairingManager::_startUpload, Qt::QueuedConnection);
    connect(this, &PairingManager::startCommand, this, &PairingManager::_startCommand, Qt::QueuedConnection);
    connect(this, &PairingManager::connectToPairedDevice, this, &PairingManager::_connectToPairedDevice, Qt::QueuedConnection);
    connect(&_reconnectTimer, &QTimer::timeout, this, &PairingManager::_autoConnect, Qt::QueuedConnection);
    connect(_toolbox->settingsManager()->appSettings()->usePairing(), &Fact::rawValueChanged, this, &PairingManager::_setEnabled, Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void
PairingManager::_setEnabled()
{
    setUsePairing(_toolbox->settingsManager()->appSettings()->usePairing()->rawValue().toBool());
}

//-----------------------------------------------------------------------------
void
PairingManager::setUsePairing(bool set)
{
    if (_usePairing == set) {
        return;
    }
    _usePairing = set;

    if (_usePairing) {
        _reconnectTimer.setSingleShot(false);
        _reconnectTimer.start(1000);
        _readPairingConfig();
        _updatePairedDeviceNameList();
    } else {
        _reconnectTimer.stop();
    }

    _toolbox->microhardManager()->updateSettings();
    if (videoCanRestart()) {
        _toolbox->videoManager()->startVideo();
    }
    emit usePairingChanged();
}

//-----------------------------------------------------------------------------
void
PairingManager::_pairingCompleted(const QString& tempName, const QString& newName, const QString& devicePublicKey)
{
    QJsonDocument jsonDoc = _getPairingJsonDoc(tempName, true);
    QJsonObject jsonObj = jsonDoc.object();
    jsonObj.insert("Name", newName);
    jsonObj.insert("PublicKey", devicePublicKey);
    jsonDoc.setObject(jsonObj);
    _writeJson(jsonDoc, _pairingCacheFile(newName));
    _updatePairedDeviceNameList();
    setPairingStatus(PairingSuccess, tr("Pairing Successfull"));
    _toolbox->microhardManager()->switchToConnectionEncryptionKey(_encryptionKey);
    // Automatically connect to newly paired device
    connectToDevice(newName);
}

//-----------------------------------------------------------------------------
int
PairingManager::_getDeviceChannel(const QString& name)
{
    if (!_devices.contains(name)) {
        return 0;
    }

    QJsonObject jsonObj = _devices[name].object();
    return jsonObj["CC"].toInt();
}

//-----------------------------------------------------------------------------
QDateTime
PairingManager::_getDeviceConnectTime(const QString& name)
{
    if (_devices.contains(name)) {
        QJsonObject jsonObj = _devices[name].object();
        QString ts = jsonObj["LastConnection"].toString();
        if (!ts.isEmpty()) {
            return QDateTime::fromString(ts, timeFormat);
        }
    }

    return QDateTime::currentDateTime().addYears(-10);
}

//-----------------------------------------------------------------------------
QString
PairingManager::_getDeviceIP(const QString& name)
{
    if (!_devices.contains(name)) {
        return "";
    }
    QJsonObject jsonObj = _devices[name].object();
    return jsonObj["IP"].toString();
}

//-----------------------------------------------------------------------------
QStringList
PairingManager::connectedDeviceNameList()
{
    QStringList list;

    QMapIterator<QString, LinkInterface*> i(_connectedDevices);
    while (i.hasNext()) {
        i.next();
        if (i.value()) {
            list.append(tr("Channel: ") + QString::number(_getDeviceChannel(i.key())).rightJustified(2, '0') + chSeparator + i.key());
        }
    }
    std::sort(list.begin(), list.end(),
        [this](const QString &name1, const QString &name2)
        {
            QDateTime dt1 = _getDeviceConnectTime(extractName(name1));
            QDateTime dt2 = _getDeviceConnectTime(extractName(name2));
            return dt1 > dt2;
        });

    return list;
}

//-----------------------------------------------------------------------------
QStringList
PairingManager::pairedDeviceNameList()
{
    QStringList list;
    for (QString name : _devices.keys())
    {
        if (!_connectedDevices.contains(name)) {
            list.append(tr("Channel: ") + QString::number(_getDeviceChannel(name)).rightJustified(2, '0') + chSeparator + name);
        }
    }
    std::sort(list.begin(), list.end(),
        [this](const QString &name1, const QString &name2)
        {
            QDateTime dt1 = _getDeviceConnectTime(extractName(name1));
            QDateTime dt2 = _getDeviceConnectTime(extractName(name2));
            return dt1 > dt2;
        });

    return list;
}

//-----------------------------------------------------------------------------
void
PairingManager::_linkActiveChanged(LinkInterface* link, bool active, int vehicleID)
{
    Q_UNUSED(vehicleID)

    QMapIterator<QString, LinkInterface*> i(_connectedDevices);
    while (i.hasNext()) {
        i.next();
        if (i.value() == link) {
            if (!active) {
                _removeUDPLink(i.key());
                connectToDevice(i.key());
            }
            break;
        }
    }
}

//-----------------------------------------------------------------------------
void
PairingManager::_createUDPLink(const QString& name, quint16 port)
{
    _removeUDPLink(name);
    UDPConfiguration* udpConfig = new UDPConfiguration(name + " (Paired)");
    udpConfig->setLocalPort(port);
    udpConfig->setDynamic(true);
    SharedLinkConfigurationPointer linkConfig = _toolbox->linkManager()->addConfiguration(udpConfig);
    LinkInterface* link = _toolbox->linkManager()->createConnectedLink(linkConfig);
    connect(link, &LinkInterface::activeChanged, this, &PairingManager::_linkActiveChanged, Qt::QueuedConnection);
    _connectedDevices[name] = link;

    QJsonDocument jsonDoc = _devices[name];
    QJsonObject jsonObj = jsonDoc.object();
    jsonObj.insert("LastConnection", QDateTime::currentDateTime().toString(timeFormat));
    jsonDoc.setObject(jsonObj);
    _devices[name] = jsonDoc;
    _writeJson(jsonDoc, _pairingCacheFile(name));

    _updateConnectedDevices();
}

//-----------------------------------------------------------------------------
void
PairingManager::_removeUDPLink(const QString& name)
{
    if (_connectedDevices.contains(name)) {
        _toolbox->linkManager()->disconnectLink(_connectedDevices[name]);
        _connectedDevices.remove(name);
        _updateConnectedDevices();
        _toolbox->videoManager()->stopVideo();
    }
}

//-----------------------------------------------------------------------------
bool
PairingManager::_connectionCompleted(const QString& response)
{
    QStringList a = QString::fromStdString(_rsa.decrypt(response.toStdString())).split(";");
    if (a.length() != 2 && !_device_rsa.verify(a[0].toStdString(), a[1].toStdString())) {
        return false;
    }
    _lastConnected = a[0];
    _createUDPLink(_lastConnected, 24550);
    _toolbox->videoManager()->startVideo();
    emit connectedVehicleChanged();
    setPairingStatus(PairingConnected, tr("Connection Successfull"));

    return true;
}

//-----------------------------------------------------------------------------
void
PairingManager::_startCommand(const QString& name, const QString& url, const QString& content)
{
    qCDebug(PairingManagerLog) << "Starting command: " << url;
    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = _uploadManager.post(req, content.toUtf8());
    reply->setProperty("name", QVariant(name));
    reply->setProperty("url", QVariant(url));
    reply->setProperty("content", QVariant(content));
    connect(reply, &QNetworkReply::finished, this, &PairingManager::_commandFinished, Qt::QueuedConnection);
    connect(this, SIGNAL(stopUpload()), reply, SLOT(abort()), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void
PairingManager::_startUpload(const QString& name, const QString& pairURL, const QJsonDocument& jsonDoc, bool signAndEncrypt, int retries)
{
    QString str = jsonDoc.toJson(QJsonDocument::JsonFormat::Compact);
    qCDebug(PairingManagerLog) << "Starting upload to: " << pairURL << " " << _removeRSAkey(str);

    std::string data = _aes.encrypt(str.toStdString());
    if (signAndEncrypt) {
        data = _device_rsa.encrypt(data + ";" + _rsa.sign(data));
    }
    _startUploadRequest(name, pairURL, QString::fromStdString(data), retries);
}

//-----------------------------------------------------------------------------
void
PairingManager::_startUploadRequest(const QString& name, const QString& url, const QString& data, int retries)
{
    if (url.contains("pair")) {
        _devicesToConnect.clear();
    }
    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = _uploadManager.post(req, data.toUtf8());
    reply->setProperty("name", QVariant(name));
    reply->setProperty("url", QVariant(url));
    reply->setProperty("content", QVariant(data));
    reply->setProperty("retries", QVariant(retries));
    connect(reply, &QNetworkReply::finished, this, &PairingManager::_uploadFinished, Qt::QueuedConnection);
    connect(this, SIGNAL(stopUpload()), reply, SLOT(abort()), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void
PairingManager::_uploadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        return;
    }
    bool removeTempFile = false;
    QString url = reply->property("url").toString();
    QString name = reply->property("name").toString();
    int retries = reply->property("retries").toInt();
    if (reply->error() == QNetworkReply::NoError) {
        qCDebug(PairingManagerLog) << "Upload finished.";
        QByteArray bytes = reply->readAll();
        QString str = QString::fromStdString(_aes.decrypt(QString::fromUtf8(bytes.data(), bytes.size()).toStdString()));
        qCDebug(PairingManagerLog) << "Reply: " << _removeRSAkey(str);
        auto a = str.split(";");
        if (a[0] == "Accepted" && a.length() > 2) {
            _pairingCompleted(name, a[1], a[2]);
            removeTempFile = true;
        } else if (a[0] == "Connected" && a.length() > 1) {
            if (!_connectionCompleted(a[1])) {
                setPairingStatus(PairingConnectionRejected, tr("Connection verification failed."));
                qCDebug(PairingManagerLog) << "Connection error: failed to verify remote vehicle ID";
            }
        } else if (a[0] == "Connection" && a.length() > 1) {
            setPairingStatus(PairingConnectionRejected, tr("Connection Rejected"));
            qCDebug(PairingManagerLog) << "Connection error: " << str;
        } else {
            removeTempFile = true;
            setPairingStatus(PairingRejected, tr("Pairing Rejected"));
            qCDebug(PairingManagerLog) << "Pairing error: " << str;
        }
    } else if(url.contains("connect")) {
        qCDebug(PairingManagerLog) << "Connect error: " + reply->errorString();
        if (!reply->errorString().contains("canceled")) {
            connectToDevice(name);
        }
    } else {
        if(--retries<=0) {
            removeTempFile = true;
            qCDebug(PairingManagerLog) << "Giving up";
            setPairingStatus(PairingError, tr("No Response From Vehicle"));
        } else {
            qCDebug(PairingManagerLog) << "Pairing error: " + reply->errorString();
            QString content = reply->property("content").toString();
            QTimer::singleShot(pairRetryWait, [this, name, url, content, retries]()
            {
                emit _startUploadRequest(name, url, content, retries);
            });
        }
    }
    reply->deleteLater();
    if (removeTempFile) {
        QFile file(_pairingCacheFile(name));
        file.remove();
    }
}

//-----------------------------------------------------------------------------
void
PairingManager::_commandFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        return;
    }
    QString url = reply->property("url").toString();
    QString content = reply->property("content").toString();

    if (reply->error() == QNetworkReply::NoError) {
        if (url.contains("channel")) {
            int channel = content.toInt();
            QString name = reply->property("name").toString();
            QJsonDocument jsonDoc = _devices[name];
            QJsonObject jsonObj = jsonDoc.object();
            jsonObj.insert("CC", channel);
            jsonDoc.setObject(jsonObj);
            _writeJson(jsonDoc, _pairingCacheFile(name));
            _devices[name] = jsonDoc;
            emit deviceListChanged();
            _toolbox->microhardManager()->setConnectChannel(channel);
            _toolbox->microhardManager()->updateSettings();
        }
    }
    reply->deleteLater();
}

//-----------------------------------------------------------------------------
void
PairingManager::connectToDevice(const QString& name)
{
    if (name.isEmpty()) {
        return;
    }

    QString ip = _getDeviceIP(name);
    // If multiple vehicles share same IP then disconnect
    for (QString n : _connectedDevices.keys()) {
        if (ip == _getDeviceIP(n)) {
            disconnectDevice(n);
            break;
        }
    }
    // If multiple vehicles share same IP then do not try to autoconnect anymore
    for (QString n : _devicesToConnect.keys()) {
        if (ip == _getDeviceIP(n)) {
            _devicesToConnect.remove(n);
            break;
        }
    }

    setPairingStatus(PairingConnecting, tr("Connecting to %1").arg(name));
    _devicesToConnect[name] = QDateTime::currentMSecsSinceEpoch() - min_time_between_connects;
}

//-----------------------------------------------------------------------------
void
PairingManager::_autoConnect()
{
    QString connectingTo = "";

    for (QString name : _devicesToConnect.keys())
    {
        if (!connectingTo.isEmpty()) {
            connectingTo += ", ";
        }
        connectingTo += name;
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        if (currentTime - _devicesToConnect.value(name) >= min_time_between_connects) {
            _devicesToConnect[name] = currentTime;
            emit connectToPairedDevice(name);
        }
    }
    if (!connectingTo.isEmpty()) {
        setPairingStatus(PairingConnecting, tr("Connecting to %1").arg(connectingTo));
    }
}

//-----------------------------------------------------------------------------
void
PairingManager::_connectToPairedDevice(const QString& name)
{
    _devicesToConnect.remove(name);
    QFile file(_pairingCacheFile(name));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString json = file.readAll();
    if (!json.isEmpty()) {
        _parsePairingJsonAndConnect(json);
    }
}

//-----------------------------------------------------------------------------
void
PairingManager::_updateConnectedDevices()
{
    _gcsJsonDoc = _getPairingJsonDoc("gcs");
    if (_gcsJsonDoc.isNull() || !_gcsJsonDoc.isObject()) {
        return;
    }

    QJsonObject jsonObj = _gcsJsonDoc.object();
    jsonObj.remove("CONNECTED_DEVICE");
    QString cd = "";
    QMapIterator<QString, LinkInterface*> i(_connectedDevices);
    while (i.hasNext()) {
        i.next();
        if (!cd.isEmpty()) {
            cd += ";";
        }
        cd += i.key();
    }
    if (!cd.isEmpty()) {
        jsonObj.insert("CONNECTED_DEVICE", cd);
    }
    _gcsJsonDoc.setObject(jsonObj);
    _writeJson(_gcsJsonDoc, _pairingCacheFile("gcs"));
    emit deviceListChanged();
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_getPairingJsonDoc(const QString& name, bool remove)
{
    QFile file(_pairingCacheFile(name));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString json = QString::fromStdString(_aes.decrypt(file.readAll().toStdString()));
    file.close();
    if (remove) {
        file.remove();
    }

    return QJsonDocument::fromJson(json.toUtf8());
}

//-----------------------------------------------------------------------------
QVariantMap
PairingManager::_getPairingMap(const QString& name)
{
    QJsonDocument jsonDoc = _getPairingJsonDoc(name);
    QJsonObject jsonObj = jsonDoc.object();
    QVariantMap map = jsonObj.toVariantMap();
    QString pport = map["PP"].toString();
    if (pport.length() == 0) {
        map["PP"] = "29351";
    }

    return map;
}

//-----------------------------------------------------------------------------
QString
PairingManager::extractChannel(const QString& name)
{
    if (name.contains(chSeparator)) {
        return name.split(chSeparator)[0];
    }
    return "";
}

//-----------------------------------------------------------------------------
QString
PairingManager::extractName(const QString& name)
{
    if (name.contains(chSeparator)) {
        return name.split(chSeparator)[1];
    }
    return name;
}

//-----------------------------------------------------------------------------
void
PairingManager::removePairedDevice(const QString& name)
{
    QVariantMap map = _getPairingMap(name);
    QFile file(_pairingCacheFile(name));
    file.remove();
    _removeUDPLink(name);

    QString unpairURL = "http://" + map["IP"].toString() + ":" + map["PP"].toString() + "/unpair";
    emit startCommand(name, unpairURL, "");
    _updatePairedDeviceNameList();
    setPairingStatus(PairingIdle, "");
}

//-----------------------------------------------------------------------------
void
PairingManager::setConnectingChannel(int channel)
{
    if (_connectedDevices.empty()) {
        _toolbox->microhardManager()->setConnectChannel(channel);
        _toolbox->microhardManager()->updateSettings();
        return;
    }

    for (QString name : _connectedDevices.keys())
    {
        _setConnectingChannel(name, channel);
    }
}

//-----------------------------------------------------------------------------
void
PairingManager::_setConnectingChannel(const QString& name, int channel)
{
    QVariantMap map = _getPairingMap(name);
    QString cmd = "http://" + map["IP"].toString() + ":" + map["PP"].toString() + "/channel";
    emit startCommand(name, cmd, QString::number(channel));
}

//-----------------------------------------------------------------------------
QString
PairingManager::_random_string(uint length)
{
    srand(static_cast<unsigned int>(time(nullptr)));
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
    _gcsJsonDoc = _getPairingJsonDoc("gcs");
    if (_gcsJsonDoc.isNull() || !_gcsJsonDoc.isObject()) {
        _resetPairingConfig();
        return;
    }

    QJsonObject jsonObj = _gcsJsonDoc.object();
    if (!jsonObj.contains("EK") || !jsonObj.contains("PublicKey") || !jsonObj.contains("PrivateKey")) {
        _resetPairingConfig();
        return;
    }

    if (jsonObj.contains("CONNECTED_DEVICE")) {
        foreach (auto cd, jsonObj.value("CONNECTED_DEVICE").toString().split(";")) {
            connectToDevice(cd);
        }
    }
    _encryptionKey = jsonObj.value("EK").toString();
    _publicKey = jsonObj.value("PublicKey").toString();
    _rsa.generate_private(jsonObj.value("PrivateKey").toString().toStdString());
    _rsa.generate_public(_publicKey.toStdString());
}

//-----------------------------------------------------------------------------
void
PairingManager::_resetPairingConfig()
{
    QJsonDocument json;
    QJsonObject   jsonObj;

    _rsa.generate();
    _publicKey = QString::fromStdString(_rsa.get_public_key());
    _encryptionKey = _random_string(8);
    jsonObj.insert("EK", _encryptionKey);
    jsonObj.insert("PublicKey", _publicKey);
    jsonObj.insert("PrivateKey", QString::fromStdString(_rsa.get_private_key()));
    json.setObject(jsonObj);

    _writeJson(json, _pairingCacheFile("gcs"));
}

//-----------------------------------------------------------------------------
void
PairingManager::_updatePairedDeviceNameList()
{
    _devices.clear();
    QDirIterator it(_pairingCacheDir().absolutePath(), QDir::Files);
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        if (fileInfo.fileName() != "gcs") {
            QString name = fileInfo.fileName();
            _devices[name] = _getPairingJsonDoc(name);
        }
    }
    if (_devices.empty()) {
        _resetPairingConfig();
    }
    emit deviceListChanged();
}

//-----------------------------------------------------------------------------
void
PairingManager::_parsePairingJsonAndConnect(const QString& jsonEnc)
{
    QString json = QString::fromStdString(_aes.decrypt(jsonEnc.toStdString()));
    if (json == "") {
        json = jsonEnc;
    }
    QJsonDocument receivedJsonDoc = QJsonDocument::fromJson(json.toUtf8());

    if (receivedJsonDoc.isNull()) {
        setPairingStatus(PairingError, tr("Invalid Pairing File"));
        qCDebug(PairingManagerLog) << "Failed to create Pairing JSON doc.";
        return;
    }
    if (!receivedJsonDoc.isObject()) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON is not an object.";
        return;
    }

    QJsonObject jsonObj = receivedJsonDoc.object();
    if (jsonObj.isEmpty()) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON object is empty.";
        return;
    }

    QVariantMap remotePairingMap = jsonObj.toVariantMap();
    QString linkType = remotePairingMap["LT"].toString();
    QString pport    = remotePairingMap["PP"].toString();
    if (pport.length() == 0) {
        pport = "29351";
    }

    QString name = remotePairingMap.contains("Name") ? remotePairingMap["Name"].toString() : "";

    if (remotePairingMap.contains("PublicKey")) {
        _device_rsa.generate_public(remotePairingMap["PublicKey"].toString().toStdString());
    }

    if (linkType.length() == 0) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON is malformed.";
        return;
    }

    QString pairURL = "http://" + remotePairingMap["IP"].toString() + ":" + pport;
    QJsonDocument responseJsonDoc;

    pairURL +=  + "/connect";
    if (linkType == "ZT") {
        responseJsonDoc = _createZeroTierConnectJson(remotePairingMap);
    } else if (linkType == "MH") {
        responseJsonDoc = _createMicrohardConnectJson(remotePairingMap);
    }

    if (linkType == "ZT") {
        _toolbox->settingsManager()->appSettings()->enableMicrohard()->setRawValue(false);
        _toolbox->settingsManager()->appSettings()->enableTaisync()->setRawValue(false);
    } else if (linkType == "MH") {
        _toolbox->settingsManager()->appSettings()->enableMicrohard()->setRawValue(true);
        _toolbox->settingsManager()->appSettings()->enableTaisync()->setRawValue(false);
        if (remotePairingMap.contains("AIP")) {
            _toolbox->microhardManager()->setRemoteIPAddr(remotePairingMap["AIP"].toString());
        }
        if (remotePairingMap.contains("CU")) {
            _toolbox->microhardManager()->setConfigUserName(remotePairingMap["CU"].toString());
        }
        if (remotePairingMap.contains("CP")) {
            _toolbox->microhardManager()->setConfigPassword(remotePairingMap["CP"].toString());
        }
    }
    if (linkType == "MH") {
        _toolbox->microhardManager()->switchToConnectionEncryptionKey(remotePairingMap["EK"].toString());
        if (remotePairingMap.contains("CC")) {
            _toolbox->microhardManager()->setConnectChannel(remotePairingMap["CC"].toInt());
        }
        _toolbox->microhardManager()->updateSettings();
    }
    emit startUpload(name, pairURL, responseJsonDoc, true, uploadRetries);
}

//-----------------------------------------------------------------------------
QString
PairingManager::_getLocalIPInNetwork(const QString& remoteIP, int num)
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
QDir
PairingManager::_pairingCacheTempDir()
{
    const QString spath(QFileInfo(QSettings().fileName()).dir().absolutePath());
    QDir dir = spath + QDir::separator() + "PairingCache" + QDir::separator() + tempPrefix;
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return dir;
}

//-----------------------------------------------------------------------------
QString
PairingManager::_pairingCacheFile(const QString& uavName)
{
    if (uavName.startsWith(tempPrefix)) {
        return _pairingCacheTempDir().filePath(uavName.mid(tempPrefix.length()));
    }
    return _pairingCacheDir().filePath(uavName);
}

//-----------------------------------------------------------------------------
QString
PairingManager::_removeRSAkey(const QString& s)
{
    int i = s.indexOf("-----BEGIN RSA");
    if (i < 0) {
        return s;
    }
    return s.left(i);
}

//-----------------------------------------------------------------------------
void
PairingManager::_writeJson(const QJsonDocument& jsonDoc, const QString& fileName)
{
    QString val = jsonDoc.toJson(QJsonDocument::JsonFormat::Compact);
    qCDebug(PairingManagerLog) << "Write json " << _removeRSAkey(val);
    QString enc = QString::fromStdString(_aes.encrypt(val.toStdString()));

    QFile file(fileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    file.write(enc.toUtf8());
    file.close();
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_createZeroTierPairingJson(const QVariantMap& remotePairingMap)
{
    QString localIP = _getLocalIPInNetwork(remotePairingMap["IP"].toString(), 2);

    QJsonObject jsonObj;
    jsonObj.insert("LT", "ZT");
    jsonObj.insert("IP", localIP);
    jsonObj.insert("P", 14550);
    jsonObj.insert("PublicKey", _publicKey);
    return QJsonDocument(jsonObj);
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_createMicrohardPairingJson(const QVariantMap& remotePairingMap)
{
    QString localIP = _getLocalIPInNetwork(remotePairingMap["IP"].toString(), 3);

    QJsonObject jsonObj;
    jsonObj.insert("LT", "MH");
    jsonObj.insert("IP", localIP);
    jsonObj.insert("EK", _encryptionKey);
    jsonObj.insert("PublicKey", _publicKey);
    jsonObj.insert("CC", _toolbox->microhardManager()->connectingChannel());
    return QJsonDocument(jsonObj);
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_createZeroTierConnectJson(const QVariantMap& remotePairingMap)
{
    QString localIP = _getLocalIPInNetwork(remotePairingMap["IP"].toString(), 2);

    QJsonObject jsonObj;
    jsonObj.insert("LT", "ZT");
    jsonObj.insert("IP", localIP);
    jsonObj.insert("P", 14550);
    return QJsonDocument(jsonObj);
}

//-----------------------------------------------------------------------------
QJsonDocument
PairingManager::_createMicrohardConnectJson(const QVariantMap& remotePairingMap)
{
    QString localIP = _getLocalIPInNetwork(remotePairingMap["IP"].toString(), 3);

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
PairingManager::_setPairingStatus(PairingStatus status, const QString& statusStr)
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

//-----------------------------------------------------------------------------
void
PairingManager::jsonReceivedStartPairing(const QString& jsonEnc)
{
    QString json = QString::fromStdString(_aes.decrypt(jsonEnc.toStdString()));
    if (json == "") {
        json = jsonEnc;
    }

    QJsonDocument receivedJsonDoc = QJsonDocument::fromJson(json.toUtf8());

    if (receivedJsonDoc.isNull()) {
        setPairingStatus(PairingError, tr("Invalid Pairing File"));
        qCDebug(PairingManagerLog) << "Failed to create Pairing JSON doc.";
        return;
    }
    if (!receivedJsonDoc.isObject()) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON is not an object.";
        return;
    }

    QJsonObject jsonObj = receivedJsonDoc.object();

    if (jsonObj.isEmpty()) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON object is empty.";
        return;
    }

    QVariantMap remotePairingMap = jsonObj.toVariantMap();
    QString linkType  = remotePairingMap["LT"].toString();
    QString pport     = remotePairingMap["PP"].toString();
    if (pport.length() == 0) {
        pport = "29351";
    }

    QString tempName = _random_string(16);
    _writeJson(receivedJsonDoc, _pairingCacheFile(tempName));

    if (remotePairingMap.contains("PublicKey")) {
        _device_rsa.generate_public(remotePairingMap["PublicKey"].toString().toStdString());
    }

    if (linkType.length() == 0) {
        setPairingStatus(PairingError, tr("Error Parsing Pairing File"));
        qCDebug(PairingManagerLog) << "Pairing JSON is malformed.";
        return;
    }

    QString pairURL = "http://" + remotePairingMap["IP"].toString() + ":" + pport + "/pair";
    QJsonDocument responseJsonDoc;
    if (linkType == "ZT") {
        responseJsonDoc = _createZeroTierPairingJson(remotePairingMap);
    } else if (linkType == "MH") {
        responseJsonDoc = _createMicrohardPairingJson(remotePairingMap);
    }

    if (linkType == "ZT") {
        _toolbox->settingsManager()->appSettings()->enableMicrohard()->setRawValue(false);
        _toolbox->settingsManager()->appSettings()->enableTaisync()->setRawValue(false);
    } else if (linkType == "MH") {
        _toolbox->settingsManager()->appSettings()->enableMicrohard()->setRawValue(true);
        _toolbox->settingsManager()->appSettings()->enableTaisync()->setRawValue(false);
        if (remotePairingMap.contains("AIP")) {
            _toolbox->microhardManager()->setRemoteIPAddr(remotePairingMap["AIP"].toString());
        }
        if (remotePairingMap.contains("CU")) {
            _toolbox->microhardManager()->setConfigUserName(remotePairingMap["CU"].toString());
        }
        if (remotePairingMap.contains("CP")) {
            _toolbox->microhardManager()->setConfigPassword(remotePairingMap["CP"].toString());
        }
        _toolbox->videoManager()->stopVideo();
        _toolbox->microhardManager()->switchToPairingEncryptionKey();
        _toolbox->microhardManager()->updateSettings();
    }
    emit startUpload(tempName, pairURL, responseJsonDoc, false, uploadRetries);
}

#if QGC_GST_MICROHARD_ENABLED
//-----------------------------------------------------------------------------
void
PairingManager::startMicrohardPairing()
{
    stopPairing();
    setPairingStatus(PairingActive, tr("Pairing..."));

    _toolbox->videoManager()->stopVideo();
    _toolbox->microhardManager()->switchToPairingEncryptionKey();
    _toolbox->microhardManager()->updateSettings();

    QJsonDocument receivedJsonDoc;
    QJsonObject   jsonObj;

    jsonObj.insert("LT", "MH");
    QString remoteIPAddr = _toolbox->microhardManager()->remoteIPAddr();
    QString ipPrefix = remoteIPAddr.left(remoteIPAddr.lastIndexOf('.'));
    jsonObj.insert("IP", ipPrefix + ".10");
    jsonObj.insert("AIP", remoteIPAddr);
    jsonObj.insert("CU", _toolbox->microhardManager()->configUserName());
    jsonObj.insert("CP", _toolbox->microhardManager()->configPassword());
    jsonObj.insert("CC", _toolbox->microhardManager()->connectingChannel());
    jsonObj.insert("EK", _encryptionKey);
    jsonObj.insert("PublicKey", _publicKey);

    QVariantMap remotePairingMap = jsonObj.toVariantMap();
    QString linkType  = remotePairingMap["LT"].toString();
    QString pport     = remotePairingMap["PP"].toString();
    if (pport.length() == 0) {
        pport = "29351";
        jsonObj.insert("PP", pport);
    }
    receivedJsonDoc.setObject(jsonObj);

    QString tempName = tempPrefix + _random_string(16);
    _writeJson(receivedJsonDoc, _pairingCacheFile(tempName));

    QString pairURL = "http://" + remotePairingMap["IP"].toString() + ":" + pport + "/pair";
    QJsonDocument responseJsonDoc = _createMicrohardPairingJson(remotePairingMap);
    emit startUpload(tempName, pairURL, responseJsonDoc, false, uploadRetries);
}
#endif

//-----------------------------------------------------------------------------
void
PairingManager::stopPairing()
{
#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    pairingNFC.stop();
#endif
    emit stopUpload();
    setPairingStatus(PairingIdle, "");
}

//-----------------------------------------------------------------------------
void
PairingManager::disconnectDevice(const QString& name)
{
    _removeUDPLink(name);
    _toolbox->videoManager()->stopVideo();
    setPairingStatus(PairingIdle, "");
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
    qgcApp()->toolbox()->pairingManager()->jsonReceivedStartPairing(ndef);
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
