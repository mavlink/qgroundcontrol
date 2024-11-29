/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkLogManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "Vehicle.h"

#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkReply>
#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(MAVLinkLogManagerLog, "qgc.vehicle.mavlinklogmanager")

static constexpr const char *kSidecarExtension = ".uploaded";

MAVLinkLogFiles::MAVLinkLogFiles(MAVLinkLogManager *manager, const QString &filePath, bool newFile)
    : QObject(manager)
{
    // qCDebug(MAVLinkLogManagerLog) << Q_FUNC_INFO << this;

    (void) connect(this, &MAVLinkLogFiles::selectedChanged, manager, &MAVLinkLogManager::selectedCountChanged);

    const QFileInfo fi(filePath);
    _name = fi.baseName();
    if (!newFile) {
        _size = fi.size();
        QString sideCar = filePath;
        sideCar = sideCar.replace(manager->logExtension(), kSidecarExtension);
        const QFileInfo sc(sideCar);
        _uploaded = sc.exists();
    }
}

MAVLinkLogFiles::~MAVLinkLogFiles()
{
    // qCDebug(MAVLinkLogManagerLog) << Q_FUNC_INFO << this;
}

void MAVLinkLogFiles::setSize(quint32 size)
{
    if (size != _size) {
        _size = size;
        emit sizeChanged();
    }
}

void MAVLinkLogFiles::setSelected(bool selected)
{
    if (selected != _selected) {
        _selected = selected;
        emit selectedChanged();
    }
}

void MAVLinkLogFiles::setUploading(bool uploading)
{
    if (uploading != _uploading) {
        _uploading = uploading;
        emit uploadingChanged();
    }
}

void MAVLinkLogFiles::setProgress(qreal progress)
{
    if (progress != _progress) {
        _progress = progress;
        emit progressChanged();
    }
}

void MAVLinkLogFiles::setWriting(bool writing)
{
    if (writing != _writing) {
        _writing = writing;
        emit writingChanged();
    }
}

void MAVLinkLogFiles::setUploaded(bool uploaded)
{
    if (uploaded != _uploaded) {
        _uploaded = uploaded;
        emit uploadedChanged();
    }
}

/*===========================================================================*/

MAVLinkLogProcessor::MAVLinkLogProcessor()
{
    // qCDebug(MAVLinkLogManagerLog) << Q_FUNC_INFO << this;
}

MAVLinkLogProcessor::~MAVLinkLogProcessor()
{
    close();

    // qCDebug(MAVLinkLogManagerLog) << Q_FUNC_INFO << this;
}

void MAVLinkLogProcessor::close()
{
    if (_file.isOpen()) {
        _file.close();
    }
}

bool MAVLinkLogProcessor::create(MAVLinkLogManager *manager, QStringView path, uint8_t id)
{
    _fileName = _fileName.asprintf(
        "%s/%03d-%s%s",
        path.toLatin1().constData(),
        id,
        QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss-zzz").toLocal8Bit().constData(),
        manager->logExtension().toLocal8Bit().constData()
    );

    _file.setFileName(_fileName);
    if (!_file.open(QIODevice::WriteOnly)) {
        qCWarning(MAVLinkLogManagerLog) << "Failed to open file for writing:" << _file.errorString();
        return false;
    }

    _record = new MAVLinkLogFiles(manager, _fileName, true);
    _record->setWriting(true);
    _sequence = -1;

    return true;
}

bool MAVLinkLogProcessor::_checkSequence(uint16_t seq, int &num_drops)
{
    num_drops = 0;
    //-- Check if a sequence is newer than the one previously received and if
    //   there were dropped messages between the last one and this.
    if (_sequence == -1) {
        _sequence = seq;
        return true;
    }

    if (static_cast<uint16_t>(_sequence) == seq) {
        return false;
    }

    if (seq > static_cast<uint16_t>(_sequence)) {
        // Account for wrap-arounds, sequence is 2 bytes
        if ((seq - _sequence) > kSequenceSize) { // Assume reordered
            return false;
        }

        num_drops = seq - _sequence - 1;
        _numDrops += num_drops;
        _sequence = seq;
        return true;
    }

    if ((_sequence - seq) > kSequenceSize) {
        num_drops = (1 << 16) - _sequence - 1 + seq;
        _numDrops += num_drops;
        _sequence = seq;
        return true;
    }

    return false;
}

