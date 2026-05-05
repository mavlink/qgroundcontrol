#include "FTPManager.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <algorithm>
#include <limits>

#include "AppMessages.h"
#include "FTPManagerJob.h"
#include "FTPManagerOperations.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(FTPManagerLog, "Vehicle.FTPManager")

FTPManager::FTPManager(Vehicle* vehicle)
    : QObject(vehicle),
      _vehicle(vehicle),
      _downloadOperation(std::make_unique<DownloadOperation>()),
      _listDirectoryOperation(std::make_unique<ListDirectoryOperation>()),
      _deleteOperation(std::make_unique<DeleteOperation>()),
      _uploadOperation(std::make_unique<UploadOperation>())
{
    _ackOrNakTimeoutTimer.setSingleShot(true);
    // Mock link responds immediately if at all, speed up unit tests with faster timeout
    _ackOrNakTimeoutTimer.setInterval(QGC::runningUnitTests() ? kTestAckTimeoutMs : _ackOrNakTimeoutMsecs);
    connect(&_ackOrNakTimeoutTimer, &QTimer::timeout, this, &FTPManager::_ackOrNakTimeout);
}

FTPManager::~FTPManager()
{
    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();

    if (_downloadOperation->inProgress() && !_downloadOperation->file.fileName().isEmpty()) {
        _downloadOperation->file.close();
        if (_downloadOperation->file.exists() && !_downloadOperation->file.remove()) {
            qCWarning(FTPManagerLog) << "Failed to remove partial download during teardown:"
                                     << _downloadOperation->file.fileName() << _downloadOperation->file.errorString();
        }
    }
}

bool FTPManager::download(uint8_t fromCompId, const QString& fromURI, const QString& toDir, const QString& fileName,
                          bool checkSize, ExistingFilePolicy existingFilePolicy)
{
    return startDownload(fromCompId, fromURI, toDir, fileName, checkSize, existingFilePolicy) != nullptr;
}

FTPDownloadJob* FTPManager::startDownload(uint8_t fromCompId, const QString& fromURI, const QString& toDir,
                                          const QString& fileName, bool checkSize,
                                          ExistingFilePolicy existingFilePolicy)
{
    qCDebug(FTPManagerLog) << "Download fromCompId:" << fromCompId << "fromURI:" << fromURI << "to:" << toDir
                           << "fileName:" << fileName;

    if (!_rgStateMachine.isEmpty()) {
        qCDebug(FTPManagerLog) << "Cannot download. Already in another operation";
        return nullptr;
    }

    _resetSessionsRetryCount = 0;
    if (!_downloadOperation->begin(fromCompId, fromURI, toDir, fileName, checkSize, existingFilePolicy, _ftpCompId)) {
        qCWarning(FTPManagerLog) << "Unable to parse MAVLink FTP URI" << fromURI;
        _rgStateMachine.clear();
        return nullptr;
    }
    _rgStateMachine = _downloadOperation->stateMachine();

    qCDebug(FTPManagerLog) << "_downloadOperation->fullPathOnVehicle:_downloadOperation->fileName"
                           << _downloadOperation->fullPathOnVehicle << _downloadOperation->fileName;

    auto* const job = new FTPDownloadJob(this);
    _activeJob = job;
    _startStateMachine();

    return job;
}

bool FTPManager::upload(uint8_t toCompId, const QString& toURI, const QString& fromFile)
{
    return startUpload(toCompId, toURI, fromFile) != nullptr;
}

FTPUploadJob* FTPManager::startUpload(uint8_t toCompId, const QString& toURI, const QString& fromFile)
{
    qCDebug(FTPManagerLog) << "upload fromFile:" << fromFile << "toURI:" << toURI << "toCompId:" << toCompId;

    if (!_rgStateMachine.isEmpty()) {
        qCDebug(FTPManagerLog) << "Cannot upload. Already in another operation";
        return nullptr;
    }

    _resetSessionsRetryCount = 0;
    const UploadOperation::BeginResult beginResult = _uploadOperation->begin(toCompId, toURI, fromFile, _ftpCompId);
    switch (beginResult) {
        case UploadOperation::BeginResult::Success:
            break;
        case UploadOperation::BeginResult::SourceMissing:
            qCWarning(FTPManagerLog) << "Cannot upload. Source file missing" << fromFile;
            return nullptr;
        case UploadOperation::BeginResult::SourceTooLarge:
            qCWarning(FTPManagerLog) << "Cannot upload. File too large" << fromFile;
            return nullptr;
        case UploadOperation::BeginResult::OpenFailed:
            qCWarning(FTPManagerLog) << "Cannot upload. Failed to open file" << fromFile
                                     << _uploadOperation->openErrorString;
            return nullptr;
        case UploadOperation::BeginResult::InvalidUri:
            qCWarning(FTPManagerLog) << "Unable to parse MAVLink FTP URI" << toURI;
            return nullptr;
    }

    _rgStateMachine = _uploadOperation->stateMachine();

    auto* const job = new FTPUploadJob(this);
    _activeJob = job;
    _startStateMachine();

    return job;
}

bool FTPManager::listDirectory(uint8_t fromCompId, const QString& fromURI, int maxEntries)
{
    return startListDirectory(fromCompId, fromURI, maxEntries) != nullptr;
}

FTPListDirectoryJob* FTPManager::startListDirectory(uint8_t fromCompId, const QString& fromURI, int maxEntries)
{
    qCDebug(FTPManagerLog) << "list directory fromURI:" << fromURI << "fromCompId:" << fromCompId;

    if (!_rgStateMachine.isEmpty()) {
        qCDebug(FTPManagerLog) << "Cannot list directory. Already in another operation";
        return nullptr;
    }

    if (maxEntries < 0) {
        qCWarning(FTPManagerLog) << "Cannot list directory with a negative entry limit" << maxEntries;
        return nullptr;
    }

    // Prefer the timestamped listing unless we already learned this vehicle doesn't support it.
    const MAV_FTP_OPCODE listOpcode = (_listDirWithTimeSupport == WithTimeSupport_t::Unsupported)
                                          ? MAV_FTP_OPCODE_LISTDIRECTORY
                                          : MAV_FTP_OPCODE_LISTDIRECTORYWITHTIME;
    if (!_listDirectoryOperation->begin(fromCompId, fromURI, maxEntries, listOpcode, _ftpCompId)) {
        qCWarning(FTPManagerLog) << "Unable to parse MAVLink FTP URI" << fromURI;
        _rgStateMachine.clear();
        return nullptr;
    }
    _rgStateMachine = _listDirectoryOperation->stateMachine();

    qCDebug(FTPManagerLog) << "_listDirectoryOperation->fullPathOnVehicle"
                           << _listDirectoryOperation->fullPathOnVehicle;

    auto* const job = new FTPListDirectoryJob(this);
    _activeJob = job;
    _startStateMachine();

    return job;
}

