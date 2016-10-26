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
static const char* kEnableDeletetKey        = "EnableDeleteKey";
static const char* kUlogExtension           = ".ulg";
static const char* kSidecarExtension        = ".uploaded";

//-----------------------------------------------------------------------------
MavlinkLogFiles::MavlinkLogFiles(MavlinkLogManager* manager, const QString& filePath, bool newFile)
    : _manager(manager)
    , _size(0)
    , _selected(false)
    , _uploading(false)
    , _progress(0)
    , _writing(false)
    , _uploaded(false)
{
    QFileInfo fi(filePath);
    _name = fi.baseName();
    if(!newFile) {
        _size = (quint32)fi.size();
        QString sideCar = filePath;
        sideCar.replace(kUlogExtension, kSidecarExtension);
        QFileInfo sc(sideCar);
        _uploaded = sc.exists();
    }
}

//-----------------------------------------------------------------------------
void
MavlinkLogFiles::setSize(quint32 size)
{
    _size = size;
    emit sizeChanged();
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
void
MavlinkLogFiles::setWriting(bool writing)
{
    _writing = writing;
    emit writingChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogFiles::setUploaded(bool uploaded)
{
    _uploaded = uploaded;
    emit uploadedChanged();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CurrentRunningLog::close()
{
    if(fd) {
        fclose(fd);
        fd = NULL;
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
MavlinkLogManager::MavlinkLogManager(QGCApplication* app)
    : QGCTool(app)
    , _enableAutoUpload(true)
    , _enableAutoStart(true)
    , _nam(NULL)
    , _currentLogfile(NULL)
    , _vehicle(NULL)
    , _logRunning(false)
    , _loggingDisabled(false)
    , _currentSavingFile(NULL)
    , _sequence(0)
    , _deleteAfterUpload(false)
{
    //-- Get saved settings
    QSettings settings;
    setEmailAddress(settings.value(kEmailAddressKey, QString()).toString());
    setDescription(settings.value(kDescriptionsKey, QString(kDefaultDescr)).toString());
    setUploadURL(settings.value(kPx4URLKey, QString(kDefaultPx4URL)).toString());
    setEnableAutoUpload(settings.value(kEnableAutoUploadKey, true).toBool());
    setEnableAutoStart(settings.value(kEnableAutoStartKey, true).toBool());
    setDeleteAfterUpload(settings.value(kEnableDeletetKey, false).toBool());
    //-- Logging location
    _logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    _logPath += "/MavlinkLogs";
    if(!QDir(_logPath).exists()) {
        if(!QDir().mkpath(_logPath)) {
            qCCritical(MavlinkLogManagerLog) << "Could not create Mavlink log download path:" << _logPath;
            _loggingDisabled = true;
        }
    }
    if(!_loggingDisabled) {
        //-- Load current list of logs
        QString filter = "*";
        filter += kUlogExtension;
        QDirIterator it(_logPath, QStringList() << filter, QDir::Files);
        while(it.hasNext()) {
            _insertNewLog(new MavlinkLogFiles(this, it.next()));
        }
        qCDebug(MavlinkLogManagerLog) << "Mavlink logs directory:" << _logPath;
    }
}

//-----------------------------------------------------------------------------
MavlinkLogManager::~MavlinkLogManager()
{
    _logFiles.clear();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<MavlinkLogManager>("QGroundControl.MavlinkLogManager", 1, 0, "MavlinkLogManager", "Reference only");
    if(!_loggingDisabled) {
        connect(toolbox->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &MavlinkLogManager::_activeVehicleChanged);
    }
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
void
MavlinkLogManager::setDeleteAfterUpload(bool enable)
{
    _deleteAfterUpload = enable;
    QSettings settings;
    settings.setValue(kEnableDeletetKey, enable);
    emit deleteAfterUploadChanged();
}

//-----------------------------------------------------------------------------
bool
MavlinkLogManager::uploading()
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
    for(int i = 0; i < _logFiles.count(); i++) {
        _currentLogfile = qobject_cast<MavlinkLogFiles*>(_logFiles.get(i));
        Q_ASSERT(_currentLogfile);
        if(_currentLogfile->selected()) {
            _currentLogfile->setSelected(false);
            if(!_currentLogfile->uploaded() && !_emailAddress.isEmpty() && !_uploadURL.isEmpty()) {
                _currentLogfile->setUploading(true);
                _currentLogfile->setProgress(0.0);
                QString filePath = _makeFilename(_currentLogfile->name());
                _sendLog(filePath);
                emit uploadingChanged();
                return;
            }
        }
    }
    _currentLogfile = NULL;
    emit uploadingChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_insertNewLog(MavlinkLogFiles* newLog)
{
    //-- Simpler than trying to sort this thing
    int count = _logFiles.count();
    if(!count) {
        _logFiles.append(newLog);
    } else {
        for(int i = 0; i < count; i++) {
            MavlinkLogFiles* f = qobject_cast<MavlinkLogFiles*>(_logFiles.get(i));
            if(newLog->name() < f->name()) {
                _logFiles.insert(i, newLog);
                return;
            }
        }
        _logFiles.append(newLog);
    }
}

//-----------------------------------------------------------------------------
int
MavlinkLogManager::_getFirstSelected()
{
    for(int i = 0; i < _logFiles.count(); i++) {
        MavlinkLogFiles* f = qobject_cast<MavlinkLogFiles*>(_logFiles.get(i));
        Q_ASSERT(f);
        if(f->selected()) {
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::deleteLog()
{
    while (true) {
        int idx = _getFirstSelected();
        if(idx < 0) {
            break;
        }
        MavlinkLogFiles* log = qobject_cast<MavlinkLogFiles*>(_logFiles.get(idx));
        _deleteLog(log);
    }
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_deleteLog(MavlinkLogFiles* log)
{
    QString filePath = _makeFilename(log->name());
    QFile gone(filePath);
    if(!gone.remove()) {
        qCWarning(MavlinkLogManagerLog) << "Could not delete Mavlink log file:" << _logPath;
    }
    //-- Remove sidecar file (if any)
    filePath.replace(kUlogExtension, kSidecarExtension);
    QFile sgone(filePath);
    if(sgone.exists()) {
        sgone.remove();
    }
    //-- Remove file from list and delete record
    _logFiles.removeOne(log);
    delete log;
    emit logFilesChanged();
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::cancelUpload()
{
    for(int i = 0; i < _logFiles.count(); i++) {
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
        if(_createNewLog()) {
            _vehicle->startMavlinkLog();
            _logRunning = true;
            emit logRunningChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::stopLogging()
{
    if(_vehicle) {
        //-- Tell vehicle to stop sending logs
        _vehicle->stopMavlinkLog();
        if(_currentSavingFile) {
            _currentSavingFile->close();
            if(_currentSavingFile->record) {
                _currentSavingFile->record->setWriting(false);
                if(_enableAutoUpload) {
                    //-- Queue log for auto upload (set selected flag)
                    _currentSavingFile->record->setSelected(true);
                    if(!uploading()) {
                        uploadLog();
                    }
                }
            }
            delete _currentSavingFile;
            _currentSavingFile = NULL;
            _logRunning = false;
            emit logRunningChanged();
        }
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
    QFile* file = new QFile(logFile);
    if(!file || !file->open(QIODevice::ReadOnly)) {
        if(file) {
            delete file;
        }
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
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
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
#if QT_VERSION > 0x050600
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    QNetworkReply* reply = _nam->post(request, multiPart);
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
MavlinkLogManager::_processUploadResponse(int http_code, QByteArray& data)
{
    qCDebug(MavlinkLogManagerLog) << "Uploaded response:" << QString::fromUtf8(data);
    emit readyRead(data);
    return http_code == 200;
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_dataAvailable()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
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
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) {
        return;
    }
    const int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    if(_processUploadResponse(http_code, data)) {
        qCDebug(MavlinkLogManagerLog) << "Log uploaded.";
        emit succeed();
        if(_deleteAfterUpload) {
            if(_currentLogfile) {
                _deleteLog(_currentLogfile);
                _currentLogfile = NULL;
            }
        } else {
            if(_currentLogfile) {
                _currentLogfile->setUploaded(true);
                //-- Write side-car file to flag it as uploaded
                QString sideCar = _makeFilename(_currentLogfile->name());
                sideCar.replace(kUlogExtension, kSidecarExtension);
                FILE* f = fopen(sideCar.toLatin1().data(), "wb");
                if(f) {
                    fclose(f);
                }
            }
        }
    } else {
        qCWarning(MavlinkLogManagerLog) << QString("Log Upload Error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
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
        if(_currentLogfile) {
            _currentLogfile->setProgress(progress);
        }
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
    //   For now, we only handle one log download at a time.
    // Disconnect the previous one (if any)
    if(_vehicle) {
        disconnect(_vehicle, &Vehicle::armedChanged,   this, &MavlinkLogManager::_armedChanged);
        disconnect(_vehicle, &Vehicle::mavlinkLogData, this, &MavlinkLogManager::_mavlinkLogData);
        _vehicle = NULL;
        emit canStartLogChanged();
    }
    // Connect new system
    if(vehicle) {
        _vehicle = vehicle;
        connect(_vehicle, &Vehicle::armedChanged,   this, &MavlinkLogManager::_armedChanged);
        connect(_vehicle, &Vehicle::mavlinkLogData, this, &MavlinkLogManager::_mavlinkLogData);
        emit canStartLogChanged();
    }
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_mavlinkLogData(Vehicle* /*vehicle*/, uint8_t /*target_system*/, uint8_t /*target_component*/, uint16_t sequence, uint8_t length, uint8_t first_message, const uint8_t* data, bool /*acked*/)
{
    if(_currentSavingFile && _currentSavingFile->fd) {
        if(sequence != _sequence) {
            qCWarning(MavlinkLogManagerLog) << "Dropped Mavlink log data";
            if(first_message < 255) {
                data += first_message;
                length -= first_message;
            } else {
                return;
            }
        }
        if(fwrite(data, 1, length, _currentSavingFile->fd) != (size_t)length) {
            qCCritical(MavlinkLogManagerLog) << "Error writing Mavlink log file:" << _currentSavingFile->fileName;
            delete _currentSavingFile;
            _currentSavingFile = NULL;
            _logRunning = false;
            _vehicle->stopMavlinkLog();
            emit logRunningChanged();
        }
    } else {
        length = 0;
        qCWarning(MavlinkLogManagerLog) << "Mavlink log data received when not expected.";
    }
    //-- Update file size
    if(_currentSavingFile) {
        if(_currentSavingFile->record) {
            quint32 size = _currentSavingFile->record->size() + length;
            _currentSavingFile->record->setSize(size);
        }
    }
    _sequence = sequence + 1;
}

//-----------------------------------------------------------------------------
bool
MavlinkLogManager::_createNewLog()
{
    if(_currentSavingFile) {
        delete _currentSavingFile;
        _currentSavingFile = NULL;
    }
    _currentSavingFile = new CurrentRunningLog;
    _currentSavingFile->fileName.sprintf("%s/%03d-%s%s",
        _logPath.toLatin1().data(),
        _vehicle->id(),
        QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss-zzz").toLatin1().data(),
        kUlogExtension);
    _currentSavingFile->fd = fopen(_currentSavingFile->fileName.toLatin1().data(), "wb");
    if(_currentSavingFile->fd) {
        MavlinkLogFiles* newLog = new MavlinkLogFiles(this, _currentSavingFile->fileName, true);
        newLog->setWriting(true);
        _insertNewLog(newLog);
        _currentSavingFile->record = newLog;
        emit logFilesChanged();
    } else {
        qCCritical(MavlinkLogManagerLog) << "Could not create Mavlink log file:" << _currentSavingFile->fileName;
        delete _currentSavingFile;
        _currentSavingFile = NULL;
    }
    _sequence = 0;
    return _currentSavingFile != NULL;
}

//-----------------------------------------------------------------------------
void
MavlinkLogManager::_armedChanged(bool armed)
{
    if(_vehicle) {
        if(armed) {
            if(_enableAutoStart) {
                startLogging();
            }
        } else {
            if(_logRunning && _enableAutoStart) {
                stopLogging();
            }
        }
    }
}

//-----------------------------------------------------------------------------
QString
MavlinkLogManager::_makeFilename(const QString& baseName)
{
    QString filePath = _logPath;
    filePath += "/";
    filePath += baseName;
    filePath += kUlogExtension;
    return filePath;
}