void MAVLinkLogProcessor::_writeData(const void *data, int len)
{
    if (_error) {
        return;
    }

    const qint64 bytesWritten = _file.write(reinterpret_cast<const char*>(data), len);
    if (bytesWritten != len) {
        _error = true;
        qCDebug(MAVLinkLogManagerLog) << "File IO error:" << len << "bytes into" << _fileName;
        return;
    }

    _written += len;
    if (_record) {
        _record->setSize(_written);
    }
}

QByteArray MAVLinkLogProcessor::_writeUlogMessage(QByteArray &data)
{
    // Write ulog data w/o integrity checking, assuming data starts with a
    // valid ulog message. returns the remaining data at the end.
    while (data.length() > 2) {
        const uint8_t *const ptr = reinterpret_cast<const uint8_t*>(data.constData());
        const int message_length = ptr[0] + (ptr[1] * 256) + kUlogMessageHeader;
        if (message_length > data.length()) {
            break;
        }

        _writeData(data.constData(), message_length);
        (void) data.remove(0, message_length);
    }

    return data;
}

bool MAVLinkLogProcessor::processStreamData(uint16_t sequence, uint8_t first_message, const QByteArray &in)
{
    int num_drops = 0;
    _error = false;

    QByteArray data(in);
    while (_checkSequence(sequence, num_drops)) {
        if (!_gotHeader) {
            if (data.size() < 16) {
                qCWarning(MAVLinkLogManagerLog) << "Corrupt log header. Canceling log download.";
                return false;
            }

            _writeData(data.constData(), 16);
            (void) data.remove(0, 16);
            _gotHeader = true;
            // What about data start offset now that we removed 16 bytes off the start?
        }

        if (_gotHeader && (num_drops > 0)) {
            if (num_drops > 25) {
                num_drops = 25;
            }

            // Write a dropout message. We don't really know the actual duration,
            // so just use the number of drops * 10 ms
            const uint8_t duration = static_cast<uint8_t>(num_drops) * 10;
            const uint8_t bogus[] = {2, 0, 79, duration, 0};
            _writeData(bogus, sizeof(bogus));
        }

        if (num_drops > 0) {
            (void) _writeUlogMessage(_ulogMessage);
            _ulogMessage.clear();

            if (first_message == 255) {
                break;
            }

            if (first_message > 0) {
                (void) data.remove(0, first_message);
                first_message = 0;
            }
        }

        if ((first_message == 255) && (!_ulogMessage.isEmpty())) {
            (void) _ulogMessage.append(data);
            break;
        }

        if (_ulogMessage.length()) {
            _writeData(_ulogMessage.constData(), _ulogMessage.length());
            if (first_message) {
                _writeData(data.left(first_message).constData(), first_message);
            }
            _ulogMessage.clear();
        }

        if (first_message) {
            (void) data.remove(0, first_message);
        }

        _ulogMessage = _writeUlogMessage(data);
        break;
    }

    return !_error;
}

/*===========================================================================*/

