#include "FTPManagerOperations.h"

#include <QtCore/QFileInfo>
#include <limits>

bool FTPManager::DownloadOperation::begin(uint8_t componentId, const QString& uri, const QString& destinationDirectory,
                                          const QString& requestedFileName, bool verifySize,
                                          ExistingFilePolicy filePolicy, uint8_t& resolvedComponentId)
{
    reset();

    const MavlinkFTP::UriParseResult parsedUri = MavlinkFTP::parseUri(componentId, uri);
    if (!parsedUri.valid()) {
        return false;
    }

    resolvedComponentId = parsedUri.componentId;
    fullPathOnVehicle = parsedUri.path;
    toDir.setPath(destinationDirectory);
    checkSize = verifySize;
    existingFilePolicy = filePolicy;

    const qsizetype lastSlash = fullPathOnVehicle.lastIndexOf(QLatin1Char('/'));
    fileName = requestedFileName.isEmpty() ? fullPathOnVehicle.sliced(lastSlash + 1) : requestedFileName;
    return true;
}

QList<FTPManager::StateFunctions_t> FTPManager::DownloadOperation::stateMachine() const
{
    return {
        {&FTPManager::_openFileROBegin, &FTPManager::_openFileROAckOrNak, &FTPManager::_openFileROTimeout},
        {&FTPManager::_burstReadFileBegin, &FTPManager::_burstReadFileAckOrNak, &FTPManager::_burstReadFileTimeout},
        {&FTPManager::_fillMissingBlocksBegin, &FTPManager::_fillMissingBlocksAckOrNak,
         &FTPManager::_fillMissingBlocksTimeout},
        {&FTPManager::_resetSessionsBegin, &FTPManager::_resetSessionsAckOrNak, &FTPManager::_resetSessionsTimeout},
        {&FTPManager::_downloadCompleteNoError, nullptr, nullptr},
    };
}

QList<FTPManager::StateFunctions_t> FTPManager::DownloadOperation::terminationStateMachine() const
{
    return {
        {&FTPManager::_terminateSessionBegin, &FTPManager::_terminateSessionAckOrNak,
         &FTPManager::_terminateSessionTimeout},
        {&FTPManager::_terminateComplete, nullptr, nullptr},
    };
}

void FTPManager::DownloadOperation::reset()
{
    sessionId = 0;
    expectedOffset = 0;
    bytesWritten = 0;
    retryCount = 0;
    fileSize = 0;
    checkSize = true;
    existingFilePolicy = ExistingFilePolicy::Replace;
    remoteSessionOpen = false;
    fullPathOnVehicle.clear();
    fileName.clear();
    pendingError.clear();
    missingData.clear();
    file.close();
    file.setFileName(QString());
}

FTPManager::UploadOperation::BeginResult FTPManager::UploadOperation::begin(uint8_t componentId, const QString& uri,
                                                                            const QString& sourceFile,
                                                                            uint8_t& resolvedComponentId)
{
    reset();

    const QFileInfo sourceInfo(sourceFile);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        return BeginResult::SourceMissing;
    }
    if (sourceInfo.size() > (std::numeric_limits<uint32_t>::max)()) {
        return BeginResult::SourceTooLarge;
    }

    localFilePath = sourceFile;
    file.setFileName(sourceFile);
    if (!file.open(QFile::ReadOnly)) {
        const QString errorString = file.errorString();
        reset();
        openErrorString = errorString;
        return BeginResult::OpenFailed;
    }

    const MavlinkFTP::UriParseResult parsedUri = MavlinkFTP::parseUri(componentId, uri);
    if (!parsedUri.valid()) {
        reset();
        return BeginResult::InvalidUri;
    }

    resolvedComponentId = parsedUri.componentId;
    fullPathOnVehicle = parsedUri.path;
    fileSize = static_cast<uint32_t>(sourceInfo.size());
    return BeginResult::Success;
}

QList<FTPManager::StateFunctions_t> FTPManager::UploadOperation::stateMachine() const
{
    return {
        {&FTPManager::_createFileBegin, &FTPManager::_createFileAckOrNak, &FTPManager::_createFileTimeout},
        {&FTPManager::_writeFileBegin, &FTPManager::_writeFileAckOrNak, &FTPManager::_writeFileTimeout},
        {&FTPManager::_resetSessionsBegin, &FTPManager::_resetSessionsAckOrNak, &FTPManager::_resetSessionsTimeout},
        {&FTPManager::_uploadFinalize, nullptr, nullptr},
    };
}

QList<FTPManager::StateFunctions_t> FTPManager::UploadOperation::terminationStateMachine() const
{
    return {
        {&FTPManager::_terminateUploadSessionBegin, &FTPManager::_terminateUploadSessionAckOrNak,
         &FTPManager::_terminateUploadSessionTimeout},
        {&FTPManager::_uploadFinalize, nullptr, nullptr},
    };
}

void FTPManager::UploadOperation::reset()
{
    sessionId = 0;
    totalBytesSent = 0;
    fileSize = 0;
    lastChunkSize = 0;
    retryCount = 0;
    cancelled = false;
    fullPathOnVehicle.clear();
    localFilePath.clear();
    openErrorString.clear();
    file.close();
    file.setFileName(QString());
}

bool FTPManager::ListDirectoryOperation::begin(uint8_t componentId, const QString& uri, int entryLimit,
                                               MAV_FTP_OPCODE listOpcode, uint8_t& resolvedComponentId)
{
    reset();
    if (entryLimit < 0) {
        return false;
    }

    const MavlinkFTP::UriParseResult parsedUri = MavlinkFTP::parseUri(componentId, uri);
    if (!parsedUri.valid()) {
        return false;
    }

    resolvedComponentId = parsedUri.componentId;
    fullPathOnVehicle = parsedUri.path;
    maxEntries = entryLimit;
    opCode = listOpcode;
    return true;
}

QList<FTPManager::StateFunctions_t> FTPManager::ListDirectoryOperation::stateMachine() const
{
    return {
        {&FTPManager::_listDirectoryBegin, &FTPManager::_listDirectoryAckOrNak, &FTPManager::_listDirectoryTimeout},
        {&FTPManager::_listDirectoryCompleteNoError, nullptr, nullptr},
    };
}

void FTPManager::ListDirectoryOperation::reset()
{
    sessionId = 0;
    expectedOffset = 0;
    fullPathOnVehicle.clear();
    directoryEntries.clear();
    retryCount = 0;
    maxEntries = 0;
    truncated = false;
    opCode = MAV_FTP_OPCODE_LISTDIRECTORY;
}

bool FTPManager::DeleteOperation::begin(uint8_t componentId, const QString& uri, uint8_t& resolvedComponentId)
{
    reset();
    const MavlinkFTP::UriParseResult parsedUri = MavlinkFTP::parseUri(componentId, uri);
    if (!parsedUri.valid()) {
        return false;
    }

    resolvedComponentId = parsedUri.componentId;
    fullPathOnVehicle = parsedUri.path;
    return true;
}

QList<FTPManager::StateFunctions_t> FTPManager::DeleteOperation::stateMachine() const
{
    return {
        {&FTPManager::_deleteFileBegin, &FTPManager::_deleteFileAckOrNak, &FTPManager::_deleteFileTimeout},
        {&FTPManager::_deleteCompleteNoError, nullptr, nullptr},
    };
}

void FTPManager::DeleteOperation::reset()
{
    fullPathOnVehicle.clear();
    retryCount = 0;
}