bool FTPManager::deleteFile(uint8_t fromCompId, const QString& fromURI)
{
    return startDeleteFile(fromCompId, fromURI) != nullptr;
}

FTPDeleteJob* FTPManager::startDeleteFile(uint8_t fromCompId, const QString& fromURI)
{
    qCDebug(FTPManagerLog) << "delete file fromURI:" << fromURI << "fromCompId:" << fromCompId;

    if (!_rgStateMachine.isEmpty()) {
        qCDebug(FTPManagerLog) << "Cannot delete file. Already in another operation";
        return nullptr;
    }

    if (!_deleteOperation->begin(fromCompId, fromURI, _ftpCompId)) {
        qCWarning(FTPManagerLog) << "Unable to parse MAVLink FTP URI" << fromURI;
        _rgStateMachine.clear();
        return nullptr;
    }
    _rgStateMachine = _deleteOperation->stateMachine();

    qCDebug(FTPManagerLog) << "_deleteOperation->fullPathOnVehicle" << _deleteOperation->fullPathOnVehicle;

    auto* const job = new FTPDeleteJob(this);
    _activeJob = job;
    _startStateMachine();

    return job;
}

void FTPManager::_cancelJob(FTPJob* job)
{
    if (!job || (job != _activeJob)) {
        return;
    }

    switch (job->type()) {
        case FTPJob::Type::Download:
            cancelDownload();
            break;
        case FTPJob::Type::Upload:
            cancelUpload();
            break;
        case FTPJob::Type::ListDirectory:
            cancelListDirectory();
            break;
        case FTPJob::Type::Delete:
            cancelDelete();
            break;
    }
}

void FTPManager::_emitCommandProgress(float value)
{
    if (_activeJob) {
        _activeJob->_emitProgress(value);
    }
    emit commandProgress(value);
}

void FTPManager::cancelDownload()
{
    if (!_downloadOperation->inProgress()) {
        return;
    }

    _terminateDownload(tr("Aborted"));
}

void FTPManager::_terminateDownload(const QString& errorMsg)
{
    _ackOrNakTimeoutTimer.stop();
    if (!_downloadOperation->remoteSessionOpen) {
        _downloadComplete(errorMsg);
        return;
    }

    _downloadOperation->pendingError = errorMsg;
    _rgStateMachine.clear();
    _rgStateMachine = _downloadOperation->terminationStateMachine();
    _downloadOperation->retryCount = 0;
    _startStateMachine();
}

void FTPManager::cancelListDirectory()
{
    if (!_listDirectoryOperation->inProgress()) {
        return;
    }

    if (_rgStateMachine.isEmpty()) {
        return;
    }

    _listDirectoryComplete(tr("Aborted"));
}

void FTPManager::cancelUpload()
{
    if (!_uploadOperation->inProgress()) {
        return;
    }

    _uploadOperation->cancelled = true;
    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();

    if (_uploadOperation->sessionId != 0) {
        _rgStateMachine = _uploadOperation->terminationStateMachine();
        _uploadOperation->retryCount = 0;
        _startStateMachine();
    } else {
        _uploadComplete(tr("Aborted"));
    }
}

void FTPManager::cancelDelete()
{
    if (!_deleteOperation->inProgress()) {
        return;
    }

    _deleteComplete(tr("Aborted"));
}

void FTPManager::_terminateSessionBegin(void)
{
    MavlinkFTP::Request request{};
    request.hdr.session = _downloadOperation->sessionId;
    request.hdr.opcode = MAV_FTP_OPCODE_TERMINATESESSION;
    _sendRequestExpectAck(&request);
}

void FTPManager::_terminateSessionAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_TERMINATESESSION, "_terminateSessionAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog)
            << "_terminateSessionAckOrNak: Ack disregarding ack for incorrect sequence actual:expected"
            << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }
    _ackOrNakTimeoutTimer.stop();
    _downloadOperation->remoteSessionOpen = false;
    _advanceStateMachine();
}

void FTPManager::_terminateSessionTimeout(void)
{
    if (++_downloadOperation->retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_terminateSessionTimeout retries exceeded");
        _downloadOperation->remoteSessionOpen = false;
        const QString cleanupError = tr("Unable to close the remote download session");
        const QString errorMsg = _downloadOperation->pendingError.isEmpty()
                                     ? cleanupError
                                     : tr("%1; %2").arg(_downloadOperation->pendingError, cleanupError);
        _downloadComplete(errorMsg);
    } else {
        // Try again
        qCDebug(FTPManagerLog)
            << QString("_terminateSessionTimeout: retrying - retryCount(%1)").arg(_downloadOperation->retryCount);
        _terminateSessionBegin();
    }
}

void FTPManager::_terminateComplete(void)
{
    _downloadComplete(_downloadOperation->pendingError);
}

/// Closes out a download session by writing the file and doing cleanup.
///     @param errorMsg Error message, empty if no error
void FTPManager::_downloadComplete(const QString& errorMsg, const QString& warningMsg)
{
    qCDebug(FTPManagerLog) << "_downloadComplete: error" << errorMsg << "warning" << warningMsg;

    if (!errorMsg.isEmpty() && _downloadOperation->remoteSessionOpen) {
        _terminateDownload(errorMsg);
        return;
    }

    const QString downloadFilePath = _downloadOperation->toDir.absoluteFilePath(_downloadOperation->fileName);
    QString finalErrorMsg = errorMsg;

    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();
    _currentStateMachineIndex = -1;
    _downloadOperation->file.close();

    if (finalErrorMsg.isEmpty() && !_downloadOperation->file.fileName().isEmpty()) {
        if ((_downloadOperation->existingFilePolicy == ExistingFilePolicy::Replace) &&
            QFileInfo::exists(downloadFilePath) && !QFile::remove(downloadFilePath)) {
            finalErrorMsg = tr("Unable to replace existing download: %1").arg(downloadFilePath);
        } else if (!_downloadOperation->file.rename(downloadFilePath)) {
            finalErrorMsg = (_downloadOperation->existingFilePolicy == ExistingFilePolicy::FailIfExists) &&
                                    QFileInfo::exists(downloadFilePath)
                                ? tr("Download destination already exists: %1").arg(downloadFilePath)
                                : tr("Unable to finalize download: %1").arg(_downloadOperation->file.errorString());
        }
    }

    if (!finalErrorMsg.isEmpty() && _downloadOperation->file.exists() && !_downloadOperation->file.remove()) {
        const QString cleanupError =
            tr("Unable to remove incomplete download: %1").arg(_downloadOperation->file.errorString());
        qCWarning(FTPManagerLog) << cleanupError << _downloadOperation->file.fileName();
        finalErrorMsg = tr("%1; %2").arg(finalErrorMsg, cleanupError);
    }

    _downloadOperation->reset();

    QPointer<FTPDownloadJob> job = qobject_cast<FTPDownloadJob*>(_activeJob);
    _activeJob = nullptr;
    if (job) {
        job->_finish();
    }
    emit downloadComplete(downloadFilePath, finalErrorMsg, warningMsg);
    if (job) {
        job->_emitFinished(downloadFilePath, finalErrorMsg, warningMsg);
        job->deleteLater();
    }
}

