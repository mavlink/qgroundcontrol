#include "MAVLinkFtpBandwidthTest.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRandomGenerator>

#include <algorithm>

#include "FTPManager.h"
#include "MAVLinkEnums.h"
#include "MAVLinkFTP.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(MAVLinkFtpBandwidthTestLog, "AnalyzeView.MAVLinkFtpBandwidthTest")

MAVLinkFtpBandwidthTest::MAVLinkFtpBandwidthTest(Vehicle* vehicle, QObject* parent)
    : QObject(parent),
      _vehicle(vehicle),
      _ftpManager(vehicle ? vehicle->ftpManager() : nullptr),
      _temporaryDirectory(QDir::tempPath() + QStringLiteral("/qgc-mavlink-bandwidth-XXXXXX"))
{
    if (_ftpManager) {
        (void) connect(_ftpManager, &FTPManager::uploadComplete, this, &MAVLinkFtpBandwidthTest::_uploadComplete);
        (void) connect(_ftpManager, &FTPManager::downloadComplete, this, &MAVLinkFtpBandwidthTest::_downloadComplete);
        (void) connect(_ftpManager, &FTPManager::deleteComplete, this, &MAVLinkFtpBandwidthTest::_deleteComplete);
        (void) connect(_ftpManager, &FTPManager::commandProgress, this, &MAVLinkFtpBandwidthTest::_commandProgress);
    }
}

MAVLinkFtpBandwidthTest::~MAVLinkFtpBandwidthTest()
{
    if (active() && _ftpManager) {
        (void) disconnect(_ftpManager, nullptr, this, nullptr);
        cancel();
    }
}

bool MAVLinkFtpBandwidthTest::start(Direction direction, int fileSizeKiB)
{
    if (active() || !_vehicle || !_ftpManager || !_temporaryDirectory.isValid()) {
        return false;
    }

    _direction = direction;
    _cancelled = false;
    _cancelStatusText.clear();
    _pendingSuccess = false;
    _pendingStatusText.clear();

    if (!_createPayloadFile(fileSizeKiB)) {
        return false;
    }

    const QString token = QString::number(QRandomGenerator::global()->generate(), 16);
    _remoteFilePath = _vehicle->px4Firmware() ? QStringLiteral("/fs/microsd/.qgc-bandwidth-%1.bin").arg(token)
                                              : QStringLiteral("/APM/.qgc-bandwidth-%1.bin").arg(token);

    if (_direction == Direction::QgcToVehicle) {
        _phase = Phase::MeasuringUpload;
        emit phaseChanged(tr("Uploading the MAVFTP test file..."));
        emit measurementStarted();
        _measurementTimer.start();
    } else {
        _phase = Phase::PreparingDownload;
        emit phaseChanged(tr("Preparing the remote MAVFTP test file..."));
    }

    if (!_startUpload()) {
        _phase = Phase::Idle;
        return false;
    }

    return true;
}

void MAVLinkFtpBandwidthTest::cancel(const QString& statusText)
{
    if (!active()) {
        return;
    }

    _cancelled = true;
    _cancelStatusText = statusText.isEmpty() ? tr("MAVFTP test stopped.") : statusText;
    emit phaseChanged(tr("Stopping the MAVFTP test and cleaning up..."));

    if (!_ftpManager) {
        _finish(false, _cancelStatusText);
        return;
    }

    switch (_phase) {
        case Phase::PreparingDownload:
        case Phase::MeasuringUpload:
            _ftpManager->cancelUpload();
            break;
        case Phase::VerifyingUpload:
        case Phase::MeasuringDownload:
            _ftpManager->cancelDownload();
            break;
        case Phase::Cleaning:
            _ftpManager->cancelDelete();
            break;
        case Phase::Idle:
            break;
    }
}

bool MAVLinkFtpBandwidthTest::active() const
{
    return _phase != Phase::Idle;
}

void MAVLinkFtpBandwidthTest::_uploadComplete(const QString& remotePath, const QString& errorMessage)
{
    Q_UNUSED(remotePath)

    if ((_phase != Phase::PreparingDownload) && (_phase != Phase::MeasuringUpload)) {
        return;
    }

    if (_cancelled || !errorMessage.isEmpty()) {
        _beginCleanup(false, _cancelled ? _cancelStatusText : tr("MAVFTP upload failed: %1").arg(errorMessage));
        return;
    }

    if (_phase == Phase::PreparingDownload) {
        _phase = Phase::MeasuringDownload;
        emit phaseChanged(tr("Downloading the MAVFTP test file..."));
        emit measurementStarted();
        _measurementTimer.start();
        if (!_startDownload(QStringLiteral("download.bin"))) {
            _beginCleanup(false, tr("Unable to start the MAVFTP download."));
        }
        return;
    }

    _completeMeasurement();
    _phase = Phase::VerifyingUpload;
    emit phaseChanged(tr("Verifying the uploaded MAVFTP test file..."));
    if (!_startDownload(QStringLiteral("verify.bin"))) {
        _beginCleanup(false, tr("Unable to verify the MAVFTP upload."));
    }
}

