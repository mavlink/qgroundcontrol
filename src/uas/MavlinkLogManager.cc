/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MavlinkLogManager.h"
#include "QGCApplication.h"
#include <QQmlContext>
#include <QQmlProperty>
#include <QQmlEngine>
#include <QtQml>
#include <QSettings>
#include <QHttpPart>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>

QGC_LOGGING_CATEGORY(MavlinkLogManagerLog, "MavlinkLogManagerLog")

static const char* kEmailAddressKey         = "MavlinkLogEmail";
static const char* kDescriptionsKey         = "MavlinkLogDescription";
static const char* kDefaultDescr            = "QGroundControl Session";
static const char* kPx4URLKey               = "MavlinkLogURL";
static const char* kDefaultPx4URL           = "http://logs.px4.io/upload";
static const char* kEnableAutoUploadKey     = "EnableAutoUploadKey";
static const char* kEnableAutoStartKey      = "EnableAutoStartKey";

//-----------------------------------------------------------------------------
MavlinkLogFiles::MavlinkLogFiles(MavlinkLogManager *manager, const QString& filePath)
    : _manager(manager)
    , _size(0)
    , _selected(false)
    , _uploading(false)
    , _progress(0)
{
    QFileInfo fi(filePath);
    _name = fi.baseName();
    _size = (quint32)fi.size();
}

