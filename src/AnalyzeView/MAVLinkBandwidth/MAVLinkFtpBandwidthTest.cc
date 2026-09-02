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

bool MAVLinkFtpBandwidthTest::start(int fileSizeKiB)
{
    if (active() || !_vehicle || !_ftpManager || !_temporaryDirectory.isValid()) {
        return false;
    }

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

    _phase = Phase::MeasuringUpload;
    emit phaseChanged(tr("Measuring MAVFTP upload..."));
    emit measurementStarted(Direction::QgcToVehicle);
    _measurementTimer.start();

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
        case Phase::MeasuringUpload:
            _ftpManager->cancelUpload();
            break;
        case Phase::MeasuringDownload:
            _ftpManager->cancelDownload();
            break;
        case Phase::Cleaning:
            _pendingSuccess = false;
            _pendingStatusText = _cancelStatusText;
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
    if (_phase != Phase::MeasuringUpload) {
        return;
    }

    if (remotePath != _remoteFilePath) {
        _beginCleanup(false, tr("MAVFTP upload returned an unexpected destination."));
        return;
    }

    if (_cancelled || !errorMessage.isEmpty()) {
        _beginCleanup(false, _cancelled ? _cancelStatusText : tr("MAVFTP upload failed: %1").arg(errorMessage));
        return;
    }

    _completeMeasurement(Direction::QgcToVehicle);
    _phase = Phase::MeasuringDownload;
    emit phaseChanged(tr("Measuring MAVFTP download..."));
    emit measurementStarted(Direction::VehicleToQgc);
    _measurementTimer.start();
    if (!_startDownload(QStringLiteral("download.bin"))) {
        _beginCleanup(false, tr("Unable to start the MAVFTP download."));
    }
}

void MAVLinkFtpBandwidthTest::_downloadComplete(const QString& localPath, const QString& errorMessage)
{
    if (_phase != Phase::MeasuringDownload) {
        return;
    }

    if (_cancelled || !errorMessage.isEmpty()) {
        _beginCleanup(false, _cancelled ? _cancelStatusText : tr("MAVFTP download failed: %1").arg(errorMessage));
        return;
    }

    _completeMeasurement(Direction::VehicleToQgc);

    if (_fileHash(localPath) != _expectedHash) {
        _beginCleanup(false, tr("MAVFTP verification failed: downloaded data does not match."));
        return;
    }

    _beginCleanup(true, tr("MAVFTP upload and download complete and verified."));
}

void MAVLinkFtpBandwidthTest::_deleteComplete(const QString& remotePath, const QString& errorMessage)
{
    if (_phase != Phase::Cleaning) {
        return;
    }

    if (!remotePath.isEmpty() && (remotePath != _remoteFilePath)) {
        _finish(false, tr("MAVFTP cleanup returned an unexpected destination."));
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
        case Phase::MeasuringUpload:
            emit measurementProgress(Direction::QgcToVehicle, progress);
            break;
        case Phase::MeasuringDownload:
            emit measurementProgress(Direction::VehicleToQgc, progress);
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

void MAVLinkFtpBandwidthTest::_completeMeasurement(Direction direction)
{
    emit measurementProgress(direction, 1.F);
    emit measurementComplete(direction, std::max<qint64>(_measurementTimer.elapsed(), 1), _payloadBytes);
}

void MAVLinkFtpBandwidthTest::_beginCleanup(bool success, const QString& statusText)
{
    _pendingSuccess = success;
    _pendingStatusText = statusText;
    _phase = Phase::Cleaning;
    emit cleanupStarted();
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
