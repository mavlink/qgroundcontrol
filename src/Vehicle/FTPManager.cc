/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FTPManager.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QDir>

QGC_LOGGING_CATEGORY(FTPManagerLog, "FTPManagerLog")

FTPManager::FTPManager(Vehicle* vehicle)
    : QObject   (vehicle)
    , _vehicle  (vehicle)
{
    _ackOrNakTimeoutTimer.setSingleShot(true);
    // Mock link responds immediately if at all, speed up unit tests with faster timoue
    _ackOrNakTimeoutTimer.setInterval(qgcApp()->runningUnitTests() ? 10 : _ackOrNakTimeoutMsecs);
    connect(&_ackOrNakTimeoutTimer, &QTimer::timeout, this, &FTPManager::_ackOrNakTimeout);
    
    // Make sure we don't have bad structure packing
    Q_ASSERT(sizeof(MavlinkFTP::RequestHeader) == 12);
}

bool FTPManager::download(uint8_t fromCompId, const QString& fromURI, const QString& toDir, const QString& fileName, bool checksize)
{
    qCDebug(FTPManagerLog) << "download fromURI:" << fromURI << "to:" << toDir << "fromCompId:" << fromCompId;

    if (!_rgStateMachine.isEmpty()) {
        qCDebug(FTPManagerLog) << "Cannot download. Already in another operation";
        return false;
    }

    static const StateFunctions_t rgDownloadStateMachine[] = {
        { &FTPManager::_openFileROBegin,            &FTPManager::_openFileROAckOrNak,           &FTPManager::_openFileROTimeout },
        { &FTPManager::_burstReadFileBegin,         &FTPManager::_burstReadFileAckOrNak,        &FTPManager::_burstReadFileTimeout },
        { &FTPManager::_fillMissingBlocksBegin,     &FTPManager::_fillMissingBlocksAckOrNak,    &FTPManager::_fillMissingBlocksTimeout },
        { &FTPManager::_resetSessionsBegin,         &FTPManager::_resetSessionsAckOrNak,        &FTPManager::_resetSessionsTimeout },
        { &FTPManager::_downloadCompleteNoError,    nullptr,                                    nullptr },
    };
    for (size_t i=0; i<sizeof(rgDownloadStateMachine)/sizeof(rgDownloadStateMachine[0]); i++) {
        _rgStateMachine.append(rgDownloadStateMachine[i]);
    }

    _downloadState.reset();
    _downloadState.toDir.setPath(toDir);
    _downloadState.checksize = checksize;

    if (!_parseURI(fromCompId, fromURI, _downloadState.fullPathOnVehicle, _ftpCompId)) {
        qCWarning(FTPManagerLog) << "_parseURI failed";
        return false;
    }

    // We need to strip off the file name from the fully qualified path. We can't use the usual QDir
    // routines because this path does not exist locally.
    int lastDirSlashIndex;
    for (lastDirSlashIndex=_downloadState.fullPathOnVehicle.size()-1; lastDirSlashIndex>=0; lastDirSlashIndex--) {
        if (_downloadState.fullPathOnVehicle[lastDirSlashIndex] == '/') {
            break;
        }
    }
    lastDirSlashIndex++; // move past slash

    if (fileName.isEmpty()) {
        _downloadState.fileName = _downloadState.fullPathOnVehicle.right(_downloadState.fullPathOnVehicle.size() - lastDirSlashIndex);
    } else {
        _downloadState.fileName = fileName;
    }

    qCDebug(FTPManagerLog) << "_downloadState.fullPathOnVehicle:_downloadState.fileName" << _downloadState.fullPathOnVehicle << _downloadState.fileName;

    _startStateMachine();

    return true;
}