MAVLinkLogManager::MAVLinkLogManager(Vehicle *vehicle, QObject *parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _networkManager(new QNetworkAccessManager(this))
    , _logFiles(new QmlObjectListModel(this))
    , _ulogExtension(QStringLiteral(".") + SettingsManager::instance()->appSettings()->logFileExtension)
    , _logPath(SettingsManager::instance()->appSettings()->logSavePath())
{
    // qCDebug(MAVLinkLogManagerLog) << Q_FUNC_INFO << this;

    (void) qmlRegisterUncreatableType<MAVLinkLogManager>("QGroundControl.MAVLinkLogManager", 1, 0, "MAVLinkLogManager", "Reference only");

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    QNetworkProxy tProxy = _networkManager->proxy();
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager->setProxy(tProxy);
#endif

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

    settings.endGroup();

    if (!QDir(_logPath).exists()) {
        if (!QDir().mkpath(_logPath)) {
            qCWarning(MAVLinkLogManagerLog) << "Could not create MAVLink log download path:" << _logPath;
            _loggingDisabled = true;
        }
    }

    if (!_loggingDisabled) {
        const QString filter = "*" + _ulogExtension;
        QDirIterator it(_logPath, QStringList() << filter, QDir::Files);
        while (it.hasNext()) {
            _insertNewLog(new MAVLinkLogFiles(this, it.next()));
        }

        qCDebug(MAVLinkLogManagerLog) << "MAVLink logs directory:" << _logPath;
        if (_vehicle->px4Firmware()) {
            _loggingDenied = false;
            (void) connect(_vehicle, &Vehicle::armedChanged, this, &MAVLinkLogManager::_armedChanged);
            (void) connect(_vehicle, &Vehicle::mavlinkLogData, this, &MAVLinkLogManager::_mavlinkLogData);
            (void) connect(_vehicle, &Vehicle::mavCommandResult, this, &MAVLinkLogManager::_mavCommandResult);
            emit canStartLogChanged();
        }
    }
}

MAVLinkLogManager::~MAVLinkLogManager()
{
    // qCDebug(MAVLinkLogManagerLog) << Q_FUNC_INFO << this;

    _logFiles->clearAndDeleteContents();
}

void MAVLinkLogManager::setEmailAddress(const QString &email)
{
    if (email != _emailAddress) {
        _emailAddress = email;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kEmailAddressKey, email);
        emit emailAddressChanged();
    }
}

void MAVLinkLogManager::setDescription(const QString &description)
{
    if (description != _description) {
        _description = description;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kDescriptionsKey, description);
        emit descriptionChanged();
    }
}

void MAVLinkLogManager::setUploadURL(const QString &url)
{
    if (url != _uploadURL) {
        _uploadURL = url.isEmpty() ? kDefaultPx4URL : url;

        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kPx4URLKey, _uploadURL);
        emit uploadURLChanged();
    }
}

void MAVLinkLogManager::setFeedback(const QString &fb)
{
    if (fb != _feedback) {
        _feedback = fb;
        emit feedbackChanged();
    }
}

void MAVLinkLogManager::setVideoURL(const QString &url)
{
    if (url != _videoURL) {
        _videoURL = url;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kVideoURLKey, url);
        emit videoURLChanged();
    }
}

void MAVLinkLogManager::setEnableAutoUpload(bool enable)
{
    if (enable != _enableAutoUpload) {
        _enableAutoUpload = enable;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kEnableAutoUploadKey, enable);
        emit enableAutoUploadChanged();
    }
}

void MAVLinkLogManager::setEnableAutoStart(bool enable)
{
    if (enable != _enableAutoStart) {
        _enableAutoStart = enable;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kEnableAutoStartKey, enable);
        emit enableAutoStartChanged();
    }
}

void MAVLinkLogManager::setDeleteAfterUpload(bool enable)
{
    if (enable != _deleteAfterUpload) {
        _deleteAfterUpload = enable;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kEnableDeletetKey, enable);
        emit deleteAfterUploadChanged();
    }
}

void MAVLinkLogManager::setWindSpeed(int speed)
{
    if (speed != _windSpeed) {
        _windSpeed = speed;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kWindSpeedKey, speed);
        emit windSpeedChanged();
    }
}

void MAVLinkLogManager::setRating(const QString &rate)
{
    if (rate != _rating) {
        _rating = rate;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kRateKey, rate);
        emit ratingChanged();
    }
}

void MAVLinkLogManager::setPublicLog(bool pub)
{
    if (pub != _publicLog) {
        _publicLog = pub;
        QSettings settings;
        settings.beginGroup(kMAVLinkLogGroup);
        settings.setValue(kPublicLogKey, pub);
        emit publicLogChanged();
    }
}

void MAVLinkLogManager::uploadLog()
{
    if (_currentLogfile) {
        _currentLogfile->setUploading(false);
    }

    for (int i = 0; i < _logFiles->count(); i++) {
        _currentLogfile = qobject_cast<MAVLinkLogFiles*>(_logFiles->get(i));
        if (!_currentLogfile) {
            qCWarning(MAVLinkLogManagerLog) << "Internal error";
            continue;
        }

        if (!_currentLogfile->selected()) {
            continue;

        }

        _currentLogfile->setSelected(false);
        if (_currentLogfile->uploaded() || _emailAddress.isEmpty() || _uploadURL.isEmpty()) {
            continue;
        }

        _currentLogfile->setUploading(true);
        _currentLogfile->setProgress(0.0);
        const QString filePath = _makeFilename(_currentLogfile->name());
        (void) _sendLog(filePath);
        emit uploadingChanged();
        return;
    }

    _currentLogfile = nullptr;
    emit uploadingChanged();
}

