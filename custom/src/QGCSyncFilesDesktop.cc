/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "QGCSyncFilesDesktop.h"
#include "QGCApplication.h"
#include "PlanMasterController.h"
#include "AppSettings.h"
#include "SettingsManager.h"

#include <QDirIterator>

//-- TODO: This is here as it defines the UDP port and URL. It needs to go upstream
#include "TyphoonHQuickInterface.h"

QGC_LOGGING_CATEGORY(QGCSyncFiles, "QGCSyncFiles")

static const char* kMissionExtension = ".plan";
static const char* kMissionWildCard  = "*.plan";

//-----------------------------------------------------------------------------
QGCSyncFilesDesktop::QGCSyncFilesDesktop(QObject* parent)
    : QThread(parent)
    , _logController(this)
    , _udpSocket(NULL)
    , _remoteNode(NULL)
    , _cancel(false)
    , _totalFiles(0)
    , _curFile(0)
    , _syncProgress(0)
    , _fileProgress(0)
    , _sendingFiles(false)
    , _syncDone(false)
{
    qmlRegisterUncreatableType<QGCSyncFilesDesktop>("QGroundControl", 1, 0, "QGCSyncFilesDesktop", "Reference only");
    //-- Start UDP listener
    _initUDPListener();
    connect(this, &QGCSyncFilesDesktop::completed,  this, &QGCSyncFilesDesktop::_completed);
    connect(this, &QGCSyncFilesDesktop::progress,   this, &QGCSyncFilesDesktop::_setSyncProgress);
    connect(this, &QGCSyncFilesDesktop::message,    this, &QGCSyncFilesDesktop::_message);
    connect(this, &QGCSyncFilesDesktop::sendMission,this, &QGCSyncFilesDesktop::_sendMission);
    connect(&_remoteMaintenanceTimer, &QTimer::timeout, this, &QGCSyncFilesDesktop::_remoteMaintenance);
    _remoteMaintenanceTimer.setSingleShot(false);
    //-- Every 15 seconds, clear stale remotes
    _remoteMaintenanceTimer.start(15000);
}