bool FTPManager::listDirectory(uint8_t fromCompId, const QString& fromURI)
{
    qCDebug(FTPManagerLog) << "list directory fromURI:" << fromURI << "fromCompId:" << fromCompId;

    if (!_rgStateMachine.isEmpty()) {
        qCDebug(FTPManagerLog) << "Cannot list directory. Already in another operation";
        return false;
    }

    static const StateFunctions_t rgStateMachine[] = {
        { &FTPManager::_listDirectoryBegin,             &FTPManager::_listDirectoryAckOrNak,        &FTPManager::_listDirectoryTimeout },
        { &FTPManager::_listDirectoryCompleteNoError,   nullptr,                                    nullptr },
    };
    for (size_t i=0; i<sizeof(rgStateMachine)/sizeof(rgStateMachine[0]); i++) {
        _rgStateMachine.append(rgStateMachine[i]);
    }

    _listDirectoryState.reset();

    if (!_parseURI(fromCompId, fromURI, _listDirectoryState.fullPathOnVehicle, _ftpCompId)) {
        qCWarning(FTPManagerLog) << "_parseURI failed";
        return false;
    }

    qCDebug(FTPManagerLog) << "_listDirectoryState.fullPathOnVehicle" << _listDirectoryState.fullPathOnVehicle;

    _startStateMachine();

    return true;
}

void FTPManager::cancelDownload()
{
    if (!_downloadState.inProgress()) {
        return;
    }

    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();
    static const StateFunctions_t rgTerminateStateMachine[] = {
        { &FTPManager::_terminateSessionBegin,  &FTPManager::_terminateSessionAckOrNak,     &FTPManager::_terminateSessionTimeout },
        { &FTPManager::_terminateComplete,      nullptr,                                    nullptr },
    };
    for (size_t i=0; i<sizeof(rgTerminateStateMachine)/sizeof(rgTerminateStateMachine[0]); i++) {
        _rgStateMachine.append(rgTerminateStateMachine[i]);
    }
    _downloadState.retryCount = 0;
    _startStateMachine();
}

void FTPManager::_terminateSessionBegin(void)
{
    MavlinkFTP::Request request{};
    request.hdr.session = _downloadState.sessionId;
    request.hdr.opcode  = MavlinkFTP::kCmdTerminateSession;
    _sendRequestExpectAck(&request);
}