/// Closes out a list directory sequence
///     @param errorMsg Error message, empty if no error
void FTPManager::_listDirectoryComplete(const QString& errorMsg)
{
    qCDebug(FTPManagerLog) << QString("_listDirectoryComplete: errorMsg(%1)").arg(errorMsg);

    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();
    _currentStateMachineIndex = -1;

    QStringList directoryEntries = _listDirectoryOperation->directoryEntries;
    const bool truncated = errorMsg.isEmpty() && _listDirectoryOperation->truncated;
    if (!errorMsg.isEmpty()) {
        directoryEntries.clear();
    }

    _listDirectoryOperation->reset();

    const QStringList completedList = errorMsg.isEmpty() ? directoryEntries : QStringList();
    QPointer<FTPListDirectoryJob> job = qobject_cast<FTPListDirectoryJob*>(_activeJob);
    _activeJob = nullptr;
    if (job) {
        job->_finish();
    }
    emit listDirectoryComplete(completedList, errorMsg, truncated);
    if (job) {
        job->_emitFinished(completedList, errorMsg, truncated);
        job->deleteLater();
    }
}

void FTPManager::_deleteFileBegin(void)
{
    qCDebug(FTPManagerLog) << "file" << _deleteOperation->fullPathOnVehicle;

    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MAV_FTP_OPCODE_REMOVEFILE;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    if (!MavlinkFTP::setRequestData(request, _deleteOperation->fullPathOnVehicle)) {
        _deleteComplete(tr("Delete failed: remote path is too long"));
        return;
    }
    _sendRequestExpectAck(&request);
}

void FTPManager::_deleteFileAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_REMOVEFILE, "_deleteFileAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_deleteFileAckOrNak: Ack disregarding ack for incorrect sequence actual:expected"
                               << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        _advanceStateMachine();
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        qCDebug(FTPManagerLog) << "_deleteFileAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
        _deleteComplete(tr("Delete failed") + ": " + _errorMsgFromNak(ackOrNak));
    }
}

void FTPManager::_deleteFileTimeout(void)
{
    if (++_deleteOperation->retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_deleteFileTimeout retries exceeded");
        _deleteComplete(tr("Delete failed"));
    } else {
        qCDebug(FTPManagerLog)
            << QString("_deleteFileTimeout: retrying - retryCount(%1)").arg(_deleteOperation->retryCount);
        _deleteFileBegin();
    }
}

void FTPManager::_deleteComplete(const QString& errorMsg)
{
    qCDebug(FTPManagerLog) << QString("_deleteComplete: errorMsg(%1)").arg(errorMsg);

    const QString deletedPath = _deleteOperation->fullPathOnVehicle;

    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();
    _currentStateMachineIndex = -1;

    _deleteOperation->reset();

    QPointer<FTPDeleteJob> job = qobject_cast<FTPDeleteJob*>(_activeJob);
    _activeJob = nullptr;
    if (job) {
        job->_finish();
    }
    emit deleteComplete(deletedPath, errorMsg);
    if (job) {
        job->_emitFinished(deletedPath, errorMsg);
        job->deleteLater();
    }
}

void FTPManager::_createFileBegin(void)
{
    qCDebug(FTPManagerLog) << "file" << _uploadOperation->fullPathOnVehicle;

    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MAV_FTP_OPCODE_CREATEFILE;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    if (!MavlinkFTP::setRequestData(request, _uploadOperation->fullPathOnVehicle)) {
        _uploadComplete(tr("Upload failed: remote path is too long"));
        return;
    }
    _sendRequestExpectAck(&request);
}

void FTPManager::_createFileAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_CREATEFILE, "_createFileAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_createFileAckOrNak: Ack disregarding ack for incorrect sequence actual:expected"
                               << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        qCDebug(FTPManagerLog) << "_createFileAckOrNak: Ack - sessionId" << ackOrNak->hdr.session;

        _uploadOperation->sessionId = ackOrNak->hdr.session;
        _advanceStateMachine();
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        qCDebug(FTPManagerLog) << "_createFileAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
        _uploadComplete(tr("Upload failed for: %1 - error: %2")
                            .arg(_uploadOperation->fullPathOnVehicle)
                            .arg(_errorMsgFromNak(ackOrNak)));
    }
}

void FTPManager::_createFileTimeout(void)
{
    qCDebug(FTPManagerLog) << "_createFileTimeout";
    _uploadComplete(tr("Upload failed for: %1 - no response from vehicle").arg(_uploadOperation->fullPathOnVehicle));
}

void FTPManager::_writeFileBegin(void)
{
    _writeFileWorker(true /* firstRequest */);
}

void FTPManager::_writeFileWorker(bool firstRequest)
{
    if (!_uploadOperation->file.isOpen()) {
        _uploadComplete(tr("Upload failed for: %1 - file not open").arg(_uploadOperation->fullPathOnVehicle));
        return;
    }

    if (_uploadOperation->totalBytesSent >= _uploadOperation->fileSize) {
        _advanceStateMachine();
        return;
    }

    qCDebug(FTPManagerLog) << "_writeFileWorker: offset:firstRequest:retryCount" << _uploadOperation->totalBytesSent
                           << firstRequest << _uploadOperation->retryCount;

    MavlinkFTP::Request request{};
    request.hdr.session = _uploadOperation->sessionId;
    request.hdr.opcode = MAV_FTP_OPCODE_WRITEFILE;
    request.hdr.offset = _uploadOperation->totalBytesSent;

    if (firstRequest) {
        _uploadOperation->retryCount = 0;
    } else {
        _expectedIncomingSeqNumber -= 2;
    }

    qint64 bytesRemaining =
        static_cast<qint64>(_uploadOperation->fileSize) - static_cast<qint64>(_uploadOperation->totalBytesSent);
    qint64 bytesToSend = bytesRemaining;
    if (bytesToSend > static_cast<qint64>(sizeof(request.data))) {
        bytesToSend = sizeof(request.data);
    }

    if (!_uploadOperation->file.seek(_uploadOperation->totalBytesSent)) {
        qCDebug(FTPManagerLog) << "_writeFileWorker: seek failed" << _uploadOperation->file.errorString();
        _uploadComplete(tr("Upload failed for: %1 - error reading file").arg(_uploadOperation->fullPathOnVehicle));
        return;
    }

    qint64 bytesRead = _uploadOperation->file.read(reinterpret_cast<char*>(request.data), bytesToSend);
    if (bytesRead != bytesToSend) {
        qCDebug(FTPManagerLog) << "_writeFileWorker: read failed" << _uploadOperation->file.errorString();
        _uploadComplete(tr("Upload failed for: %1 - error reading file").arg(_uploadOperation->fullPathOnVehicle));
        return;
    }

    request.hdr.size = static_cast<uint8_t>(bytesRead);
    _uploadOperation->lastChunkSize = static_cast<uint32_t>(bytesRead);

    _sendRequestExpectAck(&request);
}

