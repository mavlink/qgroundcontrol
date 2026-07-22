#pragma once

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <cstdint>
#include <optional>

#include "FTPManager.h"

/// Per-operation state and initialization invariants for FTPManager.
/// Protocol I/O and response handlers remain in FTPManager; these objects own
/// each operation's lifecycle, state-machine steps, and mutable state.
class FTPManager::DownloadOperation final
{
    Q_DISABLE_COPY_MOVE(DownloadOperation)

public:
    DownloadOperation() = default;

    struct MissingData
    {
        uint32_t offset = 0;
        uint32_t bytesMissing = 0;
    };

    StartError begin(uint8_t componentId, const QString& uri, const QString& destinationDirectory,
                     const QString& requestedFileName, bool verifySize, ExistingFilePolicy filePolicy,
                     std::optional<uint32_t> maximumSize, uint8_t& resolvedComponentId);
    QList<StateFunctions_t> stateMachine() const;
    QList<StateFunctions_t> terminationStateMachine() const;
    void reset();

    bool inProgress() const { return !fullPathOnVehicle.isEmpty(); }

    uint8_t sessionId = 0;
    uint32_t expectedOffset = 0;
    uint32_t bytesWritten = 0;
    QList<MissingData> missingData;
    QString fullPathOnVehicle;
    QDir toDir;
    QString fileName;
    uint32_t fileSize = 0;
    QFile file;
    int retryCount = 0;
    bool checkSize = true;
    ExistingFilePolicy existingFilePolicy = ExistingFilePolicy::Replace;
    std::optional<uint32_t> maximumFileSize = std::nullopt;
    bool remoteSessionOpen = false;
    QString pendingError;
};

class FTPManager::UploadOperation final
{
    Q_DISABLE_COPY_MOVE(UploadOperation)

public:
    UploadOperation() = default;

    enum class BeginResult : uint8_t
    {
        Success,
        SourceMissing,
        SourceTooLarge,
        OpenFailed,
        InvalidUri,
        RemotePathTooLong,
    };

    BeginResult begin(uint8_t componentId, const QString& uri, const QString& sourceFile, uint8_t& resolvedComponentId);
    QList<StateFunctions_t> stateMachine() const;
    QList<StateFunctions_t> terminationStateMachine() const;
    void reset();

    bool inProgress() const { return file.isOpen(); }

    uint8_t sessionId = 0;
    uint32_t totalBytesSent = 0;
    uint32_t fileSize = 0;
    uint32_t lastChunkSize = 0;
    QFile file;
    QString fullPathOnVehicle;
    QString localFilePath;
    QString openErrorString;
    int retryCount = 0;
    bool cancelled = false;
};

class FTPManager::ListDirectoryOperation final
{
    Q_DISABLE_COPY_MOVE(ListDirectoryOperation)

public:
    ListDirectoryOperation() = default;

    StartError begin(uint8_t componentId, const QString& uri, int entryLimit, MAV_FTP_OPCODE listOpcode,
                     uint8_t& resolvedComponentId);
    QList<StateFunctions_t> stateMachine() const;
    void reset();

    bool inProgress() const { return !fullPathOnVehicle.isEmpty(); }

    uint8_t sessionId = 0;
    uint32_t expectedOffset = 0;
    QString fullPathOnVehicle;
    QStringList directoryEntries;
    int retryCount = 0;
    int maxEntries = 0;
    bool truncated = false;
    MAV_FTP_OPCODE opCode = MAV_FTP_OPCODE_LISTDIRECTORY;
};

class FTPManager::DeleteOperation final
{
    Q_DISABLE_COPY_MOVE(DeleteOperation)

public:
    DeleteOperation() = default;

    StartError begin(uint8_t componentId, const QString& uri, uint8_t& resolvedComponentId);
    QList<StateFunctions_t> stateMachine() const;
    void reset();

    bool inProgress() const { return !fullPathOnVehicle.isEmpty(); }

    QString fullPathOnVehicle;
    int retryCount = 0;
};
