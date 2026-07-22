#include "FTPController.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <cmath>
#include <limits>

#include "FTPManager.h"
#include "FTPManagerJob.h"
#include "MultiVehicleManager.h"
#include "QGCArchiveModel.h"
#include "QGCCompression.h"
#include "QGCCompressionJob.h"
#include "QGCFileHelper.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(FTPControllerLog, "Vehicle.FTPController")

FTPController::FTPController(QObject* parent) : QObject(parent), _archiveModel(new QGCArchiveModel(this))
{
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
            &FTPController::setVehicle);
    setVehicle(MultiVehicleManager::instance()->activeVehicle());
}

Vehicle* FTPController::vehicle() const
{
    return _vehicle;
}

void FTPController::setVehicle(Vehicle* vehicle)
{
    if (vehicle == _vehicle) {
        return;
    }

    const QPointer<FTPController> self(this);
    const QPointer<FTPJob> previousJob(_activeJob);
    cancelActiveOperation();
    if (!self) {
        return;
    }
    if (previousJob) {
        disconnect(previousJob, nullptr, this, nullptr);
    }
    _activeJob = nullptr;
    _setBusy(false);
    if (!self) {
        return;
    }
    _clearOperation();
    if (!self) {
        return;
    }
    _resetDirectoryState();
    if (!self) {
        return;
    }

    _vehicle = vehicle;
    _ftpManager = vehicle ? vehicle->ftpManager() : nullptr;
    emit vehicleChanged();
}

bool FTPController::listDirectory(const QString &uri, int componentId)
{
    const QPointer<FTPController> self(this);
    if (!_ftpManager) {
        _setErrorString(tr("No active vehicle is available"));
        return false;
    }
    const QPointer<FTPManager> ftpManager(_ftpManager);
    if (_operation != Operation::None) {
        _setErrorString(tr("Another FTP operation is in progress"));
        return false;
    }

    const QString previousPath = _currentPath;

    _resetDirectoryState();
    if (!self) {
        return false;
    }
    _currentPath = uri;
    emit currentPathChanged();
    if (!self) {
        return false;
    }
    _setErrorString(QString());
    if (!self) {
        return false;
    }

    _setBusy(true);
    if (!self) {
        return false;
    }
    _setOperation(Operation::List);
    if (!self) {
        return false;
    }
    _progress = 0.0F;
    emit progressChanged();
    if (!self || (_ftpManager != ftpManager) || (_operation != Operation::List)) {
        return false;
    }

    const uint8_t compId = _componentIdForRequest(componentId);
    const FTPManager::ListDirectoryStartResult startResult =
        ftpManager->startListDirectory(compId, uri, kMaximumDirectoryEntries);
    FTPListDirectoryJob* const job = startResult.job();
    if (!job) {
        qCWarning(FTPControllerLog) << "Failed to start list operation for" << uri;
        _setErrorString(startResult.error() == FTPManager::StartError::Busy
                            ? tr("Another vehicle FTP operation is in progress")
                            : tr("Failed to list %1").arg(uri));
        if (!self) {
        return false;
        }
        _currentPath = previousPath;
        emit currentPathChanged();
        if (!self) {
        return false;
        }
        _setBusy(false);
        if (!self) {
        return false;
        }
        _clearOperation();
        return false;
    }

    _activeJob = job;
    const QPointer<FTPListDirectoryJob> guardedJob(job);
    connect(job, &FTPListDirectoryJob::finished, this,
            [this, guardedJob](const QStringList& entries, const QString& error, bool truncated) {
                if (guardedJob && (guardedJob == _activeJob)) {
                    _handleDirectoryComplete(entries, error, truncated);
                }
            });

    return true;
}