void FTPManager::_writeFileAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_WRITEFILE, "_writeFileAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.session != _uploadOperation->sessionId) {
        qCDebug(FTPManagerLog) << "_writeFileAckOrNak: Disregarding due to incorrect session id actual:expected"
                               << ackOrNak->hdr.session << _uploadOperation->sessionId;
        return;
    }

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        if (ackOrNak->hdr.seqNumber < _expectedIncomingSeqNumber) {
            qCDebug(FTPManagerLog) << "_writeFileAckOrNak: Disregarding Ack due to incorrect sequence actual:expected"
                                   << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
            return;
        }

        _ackOrNakTimeoutTimer.stop();

        if (ackOrNak->hdr.size != 0) {
            qCDebug(FTPManagerLog) << "_writeFileAckOrNak: unexpected ack size expected:actual 0" << ackOrNak->hdr.size;
        }

        _uploadOperation->totalBytesSent += _uploadOperation->lastChunkSize;
        _uploadOperation->lastChunkSize = 0;
        _expectedIncomingSeqNumber = ackOrNak->hdr.seqNumber;

        if (_uploadOperation->fileSize != 0) {
            _emitCommandProgress(static_cast<float>(_uploadOperation->totalBytesSent) /
                                 static_cast<float>(_uploadOperation->fileSize));
        }

        if (_uploadOperation->totalBytesSent >= _uploadOperation->fileSize) {
            _advanceStateMachine();
        } else {
            _writeFileWorker(true /* firstRequest */);
        }
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        _ackOrNakTimeoutTimer.stop();
        qCDebug(FTPManagerLog) << "_writeFileAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
        _uploadComplete(tr("Upload failed for: %1 - error: %2")
                            .arg(_uploadOperation->fullPathOnVehicle)
                            .arg(_errorMsgFromNak(ackOrNak)));
    }
}

void FTPManager::_writeFileTimeout(void)
{
    if (++_uploadOperation->retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_writeFileTimeout retries exceeded");
        _uploadComplete(
            tr("Upload failed for: %1 - no response from vehicle").arg(_uploadOperation->fullPathOnVehicle));
    } else {
        qCDebug(FTPManagerLog) << QString("_writeFileTimeout: retrying - retryCount(%1) offset(%2)")
                                      .arg(_uploadOperation->retryCount)
                                      .arg(_uploadOperation->totalBytesSent);
        _writeFileWorker(false /* firstRequest */);
    }
}

void FTPManager::_terminateUploadSessionBegin(void)
{
    if (_uploadOperation->sessionId == 0) {
        qCWarning(FTPManagerLog) << "_terminateUploadSessionBegin: No session to terminate";
        _advanceStateMachine();
        return;
    }

    MavlinkFTP::Request request{};
    request.hdr.session = _uploadOperation->sessionId;
    request.hdr.opcode = MAV_FTP_OPCODE_TERMINATESESSION;
    _sendRequestExpectAck(&request);
}

void FTPManager::_terminateUploadSessionAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_TERMINATESESSION, "_terminateUploadSessionAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog)
            << "_terminateUploadSessionAckOrNak: Ack disregarding ack for incorrect sequence actual:expected"
            << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        qCDebug(FTPManagerLog) << "_terminateUploadSessionAckOrNak: Ack";
        _advanceStateMachine();
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        qCDebug(FTPManagerLog) << "_terminateUploadSessionAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
        _uploadComplete(tr("Upload failed for: %1 - error: %2")
                            .arg(_uploadOperation->fullPathOnVehicle)
                            .arg(_errorMsgFromNak(ackOrNak)));
    }
}

void FTPManager::_terminateUploadSessionTimeout(void)
{
    if (++_uploadOperation->retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_terminateUploadSessionTimeout retries exceeded");
        _uploadComplete(
            tr("Upload failed for: %1 - no response from vehicle").arg(_uploadOperation->fullPathOnVehicle));
    } else {
        qCDebug(FTPManagerLog)
            << QString("_terminateUploadSessionTimeout: retrying - retryCount(%1)").arg(_uploadOperation->retryCount);
        _terminateUploadSessionBegin();
    }
}

void FTPManager::_uploadFinalize(void)
{
    QString error =
        _uploadOperation->cancelled ? tr("Aborted for: %1").arg(_uploadOperation->fullPathOnVehicle) : QString();
    _uploadComplete(error);
}

void FTPManager::_uploadComplete(const QString& errorMsg)
{
    qCDebug(FTPManagerLog) << QString("_uploadComplete: errorMsg(%1)").arg(errorMsg) << "local"
                           << _uploadOperation->localFilePath << "remote" << _uploadOperation->fullPathOnVehicle;

    QString remotePath = _uploadOperation->fullPathOnVehicle;

    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();
    _currentStateMachineIndex = -1;

    if (_uploadOperation->file.isOpen()) {
        _uploadOperation->file.close();
    }

    _uploadOperation->reset();

    QPointer<FTPUploadJob> job = qobject_cast<FTPUploadJob*>(_activeJob);
    _activeJob = nullptr;
    if (job) {
        job->_finish();
    }
    emit uploadComplete(remotePath, errorMsg);
    if (job) {
        job->_emitFinished(remotePath, errorMsg);
        job->deleteLater();
    }
}

void FTPManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL || message.sysid != _vehicle->id() ||
        message.compid != _ftpCompId) {
        return;
    }

    if (_currentStateMachineIndex == -1) {
        return;
    }

    mavlink_file_transfer_protocol_t data;
    mavlink_msg_file_transfer_protocol_decode(&message, &data);

    // Make sure we are the target system
    int qgcId = MAVLinkProtocol::instance()->getSystemId();
    if (data.target_system != qgcId) {
        return;
    }

    MavlinkFTP::Request* request = (MavlinkFTP::Request*) &data.payload[0];

    // Reject messages where hdr.size exceeds data array bounds to prevent over-reads
    if (request->hdr.size > sizeof(request->data)) {
        qCWarning(FTPManagerLog) << "_mavlinkMessageReceived: hdr.size exceeds data array, discarding."
                                 << request->hdr.size;
        return;
    }

    // Ignore old/reordered packets (handle wrap-around properly)
    uint16_t actualIncomingSeqNumber = request->hdr.seqNumber;
    if ((uint16_t) ((_expectedIncomingSeqNumber - 1) - actualIncomingSeqNumber) <
        (std::numeric_limits<uint16_t>::max() / 2)) {
        qCDebug(FTPManagerLog) << "_mavlinkMessageReceived: Received old packet seqNum expected:actual"
                               << _expectedIncomingSeqNumber << actualIncomingSeqNumber << "hdr.opcode:hdr.req_opcode"
                               << MavlinkFTP::opCodeToString(static_cast<MAV_FTP_OPCODE>(request->hdr.opcode))
                               << MavlinkFTP::opCodeToString(static_cast<MAV_FTP_OPCODE>(request->hdr.req_opcode));

        return;
    }

    qCDebug(FTPManagerLog) << "_mavlinkMessageReceived: hdr.opcode:hdr.req_opcode:seqNumber"
                           << MavlinkFTP::opCodeToString(static_cast<MAV_FTP_OPCODE>(request->hdr.opcode))
                           << MavlinkFTP::opCodeToString(static_cast<MAV_FTP_OPCODE>(request->hdr.req_opcode))
                           << request->hdr.seqNumber;

    (this->*_rgStateMachine[_currentStateMachineIndex].ackNakFn)(request);
}

void FTPManager::_startStateMachine(void)
{
    _currentStateMachineIndex = -1;
    _advanceStateMachine();
}

void FTPManager::_advanceStateMachine(void)
{
    _currentStateMachineIndex++;
    (this->*_rgStateMachine[_currentStateMachineIndex].beginFn)();
}

void FTPManager::_ackOrNakTimeout(void)
{
    (this->*_rgStateMachine[_currentStateMachineIndex].timeoutFn)();
}

QString FTPManager::_errorMsgFromNak(const MavlinkFTP::Request* nak)
{
    const std::optional<MavlinkFTP::NakError> error = MavlinkFTP::decodeNak(*nak);
    if (!error.has_value()) {
        return tr("Invalid Nak format");
    }
    if (error->errorNumber.has_value()) {
        return tr("errno %1").arg(*error->errorNumber);
    }
    return MavlinkFTP::errorCodeToString(error->code);
}

MavlinkFTP::ResponseValidationResult FTPManager::_validateResponseEnvelope(const MavlinkFTP::Request* response,
                                                                           MAV_FTP_OPCODE expectedRequestOpcode,
                                                                           const char* handlerName) const
{
    const MavlinkFTP::ResponseValidation validation = MavlinkFTP::validateResponse(*response, expectedRequestOpcode);
    if (validation.result == MavlinkFTP::ResponseValidationResult::Unrelated) {
        const auto requestOpcode = static_cast<MAV_FTP_OPCODE>(response->hdr.req_opcode);
        qCDebug(FTPManagerLog) << handlerName << "disregarding response for request opcode"
                               << MavlinkFTP::opCodeToString(requestOpcode) << "expected"
                               << MavlinkFTP::opCodeToString(expectedRequestOpcode);
        return validation.result;
    }

    switch (validation.error) {
        case MavlinkFTP::ResponseValidationError::InvalidResponseOpcode:
            qCWarning(FTPManagerLog) << handlerName << "received invalid response opcode"
                                     << MavlinkFTP::opCodeToString(static_cast<MAV_FTP_OPCODE>(response->hdr.opcode));
            break;
        case MavlinkFTP::ResponseValidationError::OversizedPayload:
            qCWarning(FTPManagerLog) << handlerName << "received oversized response payload" << response->hdr.size;
            break;
        case MavlinkFTP::ResponseValidationError::MissingNakErrorCode:
            qCWarning(FTPManagerLog) << handlerName << "received a NAK without an error code";
            break;
        case MavlinkFTP::ResponseValidationError::InvalidNakPayload:
            qCWarning(FTPManagerLog) << handlerName << "received a malformed NAK payload" << response->hdr.size;
            break;
        case MavlinkFTP::ResponseValidationError::None:
            break;
    }
    return validation.result;
}

void FTPManager::_openFileROBegin(void)
{
    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MAV_FTP_OPCODE_OPENFILERO;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    if (!MavlinkFTP::setRequestData(request, _downloadOperation->fullPathOnVehicle)) {
        _downloadComplete(tr("Download failed: remote path is too long"));
        return;
    }
    _sendRequestExpectAck(&request);
}

void FTPManager::_openFileROTimeout(void)
{
    qCDebug(FTPManagerLog) << "_openFileROTimeout";
    _downloadComplete(tr("Download failed"));
}

void FTPManager::_openFileROAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_OPENFILERO, "_openFileROAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_openFileROAckOrNak: Ack disregarding ack for incorrect sequence actual:expected"
                               << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        const std::optional<uint32_t> openFileLength = MavlinkFTP::decodeOpenFileLength(*ackOrNak);
        if (!openFileLength.has_value()) {
            _downloadComplete(tr("Download failed: malformed open-file response"));
            return;
        }
        qCDebug(FTPManagerLog) << "_openFileROAckOrNak: Ack  - sessionId:openFileLength" << ackOrNak->hdr.session
                               << *openFileLength;

        _downloadOperation->sessionId = ackOrNak->hdr.session;
        _downloadOperation->remoteSessionOpen = true;

        _downloadOperation->fileSize = *openFileLength;
        _downloadOperation->expectedOffset = 0;

        const QString finalFilePath = _downloadOperation->toDir.filePath(_downloadOperation->fileName);
        _downloadOperation->file.setFileName(finalFilePath + QStringLiteral(".part"));
        const QIODevice::OpenMode localOpenMode =
            QFile::WriteOnly |
            ((_downloadOperation->existingFilePolicy == ExistingFilePolicy::FailIfExists) ? QFile::NewOnly
                                                                                          : QFile::Truncate);
        if (_downloadOperation->file.open(localOpenMode)) {
            _advanceStateMachine();
        } else {
            qCDebug(FTPManagerLog) << "_openFileROAckOrNak: Ack _downloadOperation->file open failed"
                                   << _downloadOperation->file.errorString();
            _downloadComplete(tr("Download failed"));
        }
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        qCDebug(FTPManagerLog) << "_handlOpenFileROAck: Nak -" << _errorMsgFromNak(ackOrNak);
        _downloadComplete(tr("Download failed") + ": " + _errorMsgFromNak(ackOrNak));
    }
}

void FTPManager::_burstReadFileWorker(bool firstRequest)
{
    qCDebug(FTPManagerLog) << "_burstReadFileWorker: starting burst at offset:firstRequest:retryCount"
                           << _downloadOperation->expectedOffset << firstRequest << _downloadOperation->retryCount;

    MavlinkFTP::Request request{};
    request.hdr.session = _downloadOperation->sessionId;
    request.hdr.opcode = MAV_FTP_OPCODE_BURSTREADFILE;
    request.hdr.offset = _downloadOperation->expectedOffset;
    request.hdr.size = sizeof(request.data);

    if (firstRequest) {
        _downloadOperation->retryCount = 0;
    } else {
        // Must used same sequence number as previous request
        _expectedIncomingSeqNumber -= 2;
    }

    _sendRequestExpectAck(&request);
}