void MAVLinkLogManager::_insertNewLog(MAVLinkLogFiles *newLog)
{
    const int count = _logFiles->count();
    if (!count) {
        _logFiles->append(newLog);
        return;
    }

    for (int i = 0; i < count; i++) {
        const MAVLinkLogFiles *const f = qobject_cast<const MAVLinkLogFiles*>(_logFiles->get(i));
        if (newLog->name() < f->name()) {
            (void) _logFiles->insert(i, newLog);
            return;
        }
    }

    _logFiles->append(newLog);
}

int MAVLinkLogManager::_getFirstSelected() const
{
    for (int i = 0; i < _logFiles->count(); i++) {
        const MAVLinkLogFiles *const f = qobject_cast<const MAVLinkLogFiles*>(_logFiles->get(i));
        if (!f) {
            qCWarning(MAVLinkLogManagerLog) << "Internal error";
            continue;
        }

        if (f->selected()) {
            return i;
        }
    }

    return -1;
}

void MAVLinkLogManager::deleteLog()
{
    while (true) {
        const int idx = _getFirstSelected();
        if (idx < 0) {
            break;
        }

        MAVLinkLogFiles *const log = qobject_cast<MAVLinkLogFiles*>(_logFiles->get(idx));
        _deleteLog(log);
    }
}

void MAVLinkLogManager::_deleteLog(MAVLinkLogFiles *log)
{
    QString filePath = _makeFilename(log->name());
    QFile gone(filePath);
    if (!gone.remove()) {
        qCWarning(MAVLinkLogManagerLog) << "Could not delete MAVLink log file:" << _logPath;
    }

    filePath.replace(_ulogExtension, kSidecarExtension);
    QFile sgone(filePath);
    if (sgone.exists()) {
        (void) sgone.remove();
    }

    (void) _logFiles->removeOne(log);
    delete log;

    emit logFilesChanged();
}

void MAVLinkLogManager::cancelUpload()
{
    for (int i = 0; i < _logFiles->count(); i++) {
        MAVLinkLogFiles *const pLogFile = qobject_cast<MAVLinkLogFiles*>(_logFiles->get(i));
        if (!pLogFile) {
            qCWarning(MAVLinkLogManagerLog) << "Internal error";
            continue;
        }

        if (pLogFile->selected() && (pLogFile != _currentLogfile)) {
            pLogFile->setSelected(false);
        }
    }

    if (_currentLogfile) {
        emit abortUpload();
    }
}

void MAVLinkLogManager::startLogging()
{
    AppSettings *const appSettings = SettingsManager::instance()->appSettings();
    if (appSettings->disableAllPersistence()->rawValue().toBool()) {
        return;
    }

    if (!_vehicle || !_vehicle->px4Firmware() || _loggingDenied) {
        return;
    }

    if (!_createNewLog()) {
        return;
    }

    _vehicle->startMavlinkLog();
    _logRunning = true;
    emit logRunningChanged();
}

void MAVLinkLogManager::stopLogging()
{
    if (_vehicle && _vehicle->px4Firmware()) {
        _vehicle->stopMavlinkLog();
    }

    if (!_logProcessor) {
        return;
    }

    _logProcessor->close();
    if (_logProcessor->record()) {
        _logProcessor->record()->setWriting(false);
        if (_enableAutoUpload) {
            _logProcessor->record()->setSelected(true);
            if (!uploading()) {
                uploadLog();
            }
        }
    }

    delete _logProcessor;
    _logProcessor = nullptr;
    _logRunning = false;
    emit logRunningChanged();
}

QHttpPart MAVLinkLogManager::_createFormPart(QStringView name, QStringView value)
{
    QHttpPart formPart;
    formPart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"%1\"").arg(name));
    formPart.setBody(value.toUtf8());
    return formPart;
}

