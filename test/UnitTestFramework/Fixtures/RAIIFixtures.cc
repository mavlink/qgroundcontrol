#include "RAIIFixtures.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtNetwork/QNetworkRequest>

#include "AppSettings.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"
#include "RunGuard.h"
#include "SettingsManager.h"

#include <cstring>
#include <memory>

QGC_LOGGING_CATEGORY(RAIIFixturesLog, "Test.RAIIFixtures")

namespace TestFixtures {

// ============================================================================
// SettingsFixture Implementation
// ============================================================================

SettingsFixture::SettingsFixture()
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();

    // Save original values
    _originalFirmware = static_cast<MAV_AUTOPILOT>(QGCMAVLink::firmwareClassToAutopilot(
        static_cast<QGCMAVLink::FirmwareClass_t>(appSettings->offlineEditingFirmwareClass()->rawValue().toInt())));

    _originalVehicleType = static_cast<MAV_TYPE>(QGCMAVLink::vehicleClassToMavType(
        static_cast<QGCMAVLink::VehicleClass_t>(appSettings->offlineEditingVehicleClass()->rawValue().toInt())));
}

SettingsFixture::~SettingsFixture()
{
    // Restore original settings
    AppSettings* appSettings = SettingsManager::instance()->appSettings();

    appSettings->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(_originalFirmware));

    appSettings->offlineEditingVehicleClass()->setRawValue(QGCMAVLink::vehicleClass(_originalVehicleType));

    // Restore any saved Fact values
    for (const SavedFact& saved : _savedFacts) {
        if (saved.fact) {
            saved.fact->setRawValue(saved.originalValue);
        }
    }
}

void SettingsFixture::setOfflineFirmware(MAV_AUTOPILOT autopilot)
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(autopilot));
}

void SettingsFixture::setOfflineVehicleType(MAV_TYPE vehicleType)
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingVehicleClass()->setRawValue(QGCMAVLink::vehicleClass(vehicleType));
}

void SettingsFixture::setFactValue(Fact* fact, const QVariant& value)
{
    if (!fact) {
        qCWarning(RAIIFixturesLog) << "setFactValue: null Fact";
        return;
    }

    // Save original value for restoration
    _savedFacts.append({fact, fact->rawValue()});

    // Set new value
    fact->setRawValue(value);
}

// ============================================================================
// NetworkReplyFixture Implementation
// ============================================================================

NetworkReplyFixture::NetworkReplyFixture(const QUrl& url, QObject* parent) : QNetworkReply(parent)
{
    setUrl(url);
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    setFinished(true);
    setHeader(QNetworkRequest::ContentLengthHeader, 0);
}

NetworkReplyFixture::~NetworkReplyFixture() = default;

void NetworkReplyFixture::setHttpStatus(int statusCode)
{
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
}

void NetworkReplyFixture::setRedirectTarget(const QUrl& target)
{
    setAttribute(QNetworkRequest::RedirectionTargetAttribute, target);
}

void NetworkReplyFixture::setNetworkError(QNetworkReply::NetworkError errorCode, const QString& message)
{
    setError(errorCode, message);
}

void NetworkReplyFixture::setContentType(const QString& contentType)
{
    if (contentType.isEmpty()) {
        setHeader(QNetworkRequest::ContentTypeHeader, QVariant());
        return;
    }

    setHeader(QNetworkRequest::ContentTypeHeader, contentType);
}

void NetworkReplyFixture::setContentLength(qint64 length)
{
    setHeader(QNetworkRequest::ContentLengthHeader, length);
}

void NetworkReplyFixture::setBody(const QByteArray& body, const QString& contentType)
{
    _body = body;
    _readOffset = 0;
    setContentLength(_body.size());

    if (!contentType.isEmpty()) {
        setContentType(contentType);
    }
}

void NetworkReplyFixture::abort()
{
}

qint64 NetworkReplyFixture::readData(char* data, qint64 maxSize)
{
    if (maxSize <= 0) {
        return 0;
    }

    if (_readOffset >= _body.size()) {
        return -1;
    }

    const qint64 remaining = _body.size() - _readOffset;
    const qint64 toCopy = qMin(maxSize, remaining);
    std::memcpy(data, _body.constData() + _readOffset, static_cast<size_t>(toCopy));
    _readOffset += toCopy;
    return toCopy;
}