void MAVLinkFtpBandwidthTest::_downloadComplete(const QString& localPath, const QString& errorMessage)
{
    if ((_phase != Phase::VerifyingUpload) && (_phase != Phase::MeasuringDownload)) {
        return;
    }

    if (_cancelled || !errorMessage.isEmpty()) {
        _beginCleanup(false, _cancelled ? _cancelStatusText : tr("MAVFTP download failed: %1").arg(errorMessage));
        return;
    }

    const bool measuringDownload = _phase == Phase::MeasuringDownload;
    if (measuringDownload) {
        _completeMeasurement();
    }

    if (_fileHash(localPath) != _expectedHash) {
        _beginCleanup(false, tr("MAVFTP verification failed: downloaded data does not match."));
        return;
    }

    _beginCleanup(true, measuringDownload ? tr("MAVFTP download complete and verified.")
                                          : tr("MAVFTP upload complete and verified."));
}

void MAVLinkFtpBandwidthTest::_deleteComplete(const QString& remotePath, const QString& errorMessage)
{
    Q_UNUSED(remotePath)

    if (_phase != Phase::Cleaning) {
        return;
    }

    const bool remoteFileAlreadyAbsent =
        errorMessage.endsWith(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrFailFileNotFound));
    if (errorMessage.isEmpty() || remoteFileAlreadyAbsent) {
        _finish(_pendingSuccess, _pendingStatusText);
    } else {
        _finish(false, tr("%1 Remote cleanup failed: %2").arg(_pendingStatusText, errorMessage));
    }
}

void MAVLinkFtpBandwidthTest::_commandProgress(float progress)
{
    switch (_phase) {
        case Phase::PreparingDownload:
        case Phase::VerifyingUpload:
            emit preparationProgress(progress);
            break;
        case Phase::MeasuringUpload:
        case Phase::MeasuringDownload:
            emit measurementProgress(progress);
            break;
        default:
            break;
    }
}

bool MAVLinkFtpBandwidthTest::_createPayloadFile(int fileSizeKiB)
{
    const int boundedSizeKiB = std::clamp(fileSizeKiB, kMinimumFileSizeKiB, kMaximumFileSizeKiB);
    _payloadBytes = static_cast<quint64>(boundedSizeKiB) * 1024U;
    _sourceFilePath = _temporaryDirectory.filePath(QStringLiteral("source.bin"));

    QFile sourceFile(_sourceFilePath);
    if (!sourceFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(MAVLinkFtpBandwidthTestLog) << "Unable to create test file" << sourceFile.errorString();
        return false;
    }

    static constexpr qsizetype kBlockSize = 64 * 1024;
    QByteArray block(kBlockSize, Qt::Uninitialized);
    for (qsizetype index = 0; index < block.size(); ++index) {
        block[index] = static_cast<char>(((index * 31) + 17) & 0xFF);
    }

    quint64 remaining = _payloadBytes;
    while (remaining > 0U) {
        const qsizetype bytesToWrite = static_cast<qsizetype>(std::min<quint64>(remaining, block.size()));
        if (sourceFile.write(block.constData(), bytesToWrite) != bytesToWrite) {
            qCWarning(MAVLinkFtpBandwidthTestLog) << "Unable to write test file" << sourceFile.errorString();
            return false;
        }
        remaining -= static_cast<quint64>(bytesToWrite);
    }
    sourceFile.close();

    _expectedHash = _fileHash(_sourceFilePath);
    return !_expectedHash.isEmpty();
}

QByteArray MAVLinkFtpBandwidthTest::_fileHash(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return QByteArray();
    }
    return hash.result();
}

bool MAVLinkFtpBandwidthTest::_startUpload()
{
    return _ftpManager->upload(MAV_COMP_ID_AUTOPILOT1, _remoteFilePath, _sourceFilePath);
}

bool MAVLinkFtpBandwidthTest::_startDownload(const QString& fileName)
{
    _downloadFileName = fileName;
    const QString localPath = _temporaryDirectory.filePath(_downloadFileName);
    (void) QFile::remove(localPath);
    return _ftpManager->download(MAV_COMP_ID_AUTOPILOT1, _remoteFilePath, _temporaryDirectory.path(),
                                 _downloadFileName);
}

void MAVLinkFtpBandwidthTest::_completeMeasurement()
{
    emit measurementProgress(1.F);
    emit measurementComplete(std::max<qint64>(_measurementTimer.elapsed(), 1), _payloadBytes);
}

void MAVLinkFtpBandwidthTest::_beginCleanup(bool success, const QString& statusText)
{
    _pendingSuccess = success;
    _pendingStatusText = statusText;
    _phase = Phase::Cleaning;
    emit phaseChanged(tr("Removing the remote MAVFTP test file..."));

    if (!_ftpManager->deleteFile(MAV_COMP_ID_AUTOPILOT1, _remoteFilePath)) {
        _finish(false, tr("%1 Remote cleanup could not be started.").arg(statusText));
    }
}

void MAVLinkFtpBandwidthTest::_finish(bool success, const QString& statusText)
{
    _phase = Phase::Idle;
    emit finished(success, statusText);
}