bool MAVLinkLogManager::_sendLog(const QString &logFile)
{
    QString defaultDescription = _description;
    if (_description.isEmpty()) {
        qCWarning(MAVLinkLogManagerLog) << "Log description missing. Using defaults.";
        defaultDescription = kDefaultDescr;
    }

    if (_emailAddress.isEmpty()) {
        qCWarning(MAVLinkLogManagerLog) << "User email missing.";
        return false;
    }

    if (_uploadURL.isEmpty()) {
        qCWarning(MAVLinkLogManagerLog) << "Upload URL missing.";
        return false;
    }

    const QFileInfo fi(logFile);
    if (!fi.exists()) {
        qCWarning(MAVLinkLogManagerLog) << "Log file missing:" << logFile;
        return false;
    }

    QFile *file = new QFile(logFile, this);
    if (!file || !file->open(QIODevice::ReadOnly)) {
        delete file;
        file = nullptr;
        qCWarning(MAVLinkLogManagerLog) << "Could not open log file:" << logFile;
        return false;
    }

    QHttpMultiPart *const multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    const QHttpPart emailPart = _createFormPart(u"email", _emailAddress);
    const QHttpPart descriptionPart = _createFormPart(u"description", _description);
    const QHttpPart sourcePart = _createFormPart(u"source", u"QGroundControl");
    const QHttpPart versionPart = _createFormPart(u"version", QCoreApplication::applicationVersion());
    const QHttpPart typePart = _createFormPart(u"type", u"flightreport");
    const QHttpPart windPart = _createFormPart(u"windSpeed", QString::number(_windSpeed));
    const QHttpPart ratingPart = _createFormPart(u"rating", _rating);
    const QHttpPart publicPart = _createFormPart(u"public", _publicLog ? u"true" : u"false");

    multiPart->append(emailPart);
    multiPart->append(descriptionPart);
    multiPart->append(sourcePart);
    multiPart->append(versionPart);
    multiPart->append(typePart);
    multiPart->append(windPart);
    multiPart->append(ratingPart);
    multiPart->append(publicPart);

    QHttpPart feedbackPart;
    if (_feedback.isEmpty()) {
        feedbackPart = _createFormPart(QString(kFeedback), u"None Given");
    } else {
        feedbackPart = _createFormPart(QString(kFeedback), _feedback);
    }
    multiPart->append(feedbackPart);

    QHttpPart videoPart;
    if(_videoURL.isEmpty()) {
        videoPart = _createFormPart(QString(kVideoURL), u"None");
    } else {
        videoPart = _createFormPart(QString(kVideoURL), _videoURL);
    }
    multiPart->append(videoPart);

    QHttpPart logPart;
    logPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    logPart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"filearg\"; filename=\"%1\"").arg(fi.fileName()));
    logPart.setBodyDevice(file);
    multiPart->append(logPart);
    file->setParent(multiPart);

    QNetworkRequest request(_uploadURL);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    QNetworkReply *const reply = _networkManager->post(request, multiPart);
    (void) connect(this, &MAVLinkLogManager::abortUpload, reply, &QNetworkReply::abort);
    (void) connect(reply, &QNetworkReply::finished,  this, &MAVLinkLogManager::_uploadFinished);
    (void) connect(reply, &QNetworkReply::uploadProgress, this, &MAVLinkLogManager::_uploadProgress);
#ifdef QT_DEBUG
    if (MAVLinkLogManagerLog().isDebugEnabled()) {
        (void) connect(reply, &QNetworkReply::readyRead, this, &MAVLinkLogManager::_dataAvailable);
    }
#endif

    multiPart->setParent(reply);
    qCDebug(MAVLinkLogManagerLog) << "Log" << fi.baseName() << "Uploading." << fi.size() << "bytes.";

    return true;
}

bool MAVLinkLogManager::_processUploadResponse(int http_code, const QByteArray &data)
{
    qCDebug(MAVLinkLogManagerLog) << "Uploaded response:" << QString::fromUtf8(data);
    emit readyRead(data);

    return (http_code == 200);
}

void MAVLinkLogManager::_dataAvailable()
{
    QNetworkReply *const reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    const QByteArray data = reply->readAll();
    qCDebug(MAVLinkLogManagerLog) << "Uploaded response data:" << QString::fromUtf8(data);
}