void FTPManager::_burstReadFileBegin(void)
{
    _burstReadFileWorker(true /* firstRequestr */);
}

void FTPManager::_burstReadFileAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_BURSTREADFILE, "_burstReadFileAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.session != _downloadOperation->sessionId) {
        qCDebug(FTPManagerLog) << "_burstReadFileAckOrNak: Disregarding due to incorrect session id actual:expected"
                               << ackOrNak->hdr.session << _downloadOperation->sessionId;
        return;
    }

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        if (ackOrNak->hdr.seqNumber < _expectedIncomingSeqNumber) {
            qCDebug(FTPManagerLog)
                << "_burstReadFileAckOrNak: Disregarding Ack due to incorrect sequence actual:expected"
                << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
            return;
        }

        _ackOrNakTimeoutTimer.stop();

        qCDebug(FTPManagerLog) << QString("_burstReadFileAckOrNak: Ack offset(%1) size(%2) burstComplete(%3)")
                                      .arg(ackOrNak->hdr.offset)
                                      .arg(ackOrNak->hdr.size)
                                      .arg(ackOrNak->hdr.burstComplete);

        if (ackOrNak->hdr.offset != _downloadOperation->expectedOffset) {
            if (ackOrNak->hdr.offset > _downloadOperation->expectedOffset) {
                // There is a hole in our data, record it as missing and continue on
                DownloadOperation::MissingData missingData;
                missingData.offset = _downloadOperation->expectedOffset;
                missingData.bytesMissing = ackOrNak->hdr.offset - _downloadOperation->expectedOffset;
                _downloadOperation->missingData.append(missingData);
                qCDebug(FTPManagerLog) << "_handleBurstReadFileAck: adding missing data offset:bytesMissing"
                                       << missingData.offset << missingData.bytesMissing;
            } else {
                // Offset is past what we have already seen, disregard and wait for something usefule
                _ackOrNakTimeoutTimer.start();
                qCDebug(FTPManagerLog)
                    << "_handleBurstReadFileAck: received offset less than expected offset received:expected"
                    << ackOrNak->hdr.offset << _downloadOperation->expectedOffset;
                return;
            }
        }

        _downloadOperation->file.seek(ackOrNak->hdr.offset);
        int bytesWritten = _downloadOperation->file.write((const char*) ackOrNak->data, ackOrNak->hdr.size);
        if (bytesWritten != ackOrNak->hdr.size) {
            _downloadComplete(tr("Download failed: Error saving file"));
            return;
        }
        _downloadOperation->bytesWritten += ackOrNak->hdr.size;
        _downloadOperation->expectedOffset = ackOrNak->hdr.offset + ackOrNak->hdr.size;

        if (ackOrNak->hdr.burstComplete) {
            // The current burst is done, request next one in offset sequence
            _expectedIncomingSeqNumber = ackOrNak->hdr.seqNumber;
            _burstReadFileWorker(true /* firstRequest */);
        } else {
            // Still within a burst, next ack should come automatically
            _expectedIncomingSeqNumber = ackOrNak->hdr.seqNumber + 1;
            _ackOrNakTimeoutTimer.start();
        }

        // Emit progress last, as cancel could be called in there
        if (_downloadOperation->fileSize != 0) {
            _emitCommandProgress(static_cast<float>(_downloadOperation->bytesWritten) /
                                 static_cast<float>(_downloadOperation->fileSize));
        }
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        _ackOrNakTimeoutTimer.stop();
        MAV_FTP_ERR errorCode = static_cast<MAV_FTP_ERR>(ackOrNak->data[0]);

        if (errorCode == MAV_FTP_ERR_EOF) {
            // Burst sequence has gone through the whole file
            if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
                qCDebug(FTPManagerLog) << "_burstReadFileAckOrNak: EOF Nak"
                                          "with incorrect sequence nr actual:expected"
                                       << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
                /* We have received the EOF Nak but out of sequence, i.e. data is missing */
                _expectedIncomingSeqNumber = ackOrNak->hdr.seqNumber;
                _burstReadFileWorker(true); /* Retry from last expected offset */
            } else {
                qCDebug(FTPManagerLog) << "_burstReadFileAckOrNak EOF";
                _advanceStateMachine();
            }
        } else { /* Don't care is this is out of sequence */
            qCDebug(FTPManagerLog) << "_burstReadFileAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
            _downloadComplete(tr("Download failed"));
        }
    }
}

void FTPManager::_burstReadFileTimeout(void)
{
    if (++_downloadOperation->retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_burstReadFileTimeout retries exceeded");
        _downloadComplete(tr("Download failed"));
    } else {
        // Try again
        qCDebug(FTPManagerLog) << QString("_burstReadFileTimeout: retrying - retryCount(%1) offset(%2)")
                                      .arg(_downloadOperation->retryCount)
                                      .arg(_downloadOperation->expectedOffset);
        _burstReadFileWorker(false /* firstReqeust */);
    }
}

void FTPManager::_listDirectoryWorker(bool firstRequest)
{
    qCDebug(FTPManagerLog) << "_listDirectoryWorker: offset:firstRequest:retryCount"
                           << _listDirectoryOperation->expectedOffset << firstRequest
                           << _listDirectoryOperation->retryCount;

    MavlinkFTP::Request request{};
    request.hdr.session = _listDirectoryOperation->sessionId;
    request.hdr.opcode = _listDirectoryOperation->opCode;
    request.hdr.offset = _listDirectoryOperation->expectedOffset;
    request.hdr.size = sizeof(request.data);
    if (!MavlinkFTP::setRequestData(request, _listDirectoryOperation->fullPathOnVehicle)) {
        _listDirectoryComplete(tr("List directory failed: remote path is too long"));
        return;
    }

    if (firstRequest) {
        _listDirectoryOperation->retryCount = 0;
    } else {
        // Must used same sequence number as previous request
        _expectedIncomingSeqNumber -= 2;
    }

    _sendRequestExpectAck(&request);
}

void FTPManager::_listDirectoryBegin(void)
{
    _listDirectoryWorker(true /* firstRequest */);
}