bool FTPController::downloadFile(const QString &uri, const QString &localDir, const QString &fileName, int componentId)
{
    const QPointer<FTPController> self(this);
    if (!_ftpManager) {
        const QString error = tr("No active vehicle is available");
        _setErrorString(error);
        if (!self) {
        return false;
        }
        emit downloadComplete(QString(), error);
        return false;
    }
    if (_operation != Operation::None) {
        const QString error = tr("Another FTP operation is in progress");
        _setErrorString(error);
        if (!self) {
        return false;
        }
        emit downloadComplete(QString(), error);
        return false;
    }

    if (!QGCFileHelper::ensureDirectoryExists(localDir)) {
        const QString error = tr("Could not create directory %1").arg(localDir);
        _setErrorString(error);
        if (!self) {
        return false;
        }
        emit downloadComplete(QString(), error);
        return false;
    }
    const QPointer<FTPManager> ftpManager(_ftpManager);

    const QString absoluteLocalDir = QDir(localDir).absolutePath();

    _lastDownloadFile.clear();
    emit lastDownloadFileChanged();
    if (!self) {
        return false;
    }
    _setErrorString(QString());
    if (!self) {
        return false;
    }

    _setBusy(true);
    if (!self) {
        return false;
    }
    _setOperation(Operation::Download);
    if (!self) {
        return false;
    }
    _progress = 0.0F;
    emit progressChanged();
    if (!self || (_ftpManager != ftpManager) || (_operation != Operation::Download)) {
        return false;
    }

    const uint8_t compId = _componentIdForRequest(componentId);
    const FTPManager::DownloadStartResult startResult =
        ftpManager->startDownload(compId, uri, absoluteLocalDir, fileName);
    FTPDownloadJob* const job = startResult.job();
    if (!job) {
        qCWarning(FTPControllerLog) << "Failed to start download" << uri << absoluteLocalDir;
        const QString error = startResult.error() == FTPManager::StartError::Busy
                                  ? tr("Another vehicle FTP operation is in progress")
                                  : tr("Failed to download %1").arg(uri);
        _setErrorString(error);
        if (!self) {
        return false;
        }
        _setBusy(false);
        if (!self) {
        return false;
        }
        _clearOperation();
        if (!self) {
        return false;
        }
        emit downloadComplete(QString(), error);
        return false;
    }

    _activeJob = job;
    const QPointer<FTPDownloadJob> guardedJob(job);
    connect(job, &FTPDownloadJob::finished, this,
            [this, guardedJob](const QString& filePath, const QString& error, const QString& warning) {
                if (!guardedJob || (guardedJob != _activeJob)) {
        return;
                }
                const QPointer<FTPController> completionGuard(this);
                _handleDownloadComplete(filePath, error, warning);
                if (!completionGuard) {
        return;
                }
                emit downloadComplete(filePath, error);
            });
    connect(job, &FTPJob::progress, this, [this, guardedJob](float value) {
        if (guardedJob && (guardedJob == _activeJob)) {
            _handleCommandProgress(value);
        }
    });

    return true;
}

bool FTPController::uploadFile(const QString &localFile, const QString &uri, int componentId)
{
    const QPointer<FTPController> self(this);
    if (!_ftpManager) {
        const QString error = tr("No active vehicle is available");
        _setErrorString(error);
        if (!self) {
        return false;
        }
        emit uploadComplete(QString(), error);
        return false;
    }
    if (_operation != Operation::None) {
        const QString error = tr("Another FTP operation is in progress");
        _setErrorString(error);
        if (!self) {
        return false;
        }
        emit uploadComplete(QString(), error);
        return false;
    }

    QFileInfo sourceInfo(localFile);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        const QString error = tr("File %1 does not exist").arg(localFile);
        _setErrorString(error);
        if (!self) {
        return false;
        }
        emit uploadComplete(QString(), error);
        return false;
    }
    const QPointer<FTPManager> ftpManager(_ftpManager);

    _lastUploadTarget.clear();
    emit lastUploadTargetChanged();
    if (!self) {
        return false;
    }
    _setErrorString(QString());
    if (!self) {
        return false;
    }

    _setBusy(true);
    if (!self) {
        return false;
    }
    _setOperation(Operation::Upload);
    if (!self) {
        return false;
    }
    _progress = 0.0F;
    emit progressChanged();
    if (!self || (_ftpManager != ftpManager) || (_operation != Operation::Upload)) {
        return false;
    }

    const uint8_t compId = _componentIdForRequest(componentId);
    const FTPManager::UploadStartResult startResult =
        ftpManager->startUpload(compId, uri, sourceInfo.absoluteFilePath());
    FTPUploadJob* const job = startResult.job();
    if (!job) {
        qCWarning(FTPControllerLog) << "Failed to start upload" << sourceInfo.absoluteFilePath() << uri;
        const QString error = startResult.error() == FTPManager::StartError::Busy
                                  ? tr("Another vehicle FTP operation is in progress")
                                  : tr("Failed to upload %1").arg(sourceInfo.fileName());
        _setErrorString(error);
        if (!self) {
        return false;
        }
        _setBusy(false);
        if (!self) {
        return false;
        }
        _clearOperation();
        if (!self) {
        return false;
        }
        emit uploadComplete(QString(), error);
        return false;
    }

    _activeJob = job;
    const QPointer<FTPUploadJob> guardedJob(job);
    connect(job, &FTPUploadJob::finished, this, [this, guardedJob](const QString& remotePath, const QString& error) {
        if (!guardedJob || (guardedJob != _activeJob)) {
        return;
        }
        const QPointer<FTPController> completionGuard(this);
        _handleUploadComplete(remotePath, error);
        if (!completionGuard) {
        return;
        }
        emit uploadComplete(remotePath, error);
    });
    connect(job, &FTPJob::progress, this, [this, guardedJob](float value) {
        if (guardedJob && (guardedJob == _activeJob)) {
            _handleCommandProgress(value);
        }
    });

    return true;
}