void MAVLinkLogManager::_uploadFinished()
{
    QNetworkReply *const reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    const int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();

    if (_processUploadResponse(http_code, data)) {
        qCDebug(MAVLinkLogManagerLog) << "Log uploaded.";
        emit succeed();
        if (_deleteAfterUpload) {
            if (_currentLogfile) {
                _deleteLog(_currentLogfile);
                _currentLogfile = nullptr;
            }
        } else if (_currentLogfile) {
            _currentLogfile->setUploaded(true);
            QString sideCar = _makeFilename(_currentLogfile->name());
            (void) sideCar.replace(_ulogExtension, kSidecarExtension);

            QFile file(sideCar.toLatin1().constData());
            if (file.open(QIODevice::WriteOnly)) {
                file.close();
            }
        }
    } else {
        qCWarning(MAVLinkLogManagerLog) << QString("Log Upload Error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
        emit failed();
    }

    reply->deleteLater();
    uploadLog();
}

void MAVLinkLogManager::_uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (bytesTotal) {
        const qreal progress = static_cast<qreal>(bytesSent) / static_cast<qreal>(bytesTotal);
        if (_currentLogfile) {
            _currentLogfile->setProgress(progress);
        }
    }

    qCDebug(MAVLinkLogManagerLog) << bytesSent << "of" << bytesTotal;
}

void MAVLinkLogManager::_mavlinkLogData(Vehicle* /*vehicle*/, uint8_t /*target_system*/, uint8_t /*target_component*/, uint16_t sequence, uint8_t first_message, const QByteArray &data, bool /*acked*/)
{
    if (!_logProcessor || !_logProcessor->valid()) {
        qCDebug(MAVLinkLogManagerLog) << "MAVLink log data received when not expected.";
        return;
    }

    if (_logProcessor->processStreamData(sequence, first_message, data)) {
        return;
    }

    qCWarning(MAVLinkLogManagerLog) << "Error writing MAVLink log file:" << _logProcessor->fileName();
    delete _logProcessor;
    _logProcessor = nullptr;
    _logRunning = false;
    _vehicle->stopMavlinkLog();
    emit logRunningChanged();
}

void MAVLinkLogManager::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(component);
    Q_UNUSED(noReponseFromVehicle)

    if ((command != MAV_CMD_LOGGING_START) && (command != MAV_CMD_LOGGING_STOP)) {
        return;
    }

    if (result == MAV_RESULT_ACCEPTED) {
        return;
    }

    if (command == MAV_CMD_LOGGING_STOP) {
        qCWarning(MAVLinkLogManagerLog) << "Stop MAVLink log command failed.";
        return;
    }

    if (result == MAV_RESULT_DENIED) {
        _loggingDenied = true;
        qCWarning(MAVLinkLogManagerLog) << "Start MAVLink log command denied.";
    } else {
        qCWarning(MAVLinkLogManagerLog) << "Start MAVLink log command failed:" << result;
    }

    _discardLog();
}

void MAVLinkLogManager::_discardLog()
{
    if (_logProcessor) {
        _logProcessor->close();
        if (_logProcessor->record()) {
            _deleteLog(_logProcessor->record());
        }
        delete _logProcessor;
        _logProcessor = nullptr;
    }

    _logRunning = false;
    emit logRunningChanged();
}

bool MAVLinkLogManager::_createNewLog()
{
    delete _logProcessor;
    _logProcessor = new MAVLinkLogProcessor();

    if (_logProcessor->create(this, _logPath, static_cast<uint8_t>(_vehicle->id()))) {
        _insertNewLog(_logProcessor->record());
        emit logFilesChanged();
    } else {
        qCWarning(MAVLinkLogManagerLog) << "Could not create MAVLink log file:" << _logProcessor->fileName();
        delete _logProcessor;
        _logProcessor = nullptr;
    }

    return (_logProcessor != nullptr);
}

void MAVLinkLogManager::_armedChanged(bool armed)
{
    if (!_vehicle || !_vehicle->px4Firmware()) {
        return;
    }

    if (armed) {
        if (_enableAutoStart) {
            startLogging();
        }
        return;
    }

    if (_logRunning && _enableAutoStart) {
        stopLogging();
    }
}

QString MAVLinkLogManager::_makeFilename(const QString &baseName) const
{
    QString filePath = _logPath;
    filePath += "/";
    filePath += baseName;
    filePath += _ulogExtension;
    return filePath;
}