//-----------------------------------------------------------------------------
void
MavlinkLogFiles::setSelected(bool selected)
{
    _selected = selected;
    emit selectedChanged();
    emit _manager->selectedCountChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogFiles::setUploading(bool uploading)
{
    _uploading = uploading;
    emit uploadingChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogFiles::setProgress(qreal progress)
{
    _progress = progress;
    emit progressChanged();
}

//-----------------------------------------------------------------------------
MavlinkLogManager::MavlinkLogManager(QGCApplication* app)
    : QGCTool(app)
    , _enableAutoUpload(true)
    , _enableAutoStart(true)
    , _nam(NULL)
    , _currentLogfile(NULL)
    , _vehicle(NULL)
    , _logRunning(false)
{
    //-- Get saved settings
    QSettings settings;
    setEmailAddress(settings.value(kEmailAddressKey, QString()).toString());
    setDescription(settings.value(kDescriptionsKey, QString(kDefaultDescr)).toString());
    setUploadURL(settings.value(kPx4URLKey, QString(kDefaultPx4URL)).toString());
    setEnableAutoUpload(settings.value(kEnableAutoUploadKey, true).toBool());
    setEnableAutoStart(settings.value(kEnableAutoStartKey, true).toBool());
    //-- Logging location
    _logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    _logPath += "/MavlinkLogs";
    if(!QDir(_logPath).exists()) {
        if(QDir().mkpath(_logPath)) {
            qCCritical(MavlinkLogManagerLog) << "Could not create Mavlink log download path:" << _logPath;
        }
    }
    //-- Load current list of logs
    QDirIterator it(_logPath, QStringList() << "*.ulg", QDir::Files);
    while(it.hasNext()) {
        _logFiles.append(new MavlinkLogFiles(this, it.next()));
    }
    qCDebug(MavlinkLogManagerLog) << "Mavlink logs directory:" << _logPath;
}

//-----------------------------------------------------------------------------
MavlinkLogManager::~MavlinkLogManager()
{
    _logFiles.clear();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<MavlinkLogManager>("QGroundControl.MavlinkLogManager", 1, 0, "MavlinkLogManager", "Reference only");
   connect(toolbox->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &MavlinkLogManager::_activeVehicleChanged);

   //    _uploadURL = "http://192.168.1.21/px4";
   //    _uploadURL = "http://192.168.1.9:8080";
   //    _emailAddress = "gus.grubba.com";
   //    _description = "Test from QGroundControl - Discard";
   //    _sendLog("/Users/gus/github/work/logs/simulator.ulg");

}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::setEmailAddress(QString email)
{
    _emailAddress = email;
    QSettings settings;
    settings.setValue(kEmailAddressKey, email);
    emit emailAddressChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::setDescription(QString description)
{
    _description = description;
    QSettings settings;
    settings.setValue(kDescriptionsKey, description);
    emit descriptionChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::setUploadURL(QString url)
{
    _uploadURL = url;
    if(_uploadURL.isEmpty()) {
        _uploadURL = kDefaultPx4URL;
    }
    QSettings settings;
    settings.setValue(kPx4URLKey, _uploadURL);
    emit uploadURLChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::setEnableAutoUpload(bool enable)
{
    _enableAutoUpload = enable;
    QSettings settings;
    settings.setValue(kEnableAutoUploadKey, enable);
    emit enableAutoUploadChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::setEnableAutoStart(bool enable)
{
    _enableAutoStart = enable;
    QSettings settings;
    settings.setValue(kEnableAutoStartKey, enable);
    emit enableAutoStartChanged();
}

//-----------------------------------------------------------------------------
bool
MavlinkLogManager::busy()
{
    return _currentLogfile != NULL;
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::uploadLog()
{
    if(_currentLogfile) {
        _currentLogfile->setUploading(false);
    }
    for(int i = 0; i < _logFiles.count(); i++ ) {
        _currentLogfile = qobject_cast<MavlinkLogFiles*>(_logFiles.get(i));
        Q_ASSERT(_currentLogfile);
        if(_currentLogfile->selected()) {
            _currentLogfile->setSelected(false);
            _currentLogfile->setUploading(true);
            _currentLogfile->setProgress(0.0);
            QString filePath = _logPath;
            filePath += "/";
            filePath += _currentLogfile->name();
            filePath += ".ulg";
            _sendLog(filePath);
            emit busyChanged();
            return;
        }
    }
    _currentLogfile = NULL;
    emit busyChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::deleteLog()
{
    //-- TODO
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::cancelUpload()
{
    for(int i = 0; i < _logFiles.count(); i++ ) {
        MavlinkLogFiles* pLogFile = qobject_cast<MavlinkLogFiles*>(_logFiles.get(i));
        Q_ASSERT(pLogFile);
        if(pLogFile->selected() && pLogFile != _currentLogfile) {
            pLogFile->setSelected(false);
        }
    }
    if(_currentLogfile) {
        emit abortUpload();
    }
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::startLogging()
{
    if(_vehicle) {
        _vehicle->startMavlinkLog();
        _logRunning = true;
        emit logRunningChanged();
    }
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::stopLogging()
{
    if(_vehicle) {
        _vehicle->stopMavlinkLog();
        _logRunning = false;
        emit logRunningChanged();
    }
}

//-----------------------------------------------------------------------------
QHttpPart
create_form_part(const QString& name, const QString& value)
{
    QHttpPart formPart;
    formPart.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"%1\"").arg(name));
    formPart.setBody(value.toUtf8());
    return formPart;
}

//-----------------------------------------------------------------------------
bool
MavlinkLogManager::_sendLog(const QString& logFile)
{
    QString defaultDescription = _description;
    if(_description.isEmpty()) {
        qCWarning(MavlinkLogManagerLog) << "Log description missing. Using defaults.";
        defaultDescription = kDefaultDescr;
    }
    if(_emailAddress.isEmpty()) {
        qCCritical(MavlinkLogManagerLog) << "User email missing.";
        return false;
    }
    if(_uploadURL.isEmpty()) {
        qCCritical(MavlinkLogManagerLog) << "Upload URL missing.";
        return false;
    }
    QFileInfo fi(logFile);
    if(!fi.exists()) {
        qCCritical(MavlinkLogManagerLog) << "Log file missing:" << logFile;
        return false;
    }
    QFile *file = new QFile(logFile);
    if(!file || !file->open(QIODevice::ReadOnly)) {
        if (file) delete file;
        qCCritical(MavlinkLogManagerLog) << "Could not open log file:" << logFile;
        return false;
    }
    if(!_nam) {
      _nam = new QNetworkAccessManager(this);
    }
    QNetworkProxy savedProxy = _nam->proxy();
    QNetworkProxy tempProxy;
    tempProxy.setType(QNetworkProxy::DefaultProxy);
    _nam->setProxy(tempProxy);
    //-- Build POST request
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart emailPart = create_form_part("email", _emailAddress);
    QHttpPart descriptionPart = create_form_part("description", _description);
    QHttpPart sourcePart = create_form_part("source", "QGroundControl");
    QHttpPart versionPart = create_form_part("version", _app->applicationVersion());
    QHttpPart logPart;
    logPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    logPart.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"filearg\"; filename=\"%1\"").arg(fi.fileName()));
    logPart.setBodyDevice(file);
    //-- Assemble request and POST it
    multiPart->append(emailPart);
    multiPart->append(descriptionPart);
    multiPart->append(sourcePart);
    multiPart->append(versionPart);
    multiPart->append(logPart);
    file->setParent(multiPart);
    QNetworkRequest request(_uploadURL);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply *reply = _nam->post(request, multiPart);
    connect(reply, &QNetworkReply::finished,  this, &MavlinkLogManager::_uploadFinished);
    connect(this, &MavlinkLogManager::abortUpload, reply, &QNetworkReply::abort);
    //connect(reply, &QNetworkReply::readyRead, this, &MavlinkLogManager::_dataAvailable);
    connect(reply, &QNetworkReply::uploadProgress, this, &MavlinkLogManager::_uploadProgress);
    multiPart->setParent(reply);
    qCDebug(MavlinkLogManagerLog) << "Log" << fi.baseName() << "Uploading." << fi.size() << "bytes.";
    _nam->setProxy(savedProxy);
    return true;
}

//-----------------------------------------------------------------------------
bool
MavlinkLogManager::_processUploadResponse(int http_code, QByteArray &data)
{
    qCDebug(MavlinkLogManagerLog) << "Uploaded response:" << QString::fromUtf8(data);
    emit readyRead(data);
    return http_code == 200;
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_dataAvailable()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) {
        return;
    }
    QByteArray data = reply->readAll();
    qCDebug(MavlinkLogManagerLog) << "Uploaded response data:" << QString::fromUtf8(data);
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_uploadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) {
        return;
    }
    const int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    if(_processUploadResponse(http_code, data)) {
        qCDebug(MavlinkLogManagerLog) << "Log uploaded.";
        emit succeed();
    } else {
        qCDebug(MavlinkLogManagerLog) << QString("Log Upload Error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
        emit failed();
    }
    reply->deleteLater();
    //-- Next (if any)
    uploadLog();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if(bytesTotal) {
        qreal progress = (qreal)bytesSent / (qreal)bytesTotal;
        if(_currentLogfile)
            _currentLogfile->setProgress(progress);
    }
    qCDebug(MavlinkLogManagerLog) << bytesSent << "of" << bytesTotal;
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_activeVehicleChanged(Vehicle* vehicle)
{
    //-- TODO: This is not quite right. This is being used to detect when a vehicle
    //   connects/disconnects. In reality, if QGC is connected to multiple vehicles,
    //   this is called each time the user switches from one vehicle to another. So
    //   far, I'm working on the assumption that multiple vehicles is a rare exception.

    // Disconnect the previous one (if any)
    if (_vehicle) {
        disconnect(_vehicle, &Vehicle::armedChanged,   this, &MavlinkLogManager::_armedChanged);
        disconnect(_vehicle, &Vehicle::mavlinkLogData, this, &MavlinkLogManager::_mavlinkLogData);
        _vehicle = NULL;
        emit canStartLogChanged();
    }
    // Connect new system
    if (vehicle)
    {
        _vehicle = vehicle;
        connect(_vehicle, &Vehicle::armedChanged,   this, &MavlinkLogManager::_armedChanged);
        connect(_vehicle, &Vehicle::mavlinkLogData, this, &MavlinkLogManager::_mavlinkLogData);
        emit canStartLogChanged();
    }
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_mavlinkLogData(Vehicle * /*vehicle*/, uint8_t /*target_system*/, uint8_t /*target_component*/, uint16_t sequence, uint8_t length, uint8_t first_message, const uint8_t* data, bool /*acked*/)
{
    Q_UNUSED(data);
    qDebug() << "Mavlink Log:" << sequence << length << first_message;
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_armedChanged(bool armed)
{
    if(_vehicle) {
        if(armed) {
            if(_enableAutoStart) {
                _vehicle->startMavlinkLog();
                _logRunning = true;
                emit logRunningChanged();
            }
        } else {
            if(_logRunning && _enableAutoStart) {
                _vehicle->stopMavlinkLog();
                emit logRunningChanged();
                if(_enableAutoUpload) {
                    //-- TODO: Queue log for auto upload
                }
            }
        }
    }
}