bool FTPController::deleteFile(const QString &uri, int componentId)
{
    const QPointer<FTPController> self(this);
    if (!_ftpManager) {
        const QString error = tr("No active vehicle is available");
        _setErrorString(error);
        if (!self) {
        return false;
        }
        emit deleteComplete(QString(), error);
        return false;
    }
    if (_operation != Operation::None) {
        const QString error = tr("Another FTP operation is in progress");
        _setErrorString(error);
        if (!self) {
        return false;
        }
        emit deleteComplete(QString(), error);
        return false;
    }
    const QPointer<FTPManager> ftpManager(_ftpManager);

    _setErrorString(QString());
    if (!self) {
        return false;
    }
    _setBusy(true);
    if (!self) {
        return false;
    }
    _setOperation(Operation::Delete);
    if (!self || (_ftpManager != ftpManager) || (_operation != Operation::Delete)) {
        return false;
    }

    const uint8_t compId = _componentIdForRequest(componentId);
    const FTPManager::DeleteStartResult startResult = ftpManager->startDeleteFile(compId, uri);
    FTPDeleteJob* const job = startResult.job();
    if (!job) {
        qCWarning(FTPControllerLog) << "Failed to start delete" << uri;
        const QString error = startResult.error() == FTPManager::StartError::Busy
                                  ? tr("Another vehicle FTP operation is in progress")
                                  : tr("Failed to delete %1").arg(uri);
        _setErrorString(error);
        if (!self) {
        return false;
        }
        _setBusy(false);
        if (!self) {
        return false;
        }
        _clearOperation();
        if (!self) {
        return false;
        }
        emit deleteComplete(QString(), error);
        return false;
    }

    _activeJob = job;
    const QPointer<FTPDeleteJob> guardedJob(job);
    connect(job, &FTPDeleteJob::finished, this, [this, guardedJob](const QString& remotePath, const QString& error) {
        if (!guardedJob || (guardedJob != _activeJob)) {
        return;
        }
        const QPointer<FTPController> completionGuard(this);
        _handleDeleteComplete(remotePath, error);
        if (!completionGuard) {
        return;
        }
        emit deleteComplete(remotePath, error);
    });

    return true;
}

void FTPController::cancelActiveOperation()
{
    if (_activeJob) {
        _activeJob->cancel();
    }
}

void FTPController::_handleDownloadComplete(const QString& filePath, const QString& error, const QString& warning)
{
    if (_operation != Operation::Download) {
        return;
    }

    _activeJob = nullptr;
    const QPointer<FTPController> self(this);
    _setBusy(false);
    if (!self) {
        return;
    }

    if (error.isEmpty()) {
        if (!filePath.isEmpty()) {
            _lastDownloadFile = filePath;
            _lastDownloadIsArchive = QGCCompression::isArchiveFile(filePath);
            emit lastDownloadFileChanged();
            if (!self) {
        return;
        }
        }
        _setErrorString(warning);
        if (!self) {
        return;
        }
    } else {
        _setErrorString(error);
        if (!self) {
        return;
        }
        _lastDownloadFile.clear();
        _lastDownloadIsArchive = false;
        emit lastDownloadFileChanged();
        if (!self) {
        return;
        }
    }

    _clearOperation();
}