void FTPManager::_terminateSessionAckOrNak(const MavlinkFTP::Request *ackOrNak)
{
    MavlinkFTP::OpCode_t requestOpCode = static_cast<MavlinkFTP::OpCode_t>(ackOrNak->hdr.req_opcode);
    if (requestOpCode != MavlinkFTP::kCmdTerminateSession) {
        qCDebug(FTPManagerLog) << "_terminateSessionAckOrNak: Ack disregarding ack for incorrect requestOpCode" << MavlinkFTP::opCodeToString(requestOpCode);
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_terminateSessionAckOrNak: Ack disregarding ack for incorrect sequence actual:expected" << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }

    _ackOrNakTimeoutTimer.stop();
    _advanceStateMachine();
}

void FTPManager::_terminateSessionTimeout(void)
{
    if (++_downloadState.retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_terminateSessionTimeout retries exceeded");
        _downloadComplete(tr("Download failed"));
    } else {
        // Try again
        qCDebug(FTPManagerLog) << QString("_terminateSessionTimeout: retrying - retryCount(%1)").arg(_downloadState.retryCount);
        _terminateSessionBegin();
    }

}

void FTPManager::_terminateComplete(void)
{
    _downloadComplete("Aborted");
}

/// Closes out a download session by writing the file and doing cleanup.
///     @param errorMsg Error message, empty if no error
void FTPManager::_downloadComplete(const QString& errorMsg)
{
    qCDebug(FTPManagerLog) << QString("_downloadComplete: errorMsg(%1)").arg(errorMsg);
    
    QString downloadFilePath    = _downloadState.toDir.absoluteFilePath(_downloadState.fileName);
    QString error               = errorMsg;

    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();
    _currentStateMachineIndex = -1;
    if (_downloadState.file.isOpen()) {
        _downloadState.file.close();
        if (!errorMsg.isEmpty()) {
            _downloadState.file.remove();
        }
    }

    emit downloadComplete(downloadFilePath, errorMsg);
}

/// Closes out a list directory sequence
///     @param errorMsg Error message, empty if no error
void FTPManager::_listDirectoryComplete(const QString& errorMsg)
{
    qCDebug(FTPManagerLog) << QString("_listDirectoryComplete: errorMsg(%1)").arg(errorMsg);
    
    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();
    _currentStateMachineIndex = -1;

    QStringList rgDirectoryList = _listDirectoryState.rgDirectoryList;
    if (!errorMsg.isEmpty()) {
        rgDirectoryList.clear();
    }

    emit listDirectoryComplete(rgDirectoryList, errorMsg);
}

void FTPManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL ||
            message.sysid != _vehicle->id() || message.compid != _ftpCompId) {
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
    
    MavlinkFTP::Request* request = (MavlinkFTP::Request*)&data.payload[0];

    // Ignore old/reordered packets (handle wrap-around properly)
    uint16_t actualIncomingSeqNumber = request->hdr.seqNumber;
    if ((uint16_t)((_expectedIncomingSeqNumber - 1) - actualIncomingSeqNumber) < (std::numeric_limits<uint16_t>::max()/2)) {
        qCDebug(FTPManagerLog) << "_mavlinkMessageReceived: Received old packet seqNum expected:actual" << _expectedIncomingSeqNumber << actualIncomingSeqNumber
                               << "hdr.opcode:hdr.req_opcode" << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.opcode)) <<  MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.req_opcode));

        return;
    }

    qCDebug(FTPManagerLog) << "_mavlinkMessageReceived: hdr.opcode:hdr.req_opcode:seqNumber"
                           << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.opcode)) <<  MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.req_opcode))
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

void FTPManager::_fillRequestDataWithString(MavlinkFTP::Request* request, const QString& str)
{
    strncpy((char *)&request->data[0], str.toStdString().c_str(), sizeof(request->data));
    request->hdr.size = static_cast<uint8_t>(strnlen((const char *)&request->data[0], sizeof(request->data)));
}

QString FTPManager::_errorMsgFromNak(const MavlinkFTP::Request* nak)
{
    QString errorMsg;
    MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(nak->data[0]);

    // Nak's normally have 1 byte of data for error code, except for MavlinkFTP::kErrFailErrno which has additional byte for errno
    if ((errorCode == MavlinkFTP::kErrFailErrno && nak->hdr.size != 2) || ((errorCode != MavlinkFTP::kErrFailErrno) && nak->hdr.size != 1)) {
        errorMsg = tr("Invalid Nak format");
    } else if (errorCode == MavlinkFTP::kErrFailErrno) {
        errorMsg = tr("errno %1").arg(nak->data[1]);
    } else {
        errorMsg = MavlinkFTP::errorCodeToString(errorCode);
    }

    return errorMsg;
}

void FTPManager::_openFileROBegin(void)
{
    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode  = MavlinkFTP::kCmdOpenFileRO;
    request.hdr.offset  = 0;
    request.hdr.size    = 0;
    _fillRequestDataWithString(&request, _downloadState.fullPathOnVehicle);
    _sendRequestExpectAck(&request);
}

void FTPManager::_openFileROTimeout(void)
{
    qCDebug(FTPManagerLog) << "_openFileROTimeout";
    _downloadComplete(tr("Download failed"));
}