//-----------------------------------------------------------------------------
QGCSyncFilesDesktop::~QGCSyncFilesDesktop()
{
    _remoteMaintenanceTimer.stop();
    _cancel = true;
    if(_udpSocket) {
        _udpSocket->deleteLater();
    }
    disconnectRemote();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_initUDPListener()
{
    //-- Desktop builds listens for Mobile builds broadcasting their presences.
    if(!_udpSocket) {
        _udpSocket = new QUdpSocket(this);
    }
    QHostAddress host = QHostAddress::AnyIPv4;
    _udpSocket->setProxy(QNetworkProxy::NoProxy);
    if(_udpSocket->bind(host, QGC_UDP_BROADCAST_PORT, QAbstractSocket::ReuseAddressHint | QUdpSocket::ShareAddress)) {
        qCDebug(QGCSyncFiles) << "UDP Broadcast receiver bound to:" << QGC_UDP_BROADCAST_PORT;
        QObject::connect(_udpSocket, &QUdpSocket::readyRead, this, &QGCSyncFilesDesktop::_readUDPBytes);
    } else {
        qWarning() << "Could not bind UDP port:" << QGC_UDP_BROADCAST_PORT;
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_readUDPBytes()
{
    //-- This is a broadcast from a Mobile build. Collect its "name" and URL.
    while (_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        _udpSocket->readDatagram(datagram.data(), datagram.size(), &sender);
        QUrl url;
        url.setHost(sender.toString());
        url.setPort(QGC_RPC_PORT);
        url.setScheme("tcp");
        QString remoteName = datagram.data();
        if(!_remoteURLs.contains(remoteName)) {
            _remoteNames.append(datagram.data());
            _remoteURLs[remoteName] = url;
            _remoteTimer[remoteName] = QTime();
            _remoteTimer[remoteName].start();
            qCDebug(QGCSyncFiles) << "New node:" << url.toString();
            emit remoteListChanged();
        } else {
            _remoteTimer[remoteName].restart();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::cancelSync()
{
    _message(QString(tr("Canceling...")));
    _cancel = true;
}

//-----------------------------------------------------------------------------
bool
QGCSyncFilesDesktop::connectToRemote(QString name)
{
    //-- Find name
    if(!_remoteURLs.contains(name)) {
        return false;
    }
    if(_remoteNode) {
        disconnectRemote();
    }
    _remoteNode = new QRemoteObjectNode(this);
    qCDebug(QGCSyncFiles) << "Connect to" << _remoteURLs[name].toString();
    connect(_remoteNode, &QRemoteObjectNode::error, this, &QGCSyncFilesDesktop::_nodeError);
    if(_remoteNode->connectToNode(_remoteURLs[name])) {
        _remoteObject.reset(_remoteNode->acquire<QGCRemoteReplica>());
        if(_remoteObject.isNull()) {
            return false;
        }
        connect(_remoteObject.data(), &QRemoteObjectReplica::stateChanged, this, &QGCSyncFilesDesktop::_stateChanged);
        connect(_remoteObject.data(), &QGCRemoteReplica::syncTypeChanged,  this, &QGCSyncFilesDesktop::_syncTypeChanged);
        connect(_remoteObject.data(), &QGCRemoteReplica::receiveMission,   this, &QGCSyncFilesDesktop::_receiveMission);
        connect(_remoteObject.data(), &QGCRemoteReplica::sendLogFragment,  this, &QGCSyncFilesDesktop::_sendLogFragment);
        _currentRemote = name;
        emit currentRemoteChanged();
        emit remoteReadyChanged();
        _remoteObject->setConnectedToRemote(true);
    }
    return false;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::disconnectRemote()
{
    if(_remoteNode) {
        delete _remoteNode;
        _remoteNode = NULL;
        emit remoteReadyChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::uploadMission(QString name, PlanMasterController* controller)
{
    if(!_remoteObject.isNull()) {
        _sendingFiles   = true;
        emit sendingFilesChanged();
        _message(QString(tr("Uploading %1")).arg(name));
        _setSyncProgress(100, 50);
        QJsonDocument saveDoc(controller->saveToJson());
        _sendMission(name, saveDoc.toJson());
        //-- Should we do this?
        controller->setDirty(false);
        _message(QString(tr("%1 sent.")).arg(name));
        _setSyncProgress(100, 100);
        _completed();
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::initSync()
{
    _missions.clear();
    _syncMessage.clear();
    _sendingFiles   = false;
    _syncDone       = false;
    emit sendingFilesChanged();
    emit syncDoneChanged();
    emit syncMessageChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::initLogFetch()
{
    initSync();
    _logController.clearFileItems();
    if(_currentLog.isOpen()) {
        _currentLog.close();
    }
    QList<QGCRemoteLogEntry> allLogs = _remoteObject->logEntries();
    foreach(QGCRemoteLogEntry logEntry, allLogs) {
        QGCFileListItem* item = new QGCFileListItem(&_logController, logEntry.name(), logEntry.size());
        _logController.appendFileItem(item);
    }
    emit _logController.fileListChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::uploadAllMissions()
{
    if(_sendingFiles) {
        return;
    }
    //-- Reset Status
    _missions.clear();
    _sendingFiles   = true;
    _syncProgress   = 0;
    _syncDone       = false;
    _syncMessage.clear();
    emit syncProgressChanged();
    emit sendingFilesChanged();
    emit syncMessageChanged();
    emit syncDoneChanged();
    if(!remoteReady()) {
        _message(QString(tr("Not Connected To Remote")));
        _completed();
        return;
    }
    QStringList allMissions;
    //-- Where missions are stored
    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    //-- Find all of them
    QDirIterator it(missionPath, QStringList() << kMissionWildCard, QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        _missions << fi.filePath();
        allMissions << fi.fileName();
    }
    if(!_missions.size()) {
        _message(QString(tr("No missions to send")));
        _completed();
        return;
    }
    //-- If cloning, prune extra files
    if(syncType() == SyncClone) {
        _remoteObject->pruneExtraMissions(allMissions);
    }
    _doSync();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::downloadAllMissions()
{
    if(_sendingFiles) {
        return;
    }
    _missions.clear();
    _sendingFiles   = true;
    _syncProgress   = 0;
    _syncDone       = false;
    _syncMessage.clear();
    emit syncProgressChanged();
    emit sendingFilesChanged();
    emit syncMessageChanged();
    emit syncDoneChanged();
    if(!remoteReady()) {
        _message(QString(tr("Not Connected To Remote")));
        _completed();
        return;
    }
    QStringList allMissions = _remoteObject->currentMissions();
    _curFile = 0;
    _totalFiles = allMissions.size();
    qCDebug(QGCSyncFiles) << "Requesting all mission files";
    qCDebug(QGCSyncFiles) << "Sync Type:" << syncType();
    //-- If cloning, remove extra files
    if(syncType() == SyncClone) {
        QStringList missionsToPrune;
        //-- Where missions are stored
        QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
        //-- Find all of them
        QDirIterator it(missionPath, QStringList() << kMissionWildCard, QDir::Files, QDirIterator::NoIteratorFlags);
        while(it.hasNext()) {
            QFileInfo fi(it.next());
            //-- If the remote doesn't have it, prune it.
            if(!allMissions.contains(fi.fileName())) {
                missionsToPrune << fi.filePath();
            }
        }
        foreach(QString missionToZap, missionsToPrune) {
            qCDebug(QGCSyncFiles) << "Pruning extra mission:" << missionToZap;
            QFile f(missionToZap);
            f.remove();
        }
    }
    _remoteObject->requestMissions(allMissions);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::downloadSelectedLogs(QString path)
{
    if(_sendingFiles) {
        return;
    }
    _sendingFiles   = true;
    _syncProgress   = 0;
    _syncDone       = false;
    _syncMessage.clear();
    emit syncProgressChanged();
    emit sendingFilesChanged();
    emit syncMessageChanged();
    emit syncDoneChanged();
    if(!remoteReady()) {
        _message(QString(tr("Not Connected To Remote")));
        _completed();
        return;
    }
    //-- Target Path
    _logPath = path;
    if(_logPath.startsWith("file://")) _logPath.replace("file://", "");
#ifdef Q_OS_WIN
    if(!_logPath.endsWith("\\") && !_logPath.endsWith("/")) _logPath += "/";
#else
    if(!_logPath.endsWith("/")) _logPath += "/";
#endif
    QDir destDir(_logPath);
    if (!destDir.exists()) {
        if(!destDir.mkpath(".")) {
            _message(QString(tr("Error creating destination %1")).arg(_logPath));
            return;
        }
    }
    //-- Build log list
    QStringList requestedLogs;
    for(int i = 0; i < _logController.fileListV().size(); i++) {
        if(_logController.fileListV()[i] && _logController.fileListV()[i]->selected()) {
            requestedLogs << _logController.fileListV()[i]->fileName();
        }
    }
    //-- Request logs
    _curFile = 0;
    _totalFiles = requestedLogs.size();
    qCDebug(QGCSyncFiles) << "Requesting log files";
    _remoteObject->requestLogs(requestedLogs);
}

//-----------------------------------------------------------------------------
bool
QGCSyncFilesDesktop::remoteReady()
{
    bool nodeReady = !_remoteObject.isNull() && _remoteObject->state() == QRemoteObjectReplica::Valid;
    qCDebug(QGCSyncFiles) << "State:" << nodeReady;
    return nodeReady;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_setSyncProgress(quint32 total, quint32 current)
{
    if(!total) {
        _syncProgress = 100;
    } else {
        _syncProgress = (int)((double)current / (double)total * 100.0);
    }
    emit syncProgressChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_setFileProgress(quint32 total, quint32 current)
{
    if(!total) {
        _fileProgress = 100;
    } else {
        _fileProgress = (int)((double)current / (double)total * 100.0);
    }
    emit fileProgressChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_message(QString message)
{
    _syncMessage = message;
    emit syncMessageChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_stateChanged(QRemoteObjectReplica::State state, QRemoteObjectReplica::State)
{
    qCDebug(QGCSyncFiles) << "State changed to" << state;
    emit remoteReadyChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_nodeError(QRemoteObjectNode::ErrorCode errorCode)
{
    qCDebug(QGCSyncFiles) << "Node error:" << errorCode;
}

//-----------------------------------------------------------------------------
bool
QGCSyncFilesDesktop::_doSync()
{
    if(_remoteObject.isNull()) {
        emit completed();
        return false;
    }
    _cancel = false;
    this->start(QThread::NormalPriority);
    return true;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_completed()
{
    _sendingFiles   = false;
    _syncDone       = true;
    emit sendingFilesChanged();
    emit syncDoneChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::run()
{
    _curFile    = 0;
    _totalFiles = _missions.size();
    emit progress(_totalFiles, 0);
    emit message(QString(tr("Sending mission files...")));
    foreach(QString mission, _missions) {
        if(_cancel) {
            emit message(QString(tr("Operation Canceled")));
            break;
        }
        QFileInfo fi(mission);
        QFile missionFile(mission);
        if (!missionFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open file" << mission;
        } else {
            QByteArray bytes = missionFile.readAll();
            missionFile.close();
            qCDebug(QGCSyncFiles) << "Sending:" << fi.filePath();
            emit sendMission(fi.fileName(), bytes);
        }
        emit progress(_totalFiles, ++_curFile);
    }
    if(!_cancel) {
        emit message(QString(tr("%1 files sent")).arg(_totalFiles));
        emit progress(1, 1);
    }
    emit completed();
}

//-----------------------------------------------------------------------------
bool
QGCSyncFilesDesktop::_sendMission(QString name, QByteArray mission)
{
    if(_remoteObject.isNull()) {
        return false;
    }
    if(!name.endsWith(kMissionExtension)) name += kMissionExtension;
    _remoteObject->sendMission(QGCNewMission(name, mission));
    return true;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_remoteMaintenance()
{
    //-- Clear stale remotes
    foreach(QString remote, _remoteTimer.keys()) {
        if(_remoteTimer[remote].elapsed() > 15000) {
            _remoteTimer.remove(remote);
            _remoteURLs.remove(remote);
            _remoteNames.removeOne(remote);
            emit remoteListChanged();
            if(_currentRemote == remote) {
                _currentRemote.clear();
                emit currentRemoteChanged();
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_syncTypeChanged(QGCRemoteReplica::SyncType)
{
    emit syncTypeChanged();
}

//-----------------------------------------------------------------------------
QGCSyncFilesDesktop::SyncType
QGCSyncFilesDesktop::syncType()
{
    if(_remoteObject.isNull()) {
        return SyncClone;
    }
    return (SyncType)_remoteObject->syncType();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::setSyncType(SyncType type)
{
    if(!_remoteObject.isNull()) {
        _remoteObject->setSyncType((QGCRemoteReplica::SyncType)type);
    }
}

//-----------------------------------------------------------------------------
bool
QGCSyncFilesDesktop::_processIncomingMission(QString name, int count, QString& missionFile)
{
    missionFile = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    if(!missionFile.endsWith("/")) missionFile += "/";
    missionFile += name;
    if(!missionFile.endsWith(kMissionExtension)) missionFile += kMissionExtension;
    //-- Add a (unique) count if told to do so
    if(count) {
        missionFile.replace(kMissionExtension, QString("-%1%2").arg(count).arg(kMissionExtension));
    }
    QFile f(missionFile);
    return f.exists();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_receiveMission(QGCNewMission mission)
{
    _setSyncProgress(_totalFiles, ++_curFile);
    if(mission.mission().size()) {
        QString missionFile;
        int count = 0;
        //-- If we are appending, we need to make sure not to overwrite
        do {
            if(!_processIncomingMission(mission.name(), count++, missionFile)) {
                break;
            }
        } while(syncType() == SyncAppend);
        qCDebug(QGCSyncFiles) << "Receiving:" << mission.name();
        QFile file(missionFile);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(mission.mission());
        }
    } else {
        qWarning() << "Error receiving" << mission.name();
    }
    if(_totalFiles <= _curFile) {
        _message(QString(tr("%1 files received")).arg(_totalFiles));
        _setSyncProgress(1, 1);
        _completed();
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_sendLogFragment(QGCLogFragment fragment)
{
    QString logFile = _logPath;
    logFile += fragment.name();
    if(_currentLog.isOpen()) {
        if(_currentLog.fileName() != logFile) {
            _currentLog.close();
        } else {
            if(fragment.data().size()) {
                _currentLog.write(fragment.data());
                _setFileProgress(fragment.total(), fragment.current());
                return;
            }
        }
    }
    if(!_currentLog.isOpen()) {
       _currentLog.setFileName(logFile);
       qCDebug(QGCSyncFiles) << "Receiving:" << logFile;
       _message(QString(tr("Receiving %1")).arg(fragment.name()));
       if (_currentLog.open(QIODevice::WriteOnly)) {
           _curFile++;
           _currentLog.write(fragment.data());
           _setSyncProgress(_totalFiles, _curFile);
           _setFileProgress(fragment.total(), fragment.current());
           return;
       } else {
           qWarning() << "Error creating" << logFile;
       }
    }
    //-- Check for end of sync
    if(_totalFiles <= _curFile && fragment.total() <= fragment.current()) {
        _currentLog.close();
        _setSyncProgress(1, 1);
        _completed();
        _message(QString(tr("%1 files received")).arg(_totalFiles));
    }
}
