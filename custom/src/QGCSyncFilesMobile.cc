/*!
 *   @brief Desktop/Mobile RPC
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "QGCSyncFilesMobile.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"

//-- TODO: This is here as it defines the UDP port and URL. It needs to go upstream
#include "TyphoonHQuickInterface.h"

QGC_LOGGING_CATEGORY(QGCSyncFiles, "QGCSyncFiles")

static const char* kMissionExtension = ".plan";
static const char* kMissionWildCard  = "*.plan";

//-----------------------------------------------------------------------------
QGCSyncFilesMobile::QGCSyncFilesMobile(QObject* parent)
    : QGCRemoteSimpleSource(parent)
    , _udpSocket(NULL)
    , _remoteObject(NULL)
    , _logWorker(NULL)
    , _mapWorker(NULL)
    , _mapFile(NULL)
    , _lastMapExportProgress(0)
{
    qmlRegisterUncreatableType<QGCSyncFilesMobile>("QGroundControl", 1, 0, "QGCSyncFilesMobile", "Reference only");
    connect(&_broadcastTimer, &QTimer::timeout, this, &QGCSyncFilesMobile::_broadcastPresence);
    connect(this, &QGCRemoteSimpleSource::cancelChanged, this, &QGCSyncFilesMobile::_canceled);
    QGCMapEngineManager* mapMgr = qgcApp()->toolbox()->mapEngineManager();
    connect(mapMgr, &QGCMapEngineManager::tileSetsChanged, this, &QGCSyncFilesMobile::_tileSetsChanged);
    _broadcastTimer.setSingleShot(false);
    _updateMissionsOnMobile();
    _updateLogEntriesOnMobile();
    mapMgr->loadTileSets();
    //-- Start UDP broadcast
    _broadcastTimer.start(5000);
    //-- Initialize Remote Object
    QUrl url;
    url.setHost(QString("0.0.0.0"));
    url.setPort(QGC_RPC_PORT);
    url.setScheme("tcp");
    qCDebug(QGCSyncFiles) << "Remote Object URL:" << url.toString();
    _remoteObject = new QRemoteObjectHost(url);
    _remoteObject->enableRemoting(this);
    //-- TODO: Connect to vehicle and check when it's disarmed. Update log entries.
    //-- TODO: Find better way to determine if we are connected to the desktop
}

//-----------------------------------------------------------------------------
QGCSyncFilesMobile::~QGCSyncFilesMobile()
{
    if(_udpSocket) {
        _udpSocket->deleteLater();
    }
    _workerThread.quit();
    _workerThread.wait();
    if(_remoteObject) {
        _remoteObject->deleteLater();
    }
}

//-----------------------------------------------------------------------------
bool
QGCSyncFilesMobile::_processIncomingMission(QString name, int count, QString& missionFile)
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
//-- Slot for Desktop missionToMobile
void
QGCSyncFilesMobile::missionToMobile(QGCNewMission mission)
{
    QString missionFile;
    int count = 0;
    //-- If we are appending, we need to make sure not to overwrite
    do {
        if(!_processIncomingMission(mission.name(), count++, missionFile)) {
            break;
        }
    } while(syncType() == SyncAppend);
    qCDebug(QGCSyncFiles) << "Receiving:" << missionFile;
    qCDebug(QGCSyncFiles) << "Sync Type:" << syncType();
    QFile file(missionFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(mission.mission());
        _updateMissionsOnMobile();
    } else {
        qWarning() << "Error writing" << missionFile;
    }
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop missionToMobile
void
QGCSyncFilesMobile::mapToMobile(QGCMapFragment fragment)
{
    //-- Check for cancel
    if(cancel()) {
        if(_mapFile) {
            delete _mapFile;
            _mapFile = NULL;
        }
        qCDebug(QGCSyncFiles) << "Operation Canceled";
        return;
    }
    //-- Check for non data
    if(fragment.current() == 0 && fragment.total() == 0) {
        //-- Check for progress
        if(fragment.data().size()) {
            return;
        }
        //-- Error
        if(_mapFile) {
            delete _mapFile;
            _mapFile = NULL;
        }
        qCDebug(QGCSyncFiles) << "Remote Error";
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
            qCWarning(QGCSyncFiles) << "Error creating" << _mapFile->fileName();
            delete _mapFile;
            _mapFile = NULL;
            return;
        } else {
            //-- Temp file created
            qCDebug(QGCSyncFiles) << "Receiving:" << _mapFile->fileName();
        }
    }
    if(_mapFile) {
        if(fragment.data().size()) {
           _mapFile->write(fragment.data());
        }
       //-- Check for end of file
       if(fragment.total() <= fragment.current()) {
           _mapFile->close();
           qCDebug(QGCSyncFiles) << "Importing map data";
           //-- Import map tiles
           QGCImportTileTask* task = new QGCImportTileTask(_mapFile->fileName(), qgcApp()->toolbox()->mapEngineManager()->importReplace());
           connect(task, &QGCImportTileTask::actionCompleted, this, &QGCSyncFilesMobile::_mapImportCompleted);
           connect(task, &QGCMapTask::error, this, &QGCSyncFilesMobile::_mapImportError);
           getQGCMapEngine()->addTask(task);
       }
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_mapImportError(QGCMapTask::TaskType, QString errorString)
{
    qWarning() << "Map import error:" << errorString;
    if(_mapFile) {
        delete _mapFile;
        _mapFile = NULL;
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_mapImportCompleted()
{
    if(_mapFile) {
        qCDebug(QGCSyncFiles) << "Map import complete";
        delete _mapFile;
        _mapFile = NULL;
    }
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop pruneMission (Clone)
void
QGCSyncFilesMobile::pruneExtraMissionsOnMobile(QStringList allMissions)
{
    QStringList missionsToPrune;
    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    QDirIterator it(missionPath, QStringList() << kMissionWildCard, QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        if(!allMissions.contains(fi.fileName())) {
            missionsToPrune << fi.filePath();
        }
    }
    foreach(QString missionFile, missionsToPrune) {
        qCDebug(QGCSyncFiles) << "Pruning extra mission:" << missionFile;
        QFile f(missionFile);
        f.remove();
    }
    _updateMissionsOnMobile();
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop mission request
void
QGCSyncFilesMobile::requestMissionsFromMobile(QStringList missions)
{
    setCancel(false);
    QStringList missionsToSend;
    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    QDirIterator it(missionPath, QStringList() << kMissionWildCard, QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QCoreApplication::processEvents();
        if(cancel()) return;
        QFileInfo fi(it.next());
        if(missions.contains(fi.fileName())) {
            missionsToSend << fi.filePath();
        }
    }
    foreach(QString missionFile, missionsToSend) {
        QCoreApplication::processEvents();
        if(cancel()) return;
        qCDebug(QGCSyncFiles) << "Sending mission:" << missionFile;
        QFileInfo fi(missionFile);
        QFile f(missionFile);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open file" << missionFile;
            QGCNewMission mission(fi.fileName(), QByteArray());
            emit missionFromMobile(mission);
        } else {
            QByteArray bytes = f.readAll();
            f.close();
            QGCNewMission mission(fi.fileName(), bytes);
            emit missionFromMobile(mission);
        }
    }
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop log request
void
QGCSyncFilesMobile::requestLogsFromMobile(QStringList logs)
{
    qCDebug(QGCSyncFiles) << "Log Request";
    QStringList logsToSend;
    QString logPath = qgcApp()->toolbox()->settingsManager()->appSettings()->telemetrySavePath();
    QDirIterator it(logPath, QStringList() << "*.tlog", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        if(logs.contains(fi.fileName())) {
            logsToSend << fi.filePath();
            qCDebug(QGCSyncFiles) << "Request" << fi.filePath();
        }
    }
    //-- If nothing to send or worker thread is still up for some reason, bail
    if(!logsToSend.size() || _logWorker) {
        qCDebug(QGCSyncFiles) << "Nothing to send";
        QGCLogFragment logFrag(QString(), 0, 0, QByteArray());
        _logFragment(logFrag);
        return;
    }
    //-- Start Worker Thread
    setCancel(false);
    _logWorker = new QGCLogUploadWorker(this);
    _logWorker->moveToThread(&_workerThread);
    connect(this, &QGCSyncFilesMobile::doLogSync, _logWorker, &QGCLogUploadWorker::doLogSync);
    connect(_logWorker, &QGCLogUploadWorker::logFragment, this, &QGCSyncFilesMobile::_logFragment);
    connect(_logWorker, &QGCLogUploadWorker::done, this, &QGCSyncFilesMobile::_logWorkerDone);
    qCDebug(QGCSyncFiles) << "Starting log upload thread";
    _workerThread.start();
    emit doLogSync(logsToSend);
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop map request
void
QGCSyncFilesMobile::requestMapTilesFromMobile(QStringList sets)
{
    qCDebug(QGCSyncFiles) << "Map Request";
    foreach(QString setName, sets) {
        qCDebug(QGCSyncFiles) << "Requesting" << setName;
    }
    QGCMapEngineManager* mapMgr = qgcApp()->toolbox()->mapEngineManager();
    QmlObjectListModel& tileSets = (*mapMgr->tileSets());
    //-- Collect sets to export
    QVector<QGCCachedTileSet*> setsToExport;
    for(int i = 0; i < tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(tileSets.get(i));
        qCDebug(QGCSyncFiles) << "Testing" << set->name();
        if(set && sets.contains(set->name())) {
            setsToExport.append(set);
        }
    }
    //-- Temp file to save the exported set
    _mapFile = new QTemporaryFile;
    //-- If cannot create file, there is nothing to send or worker thread is still up for some reason, bail
    if (!_mapFile->open() || !setsToExport.size() || _mapWorker) {
        if(!setsToExport.size()) {
            qCDebug(QGCSyncFiles) << "Nothing to send";
        } else if (_mapWorker) {
            qCDebug(QGCSyncFiles) << "Worker thread still busy";
        } else {
            qCDebug(QGCSyncFiles) << "Error creating temp map export file" << _mapFile->fileName();
        }
        QGCMapFragment logFrag(0, 0, QByteArray(), 0);
        _mapFragment(logFrag);
        delete _mapFile;
        _mapFile = NULL;
        return;
    }
    _mapFile->close();
    //-- Start Worker Thread
    setCancel(false);
    _mapWorker = new QGCMapUploadWorker(this);
    _mapWorker->moveToThread(&_workerThread);
    connect(this, &QGCSyncFilesMobile::doMapSync, _mapWorker, &QGCMapUploadWorker::doMapSync);
    connect(_mapWorker, &QGCMapUploadWorker::mapFragment, this, &QGCSyncFilesMobile::_mapFragment);
    connect(_mapWorker, &QGCMapUploadWorker::done, this, &QGCSyncFilesMobile::_mapWorkerDone);
    //-- Request map export
    _lastMapExportProgress = 0;
    QGCExportTileTask* task = new QGCExportTileTask(setsToExport, _mapFile->fileName());
    connect(task, &QGCExportTileTask::actionCompleted, this, &QGCSyncFilesMobile::_mapExportDone);
    connect(task, &QGCExportTileTask::actionProgress, this, &QGCSyncFilesMobile::_mapExportProgressChanged);
    connect(task, &QGCMapTask::error, this, &QGCSyncFilesMobile::_mapExportError);
    getQGCMapEngine()->addTask(task);
    qCDebug(QGCSyncFiles) << "Exporting map set to" << _mapFile->fileName();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_mapExportError(QGCMapTask::TaskType, QString errorString)
{
    qWarning() << "Map export error:" << errorString;
    if(_mapFile) {
        QGCMapFragment logFrag(0, 0, QByteArray(), 0);
        _mapFragment(logFrag);
        delete _mapFile;
        _mapFile = NULL;
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_mapExportDone()
{
    if(_mapFile) {
        qCDebug(QGCSyncFiles) << "Starting map upload thread";
        _workerThread.start();
        emit doMapSync(_mapFile);
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_mapExportProgressChanged(int percentage)
{
    //-- Progress from map engine can go over 100% some times
    if(_mapFile && _lastMapExportProgress != percentage && percentage <= 100) {
        _lastMapExportProgress = percentage;
        qCDebug(QGCSyncFiles) << "Map export progress" << percentage;
        QGCMapFragment mapFrag(0, 0, QByteArray(1,'1'), percentage);
        _mapFragment(mapFrag);
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_logWorkerDone()
{
    _logWorker->deleteLater();
    _logWorker = NULL;
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_mapWorkerDone()
{
    qCDebug(QGCSyncFiles) << "Destroying map upload thread";
    _mapWorker->deleteLater();
    _mapWorker = NULL;
    if(_mapFile) {
        delete _mapFile;
        _mapFile = NULL;
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_canceled(bool cancel)
{
    //-- For some stupid reason, connecting the cancelChanged signal directly
    //   to the worker thread ain't working.
    if(cancel) {
        qDebug() << "Canceled from Desktop";
    }
}

//-----------------------------------------------------------------------------
//-- Send log fragment on main thread
void
QGCSyncFilesMobile::_logFragment(QGCLogFragment fragment)
{
    if(!cancel()) {
        emit logFragment(fragment);
    }
}

//-----------------------------------------------------------------------------
//-- Send map fragment on main thread
void
QGCSyncFilesMobile::_mapFragment(QGCMapFragment fragment)
{
    if(!cancel()) {
        emit mapFragment(fragment);
    }
}

//-----------------------------------------------------------------------------
//-- Log upload thread
void
QGCLogUploadWorker::doLogSync(QStringList logsToSend)
{
    qCDebug(QGCSyncFiles) << "Log upload thread started with" << logsToSend.size() << "logs to upload";
    foreach(QString logFile, logsToSend) {
        if(_pSync->cancel()) break;
        qCDebug(QGCSyncFiles) << "Sending log:" << logFile;
        QFileInfo fi(logFile);
        QFile f(logFile);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open file" << logFile;
            QGCLogFragment logFrag(fi.fileName(), 0, 0, QByteArray());
            emit logFragment(logFrag);
            break;
        } else {
            quint64 sofar = 0;
            quint64 total = fi.size();
            while(true) {
                if(_pSync->cancel()) break;
                //-- Send in 1M chuncks
                QByteArray bytes = f.read(1024 * 1024);
                if(bytes.size() != 0) {
                    sofar += bytes.size();
                    QGCLogFragment logFrag(fi.fileName(), sofar, total, bytes);
                    emit logFragment(logFrag);
                }
                if(sofar >= total || bytes.size() == 0) {
                    break;
                }
            }
        }
    }
    if(_pSync->cancel()) {
        qCDebug(QGCSyncFiles) << "Thread canceled";
    }
    //-- We're done
    emit done();
}

//-----------------------------------------------------------------------------
//-- Map upload thread
void
QGCMapUploadWorker::doMapSync(QTemporaryFile* mapFile)
{
    bool error = true;
    qCDebug(QGCSyncFiles) << "Map upload thread started";
    if (!mapFile) {
        qWarning() << "Map file not created";
    } else {
        QFileInfo fi(mapFile->fileName());
        quint64 total = fi.size();
        if (!total) {
            qWarning() << "File is empty" << mapFile->fileName();
        } else {
            QFile f(mapFile->fileName());
            if (!f.open(QIODevice::ReadOnly)) {
                qWarning() << "Unable to open map file" << mapFile->fileName();
            } else {
                quint64 sofar = 0;
                int segment = 0;
                qCDebug(QGCSyncFiles) << "Uploading" << total << "bytes";
                error = false;
                while(true) {
                    if(_pSync->cancel()) break;
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
    if(_pSync->cancel()) {
        qCDebug(QGCSyncFiles) << "Thread canceled";
    } else if(error) {
        QGCMapFragment mapFrag(0, 0, QByteArray(), 0);
        emit mapFragment(mapFrag);
    }
    //-- We're done
    qCDebug(QGCSyncFiles) << "Map upload thread ended";
    emit done();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_broadcastPresence()
{
    //-- Mobile builds will broadcast their presence every 5 seconds so Desktop builds
    //   can find it.
    if(!_udpSocket) {
        _udpSocket = new QUdpSocket(this);
    }
    if(_macAddress.isEmpty()) {
        QUrl url;
        //-- Get first interface with a MAC address
        foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces()) {
            _macAddress = interface.hardwareAddress();
            if(!_macAddress.isEmpty() && !_macAddress.endsWith("00:00:00")) {
                break;
            }
        }
        if(_macAddress.length() > 9) {
            //-- Got one
            _macAddress = _macAddress.mid(9);
            _macAddress.replace(":", "");
        } else {
            //-- Make something up
            _macAddress.sprintf("%06d", (qrand() % 999999));
            qWarning() << "Could not get a proper MAC address. Using a random value.";
        }
        _macAddress = QGC_MOBILE_NAME + _macAddress;
        emit macAddressChanged();
        qCDebug(QGCSyncFiles) << "MAC Address:" << _macAddress;
    }
    _udpSocket->writeDatagram(_macAddress.toLocal8Bit(), QHostAddress::Broadcast, QGC_UDP_BROADCAST_PORT);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_updateMissionsOnMobile()
{
    QStringList missions;
    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    QDirIterator it(missionPath, QStringList() << kMissionWildCard, QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        missions << fi.fileName();
    }
    setMissionsOnMobile(missions);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_updateLogEntriesOnMobile()
{
    QList<QGCRemoteLogEntry> logs;
    QString logPath = qgcApp()->toolbox()->settingsManager()->appSettings()->telemetrySavePath();
    QDirIterator it(logPath, QStringList() << "*.tlog", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        QGCRemoteLogEntry l(fi.fileName(), fi.size());
        logs.append(l);
    }
    setLogEntriesOnMobile(logs);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_tileSetsChanged()
{
    QList<QGCSyncTileSet> sets;
    QGCMapEngineManager* mapMgr = qgcApp()->toolbox()->mapEngineManager();
    QmlObjectListModel&  tileSets = (*mapMgr->tileSets());
    for(int i = 0; i < tileSets.count(); i++ ) {
        QGCCachedTileSet* set = qobject_cast<QGCCachedTileSet*>(tileSets.get(i));
        if(set) {
            QGCSyncTileSet s(set->name(), set->totalTileCount(), set->totalTilesSize());
            sets.append(s);
        }
    }
    setTileSetsOnMobile(sets);
}