void FTPManager::_openFileROAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    MavlinkFTP::OpCode_t requestOpCode = static_cast<MavlinkFTP::OpCode_t>(ackOrNak->hdr.req_opcode);
    if (requestOpCode != MavlinkFTP::kCmdOpenFileRO) {
        qCDebug(FTPManagerLog) << "_openFileROAckOrNak: Ack disregarding ack for incorrect requestOpCode" << MavlinkFTP::opCodeToString(requestOpCode);
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_openFileROAckOrNak: Ack disregarding ack for incorrect sequence actual:expected" << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MavlinkFTP::kRspAck) {
        qCDebug(FTPManagerLog) << "_openFileROAckOrNak: Ack  - sessionId:openFileLength" << ackOrNak->hdr.session << ackOrNak->openFileLength;

        if (ackOrNak->hdr.size != sizeof(uint32_t)) {
            qCDebug(FTPManagerLog) << "_openFileROAckOrNak: Ack ack->hdr.size != sizeof(uint32_t)" << ackOrNak->hdr.size << sizeof(uint32_t);
            _downloadComplete(tr("Download failed"));
            return;
        }

        _downloadState.sessionId        = ackOrNak->hdr.session;
        _downloadState.fileSize         = ackOrNak->openFileLength;
        _downloadState.expectedOffset   = 0;

        _downloadState.file.setFileName(_downloadState.toDir.filePath(_downloadState.fileName));
        if (_downloadState.file.open(QFile::WriteOnly | QFile::Truncate)) {
            _advanceStateMachine();
        } else {
            qCDebug(FTPManagerLog) << "_openFileROAckOrNak: Ack _downloadState.file open failed" << _downloadState.file.errorString();
            _downloadComplete(tr("Download failed"));
        }
    } else if (ackOrNak->hdr.opcode == MavlinkFTP::kRspNak) {
        qCDebug(FTPManagerLog) << "_handlOpenFileROAck: Nak -" << _errorMsgFromNak(ackOrNak);
        _downloadComplete(tr("Download failed") + ": " + _errorMsgFromNak(ackOrNak));
    }
}