void FTPManager::_listDirectoryAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    const MavlinkFTP::ResponseValidationResult validation =
        _validateResponseEnvelope(ackOrNak, _listDirectoryOperation->opCode, "_listDirectoryAckOrNak:");
    if (validation == MavlinkFTP::ResponseValidationResult::Unrelated) {
        return;
    }
    if (validation == MavlinkFTP::ResponseValidationResult::Malformed) {
        _listDirectoryComplete(tr("List directory failed: malformed response"));
        return;
    }

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
            qCDebug(FTPManagerLog)
                << "_listDirectoryAckOrNak: Disregarding Ack due to incorrect sequence actual:expected"
                << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
            return;
        }

        _ackOrNakTimeoutTimer.stop();

        if (_listDirectoryOperation->opCode == MAV_FTP_OPCODE_LISTDIRECTORYWITHTIME) {
            _listDirWithTimeSupport = WithTimeSupport_t::Supported;
        }

        qCDebug(FTPManagerLog) << QString("_listDirectoryAckOrNak: Ack size(%1)").arg(ackOrNak->hdr.size);

        const MavlinkFTP::DirectoryPayloadParseResult payload = MavlinkFTP::parseDirectoryPayload(*ackOrNak);
        if ((payload.error == MavlinkFTP::DirectoryPayloadParseError::Empty) ||
            (payload.error == MavlinkFTP::DirectoryPayloadParseError::Oversized)) {
            qCWarning(FTPManagerLog) << "Invalid directory-list response size" << ackOrNak->hdr.size;
            _listDirectoryComplete(tr("List directory failed: malformed response"));
            return;
        }
        if (payload.error == MavlinkFTP::DirectoryPayloadParseError::EmptyEntry) {
            qCWarning(FTPManagerLog) << "Directory-list response contains an empty entry";
            _listDirectoryComplete(tr("List directory failed: malformed response"));
            return;
        }
        if (payload.error == MavlinkFTP::DirectoryPayloadParseError::UnterminatedEntry) {
            qCWarning(FTPManagerLog) << "Directory-list response contains an unterminated entry";
            _listDirectoryComplete(tr("List directory failed: malformed response"));
            return;
        }
        if (payload.error == MavlinkFTP::DirectoryPayloadParseError::InvalidUtf8) {
            qCWarning(FTPManagerLog) << "Directory-list response contains invalid UTF-8";
            _listDirectoryComplete(tr("List directory failed: malformed response"));
            return;
        }

        for (const QString& directoryEntry : payload.records) {
            if ((_listDirectoryOperation->maxEntries > 0) &&
                (_listDirectoryOperation->directoryEntries.size() >= _listDirectoryOperation->maxEntries)) {
                _listDirectoryOperation->truncated = true;
                break;
            }

            _listDirectoryOperation->directoryEntries.append(directoryEntry);
            _listDirectoryOperation->expectedOffset++;
        }

        _expectedIncomingSeqNumber = ackOrNak->hdr.seqNumber;
        if (_listDirectoryOperation->truncated) {
            _advanceStateMachine();
        } else {
            // Request next set of directory entries
            _listDirectoryWorker(true /* firstRequest */);
        }
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        _ackOrNakTimeoutTimer.stop();
        MAV_FTP_ERR errorCode = static_cast<MAV_FTP_ERR>(ackOrNak->data[0]);

        if (errorCode == MAV_FTP_ERR_EOF) {
            // All entries returned
            if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
                qCDebug(FTPManagerLog)
                    << "_listDirectoryAckOrNak: Disregarding Nak due to incorrect sequence actual:expected"
                    << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
                _ackOrNakTimeoutTimer.start();
                return;
            } else {
                qCDebug(FTPManagerLog) << "_listDirectoryAckOrNak EOF";
                if (_listDirectoryOperation->opCode == MAV_FTP_OPCODE_LISTDIRECTORYWITHTIME) {
                    _listDirWithTimeSupport = WithTimeSupport_t::Supported;
                }
                _advanceStateMachine();
            }
        } else if (errorCode == MAV_FTP_ERR_UNKNOWNCOMMAND &&
                   _listDirectoryOperation->opCode == MAV_FTP_OPCODE_LISTDIRECTORYWITHTIME) {
            // The server does not implement timestamped listings. Remember that and fall back to the
            // plain listing and restart from the beginning. The UnknownCommand Nak is a definitive
            // capability statement so we act on it without a strict sequence check; the restart is
            // idempotent (offset and accumulated entries are reset).
            qCDebug(FTPManagerLog)
                << "_listDirectoryAckOrNak: timestamped listing unsupported, falling back to plain listing";
            _listDirWithTimeSupport = WithTimeSupport_t::Unsupported;
            _listDirectoryOperation->opCode = MAV_FTP_OPCODE_LISTDIRECTORY;
            _listDirectoryOperation->expectedOffset = 0;
            _listDirectoryOperation->directoryEntries.clear();
            _expectedIncomingSeqNumber = ackOrNak->hdr.seqNumber;
            _listDirectoryWorker(true /* firstRequest */);
        } else { /* Don't care is this is out of sequence */
            qCDebug(FTPManagerLog) << "_listDirectoryAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
            _listDirectoryComplete(tr("List directory failed"));
        }
    }
}

void FTPManager::_listDirectoryTimeout(void)
{
    if (++_listDirectoryOperation->retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_listDirectoryTimeout retries exceeded");
        _listDirectoryComplete(tr("List directory failed"));
    } else {
        // Try again
        qCDebug(FTPManagerLog) << QString("_listDirectoryTimeout: retrying - retryCount(%1) offset(%2)")
                                      .arg(_listDirectoryOperation->retryCount)
                                      .arg(_listDirectoryOperation->expectedOffset);
        _listDirectoryWorker(false /* firstReqeust */);
    }
}

void FTPManager::_fillMissingBlocksWorker(bool firstRequest)
{
    if (_downloadOperation->missingData.count()) {
        MavlinkFTP::Request request{};
        DownloadOperation::MissingData& missingData = _downloadOperation->missingData.first();

        uint32_t cBytesToRead = qMin((uint32_t) sizeof(request.data), missingData.bytesMissing);

        qCDebug(FTPManagerLog) << "_fillMissingBlocksBegin: offset:cBytesToRead" << missingData.offset << cBytesToRead;

        request.hdr.session = _downloadOperation->sessionId;
        request.hdr.opcode = MAV_FTP_OPCODE_READFILE;
        request.hdr.offset = missingData.offset;
        request.hdr.size = cBytesToRead;

        if (firstRequest) {
            _downloadOperation->retryCount = 0;
        } else {
            // Must used same sequence number as previous request
            _expectedIncomingSeqNumber -= 2;
        }
        _downloadOperation->expectedOffset = request.hdr.offset;

        _sendRequestExpectAck(&request);
    } else {
        // We should have the full file now
        if (_downloadOperation->checkSize == false ||
            _downloadOperation->bytesWritten == _downloadOperation->fileSize) {
            _advanceStateMachine();
        } else {
            qCDebug(FTPManagerLog)
                << "_fillMissingBlocksWorker: no missing blocks but file still incomplete - bytesWritten:fileSize"
                << _downloadOperation->bytesWritten << _downloadOperation->fileSize;
            _downloadComplete(tr("Download failed"));
        }
    }
}

