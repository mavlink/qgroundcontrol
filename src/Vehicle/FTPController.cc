#include "FTPController.h"

#include "FTPManager.h"
#include "MultiVehicleManager.h"
#include "QGCFileHelper.h"
#include "Vehicle.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(FTPControllerLog, "Vehicle.FTPController")

FTPController::FTPController(QObject *parent)
    : QObject(parent)
    , _vehicle(MultiVehicleManager::instance()->activeVehicle())
    , _ftpManager(_vehicle->ftpManager())
{
    connect(_ftpManager, &FTPManager::downloadComplete, this, &FTPController::_handleDownloadComplete);
    connect(_ftpManager, &FTPManager::downloadComplete, this, &FTPController::downloadComplete);
    connect(_ftpManager, &FTPManager::uploadComplete, this, &FTPController::_handleUploadComplete);
    connect(_ftpManager, &FTPManager::uploadComplete, this, &FTPController::uploadComplete);
    connect(_ftpManager, &FTPManager::listDirectoryComplete, this, &FTPController::_handleDirectoryComplete);
    connect(_ftpManager, &FTPManager::commandProgress, this, &FTPController::_handleCommandProgress);
    connect(_ftpManager, &FTPManager::deleteComplete, this, &FTPController::_handleDeleteComplete);
    connect(_ftpManager, &FTPManager::deleteComplete, this, &FTPController::deleteComplete);
}

bool FTPController::listDirectory(const QString &uri, int componentId)
{
    if (_operation != Operation::None) {
        _setErrorString(tr("Another FTP operation is in progress"));
        return false;
    }

    const QString previousPath = _currentPath;

    _resetDirectoryState();
    _currentPath = uri;
    emit currentPathChanged();
    _setErrorString(QString());

    _setBusy(true);
    _setOperation(Operation::List);
    _progress = 0.0F;
    emit progressChanged();

    const uint8_t compId = _componentIdForRequest(componentId);
    if (!_ftpManager->listDirectory(compId, uri)) {
        qCWarning(FTPControllerLog) << "Failed to start list operation for" << uri;
        _setErrorString(tr("Failed to list %1").arg(uri));
        _currentPath = previousPath;
        emit currentPathChanged();
        _setBusy(false);
        _clearOperation();
        return false;
    }

    return true;
}

bool FTPController::downloadFile(const QString &uri, const QString &localDir, const QString &fileName, int componentId)
{
    if (_operation != Operation::None) {
        const QString error = tr("Another FTP operation is in progress");
        _setErrorString(error);
        emit downloadComplete(QString(), error);
        return false;
    }

    if (!QGCFileHelper::ensureDirectoryExists(localDir)) {
        const QString error = tr("Could not create directory %1").arg(localDir);
        _setErrorString(error);
        emit downloadComplete(QString(), error);
        return false;
    }

    const QString absoluteLocalDir = QDir(localDir).absolutePath();

    _lastDownloadFile.clear();
    emit lastDownloadFileChanged();
    _setErrorString(QString());

    _setBusy(true);
    _setOperation(Operation::Download);
    _progress = 0.0F;
    emit progressChanged();

    const uint8_t compId = _componentIdForRequest(componentId);
    if (!_ftpManager->download(compId, uri, absoluteLocalDir, fileName)) {
        qCWarning(FTPControllerLog) << "Failed to start download" << uri << absoluteLocalDir;
        const QString error = tr("Failed to download %1").arg(uri);
        _setErrorString(error);
        _setBusy(false);
        _clearOperation();
        emit downloadComplete(QString(), error);
        return false;
    }

    return true;
}

