/*!
 *   @brief  Desktop/Mobile Sync: Desktop implementation
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

QGC_LOGGING_CATEGORY(QGCRemoteSync, "QGCRemoteSync")

static const char* kMissionExtension = ".plan";
static const char* kMissionWildCard  = "*.plan";

//-----------------------------------------------------------------------------
QGCSyncFilesDesktop::QGCSyncFilesDesktop(QObject* parent)
    : QObject(parent)
    , _logController(this)
    , _mapController(this)
    , _udpSocket(NULL)
    , _remoteNode(NULL)
    , _cancel(false)
    , _totalFiles(0)
    , _curFile(0)
    , _syncProgress(0)
    , _fileProgress(0)
    , _sendingFiles(false)
    , _syncDone(false)
    , _disconnecting(false)
    , _connecting(false)
    , _mapFile(NULL)
    , _lastMapExportProgress(0)
    , _importReplace(false)
{
    qmlRegisterUncreatableType<QGCSyncFilesDesktop>("QGroundControl", 1, 0, "QGCSyncFilesDesktop", "Reference only");
    //-- Start UDP listener
    _initUDPListener();
    connect(this, &QGCSyncFilesDesktop::completed,  this, &QGCSyncFilesDesktop::_completed);
    connect(this, &QGCSyncFilesDesktop::progress,   this, &QGCSyncFilesDesktop::_setSyncProgress);
    connect(this, &QGCSyncFilesDesktop::message,    this, &QGCSyncFilesDesktop::_message);
    connect(this, &QGCSyncFilesDesktop::missionToMobile,this, &QGCSyncFilesDesktop::_missionToMobile);
    connect(&_remoteMaintenanceTimer, &QTimer::timeout, this, &QGCSyncFilesDesktop::_remoteMaintenance);
    _remoteMaintenanceTimer.setSingleShot(false);
    //-- Every 15 seconds, clear stale remotes
    _remoteMaintenanceTimer.start(5000);
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
    _workerThread.quit();
    _workerThread.wait();
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
        qCDebug(QGCRemoteSync) << "UDP Broadcast receiver bound to:" << QGC_UDP_BROADCAST_PORT;
        QObject::connect(_udpSocket, &QUdpSocket::readyRead, this, &QGCSyncFilesDesktop::_readUDPBytes);
    } else {
        qWarning() << "Could not bind UDP port:" << QGC_UDP_BROADCAST_PORT;
    }
}

//-----------------------------------------------------------------------------
QByteArray
classinfo_signature(const QMetaObject *metaObject)
{
    static const QByteArray s_classinfoRemoteobjectSignature(QCLASSINFO_REMOTEOBJECT_SIGNATURE);
    if (!metaObject)
        return QByteArray{};
    for (int i = metaObject->classInfoOffset(); i < metaObject->classInfoCount(); ++i) {
        auto ci = metaObject->classInfo(i);
        if (s_classinfoRemoteobjectSignature == ci.name())
            return ci.value();
    }
    return QByteArray{};
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_readUDPBytes()
{
    static QString signature;
    if(signature.isEmpty()) {
        signature = classinfo_signature(&QGCRemoteReplica::staticMetaObject);
        qCWarning(QGCRemoteSync) << "Signature:" << signature;
    }
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
        QString payload = datagram.data();
        QStringList remoteIdentifier = payload.split("|");
        if(remoteIdentifier.size() == 2) {
            if(remoteIdentifier[1] == signature) {
                QString remoteName = remoteIdentifier[0];
                if(!_remoteURLs.contains(remoteName)) {
                    _remoteNames.append(remoteName);
                    _remoteURLs[remoteName] = url;
                    _remoteTimer[remoteName] = QTime();
                    _remoteTimer[remoteName].start();
                    qCDebug(QGCRemoteSync) << "New node:" << remoteName << url.toString();
                    emit remoteListChanged();
                } else {
                    //-- Restart keepalive timer
                    _remoteTimer[remoteName].restart();
                }
            } else {
                qCWarning(QGCRemoteSync) << "Ignored invalid node version:" << payload;
            }
        } else {
            qCWarning(QGCRemoteSync) << "Ignored invalid node:" << payload;
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::cancelSync()
{
    _message(QString(tr("Canceling...")));
    _cancel = true;
    emit canceledChanged();
    if(!_remoteObject.isNull()) {
        _remoteObject->setCancel(true);
    }
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
        _delayedDisconnect();
    }
    _disconnecting = false;
    _remoteNode = new QRemoteObjectNode(this);
    connect(_remoteNode, &QRemoteObjectNode::error, this, &QGCSyncFilesDesktop::_remoteObjError);
    qCDebug(QGCRemoteSync) << "Connect to" << _remoteURLs[name].toString();
    connect(_remoteNode, &QRemoteObjectNode::error, this, &QGCSyncFilesDesktop::_nodeError);
    if(_remoteNode->connectToNode(_remoteURLs[name])) {
        _remoteObject.reset(_remoteNode->acquire<QGCRemoteReplica>());
        if(_remoteObject.isNull()) {
            qCWarning(QGCRemoteSync) << "Error with replica acquisition.";
            return false;
        }
        connect(_remoteObject.data(), &QRemoteObjectReplica::stateChanged, this, &QGCSyncFilesDesktop::_stateChanged);
        connect(_remoteObject.data(), &QGCRemoteReplica::syncTypeChanged,  this, &QGCSyncFilesDesktop::_syncTypeChanged);
        connect(_remoteObject.data(), &QGCRemoteReplica::missionFromMobile,   this, &QGCSyncFilesDesktop::_missionFromMobile);
        connect(_remoteObject.data(), &QGCRemoteReplica::logFragment,  this, &QGCSyncFilesDesktop::_logFragment);
        connect(_remoteObject.data(), &QGCRemoteReplica::mapFragment,  this, &QGCSyncFilesDesktop::_mapFragmentFromMobile);
        _currentRemote = name;
        emit currentRemoteChanged();
        emit remoteReadyChanged();
        _remoteMaintenanceTimer.start(5000);
        _connecting = true;
        qCDebug(QGCRemoteSync) << "Connected";
        return true;
    }
    qCWarning(QGCRemoteSync) << "Connection error.";
    return false;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_remoteObjError(QRemoteObjectNode::ErrorCode errorCode)
{
    qCWarning(QGCRemoteSync) << "Connection error singaled:" << errorCode;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::disconnectRemote()
{
    if(_remoteNode) {
        if(!_remoteObject.isNull()) {
            //-- Tell remote we're bailing
            _remoteObject->setDesktopConnected(false);
            _disconnecting = true;
            emit remoteReadyChanged();
            QTimer::singleShot(500, this, &QGCSyncFilesDesktop::_delayedDisconnect);
        } else {
            //-- No one home. Bail now.
            _delayedDisconnect();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_delayedDisconnect()
{
    _remoteNode->deleteLater();
    _remoteNode = NULL;
    emit remoteReadyChanged();
    if(_currentLog.isOpen()) {
        _currentLog.close();
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
        _missionToMobile(name, saveDoc.toJson());
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
    _syncMessage.clear();
    _sendingFiles   = false;
    _syncDone       = false;
    _syncProgress   = 0;
    _cancel         = false;
    emit canceledChanged();
    emit sendingFilesChanged();
    emit syncDoneChanged();
    emit syncMessageChanged();
    emit syncProgressChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::initLogFetch()
{
    initSync();
    _logController.clearFileItems();
    _fileProgress = 0;
    emit fileProgressChanged();
    if(_currentLog.isOpen()) {
        _currentLog.close();
    }
    QList<QGCRemoteLogEntry> allLogs = _remoteObject->logEntriesOnMobile();
    foreach(QGCRemoteLogEntry logEntry, allLogs) {
        QGCFileListItem* item = new QGCFileListItem(&_logController, logEntry.name(), logEntry.size());
        _logController.appendFileItem(item);
    }
    std::sort(_logController.fileListV().begin(), _logController.fileListV().end(), [](QGCFileListItem* a, QGCFileListItem* b) { return a->fileName() > b->fileName(); });
    emit _logController.fileListChanged();
    qCDebug(QGCRemoteSync) << "Remote has" << allLogs.size() << "log entries";
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::initMapFetch()
{
    initSync();
    _fileProgress = 0;
    emit fileProgressChanged();
    //-- Init map engine
    qgcApp()->toolbox()->mapEngineManager()->loadTileSets();
    //-- Get remote tile set list
    QList<QGCSyncTileSet> allSets = _remoteObject->tileSetsOnMobile();
    foreach(QGCSyncTileSet set, allSets) {
        QGCFileListItem* item = new QGCFileListItem(&_mapController, set.name(), set.count());
        _mapController.appendFileItem(item);
    }
    std::sort(_mapController.fileListV().begin(), _mapController.fileListV().end(), [](QGCFileListItem* a, QGCFileListItem* b) { return a->fileName() < b->fileName(); });
    emit _mapController.fileListChanged();
    qCDebug(QGCRemoteSync) << "Remote has" << allSets.size() << "map tile entries";
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::uploadAllMissions()
{
    if(_sendingFiles) {
        return;
    }
    //-- Reset Status
    _sendingFiles   = true;
    _syncProgress   = 0;
    _syncDone       = false;
    emit syncProgressChanged();
    emit sendingFilesChanged();
    emit syncDoneChanged();
    if(!remoteReady()) {
        _message(QString(tr("Not Connected To Remote")));
        _completed();
        return;
    }
    QStringList missionNames;
    QStringList missionFiles;
    //-- Where missions are stored
    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    //-- Find all of them
    QDirIterator it(missionPath, QStringList() << kMissionWildCard, QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        missionFiles << fi.filePath();
        missionNames << fi.fileName();
    }
    if(!missionFiles.size()) {
        _message(QString(tr("No missions to send")));
        _completed();
        return;
    }
    _message(QString(tr("Sending mission files...")));
    //-- If cloning, prune extra files
    if(syncType() == SyncClone) {
        _remoteObject->pruneExtraMissionsOnMobile(missionNames);
    }
    //-- Start Worker Thread
    QGCMissionUploadWorker* missionWorker = new QGCMissionUploadWorker(this);
    missionWorker->moveToThread(&_workerThread);
    connect(this, &QGCSyncFilesDesktop::doMissionSync, missionWorker, &QGCMissionUploadWorker::doMissionSync);
    qCDebug(QGCRemoteSync) << "Starting log upload thread";
    _workerThread.start();
    emit doMissionSync(missionFiles);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::downloadAllMissions()
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
    QStringList allMissions = _remoteObject->missionsOnMobile();
    _curFile = 0;
    _totalFiles = allMissions.size();
    qCDebug(QGCRemoteSync) << "Requesting all mission files";
    qCDebug(QGCRemoteSync) << "Sync Type:" << syncType();
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
            qCDebug(QGCRemoteSync) << "Pruning extra mission:" << missionToZap;
            QFile f(missionToZap);
            f.remove();
        }
    }
    _remoteObject->requestMissionsFromMobile(allMissions);
}

//-----------------------------------------------------------------------------
bool
QGCSyncFilesDesktop::_prepareSync()
{
    if(_sendingFiles) {
        return false;
    }
    _sendingFiles   = true;
    _syncProgress   = 0;
    _syncDone       = false;
    emit syncProgressChanged();
    emit sendingFilesChanged();
    emit syncDoneChanged();
    if(!remoteReady()) {
        _message(QString(tr("Not Connected To Remote")));
        _completed();
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::downloadSelectedLogs(QString path)
{
    if(!_prepareSync()) {
        return;
    }
    //-- Target Path
    _logPath = path;
#ifdef Q_OS_WIN
    if(_logPath.startsWith("file:///")) _logPath.replace("file:///", "");
    if(_logPath.startsWith("file://")) _logPath.replace("file://", "");
    if(!_logPath.endsWith("\\") && !_logPath.endsWith("/")) _logPath += "/";
#else
    if(_logPath.startsWith("file://")) _logPath.replace("file://", "");
    if(!_logPath.endsWith("/")) _logPath += "/";
#endif
    QDir destDir(_logPath);
    if (!destDir.exists()) {
        if(!destDir.mkpath(".")) {
            _message(QString(tr("Error creating destination %1")).arg(_logPath));
            _completed();
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
    QString text = QString(tr("Requesing %1 log files").arg(requestedLogs.size()));
    _message(text);
    qCDebug(QGCRemoteSync) << text;
    _remoteObject->requestLogsFromMobile(requestedLogs);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::uploadSelectedTiles()
{
    if(!_prepareSync()) {
        return;
    }
    //-- Collect sets to export
    QGCMapEngineManager* mapMgr = qgcApp()->toolbox()->mapEngineManager();
    QmlObjectListModel& tileSets = (*mapMgr->tileSets());
    QVector<QGCCachedTileSet*> setsToExport;
    for(int i = 0; i < tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(tileSets.get(i));
        qCDebug(QGCRemoteSync) << "Testing" << set->name();
        if(set && set->selected()) {
            setsToExport.append(set);
        }
    }
    if(!setsToExport.size()) {
        _message(QString(tr("No tiles to send")));
        _completed();
        return;
    }
    //-- Temp file to save the exported set
    _mapFile = new QTemporaryFile;
    //-- If cannot create file, bail
    if (!_mapFile->open()) {
        _message(QString(tr("Error creating temporary map tile set")));
        _completed();
        return;
    }
    _mapFile->close();
    //-- Export selected sets
    _message(QString(tr("Exporting selected map tile sets")));
    _lastMapExportProgress = 0;
    QGCExportTileTask* task = new QGCExportTileTask(setsToExport, _mapFile->fileName());
    connect(task, &QGCExportTileTask::actionCompleted, this, &QGCSyncFilesDesktop::_mapExportDone);
    connect(task, &QGCExportTileTask::actionProgress, this, &QGCSyncFilesDesktop::_mapExportProgressChanged);
    connect(task, &QGCMapTask::error, this, &QGCSyncFilesDesktop::_mapExportError);
    getQGCMapEngine()->addTask(task);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_mapExportDone()
{
    //-- Start Worker Thread
    qCDebug(QGCRemoteSync) << "Starting map upload thread";
    QGCMapUploadWorker* mapWorker = new QGCMapUploadWorker(this);
    mapWorker->moveToThread(&_workerThread);
    connect(this, &QGCSyncFilesDesktop::doMapSync, mapWorker, &QGCMapUploadWorker::doMapSync);
    connect(mapWorker, &QGCMapUploadWorker::mapFragment, this, &QGCSyncFilesDesktop::_mapFragmentToMobile);
    _workerThread.start();
    //-- Let mobile know map tiles are comming and what to do with them
    _remoteObject->mapToMobile(_importReplace);
    emit doMapSync(_mapFile);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_mapExportError(QGCMapTask::TaskType, QString errorString)
{
    qWarning() << "Map export error:" << errorString;
    if(_mapFile) {
        delete _mapFile;
        _mapFile = NULL;
    }
    _message(QString(tr("Error exporting map tile sets")));
    _completed();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_mapExportProgressChanged(int percentage)
{
    //-- Progress from map engine can go over 100% some times
    if(_lastMapExportProgress != percentage && percentage <= 100) {
        _lastMapExportProgress = percentage;
        qCDebug(QGCRemoteSync) << "Map export progress" << percentage;
        _setSyncProgress(100, percentage);
    }
}

//-----------------------------------------------------------------------------
//-- Send map fragment on main thread
void
QGCSyncFilesDesktop::_mapFragmentToMobile(QGCMapFragment fragment)
{
    if(!_cancel && !_remoteObject.isNull()) {
        _remoteObject->mapFragmentToMobile(fragment);
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::downloadSelectedTiles()
{
    if(!_prepareSync()) {
        return;
    }
    //-- Build map tile set list
    QStringList requestedSets;
    for(int i = 0; i < _mapController.fileListV().size(); i++) {
        if(_mapController.fileListV()[i] && _mapController.fileListV()[i]->selected()) {
            requestedSets << _mapController.fileListV()[i]->fileName();
        }
    }
    //-- Request map tiles
    qCDebug(QGCRemoteSync) << "Requesting" <<  requestedSets.size() << "map tile sets";
    _message(QString(tr("Remote is exporting map tiles")));
    _remoteObject->requestMapTilesFromMobile(requestedSets);
}

//-----------------------------------------------------------------------------
bool
QGCSyncFilesDesktop::remoteReady()
{
    bool nodeReady = !_remoteObject.isNull() && _remoteObject->state() == QRemoteObjectReplica::Valid && !_disconnecting;
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
QGCSyncFilesDesktop::_stateChanged(QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState)
{
    qCDebug(QGCRemoteSync) << "State changed to" << state;
    emit remoteReadyChanged();
    if(oldState == QRemoteObjectReplica::Valid && state != QRemoteObjectReplica::Valid) {
        //-- Remote went kaput
        if(_remoteTimer.contains(_currentRemote)) {
            _remoteTimer.remove(_currentRemote);
            _remoteURLs.remove(_currentRemote);
            _remoteNames.removeOne(_currentRemote);
            emit remoteListChanged();
            _currentRemote.clear();
            emit currentRemoteChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_nodeError(QRemoteObjectNode::ErrorCode errorCode)
{
    qCDebug(QGCRemoteSync) << "Node error:" << errorCode;
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
bool
QGCSyncFilesDesktop::_missionToMobile(QString name, QByteArray mission)
{
    if(_remoteObject.isNull()) {
        return false;
    }
    if(!name.endsWith(kMissionExtension)) name += kMissionExtension;
    _remoteObject->missionToMobile(QGCNewMission(name, mission));
    return true;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_remoteMaintenance()
{
    //if(_remoteObject) {
    //    qDebug() << "State: " << _remoteObject->state();
    //}
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
    //-- Tell newly connected remote we're around
    if(remoteReady() && _connecting) {
        _remoteObject->setDesktopConnected(true);
        _connecting = false;
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
#ifdef Q_OS_WIN
    if(!missionFile.endsWith("/") && !missionFile.endsWith("\\")) missionFile += "\\";
#else
    if(!missionFile.endsWith("/")) missionFile += "/";
#endif
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
QGCSyncFilesDesktop::_missionFromMobile(QGCNewMission mission)
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
        qCDebug(QGCRemoteSync) << "Receiving:" << mission.name();
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
QGCSyncFilesDesktop::_logFragment(QGCLogFragment fragment)
{
    //-- Check for error
    if(fragment.name().isEmpty()) {
        _currentLog.close();
        _setSyncProgress(1, 1);
        _completed();
        _message(QString(tr("Remote Error")));
        return;
    }
    QString logFile = _logPath;
    logFile += fragment.name();
    if(_currentLog.isOpen()) {
        if(_currentLog.fileName() != logFile) {
            _currentLog.close();
        } else {
            if(fragment.data().size()) {
                _currentLog.write(fragment.data());
                _setFileProgress(fragment.total(), fragment.current());
            }
        }
    }
    if(!_currentLog.isOpen()) {
       _currentLog.setFileName(logFile);
       qCDebug(QGCRemoteSync) << "Receiving:" << logFile;
       if(!_cancel) {
           _message(QString(tr("Receiving %1")).arg(fragment.name()));
       }
       if (_currentLog.open(QIODevice::WriteOnly)) {
           _curFile++;
           _currentLog.write(fragment.data());
           _setSyncProgress(_totalFiles, _curFile);
           _setFileProgress(fragment.total(), fragment.current());
       } else {
           qWarning() << "Error creating" << logFile;
       }
    }
    //-- Check for end of sync
    if(_totalFiles <= _curFile && fragment.total() <= fragment.current()) {
        _currentLog.close();
        _setSyncProgress(1, 1);
        _completed();
        if(!_cancel) {
            _message(QString(tr("%1 files received")).arg(_totalFiles));
        } else {
            _message(QString(tr("Operation Canceled")));
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_mapFragmentFromMobile(QGCMapFragment fragment)
{
    //-- Check for cancel
    if(_cancel) {
        if(_mapFile) {
            delete _mapFile;
            _mapFile = NULL;
        }
        _message(QString(tr("Operation Canceled")));
        return;
    }
    //-- Check for non data
    if(fragment.current() == 0 && fragment.total() == 0) {
        //-- Check for progress
        if(fragment.data().size()) {
            _setSyncProgress(100, fragment.progress());
            return;
        }
        //-- Error
        if(_mapFile) {
            delete _mapFile;
            _mapFile = NULL;
        }
        _setSyncProgress(1, 1);
        _completed();
        _message(QString(tr("Remote Error")));
        return;
    }
    //-- Check for first fragment
    if(fragment.progress() == 0) {
        if(_mapFile) {
            delete _mapFile;
            _mapFile = NULL;
        }
        //-- Create temp file
        _mapFile = new QTemporaryFile;
        if(!_mapFile->open()) {
            qWarning() << "Error creating" << _mapFile->fileName();
            _message(QString(tr("Error creating incoming map file")));
            delete _mapFile;
            _mapFile = NULL;
            _setSyncProgress(1, 1);
            _completed();
            return;
        } else {
            //-- Temp file created
            _message(QString(tr("Receiving map data")));
            qCDebug(QGCRemoteSync) << "Receiving:" << _mapFile->fileName();
        }
    }
    if(_mapFile) {
        if(fragment.data().size()) {
           _mapFile->write(fragment.data());
           _setSyncProgress(fragment.total(), fragment.current());
        }
       //-- Check for end of file
       if(fragment.total() <= fragment.current()) {
           _mapFile->close();
           _setSyncProgress(1, 1);
           _message(QString(tr("Importing map data")));
           //-- Import map tiles
           QGCImportTileTask* task = new QGCImportTileTask(_mapFile->fileName(), qgcApp()->toolbox()->mapEngineManager()->importReplace());
           connect(task, &QGCImportTileTask::actionCompleted, this, &QGCSyncFilesDesktop::_mapImportCompleted);
           connect(task, &QGCImportTileTask::actionProgress, this, &QGCSyncFilesDesktop::_mapImportProgress);
           connect(task, &QGCMapTask::error, this, &QGCSyncFilesDesktop::_mapImportError);
           getQGCMapEngine()->addTask(task);
       }
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_mapImportProgress(int percentage)
{
    _setSyncProgress(100, percentage);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_mapImportError(QGCMapTask::TaskType, QString errorString)
{
    qWarning() << "Map import error:" << errorString;
    _completed();
    _message(QString(tr("Map import error")));
    if(_mapFile) {
        delete _mapFile;
        _mapFile = NULL;
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::_mapImportCompleted()
{
    if(_mapFile) {
        qCDebug(QGCRemoteSync) << "Map import complete";
        _completed();
        _message(QString(tr("Map tiles received")));
        delete _mapFile;
        _mapFile = NULL;
    }
}

//-----------------------------------------------------------------------------
void
QGCMissionUploadWorker::doMissionSync(QStringList missions)
{
    int curFile    = 0;
    int totalFiles = missions.size();
    emit _pSync->progress(totalFiles, 0);
    foreach(QString mission, missions) {
        if(_pSync->canceled()) {
            emit _pSync->message(QString(tr("Operation Canceled")));
            break;
        }
        QFileInfo fi(mission);
        QFile missionFile(mission);
        if (!missionFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open file" << mission;
        } else {
            QByteArray bytes = missionFile.readAll();
            missionFile.close();
            qCDebug(QGCRemoteSync) << "Sending:" << fi.filePath();
            emit _pSync->missionToMobile(fi.fileName(), bytes);
        }
        emit _pSync->progress(totalFiles, ++curFile);
    }
    if(!_pSync->canceled()) {
        emit _pSync->message(QString(tr("%1 files sent")).arg(totalFiles));
        emit _pSync->progress(1, 1);
    }
    emit _pSync->completed();
    this->deleteLater();
}

//-----------------------------------------------------------------------------
void
QGCMapUploadWorker::doMapSync(QTemporaryFile* mapFile)
{
    QString error;
    qCDebug(QGCRemoteSync) << "Map upload thread started";
    if (!mapFile) {
        error = QString(tr("Internal error. Map file not created."));
    } else {
        QFileInfo fi(mapFile->fileName());
        quint64 total = fi.size();
        if (!total) {
            qWarning() << "File is empty" << mapFile->fileName();
            error = QString(tr("Internal error. Exported map file is empty"));
        } else {
            QFile f(mapFile->fileName());
            if (!f.open(QIODevice::ReadOnly)) {
                qWarning() << "File open error" << mapFile->fileName();
                error = QString(tr("Unable to open exported map file"));
            } else {
                quint64 sofar = 0;
                int segment = 0;
                qCDebug(QGCRemoteSync) << "Uploading" << total << "bytes";
                emit _pSync->message(QString(tr("Uploading map tiles")));
                while(true) {
                    if(_pSync->canceled()) break;
                    //-- Send in 1M chuncks
                    QByteArray bytes = f.read(1024 * 1024);
                    if(bytes.size() != 0) {
                        sofar += bytes.size();
                        QGCMapFragment mapFrag(sofar, total, bytes, segment++);
                        emit mapFragment(mapFrag);
                    }
                    if(sofar >= total || bytes.size() == 0) {
                        break;
                    }
                }
            }
        }
    }
    if(_pSync->canceled()) {
        qCDebug(QGCRemoteSync) << "Thread canceled";
    } else if(!error.isEmpty()) {
        emit _pSync->message(error);
        QGCMapFragment mapFrag(0, 0, QByteArray(), 0);
        emit mapFragment(mapFrag);
    } else {
        emit _pSync->message(QString(tr("Upload completed")));
    }
    //-- We're done
    qCDebug(QGCRemoteSync) << "Map upload thread ended";
    emit _pSync->completed();
    this->deleteLater();
}