void FTPController::_handleUploadComplete(const QString &remotePath, const QString &error)
{
    if (_operation != Operation::Upload) {
        return;
    }

    _activeJob = nullptr;
    const QPointer<FTPController> self(this);
    _setBusy(false);
    if (!self) {
        return;
    }

    if (error.isEmpty()) {
        _lastUploadTarget = remotePath;
        emit lastUploadTargetChanged();
        if (!self) {
        return;
        }
        _setErrorString(QString());
        if (!self) {
        return;
        }
    } else {
        _setErrorString(error);
        if (!self) {
        return;
        }
        _lastUploadTarget.clear();
        emit lastUploadTargetChanged();
        if (!self) {
        return;
        }
    }

    _clearOperation();
}

void FTPController::_handleDirectoryComplete(const QStringList& entries, const QString& error, bool truncated)
{
    if (_operation != Operation::List) {
        return;
    }

    _activeJob = nullptr;
    const QPointer<FTPController> self(this);
    _setBusy(false);
    if (!self) {
        return;
    }

    if (error.isEmpty()) {
        if (_directoryEntries != entries) {
            _directoryEntries = entries;
            emit directoryEntriesChanged();
            if (!self) {
        return;
        }
        }
        if (_directoryTruncated != truncated) {
            _directoryTruncated = truncated;
            emit directoryTruncatedChanged();
            if (!self) {
        return;
            }
        }
        _setErrorString(truncated ? tr("Directory listing was limited to %1 entries").arg(kMaximumDirectoryEntries)
                                  : QString());
        if (!self) {
        return;
        }
    } else {
        _setErrorString(error);
        if (!self) {
        return;
        }
        _resetDirectoryState();
        if (!self) {
        return;
        }
    }

    _clearOperation();
}

void FTPController::_handleDeleteComplete(const QString &remotePath, const QString &error)
{
    Q_UNUSED(remotePath)

    if (_operation != Operation::Delete) {
        return;
    }

    _activeJob = nullptr;
    const QPointer<FTPController> self(this);
    _setBusy(false);
    if (!self) {
        return;
    }

    if (error.isEmpty()) {
        _setErrorString(QString());
    } else {
        _setErrorString(error);
    }
    if (!self) {
        return;
    }

    _clearOperation();
}

void FTPController::_handleCommandProgress(float value)
{
    if (_operation == Operation::Download || _operation == Operation::Upload) {
        if (std::fabs(_progress - value) > std::numeric_limits<float>::epsilon()) {
            _progress = value;
            emit progressChanged();
        }
    }
}

void FTPController::_setBusy(bool busy)
{
    if (_busy == busy) {
        return;
    }

    _busy = busy;
    emit busyChanged();
}

void FTPController::_setOperation(Operation operation)
{
    if (_operation == operation) {
        return;
    }

    _operation = operation;
    emit activeOperationChanged();
}

void FTPController::_clearOperation()
{
    const QPointer<FTPController> self(this);
    _setOperation(Operation::None);
    if (!self) {
        return;
    }
    if (std::fabs(_progress) > std::numeric_limits<float>::epsilon()) {
        _progress = 0.0F;
        emit progressChanged();
    }
}

void FTPController::_setErrorString(const QString &error)
{
    if (_errorString == error) {
        return;
    }

    _errorString = error;
    emit errorStringChanged();
}

void FTPController::_resetDirectoryState()
{
    const QPointer<FTPController> self(this);
    if (!_directoryEntries.isEmpty()) {
        _directoryEntries.clear();
        emit directoryEntriesChanged();
        if (!self) {
        return;
        }
    }
    if (_directoryTruncated) {
        _directoryTruncated = false;
        emit directoryTruncatedChanged();
    }
}