bool FTPController::uploadFile(const QString &localFile, const QString &uri, int componentId)
{
    if (_operation != Operation::None) {
        const QString error = tr("Another FTP operation is in progress");
        _setErrorString(error);
        emit uploadComplete(QString(), error);
        return false;
    }

    QFileInfo sourceInfo(localFile);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        const QString error = tr("File %1 does not exist").arg(localFile);
        _setErrorString(error);
        emit uploadComplete(QString(), error);
        return false;
    }

    _lastUploadTarget.clear();
    emit lastUploadTargetChanged();
    _setErrorString(QString());

    _setBusy(true);
    _setOperation(Operation::Upload);
    _progress = 0.0F;
    emit progressChanged();

    const uint8_t compId = _componentIdForRequest(componentId);
    if (!_ftpManager->upload(compId, uri, sourceInfo.absoluteFilePath())) {
        qCWarning(FTPControllerLog) << "Failed to start upload" << sourceInfo.absoluteFilePath() << uri;
        const QString error = tr("Failed to upload %1").arg(sourceInfo.fileName());
        _setErrorString(error);
        _setBusy(false);
        _clearOperation();
        emit uploadComplete(QString(), error);
        return false;
    }

    return true;
}

bool FTPController::deleteFile(const QString &uri, int componentId)
{
    if (_operation != Operation::None) {
        const QString error = tr("Another FTP operation is in progress");
        _setErrorString(error);
        emit deleteComplete(QString(), error);
        return false;
    }

    _setErrorString(QString());
    _setBusy(true);
    _setOperation(Operation::Delete);

    const uint8_t compId = _componentIdForRequest(componentId);
    if (!_ftpManager->deleteFile(compId, uri)) {
        qCWarning(FTPControllerLog) << "Failed to start delete" << uri;
        const QString error = tr("Failed to delete %1").arg(uri);
        _setErrorString(error);
        _setBusy(false);
        _clearOperation();
        emit deleteComplete(QString(), error);
        return false;
    }

    return true;
}

void FTPController::cancelActiveOperation()
{
    switch (_operation) {
    case Operation::Download:
        _ftpManager->cancelDownload();
        break;
    case Operation::Upload:
        _ftpManager->cancelUpload();
        break;
    case Operation::List:
        _ftpManager->cancelListDirectory();
        break;
    case Operation::Delete:
        _ftpManager->cancelDelete();
        break;
    case Operation::None:
        break;
    }
}

void FTPController::_handleDownloadComplete(const QString &filePath, const QString &error)
{
    if (_operation != Operation::Download) {
        return;
    }

    _setBusy(false);

    if (error.isEmpty()) {
        if (!filePath.isEmpty()) {
            _lastDownloadFile = filePath;
            emit lastDownloadFileChanged();
        }
        _setErrorString(QString());
    } else {
        _setErrorString(error);
        _lastDownloadFile.clear();
        emit lastDownloadFileChanged();
    }

    _clearOperation();
}

void FTPController::_handleUploadComplete(const QString &remotePath, const QString &error)
{
    if (_operation != Operation::Upload) {
        return;
    }

    _setBusy(false);

    if (error.isEmpty()) {
        _lastUploadTarget = remotePath;
        emit lastUploadTargetChanged();
        _setErrorString(QString());
    } else {
        _setErrorString(error);
        _lastUploadTarget.clear();
        emit lastUploadTargetChanged();
    }

    _clearOperation();
}

void FTPController::_handleDirectoryComplete(const QStringList &entries, const QString &error)
{
    if (_operation != Operation::List) {
        return;
    }

    _setBusy(false);

    if (error.isEmpty()) {
        if (_directoryEntries != entries) {
            _directoryEntries = entries;
            emit directoryEntriesChanged();
        }
        _setErrorString(QString());
    } else {
        _setErrorString(error);
        _resetDirectoryState();
    }

    _clearOperation();
}

void FTPController::_handleDeleteComplete(const QString &remotePath, const QString &error)
{
    Q_UNUSED(remotePath)

    if (_operation != Operation::Delete) {
        return;
    }

    _setBusy(false);

    if (error.isEmpty()) {
        _setErrorString(QString());
    } else {
        _setErrorString(error);
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
    _setOperation(Operation::None);
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
    if (!_directoryEntries.isEmpty()) {
        _directoryEntries.clear();
        emit directoryEntriesChanged();
    }
}

uint8_t FTPController::_componentIdForRequest(int componentId) const
{
    if (componentId <= 0 || componentId > std::numeric_limits<uint8_t>::max()) {
        return MAV_COMP_ID_AUTOPILOT1;
    }

    return static_cast<uint8_t>(componentId);
}
