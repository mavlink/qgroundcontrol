/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkLogManager.h"
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

QGC_LOGGING_CATEGORY(MAVLinkLogManagerLog, "MAVLinkLogManagerLog")

static const char* kMAVLinkLogGroup         = "MAVLinkLogGroup";
static const char* kEmailAddressKey         = "Email";
static const char* kDescriptionsKey         = "Description";
static const char* kDefaultDescr            = "QGroundControl Session";
static const char* kPx4URLKey               = "LogURL";
static const char* kDefaultPx4URL           = "http://logs.px4.io/upload";
static const char* kEnableAutoUploadKey     = "EnableAutoUpload";
static const char* kEnableAutoStartKey      = "EnableAutoStart";
static const char* kEnableDeletetKey        = "EnableDelete";
static const char* kUlogExtension           = ".ulg";
static const char* kSidecarExtension        = ".uploaded";
static const char* kVideoURLKey             = "VideoURL";
static const char* kWindSpeedKey            = "WindSpeed";
static const char* kRateKey                 = "RateKey";
static const char* kPublicLogKey            = "PublicLog";
static const char* kFeedback                = "feedback";
static const char* kVideoURL                = "videoUrl";

//-----------------------------------------------------------------------------
MAVLinkLogFiles::MAVLinkLogFiles(MAVLinkLogManager* manager, const QString& filePath, bool newFile)
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
MAVLinkLogFiles::setSize(quint32 size)
{
    _size = size;
    emit sizeChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogFiles::setSelected(bool selected)
{
    _selected = selected;
    emit selectedChanged();
    emit _manager->selectedCountChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogFiles::setUploading(bool uploading)
{
    _uploading = uploading;
    emit uploadingChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogFiles::setProgress(qreal progress)
{
    _progress = progress;
    emit progressChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogFiles::setWriting(bool writing)
{
    _writing = writing;
    emit writingChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogFiles::setUploaded(bool uploaded)
{
    _uploaded = uploaded;
    emit uploadedChanged();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
MAVLinkLogProcessor::MAVLinkLogProcessor()
    : _fd(NULL)
    , _written(0)
    , _sequence(-1)
    , _numDrops(0)
    , _gotHeader(false)
    , _error(false)
    , _record(NULL)
{
}

//-----------------------------------------------------------------------------
MAVLinkLogProcessor::~MAVLinkLogProcessor()
{
    close();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogProcessor::close()
{
    if(_fd) {
        fclose(_fd);
        _fd = NULL;
    }
}

//-----------------------------------------------------------------------------
bool
MAVLinkLogProcessor::valid()
{
    return (_fd != NULL) && (_record != NULL);
}

//-----------------------------------------------------------------------------
bool
MAVLinkLogProcessor::create(MAVLinkLogManager* manager, const QString path, uint8_t id)
{
    _fileName.sprintf("%s/%03d-%s%s",
        path.toLatin1().data(),
        id,
        QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss-zzz").toLatin1().data(),
        kUlogExtension);
    _fd = fopen(_fileName.toLatin1().data(), "wb");
    if(_fd) {
        _record = new MAVLinkLogFiles(manager, _fileName, true);
        _record->setWriting(true);
        _sequence = -1;
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
MAVLinkLogProcessor::_checkSequence(uint16_t seq, int& num_drops)
{
    num_drops = 0;
    //-- Check if a sequence is newer than the one previously received and if
    //   there were dropped messages between the last one and this.
    if(_sequence == -1) {
        _sequence = seq;
        return true;
    }
    if((uint16_t)_sequence == seq) {
        return false;
    }
    if(seq > (uint16_t)_sequence) {
        // Account for wrap-arounds, sequence is 2 bytes
        if((seq - _sequence) > (1 << 15)) { // Assume reordered
            return false;
        }
        num_drops = seq - _sequence - 1;
        _numDrops += num_drops;
        _sequence = seq;
        return true;
    } else {
        if((_sequence - seq) > (1 << 15)) {
            num_drops = (1 << 16) - _sequence - 1 + seq;
            _numDrops += num_drops;
            _sequence = seq;
            return true;
        }
        return false;
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkLogProcessor::_writeData(void* data, int len)
{
    if(!_error) {
        _error = fwrite(data, 1, len, _fd) != (size_t)len;
        if(!_error) {
            _written += len;
            if(_record) {
                _record->setSize(_written);
            }
        } else {
            qCDebug(MAVLinkLogManagerLog) << "File IO error:" << len << "bytes into" << _fileName;
        }
    }
}

//-----------------------------------------------------------------------------
QByteArray
MAVLinkLogProcessor::_writeUlogMessage(QByteArray& data)
{
    //-- Write ulog data w/o integrity checking, assuming data starts with a
    //   valid ulog message. returns the remaining data at the end.
    while(data.length() > 2) {
        uint8_t* ptr = (uint8_t*)data.data();
        int message_length = ptr[0] + (ptr[1] * 256) + 3; // 3 = ULog msg header
        if(message_length > data.length())
            break;
        _writeData(data.data(), message_length);
        data.remove(0, message_length);
    }
    return data;
}

//-----------------------------------------------------------------------------
bool
MAVLinkLogProcessor::processStreamData(uint16_t sequence, uint8_t first_message, QByteArray data)
{
    int num_drops = 0;
    _error = false;
    while(_checkSequence(sequence, num_drops)) {
        //-- The first 16 bytes need special treatment (this sounds awfully brittle)
        if(!_gotHeader) {
            if(data.size() < 16) {
                //-- Shouldn't happen but if it does, we might as well close shop.
                qCWarning(MAVLinkLogManagerLog) << "Corrupt log header. Canceling log download.";
                return false;
            }
            //-- Write header
            _writeData(data.data(), 16);
            data.remove(0, 16);
            _gotHeader = true;
            // What about data start offset now that we removed 16 bytes off the start?
        }
        if(_gotHeader && num_drops > 0) {
            if(num_drops > 25) num_drops = 25;
            //-- Hocus Pocus
            //   Write a dropout message. We don't really know the actual duration,
            //   so just use the number of drops * 10 ms
            uint8_t bogus[] = {2, 0, 79, 0, 0};
            bogus[3] = num_drops * 10;
            _writeData(bogus, sizeof(bogus));
        }
        if(num_drops > 0) {
            _writeUlogMessage(_ulogMessage);
            _ulogMessage.clear();
            //-- If no usefull information in this message. Drop it.
            if(first_message == 255) {
                break;
            }
            if(first_message > 0) {
                data.remove(0, first_message);
                first_message = 0;
            }
        }
        if(first_message == 255 && _ulogMessage.length() > 0) {
            _ulogMessage.append(data);
            break;
        }
        if(_ulogMessage.length()) {
            _writeData(_ulogMessage.data(), _ulogMessage.length());
            if(first_message) {
                _writeData(data.left(first_message).data(), first_message);
            }
            _ulogMessage.clear();
        }
        if(first_message) {
            data.remove(0, first_message);
        }
        _ulogMessage = _writeUlogMessage(data);
        break;
    }
    return !_error;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
MAVLinkLogManager::MAVLinkLogManager(QGCApplication* app)
    : QGCTool(app)
    , _enableAutoUpload(true)
    , _enableAutoStart(false)
    , _nam(NULL)
    , _currentLogfile(NULL)
    , _vehicle(NULL)
    , _logRunning(false)
    , _loggingDisabled(false)
    , _logProcessor(NULL)
    , _deleteAfterUpload(false)
    , _windSpeed(-1)
    , _publicLog(false)
{
    //-- Get saved settings
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    setEmailAddress(settings.value(kEmailAddressKey, QString()).toString());
    setDescription(settings.value(kDescriptionsKey, QString(kDefaultDescr)).toString());
    setUploadURL(settings.value(kPx4URLKey, QString(kDefaultPx4URL)).toString());
    setVideoURL(settings.value(kVideoURLKey, QString()).toString());
    setEnableAutoUpload(settings.value(kEnableAutoUploadKey, true).toBool());
    setEnableAutoStart(settings.value(kEnableAutoStartKey, false).toBool());
    setDeleteAfterUpload(settings.value(kEnableDeletetKey, false).toBool());
    setWindSpeed(settings.value(kWindSpeedKey, -1).toInt());
    setRating(settings.value(kRateKey, "notset").toString());
    setPublicLog(settings.value(kPublicLogKey, true).toBool());
    //-- Logging location
    _logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    _logPath += "/MAVLinkLogs";
    if(!QDir(_logPath).exists()) {
        if(!QDir().mkpath(_logPath)) {
            qCWarning(MAVLinkLogManagerLog) << "Could not create MAVLink log download path:" << _logPath;
            _loggingDisabled = true;
        }
    }
    if(!_loggingDisabled) {
        //-- Load current list of logs
        QString filter = "*";
        filter += kUlogExtension;
        QDirIterator it(_logPath, QStringList() << filter, QDir::Files);
        while(it.hasNext()) {
            _insertNewLog(new MAVLinkLogFiles(this, it.next()));
        }
        qCDebug(MAVLinkLogManagerLog) << "MAVLink logs directory:" << _logPath;
    }
}

//-----------------------------------------------------------------------------
MAVLinkLogManager::~MAVLinkLogManager()
{
    _logFiles.clear();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<MAVLinkLogManager>("QGroundControl.MAVLinkLogManager", 1, 0, "MAVLinkLogManager", "Reference only");
    if(!_loggingDisabled) {
        connect(toolbox->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &MAVLinkLogManager::_activeVehicleChanged);
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setEmailAddress(QString email)
{
    _emailAddress = email;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kEmailAddressKey, email);
    emit emailAddressChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setDescription(QString description)
{
    _description = description;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kDescriptionsKey, description);
    emit descriptionChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setUploadURL(QString url)
{
    _uploadURL = url;
    if(_uploadURL.isEmpty()) {
        _uploadURL = kDefaultPx4URL;
    }
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kPx4URLKey, _uploadURL);
    emit uploadURLChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setFeedback(QString fb)
{
    _feedback = fb;
    emit feedbackChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setVideoURL(QString url)
{
    _videoURL = url;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kVideoURLKey, url);
    emit videoURLChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setEnableAutoUpload(bool enable)
{
    _enableAutoUpload = enable;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kEnableAutoUploadKey, enable);
    emit enableAutoUploadChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setEnableAutoStart(bool enable)
{
    _enableAutoStart = enable;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kEnableAutoStartKey, enable);
    emit enableAutoStartChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setDeleteAfterUpload(bool enable)
{
    _deleteAfterUpload = enable;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kEnableDeletetKey, enable);
    emit deleteAfterUploadChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setWindSpeed(int speed)
{
    _windSpeed = speed;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kWindSpeedKey, speed);
    emit windSpeedChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setRating(QString rate)
{
    _rating = rate;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kRateKey, rate);
    emit ratingChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::setPublicLog(bool pub)
{
    _publicLog = pub;
    QSettings settings;
    settings.beginGroup(kMAVLinkLogGroup);
    settings.setValue(kPublicLogKey, pub);
    emit publicLogChanged();
}

//-----------------------------------------------------------------------------
bool
MAVLinkLogManager::uploading()
{
    return _currentLogfile != NULL;
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::uploadLog()
{
    if(_currentLogfile) {
        _currentLogfile->setUploading(false);
    }
    for(int i = 0; i < _logFiles.count(); i++) {
        _currentLogfile = qobject_cast<MAVLinkLogFiles*>(_logFiles.get(i));
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
MAVLinkLogManager::_insertNewLog(MAVLinkLogFiles* newLog)
{
    //-- Simpler than trying to sort this thing
    int count = _logFiles.count();
    if(!count) {
        _logFiles.append(newLog);
    } else {
        for(int i = 0; i < count; i++) {
            MAVLinkLogFiles* f = qobject_cast<MAVLinkLogFiles*>(_logFiles.get(i));
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
MAVLinkLogManager::_getFirstSelected()
{
    for(int i = 0; i < _logFiles.count(); i++) {
        MAVLinkLogFiles* f = qobject_cast<MAVLinkLogFiles*>(_logFiles.get(i));
        Q_ASSERT(f);
        if(f->selected()) {
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::deleteLog()
{
    while (true) {
        int idx = _getFirstSelected();
        if(idx < 0) {
            break;
        }
        MAVLinkLogFiles* log = qobject_cast<MAVLinkLogFiles*>(_logFiles.get(idx));
        _deleteLog(log);
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_deleteLog(MAVLinkLogFiles* log)
{
    QString filePath = _makeFilename(log->name());
    QFile gone(filePath);
    if(!gone.remove()) {
        qCWarning(MAVLinkLogManagerLog) << "Could not delete MAVLink log file:" << _logPath;
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
MAVLinkLogManager::cancelUpload()
{
    for(int i = 0; i < _logFiles.count(); i++) {
        MAVLinkLogFiles* pLogFile = qobject_cast<MAVLinkLogFiles*>(_logFiles.get(i));
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
MAVLinkLogManager::startLogging()
{
    if(_vehicle && _vehicle->px4Firmware()) {
        if(_createNewLog()) {
            _vehicle->startMavlinkLog();
            _logRunning = true;
            emit logRunningChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::stopLogging()
{
    if(_vehicle && _vehicle->px4Firmware()) {
        //-- Tell vehicle to stop sending logs
        _vehicle->stopMavlinkLog();
    }
    if(_logProcessor) {
        _logProcessor->close();
        if(_logProcessor->record()) {
            _logProcessor->record()->setWriting(false);
            if(_enableAutoUpload) {
                //-- Queue log for auto upload (set selected flag)
                _logProcessor->record()->setSelected(true);
                if(!uploading()) {
                    uploadLog();
                }
            }
        }
        delete _logProcessor;
        _logProcessor = NULL;
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
MAVLinkLogManager::_sendLog(const QString& logFile)
{
    QString defaultDescription = _description;
    if(_description.isEmpty()) {
        qCWarning(MAVLinkLogManagerLog) << "Log description missing. Using defaults.";
        defaultDescription = kDefaultDescr;
    }
    if(_emailAddress.isEmpty()) {
        qCWarning(MAVLinkLogManagerLog) << "User email missing.";
        return false;
    }
    if(_uploadURL.isEmpty()) {
        qCWarning(MAVLinkLogManagerLog) << "Upload URL missing.";
        return false;
    }
    QFileInfo fi(logFile);
    if(!fi.exists()) {
        qCWarning(MAVLinkLogManagerLog) << "Log file missing:" << logFile;
        return false;
    }
    QFile* file = new QFile(logFile);
    if(!file || !file->open(QIODevice::ReadOnly)) {
        if(file) {
            delete file;
        }
        qCWarning(MAVLinkLogManagerLog) << "Could not open log file:" << logFile;
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
    QHttpPart sourcePart      = create_form_part("source", "QGroundControl");
    QHttpPart versionPart     = create_form_part("version", _app->applicationVersion());
    QHttpPart typePart        = create_form_part("type", "flightreport");
    QHttpPart windPart        = create_form_part("windSpeed", QString::number(_windSpeed));
    QHttpPart ratingPart      = create_form_part("rating", _rating);
    QHttpPart publicPart      = create_form_part("public", _publicLog ? "true" : "false");
    //-- Assemble request and POST it
    multiPart->append(emailPart);
    multiPart->append(descriptionPart);
    multiPart->append(sourcePart);
    multiPart->append(versionPart);
    multiPart->append(typePart);
    multiPart->append(windPart);
    multiPart->append(ratingPart);
    multiPart->append(publicPart);
    //-- Optional
    QHttpPart feedbackPart;
    if(_feedback.isEmpty()) {
        feedbackPart = create_form_part(kFeedback, "None Given");
    } else {
        feedbackPart = create_form_part(kFeedback, _feedback);
    }
    multiPart->append(feedbackPart);
    QHttpPart videoPart;
    if(_videoURL.isEmpty()) {
        videoPart = create_form_part(kVideoURL, "None");
    } else {
        videoPart = create_form_part(kVideoURL, _videoURL);
    }
    multiPart->append(videoPart);
    //-- Actual Log File
    QHttpPart logPart;
    logPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    logPart.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"filearg\"; filename=\"%1\"").arg(fi.fileName()));
    logPart.setBodyDevice(file);
    multiPart->append(logPart);
    file->setParent(multiPart);
    QNetworkRequest request(_uploadURL);
#if QT_VERSION > 0x050600
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    QNetworkReply* reply = _nam->post(request, multiPart);
    connect(reply, &QNetworkReply::finished,  this, &MAVLinkLogManager::_uploadFinished);
    connect(this, &MAVLinkLogManager::abortUpload, reply, &QNetworkReply::abort);
    //connect(reply, &QNetworkReply::readyRead, this, &MAVLinkLogManager::_dataAvailable);
    connect(reply, &QNetworkReply::uploadProgress, this, &MAVLinkLogManager::_uploadProgress);
    multiPart->setParent(reply);
    qCDebug(MAVLinkLogManagerLog) << "Log" << fi.baseName() << "Uploading." << fi.size() << "bytes.";
    _nam->setProxy(savedProxy);
    return true;
}

//-----------------------------------------------------------------------------
bool
MAVLinkLogManager::_processUploadResponse(int http_code, QByteArray& data)
{
    qCDebug(MAVLinkLogManagerLog) << "Uploaded response:" << QString::fromUtf8(data);
    emit readyRead(data);
    return http_code == 200;
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_dataAvailable()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) {
        return;
    }
    QByteArray data = reply->readAll();
    qCDebug(MAVLinkLogManagerLog) << "Uploaded response data:" << QString::fromUtf8(data);
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_uploadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) {
        return;
    }
    const int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    if(_processUploadResponse(http_code, data)) {
        qCDebug(MAVLinkLogManagerLog) << "Log uploaded.";
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
        qCWarning(MAVLinkLogManagerLog) << QString("Log Upload Error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
        emit failed();
    }
    reply->deleteLater();
    //-- Next (if any)
    uploadLog();
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if(bytesTotal) {
        qreal progress = (qreal)bytesSent / (qreal)bytesTotal;
        if(_currentLogfile) {
            _currentLogfile->setProgress(progress);
        }
    }
    qCDebug(MAVLinkLogManagerLog) << bytesSent << "of" << bytesTotal;
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_activeVehicleChanged(Vehicle* vehicle)
{
    //-- TODO: This is not quite right. This is being used to detect when a vehicle
    //   connects/disconnects. In reality, if QGC is connected to multiple vehicles,
    //   this is called each time the user switches from one vehicle to another. So
    //   far, I'm working on the assumption that multiple vehicles is a rare exception.
    //   For now, we only handle one log download at a time.
    // Disconnect the previous one (if any)
    if(_vehicle && _vehicle->px4Firmware()) {
        disconnect(_vehicle, &Vehicle::armedChanged,        this, &MAVLinkLogManager::_armedChanged);
        disconnect(_vehicle, &Vehicle::mavlinkLogData,      this, &MAVLinkLogManager::_mavlinkLogData);
        disconnect(_vehicle, &Vehicle::mavCommandResult,    this, &MAVLinkLogManager::_mavCommandResult);
        _vehicle = NULL;
        //-- Stop logging (if that's the case)
        stopLogging();
        emit canStartLogChanged();
    }
    // Connect new system
    if(_vehicle && _vehicle->px4Firmware()) {
        _vehicle = vehicle;
        connect(_vehicle, &Vehicle::armedChanged,       this, &MAVLinkLogManager::_armedChanged);
        connect(_vehicle, &Vehicle::mavlinkLogData,     this, &MAVLinkLogManager::_mavlinkLogData);
        connect(_vehicle, &Vehicle::mavCommandResult,   this, &MAVLinkLogManager::_mavCommandResult);
        emit canStartLogChanged();
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_mavlinkLogData(Vehicle* /*vehicle*/, uint8_t /*target_system*/, uint8_t /*target_component*/, uint16_t sequence, uint8_t first_message, QByteArray data, bool /*acked*/)
{
    if(_logProcessor && _logProcessor->valid()) {
        if(!_logProcessor->processStreamData(sequence, first_message, data)) {
            qCWarning(MAVLinkLogManagerLog) << "Error writing MAVLink log file:" << _logProcessor->fileName();
            delete _logProcessor;
            _logProcessor = NULL;
            _logRunning = false;
            _vehicle->stopMavlinkLog();
            emit logRunningChanged();
        }
    } else {
        qCWarning(MAVLinkLogManagerLog) << "MAVLink log data received when not expected.";
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(component);
    Q_UNUSED(noReponseFromVehicle)

    if(command == MAV_CMD_LOGGING_START || command == MAV_CMD_LOGGING_STOP) {
        //-- Did it fail?
        if(result != MAV_RESULT_ACCEPTED) {
            if(command == MAV_CMD_LOGGING_STOP) {
                //-- Not that it could happen but...
                qCWarning(MAVLinkLogManagerLog) << "Stop MAVLink log command failed.";
            } else {
                //-- Could not start logging for some reason.
                qCWarning(MAVLinkLogManagerLog) << "Start MAVLink log command failed.";
                _discardLog();
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_discardLog()
{
    //-- Delete (empty) log file (and record)
    if(_logProcessor) {
        _logProcessor->close();
        if(_logProcessor->record()) {
            _deleteLog(_logProcessor->record());
        }
        delete _logProcessor;
        _logProcessor = NULL;
    }
    _logRunning = false;
    emit logRunningChanged();
}

//-----------------------------------------------------------------------------
bool
MAVLinkLogManager::_createNewLog()
{
    if(_logProcessor) {
        delete _logProcessor;
        _logProcessor = NULL;
    }
    _logProcessor = new MAVLinkLogProcessor;
    if(_logProcessor->create(this, _logPath, _vehicle->id())) {
        _insertNewLog(_logProcessor->record());
        emit logFilesChanged();
    } else {
        qCWarning(MAVLinkLogManagerLog) << "Could not create MAVLink log file:" << _logProcessor->fileName();
        delete _logProcessor;
        _logProcessor = NULL;
    }
    return _logProcessor != NULL;
}

//-----------------------------------------------------------------------------
void
MAVLinkLogManager::_armedChanged(bool armed)
{
    if(_vehicle && _vehicle->px4Firmware()) {
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
MAVLinkLogManager::_makeFilename(const QString& baseName)
{
    QString filePath = _logPath;
    filePath += "/";
    filePath += baseName;
    filePath += kUlogExtension;
    return filePath;
}