uint8_t FTPController::_componentIdForRequest(int componentId) const
{
    if (componentId <= 0 || componentId > (std::numeric_limits<uint8_t>::max)()) {
        return MAV_COMP_ID_AUTOPILOT1;
    }

    return static_cast<uint8_t>(componentId);
}

bool FTPController::browseArchive(const QString &archivePath)
{
    if (archivePath.isEmpty()) {
        _setErrorString(tr("No archive path specified"));
        return false;
    }

    if (!QFile::exists(archivePath)) {
        _setErrorString(tr("Archive file not found: %1").arg(archivePath));
        return false;
    }

    if (!QGCCompression::isArchiveFile(archivePath)) {
        _setErrorString(tr("Not a supported archive format: %1").arg(archivePath));
        return false;
    }

    _archiveModel->setArchivePath(archivePath);
    _setErrorString(QString());
    return true;
}

bool FTPController::extractArchive(const QString &archivePath, const QString &outputDir)
{
    if (_extracting || _extractionFinishing) {
        _setErrorString(tr("Extraction already in progress"));
        return false;
    }

    if (archivePath.isEmpty()) {
        _setErrorString(tr("No archive path specified"));
        return false;
    }

    if (!QFile::exists(archivePath)) {
        _setErrorString(tr("Archive file not found: %1").arg(archivePath));
        return false;
    }

    QString targetDir = outputDir;
    if (targetDir.isEmpty()) {
        targetDir = QFileInfo(archivePath).absolutePath();
    }

    if (!QGCFileHelper::ensureDirectoryExists(targetDir)) {
        _setErrorString(tr("Could not create output directory: %1").arg(targetDir));
        return false;
    }

    const quint64 generation = ++_extractionGeneration;
    QGCCompressionJob* const job = new QGCCompressionJob(this);
    _extractionJob = job;
    connect(
        job, &QGCCompressionJob::progressChanged, this,
        [this, generation, job](qreal progress) { _handleExtractionProgress(generation, job, progress); },
        Qt::QueuedConnection);
    connect(
        job, &QGCCompressionJob::finished, this,
        [this, generation, job, targetDir](bool success) {
            _handleExtractionFinished(generation, job, targetDir, success);
        },
        Qt::QueuedConnection);

    _extracting = true;
    _extractionProgress = 0.0F;
    const QPointer<FTPController> self(this);
    emit extractingChanged();
    if (!self || (generation != _extractionGeneration) || (_extractionJob != job)) {
        return false;
    }
    emit extractionProgressChanged();
    if (!self || (generation != _extractionGeneration) || (_extractionJob != job)) {
        return false;
    }

    job->extractArchive(archivePath, targetDir);
    return true;
}

void FTPController::cancelExtraction()
{
    if (_extractionJob != nullptr && _extracting) {
        _extractionJob->cancel();
    }
}

void FTPController::_handleExtractionProgress(quint64 generation, QGCCompressionJob* job, qreal progress)
{
    if (!_extracting || _extractionFinishing || (generation != _extractionGeneration) || (_extractionJob != job)) {
        return;
    }

    const auto newProgress = static_cast<float>(progress);
    if (std::fabs(_extractionProgress - newProgress) > std::numeric_limits<float>::epsilon()) {
        _extractionProgress = newProgress;
        emit extractionProgressChanged();
    }
}

void FTPController::_handleExtractionFinished(quint64 generation, QGCCompressionJob* job, const QString& outputDir,
                                              bool success)
{
    if (!_extracting || _extractionFinishing || (generation != _extractionGeneration) || (_extractionJob != job)) {
        return;
    }

    const QString jobError = job->errorString();
    const QString error = success ? QString() : (jobError.isEmpty() ? tr("Extraction failed") : jobError);
    _extractionFinishing = true;
    _extractionJob = nullptr;
    job->deleteLater();
    _extractionProgress = 0.0F;
    _extracting = false;

    const QPointer<FTPController> self(this);
        _setErrorString(error);
    if (!self) {
        return;
    }
    emit extractingChanged();
    if (!self) {
        return;
    }
    emit extractionProgressChanged();
    if (!self) {
        return;
}

    _extractionFinishing = false;
    emit extractionComplete(success ? outputDir : QString(), error);
}
