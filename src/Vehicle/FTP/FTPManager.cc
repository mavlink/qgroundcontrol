#include "FTPManager.h"
#include "FTPDeleteStateMachine.h"
#include "FTPDownloadStateMachine.h"
#include "FTPListDirectoryStateMachine.h"
#include "FTPUploadStateMachine.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRegularExpression>

QGC_LOGGING_CATEGORY(FTPManagerLog, "Vehicle.FTPManager")

FTPManager::FTPManager(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{
    // Create state machines
    _deleteStateMachine = new FTPDeleteStateMachine(vehicle, this);
    _downloadStateMachine = new FTPDownloadStateMachine(vehicle, this);
    _listDirectoryStateMachine = new FTPListDirectoryStateMachine(vehicle, this);
    _uploadStateMachine = new FTPUploadStateMachine(vehicle, this);

    // Connect delete signals
    connect(_deleteStateMachine, &FTPDeleteStateMachine::deleteComplete,
            this, &FTPManager::deleteComplete);

    // Connect download signals
    connect(_downloadStateMachine, &FTPDownloadStateMachine::downloadComplete,
            this, &FTPManager::downloadComplete);
    connect(_downloadStateMachine, &FTPDownloadStateMachine::progressChanged,
            this, &FTPManager::commandProgress);

    // Connect list directory signals
    connect(_listDirectoryStateMachine, &FTPListDirectoryStateMachine::listDirectoryComplete,
            this, &FTPManager::listDirectoryComplete);

    // Connect upload signals
    connect(_uploadStateMachine, &FTPUploadStateMachine::uploadComplete,
            this, &FTPManager::uploadComplete);
    connect(_uploadStateMachine, &FTPUploadStateMachine::progressChanged,
            this, &FTPManager::commandProgress);
}

FTPManager::~FTPManager()
{
}

bool FTPManager::download(uint8_t fromCompId, const QString& fromURI, const QString& toDir, const QString& fileName, bool checksize)
{
    qCDebug(FTPManagerLog) << "download fromCompId:" << fromCompId
                           << "fromURI:" << fromURI
                           << "toDir:" << toDir
                           << "fileName:" << fileName;

    if (isOperationInProgress()) {
        qCDebug(FTPManagerLog) << "Cannot download. Already in another operation";
        return false;
    }

    QString parsedURI;
    uint8_t compId;
    if (!_parseURI(fromCompId, fromURI, parsedURI, compId)) {
        qCWarning(FTPManagerLog) << "_parseURI failed";
        return false;
    }

    return _downloadStateMachine->download(compId, parsedURI, toDir, fileName, checksize);
}

bool FTPManager::upload(uint8_t toCompId, const QString& toURI, const QString& fromFile)
{
    qCDebug(FTPManagerLog) << "upload fromFile:" << fromFile << "toURI:" << toURI << "toCompId:" << toCompId;

    if (isOperationInProgress()) {
        qCDebug(FTPManagerLog) << "Cannot upload. Already in another operation";
        return false;
    }

    QString parsedURI;
    uint8_t compId;
    if (!_parseURI(toCompId, toURI, parsedURI, compId)) {
        qCWarning(FTPManagerLog) << "_parseURI failed";
        return false;
    }

    return _uploadStateMachine->upload(compId, fromFile, parsedURI);
}

bool FTPManager::listDirectory(uint8_t fromCompId, const QString& fromURI)
{
    qCDebug(FTPManagerLog) << "listDirectory fromURI:" << fromURI << "fromCompId:" << fromCompId;

    if (isOperationInProgress()) {
        qCDebug(FTPManagerLog) << "Cannot list directory. Already in another operation";
        return false;
    }

    QString parsedURI;
    uint8_t compId;
    if (!_parseURI(fromCompId, fromURI, parsedURI, compId)) {
        qCWarning(FTPManagerLog) << "_parseURI failed";
        return false;
    }

    return _listDirectoryStateMachine->listDirectory(compId, parsedURI);
}

bool FTPManager::deleteFile(uint8_t fromCompId, const QString& fromURI)
{
    qCDebug(FTPManagerLog) << "deleteFile fromURI:" << fromURI << "fromCompId:" << fromCompId;

    if (isOperationInProgress()) {
        qCDebug(FTPManagerLog) << "Cannot delete file. Already in another operation";
        return false;
    }

    QString parsedURI;
    uint8_t compId;
    if (!_parseURI(fromCompId, fromURI, parsedURI, compId)) {
        qCWarning(FTPManagerLog) << "_parseURI failed";
        return false;
    }

    return _deleteStateMachine->deleteFile(compId, parsedURI);
}

void FTPManager::cancelDownload()
{
    _downloadStateMachine->cancel();
}

void FTPManager::cancelListDirectory()
{
    _listDirectoryStateMachine->cancel();
}

void FTPManager::cancelDelete()
{
    _deleteStateMachine->cancel();
}

void FTPManager::cancelUpload()
{
    _uploadStateMachine->cancel();
}

bool FTPManager::isOperationInProgress() const
{
    return _deleteStateMachine->isOperationInProgress() ||
           _downloadStateMachine->isOperationInProgress() ||
           _listDirectoryStateMachine->isOperationInProgress() ||
           _uploadStateMachine->isOperationInProgress();
}

void FTPManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    // Route to the active state machine
    if (_deleteStateMachine->isOperationInProgress()) {
        _deleteStateMachine->handleFTPMessage(message);
    }
    if (_downloadStateMachine->isOperationInProgress()) {
        _downloadStateMachine->handleFTPMessage(message);
    }
    if (_listDirectoryStateMachine->isOperationInProgress()) {
        _listDirectoryStateMachine->handleFTPMessage(message);
    }
    if (_uploadStateMachine->isOperationInProgress()) {
        _uploadStateMachine->handleFTPMessage(message);
    }
}

bool FTPManager::_parseURI(uint8_t fromCompId, const QString& uri, QString& parsedURI, uint8_t& compId)
{
    parsedURI = uri;
    compId = (fromCompId == MAV_COMP_ID_ALL) ? static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1) : fromCompId;

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
    QRegularExpression regEx("^/??\\[\\;comp\\=(\\d+)\\]");
    QRegularExpressionMatch match = regEx.match(parsedURI);
    if (match.hasMatch()) {
        bool ok;
        compId = match.captured(1).toUInt(&ok);
        if (!ok) {
            qCWarning(FTPManagerLog) << "Incorrect format for component id" << uri;
            return false;
        }

        qCDebug(FTPManagerLog) << "Found compId in MAVLink FTP URI:" << compId;
        parsedURI.replace(QRegularExpression("\\[\\;comp\\=\\d+\\]"), "");
    }

    return true;
}