// ============================================================================
// SingleInstanceLockFixture Implementation
// ============================================================================

SingleInstanceLockFixture::SingleInstanceLockFixture(const QString& lockKey, bool acquireOnCreate)
    : _lockKey(lockKey)
    , _guard(std::make_unique<RunGuard>(lockKey))
{
    if (acquireOnCreate) {
        (void) tryAcquire();
    }
}

SingleInstanceLockFixture::~SingleInstanceLockFixture()
{
    release();
}

bool SingleInstanceLockFixture::tryAcquire()
{
    return _guard && _guard->tryToRun();
}

void SingleInstanceLockFixture::release()
{
    if (_guard && _guard->isLocked()) {
        _guard->release();
    }
}

bool SingleInstanceLockFixture::isLocked() const
{
    return _guard && _guard->isLocked();
}

QString SingleInstanceLockFixture::key() const
{
    return _lockKey;
}

// ============================================================================
// TempFileFixture Implementation
// ============================================================================

TempFileFixture::TempFileFixture(const QString& templateName)
{
    if (templateName.isEmpty()) {
        _file = std::make_unique<QTemporaryFile>();
    } else {
        _file = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/" + templateName);
    }

    if (!_file->open()) {
        qCWarning(RAIIFixturesLog) << "TempFileFixture: failed to create temp file:" << _file->errorString();
        _file.reset();
    }
}

TempFileFixture::~TempFileFixture() = default;

QString TempFileFixture::path() const
{
    return _file ? _file->fileName() : QString();
}

bool TempFileFixture::write(const QByteArray& content)
{
    if (!_file)
        return false;

    _file->seek(0);
    _file->resize(0);
    return _file->write(content) == content.size();
}

bool TempFileFixture::write(const QString& content)
{
    return write(content.toUtf8());
}

QByteArray TempFileFixture::readAll()
{
    if (!_file)
        return QByteArray();

    _file->seek(0);
    return _file->readAll();
}

// ============================================================================
// TempJsonFileFixture Implementation
// ============================================================================

TempJsonFileFixture::TempJsonFileFixture(const QString& templateName) : _file(templateName)
{
}

TempJsonFileFixture::~TempJsonFileFixture() = default;

bool TempJsonFileFixture::isValid() const
{
    return _file.isValid();
}

QString TempJsonFileFixture::path() const
{
    return _file.path();
}

bool TempJsonFileFixture::writeJson(const QJsonDocument& json, bool compact)
{
    if (!_file.isValid()) {
        return false;
    }

    return _file.write(compact ? json.toJson(QJsonDocument::Compact) : json.toJson(QJsonDocument::Indented));
}

bool TempJsonFileFixture::writeJson(const QJsonObject& object, bool compact)
{
    return writeJson(QJsonDocument(object), compact);
}

QJsonDocument TempJsonFileFixture::readJson(QJsonParseError* error)
{
    if (!_file.isValid()) {
        if (error) {
            error->error = QJsonParseError::IllegalValue;
            error->offset = 0;
        }
        return {};
    }

    return QJsonDocument::fromJson(_file.readAll(), error);
}

// ============================================================================
// TempDirFixture Implementation
// ============================================================================

TempDirFixture::TempDirFixture() : _dir(std::make_unique<QTemporaryDir>())
{
    if (!_dir->isValid()) {
        qCWarning(RAIIFixturesLog) << "TempDirFixture: failed to create temp directory";
        _dir.reset();
    }
}

TempDirFixture::~TempDirFixture() = default;

QString TempDirFixture::path() const
{
    return _dir ? _dir->path() : QString();
}

QString TempDirFixture::createFile(const QString& relativePath, const QByteArray& content)
{
    if (!_dir)
        return QString();

    const QString fullPath = _dir->path() + "/" + relativePath;

    // Create parent directories if needed
    const QFileInfo fileInfo(fullPath);
    QDir().mkpath(fileInfo.absolutePath());

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(RAIIFixturesLog) << "TempDirFixture::createFile: failed to create file:" << fullPath;
        return QString();
    }

    if (!content.isEmpty()) {
        file.write(content);
    }

    return fullPath;
}

}  // namespace TestFixtures