void FTPManager::_fillMissingBlocksBegin(void)
{
    _fillMissingBlocksWorker(true /* firstRequest */);
}

void FTPManager::_fillMissingBlocksAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_READFILE, "_fillMissingBlocksAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Disregarding due to incorrect sequence actual:expected"
                               << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }
    if (ackOrNak->hdr.session != _downloadOperation->sessionId) {
        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Disregarding due to incorrect session id actual:expected"
                               << ackOrNak->hdr.session << _downloadOperation->sessionId;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Ack offset:size" << ackOrNak->hdr.offset
                               << ackOrNak->hdr.size;

        if (ackOrNak->hdr.offset != _downloadOperation->expectedOffset) {
            if (++_downloadOperation->retryCount > _maxRetry) {
                qCDebug(FTPManagerLog) << QString("_fillMissingBlocksAckOrNak: offset mismatch, retries exceeded");
                _downloadComplete(tr("Download failed"));
                return;
            }

            // Ask for current offset again
            qCDebug(FTPManagerLog)
                << QString("_fillMissingBlocksAckOrNak: Ack offset mismatch retry, retryCount(%1) offset(%2)")
                       .arg(_downloadOperation->retryCount)
                       .arg(_downloadOperation->expectedOffset);
            _fillMissingBlocksWorker(false /* firstReqeust */);
            return;
        }

        _downloadOperation->file.seek(ackOrNak->hdr.offset);
        int bytesWritten = _downloadOperation->file.write((const char*) ackOrNak->data, ackOrNak->hdr.size);
        if (bytesWritten != ackOrNak->hdr.size) {
            _downloadComplete(tr("Download failed: Error saving file"));
            return;
        }
        _downloadOperation->bytesWritten += ackOrNak->hdr.size;

        DownloadOperation::MissingData& missingData = _downloadOperation->missingData.first();
        missingData.offset += ackOrNak->hdr.size;
        missingData.bytesMissing -= ackOrNak->hdr.size;
        if (missingData.bytesMissing == 0) {
            // This block is finished, remove it
            _downloadOperation->missingData.takeFirst();
        }

        // Move on to fill in possible next hole
        _fillMissingBlocksWorker(true /* firstReqeust */);

        // Emit progress last, as cancel could be called in there
        if (_downloadOperation->fileSize != 0) {
            _emitCommandProgress(static_cast<float>(_downloadOperation->bytesWritten) /
                                 static_cast<float>(_downloadOperation->fileSize));
        }
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        MAV_FTP_ERR errorCode = static_cast<MAV_FTP_ERR>(ackOrNak->data[0]);

        if (errorCode == MAV_FTP_ERR_EOF) {
            qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak EOF";
            if (_downloadOperation->checkSize == false ||
                _downloadOperation->bytesWritten == _downloadOperation->fileSize) {
                // We've successfully complete filling in all missing blocks
                _advanceStateMachine();
                return;
            }
        }

        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
        _downloadComplete(tr("Download failed"));
    }
}

void FTPManager::_fillMissingBlocksTimeout(void)
{
    if (++_downloadOperation->retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_fillMissingBlocksTimeout retries exceeded");
        _downloadComplete(tr("Download failed"));
    } else {
        // Ask for current offset again
        qCDebug(FTPManagerLog) << QString("_fillMissingBlocksTimeout: retrying - retryCount(%1) offset(%2)")
                                      .arg(_downloadOperation->retryCount)
                                      .arg(_downloadOperation->expectedOffset);
        _fillMissingBlocksWorker(false /* firstReqeust */);
    }
}

void FTPManager::_resetSessionsBegin(void)
{
    MavlinkFTP::Request request{};
    request.hdr.opcode = MAV_FTP_OPCODE_RESETSESSION;
    request.hdr.size = 0;
    _sendRequestExpectAck(&request);
}

void FTPManager::_resetSessionsAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    if (_validateResponseEnvelope(ackOrNak, MAV_FTP_OPCODE_RESETSESSION, "_resetSessionsAckOrNak:") !=
        MavlinkFTP::ResponseValidationResult::Valid) {
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_resetSessionsAckOrNak: Disregarding due to incorrect sequence actual:expected"
                               << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }

    _ackOrNakTimeoutTimer.stop();
    _downloadOperation->remoteSessionOpen = false;

    if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_ACK) {
        qCDebug(FTPManagerLog) << "_resetSessionsAckOrNak: Ack";
        _advanceStateMachine();
    } else if (ackOrNak->hdr.opcode == MAV_FTP_OPCODE_NAK) {
        qCDebug(FTPManagerLog) << "_resetSessionsAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
        if (_uploadOperation->inProgress()) {
            _uploadComplete(tr("Upload failed for: %1 - unable to reset remote sessions: %2")
                                .arg(_uploadOperation->fullPathOnVehicle, _errorMsgFromNak(ackOrNak)));
        } else {
            _downloadComplete(
                QString(),
                tr("Downloaded file, but unable to reset remote sessions: %1").arg(_errorMsgFromNak(ackOrNak)));
        }
    }
}

void FTPManager::_resetSessionsTimeout(void)
{
    qCDebug(FTPManagerLog) << "_resetSessionsTimeout";
    _downloadOperation->remoteSessionOpen = false;
    if (!_uploadOperation->inProgress()) {
        _downloadComplete(QString(), tr("Downloaded file, but no response while resetting remote sessions"));
        return;
    }

    if (++_resetSessionsRetryCount > _maxRetry) {
        _uploadComplete(tr("Upload failed for: %1 - no response while resetting remote sessions")
                            .arg(_uploadOperation->fullPathOnVehicle));
    } else {
        qCDebug(FTPManagerLog) << "_resetSessionsTimeout: retrying" << _resetSessionsRetryCount;
        _resetSessionsBegin();
    }
}

void FTPManager::_sendRequestExpectAck(MavlinkFTP::Request* request)
{
    _ackOrNakTimeoutTimer.start();

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        request->hdr.seqNumber = _expectedIncomingSeqNumber + 1;  // Outgoing is 1 past last incoming
        _expectedIncomingSeqNumber += 2;

        qCDebug(FTPManagerLog) << "_sendRequestExpectAck opcode:"
                               << MavlinkFTP::opCodeToString(static_cast<MAV_FTP_OPCODE>(request->hdr.opcode))
                               << "seqNumber:" << request->hdr.seqNumber;

        mavlink_message_t message;
        mavlink_msg_file_transfer_protocol_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                     MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(),
                                                     &message,
                                                     0,                    // Target network, 0=broadcast?
                                                     _vehicle->id(), _ftpCompId,
                                                     (uint8_t*) request);  // Payload
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    } else {
        qCDebug(FTPManagerLog) << "_sendRequestExpectAck No primary link. Allowing timeout to fail sequence.";
    }
}