void FTPManager::_burstReadFileWorker(bool firstRequest)
{
    qCDebug(FTPManagerLog) << "_burstReadFileWorker: starting burst at offset:firstRequest:retryCount" << _downloadState.expectedOffset << firstRequest << _downloadState.retryCount;

    MavlinkFTP::Request request{};
    request.hdr.session = _downloadState.sessionId;
    request.hdr.opcode  = MavlinkFTP::kCmdBurstReadFile;
    request.hdr.offset  = _downloadState.expectedOffset;
    request.hdr.size    = sizeof(request.data);

    if (firstRequest) {
        _downloadState.retryCount = 0;
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
    MavlinkFTP::OpCode_t requestOpCode = static_cast<MavlinkFTP::OpCode_t>(ackOrNak->hdr.req_opcode);

    if (requestOpCode != MavlinkFTP::kCmdBurstReadFile) {
        qCDebug(FTPManagerLog) << "_burstReadFileAckOrNak: Disregarding due to incorrect requestOpCode" << MavlinkFTP::opCodeToString(requestOpCode);
        return;
    }
    if (ackOrNak->hdr.session != _downloadState.sessionId) {
        qCDebug(FTPManagerLog) << "_burstReadFileAckOrNak: Disregarding due to incorrect session id actual:expected" << ackOrNak->hdr.session << _downloadState.sessionId;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MavlinkFTP::kRspAck) {
        if (ackOrNak->hdr.seqNumber < _expectedIncomingSeqNumber) {
            qCDebug(FTPManagerLog) << "_burstReadFileAckOrNak: Disregarding Ack due to incorrect sequence actual:expected" << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
            return;
        }

        qCDebug(FTPManagerLog) << QString("_burstReadFileAckOrNak: Ack offset(%1) size(%2) burstComplete(%3)").arg(ackOrNak->hdr.offset).arg(ackOrNak->hdr.size).arg(ackOrNak->hdr.burstComplete);

        if (ackOrNak->hdr.offset != _downloadState.expectedOffset) {
            if (ackOrNak->hdr.offset > _downloadState.expectedOffset) {
                // There is a hole in our data, record it as missing and continue on
                MissingData_t missingData;
                missingData.offset          = _downloadState.expectedOffset;
                missingData.cBytesMissing   = ackOrNak->hdr.offset - _downloadState.expectedOffset;
                _downloadState.rgMissingData.append(missingData);
                qCDebug(FTPManagerLog) << "_handleBurstReadFileAck: adding missing data offset:cBytesMissing" << missingData.offset << missingData.cBytesMissing;
            } else {
                // Offset is past what we have already seen, disregard and wait for something usefule
                _ackOrNakTimeoutTimer.start();
                qCDebug(FTPManagerLog) << "_handleBurstReadFileAck: received offset less than expected offset received:expected" << ackOrNak->hdr.offset << _downloadState.expectedOffset;
                return;
            }
        }

        _downloadState.file.seek(ackOrNak->hdr.offset);
        int bytesWritten = _downloadState.file.write((const char*)ackOrNak->data, ackOrNak->hdr.size);
        if (bytesWritten != ackOrNak->hdr.size) {
            _downloadComplete(tr("Download failed: Error saving file"));
            return;
        }
        _downloadState.bytesWritten += ackOrNak->hdr.size;
        _downloadState.expectedOffset = ackOrNak->hdr.offset + ackOrNak->hdr.size;

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
        if (_downloadState.fileSize != 0) {
            emit commandProgress((float)(_downloadState.bytesWritten) / (float)_downloadState.fileSize);
        }
    } else if (ackOrNak->hdr.opcode == MavlinkFTP::kRspNak) {
        MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(ackOrNak->data[0]);

        if (errorCode == MavlinkFTP::kErrEOF) {
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
    if (++_downloadState.retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_burstReadFileTimeout retries exceeded");
        _downloadComplete(tr("Download failed"));
    } else {
        // Try again
        qCDebug(FTPManagerLog) << QString("_burstReadFileTimeout: retrying - retryCount(%1) offset(%2)").arg(_downloadState.retryCount).arg(_downloadState.expectedOffset);
        _burstReadFileWorker(false /* firstReqeust */);
    }
}

void FTPManager::_listDirectoryWorker(bool firstRequest)
{
    qCDebug(FTPManagerLog) << "_listDirectoryWorker: offset:firstRequest:retryCount" << _listDirectoryState.expectedOffset << firstRequest << _listDirectoryState.retryCount;

    MavlinkFTP::Request request{};
    request.hdr.session = _downloadState.sessionId;
    request.hdr.opcode  = MavlinkFTP::kCmdListDirectory;
    request.hdr.offset  = _listDirectoryState.expectedOffset;
    request.hdr.size    = sizeof(request.data);
    _fillRequestDataWithString(&request, _listDirectoryState.fullPathOnVehicle);
    
    if (firstRequest) {
        _listDirectoryState.retryCount = 0;
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
    MavlinkFTP::OpCode_t requestOpCode = static_cast<MavlinkFTP::OpCode_t>(ackOrNak->hdr.req_opcode);

    if (requestOpCode != MavlinkFTP::kCmdListDirectory) {
        qCDebug(FTPManagerLog) << "_listDirectoryAckOrNak: Disregarding due to incorrect requestOpCode" << MavlinkFTP::opCodeToString(requestOpCode);
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MavlinkFTP::kRspAck) {
        if (ackOrNak->hdr.seqNumber < _expectedIncomingSeqNumber) {
            qCDebug(FTPManagerLog) << "_listDirectoryAckOrNak: Disregarding Ack due to incorrect sequence actual:expected" << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
            return;
        }

        qCDebug(FTPManagerLog) << QString("_listDirectoryAckOrNak: Ack size(%1)").arg(ackOrNak->hdr.size);

        // Parse entries in ackOrNak->data into _listDirectoryState.rgDirectoryList
        const char* curDataPtr = (const char*)ackOrNak->data;
        while (curDataPtr < (const char*)ackOrNak->data + ackOrNak->hdr.size) {
            QString dirEntry = curDataPtr;
            curDataPtr += dirEntry.size() + 1;
            _listDirectoryState.rgDirectoryList.append(dirEntry);
            _listDirectoryState.expectedOffset++;
        }

        // Request next set of directory entries
        _expectedIncomingSeqNumber = ackOrNak->hdr.seqNumber;
        _listDirectoryWorker(true /* firstRequest */);
    } else if (ackOrNak->hdr.opcode == MavlinkFTP::kRspNak) {
        MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(ackOrNak->data[0]);

        if (errorCode == MavlinkFTP::kErrEOF) {
            // All entries returned
            if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
                qCDebug(FTPManagerLog) << "_listDirectoryAckOrNak: Disregarding Nak due to incorrect sequence actual:expected" << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
                _ackOrNakTimeoutTimer.start();
                return;
            } else {
                qCDebug(FTPManagerLog) << "_listDirectoryAckOrNak EOF";
                _advanceStateMachine();
            }
        } else { /* Don't care is this is out of sequence */
            qCDebug(FTPManagerLog) << "_listDirectoryAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
            _listDirectoryComplete(tr("List directory failed"));
        }
    }
}

void FTPManager::_listDirectoryTimeout(void)
{
    if (++_listDirectoryState.retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_listDirectoryTimeout retries exceeded");
        _listDirectoryComplete(tr("List directory failed"));
    } else {
        // Try again
        qCDebug(FTPManagerLog) << QString("_listDirectoryTimeout: retrying - retryCount(%1) offset(%2)").arg(_listDirectoryState.retryCount).arg(_listDirectoryState.expectedOffset);
        _listDirectoryWorker(false /* firstReqeust */);
    }
}

void FTPManager::_fillMissingBlocksWorker(bool firstRequest)
{
    if (_downloadState.rgMissingData.count()) {
        MavlinkFTP::Request request{};
        MissingData_t&      missingData = _downloadState.rgMissingData.first();

        uint32_t cBytesToRead = qMin((uint32_t)sizeof(request.data), missingData.cBytesMissing);

        qCDebug(FTPManagerLog) << "_fillMissingBlocksBegin: offset:cBytesToRead" << missingData.offset << cBytesToRead;

        request.hdr.session                 = _downloadState.sessionId;
        request.hdr.opcode                  = MavlinkFTP::kCmdReadFile;
        request.hdr.offset                  = missingData.offset;
        request.hdr.size                    = cBytesToRead;

        if (firstRequest) {
            _downloadState.retryCount = 0;
        } else {
            // Must used same sequence number as previous request
            _expectedIncomingSeqNumber -= 2;
        }
        _downloadState.expectedOffset = request.hdr.offset;

        _sendRequestExpectAck(&request);
    } else {
        // We should have the full file now
        if (_downloadState.checksize == false || _downloadState.bytesWritten == _downloadState.fileSize) {
            _advanceStateMachine();
        } else {
            qCDebug(FTPManagerLog) << "_fillMissingBlocksWorker: no missing blocks but file still incomplete - bytesWritten:fileSize" << _downloadState.bytesWritten << _downloadState.fileSize;
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
    MavlinkFTP::OpCode_t requestOpCode = static_cast<MavlinkFTP::OpCode_t>(ackOrNak->hdr.req_opcode);

    if (requestOpCode != MavlinkFTP::kCmdReadFile) {
        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Disregarding due to incorrect requestOpCode" << MavlinkFTP::opCodeToString(requestOpCode);
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Disregarding due to incorrect sequence actual:expected" << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }
    if (ackOrNak->hdr.session != _downloadState.sessionId) {
        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Disregarding due to incorrect session id actual:expected" << ackOrNak->hdr.session << _downloadState.sessionId;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MavlinkFTP::kRspAck) {
        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Ack offset:size" << ackOrNak->hdr.offset << ackOrNak->hdr.size;

        if (ackOrNak->hdr.offset != _downloadState.expectedOffset) {
            if (++_downloadState.retryCount > _maxRetry) {
                qCDebug(FTPManagerLog) << QString("_fillMissingBlocksAckOrNak: offset mismatch, retries exceeded");
                _downloadComplete(tr("Download failed"));
                return;
            }

            // Ask for current offset again
            qCDebug(FTPManagerLog) << QString("_fillMissingBlocksAckOrNak: Ack offset mismatch retry, retryCount(%1) offset(%2)").arg(_downloadState.retryCount).arg(_downloadState.expectedOffset);
            _fillMissingBlocksWorker(false /* firstReqeust */);
            return;
        }

        _downloadState.file.seek(ackOrNak->hdr.offset);
        int bytesWritten = _downloadState.file.write((const char*)ackOrNak->data, ackOrNak->hdr.size);
        if (bytesWritten != ackOrNak->hdr.size) {
            _downloadComplete(tr("Download failed: Error saving file"));
            return;
        }
        _downloadState.bytesWritten += ackOrNak->hdr.size;

        MissingData_t& missingData = _downloadState.rgMissingData.first();
        missingData.offset += ackOrNak->hdr.size;
        missingData.cBytesMissing -= ackOrNak->hdr.size;
        if (missingData.cBytesMissing == 0) {
            // This block is finished, remove it
            _downloadState.rgMissingData.takeFirst();
        }

        // Move on to fill in possible next hole
        _fillMissingBlocksWorker(true /* firstReqeust */);

        // Emit progress last, as cancel could be called in there
        if (_downloadState.fileSize != 0) {
            emit commandProgress((float)(_downloadState.bytesWritten) / (float)_downloadState.fileSize);
        }
    } else if (ackOrNak->hdr.opcode == MavlinkFTP::kRspNak) {
        MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(ackOrNak->data[0]);

        if (errorCode == MavlinkFTP::kErrEOF) {
            qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak EOF";
            if (_downloadState.checksize == false || _downloadState.bytesWritten == _downloadState.fileSize) {
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
    if (++_downloadState.retryCount > _maxRetry) {
        qCDebug(FTPManagerLog) << QString("_fillMissingBlocksTimeout retries exceeded");
        _downloadComplete(tr("Download failed"));
    } else {
        // Ask for current offset again
        qCDebug(FTPManagerLog) << QString("_fillMissingBlocksTimeout: retrying - retryCount(%1) offset(%2)").arg(_downloadState.retryCount).arg(_downloadState.expectedOffset);
        _fillMissingBlocksWorker(false /* firstReqeust */);
    }
}

void FTPManager::_resetSessionsBegin(void)
{
    MavlinkFTP::Request request{};
    request.hdr.opcode  = MavlinkFTP::kCmdResetSessions;
    request.hdr.size    = 0;
    _sendRequestExpectAck(&request);
}

void FTPManager::_resetSessionsAckOrNak(const MavlinkFTP::Request* ackOrNak)
{
    MavlinkFTP::OpCode_t requestOpCode = static_cast<MavlinkFTP::OpCode_t>(ackOrNak->hdr.req_opcode);

    if (requestOpCode != MavlinkFTP::kCmdResetSessions) {
        qCDebug(FTPManagerLog) << "_fillMissingBlocksAckOrNak: Disregarding due to incorrect requestOpCode" << MavlinkFTP::opCodeToString(requestOpCode);
        return;
    }
    if (ackOrNak->hdr.seqNumber != _expectedIncomingSeqNumber) {
        qCDebug(FTPManagerLog) << "_resetSessionsAckOrNak: Disregarding due to incorrect sequence actual:expected" << ackOrNak->hdr.seqNumber << _expectedIncomingSeqNumber;
        return;
    }

    _ackOrNakTimeoutTimer.stop();

    if (ackOrNak->hdr.opcode == MavlinkFTP::kRspAck) {
        qCDebug(FTPManagerLog) << "_resetSessionsAckOrNak: Ack";
        _advanceStateMachine();
    } else if (ackOrNak->hdr.opcode == MavlinkFTP::kRspNak) {
        qCDebug(FTPManagerLog) << "_resetSessionsAckOrNak: Nak -" << _errorMsgFromNak(ackOrNak);
        _downloadComplete(QString());
    }
}

void FTPManager::_resetSessionsTimeout(void)
{
    qCDebug(FTPManagerLog) << "_resetSessionsTimeout";
    _downloadComplete(QString());
}

void FTPManager::_sendRequestExpectAck(MavlinkFTP::Request* request)
{
    _ackOrNakTimeoutTimer.start();
    
    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        request->hdr.seqNumber = _expectedIncomingSeqNumber + 1;    // Outgoing is 1 past last incoming
        _expectedIncomingSeqNumber += 2;

        qCDebug(FTPManagerLog) << "_sendRequestExpectAck opcode:" << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.opcode)) << "seqNumber:" << request->hdr.seqNumber;

        mavlink_message_t message;
        mavlink_msg_file_transfer_protocol_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                     MAVLinkProtocol::getComponentId(),
                                                     sharedLink->mavlinkChannel(),
                                                     &message,
                                                     0,                                                     // Target network, 0=broadcast?
                                                     _vehicle->id(),
                                                     _ftpCompId,
                                                     (uint8_t*)request);                                    // Payload
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    } else {
        qCDebug(FTPManagerLog) << "_sendRequestExpectAck No primary link. Allowing timeout to fail sequence.";
    }
}

bool FTPManager::_parseURI(uint8_t fromCompId, const QString& uri, QString& parsedURI, uint8_t& compId)
{
    parsedURI   = uri;
    compId      = (fromCompId == MAV_COMP_ID_ALL) ? (uint8_t)MAV_COMP_ID_AUTOPILOT1 : fromCompId;

    // Pull scheme off the front if there
    QString ftpPrefix(QStringLiteral("%1://").arg(mavlinkFTPScheme));
    if (parsedURI.startsWith(ftpPrefix, Qt::CaseInsensitive)) {
        parsedURI = parsedURI.right(parsedURI.length() - ftpPrefix.length() + 1);
    }
    if (parsedURI.contains("://")) {
        qCWarning(FTPManagerLog) << "Incorrect uri scheme or format" << uri;
        return false;
    }

    // Pull component id off the front if there
    QRegularExpression      regEx("^/??\\[\\;comp\\=(\\d+)\\]");
    QRegularExpressionMatch match = regEx.match(parsedURI);
    if (match.hasMatch()) {
        bool ok;
        compId = match.captured(1).toUInt(&ok);
        if (!ok) {
            qCWarning(FTPManagerLog) << "Incorrect format for component id" << uri;
            return false;
        }

        qCDebug(FTPManagerLog) << "Found compId in MAVLink FTP URI: " << compId;
        parsedURI.replace(QRegularExpression("\\[\\;comp\\=\\d+\\]"), "");
    }

    return true;
}

bool FTPManager::_isListDirectoryStateMachine(void)
{
    if (_rgStateMachine.isEmpty()) {
        qCWarning(FTPManagerLog) << "INTERNAL ERROR: _isListDirectoryStateMachine called with empty state machine";
        return false;
    }

    if (_rgStateMachine[0].beginFn == &FTPManager::_listDirectoryBegin) {
        return true;
    } else if (_rgStateMachine[0].beginFn == &FTPManager::_openFileROBegin) {
        return false;
    } else {
        qCWarning(FTPManagerLog) << "INTERNAL ERROR: _isListDirectoryStateMachine called with invalid state machine";
        return false;
    }
}
