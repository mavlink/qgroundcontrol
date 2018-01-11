/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "QGCFileListController.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngineManager.h"

#include <QtRemoteObjects>
#include <QThread>

//-- This is built at compile time from QGCRemote.rep (full of unused variable warnings)
#include "rep_QGCRemote_replica.h"

Q_DECLARE_LOGGING_CATEGORY(QGCSyncFiles)

class PlanMasterController;
class QGCSyncFilesDesktop;

//-----------------------------------------------------------------------------
class QGCMapUploadWorker : public QObject
{
    Q_OBJECT
public:
    QGCMapUploadWorker(QGCSyncFilesDesktop* parent) : _pSync(parent) {}
public slots:
    void doMapSync      (QTemporaryFile* mapFile);
signals:
    void mapFragment(QGCMapFragment fragment);
    void done           ();
private:
    QGCSyncFilesDesktop* _pSync;
};

//-----------------------------------------------------------------------------
class QGCMissionUploadWorker : public QObject
{
    Q_OBJECT
public:
    QGCMissionUploadWorker(QGCSyncFilesDesktop* parent) : _pSync(parent) {}
public slots:
    void doMissionSync  (QStringList missions);
private:
    QGCSyncFilesDesktop* _pSync;
};

//-----------------------------------------------------------------------------
class QGCSyncFilesDesktop : public QObject
{
    Q_OBJECT
public:
    QGCSyncFilesDesktop                 (QObject* parent);
    ~QGCSyncFilesDesktop                ();

    Q_PROPERTY(QStringList          remoteList      READ    remoteList      NOTIFY remoteListChanged)
    Q_PROPERTY(bool                 remoteReady     READ    remoteReady     NOTIFY remoteReadyChanged)
    Q_PROPERTY(QString              currentRemote   READ    currentRemote   WRITE  setCurrentRemote     NOTIFY currentRemoteChanged)
    Q_PROPERTY(bool                 sendingFiles    READ    sendingFiles    NOTIFY sendingFilesChanged)
    Q_PROPERTY(bool                 syncDone        READ    syncDone        NOTIFY syncDoneChanged)
    Q_PROPERTY(bool                 canceled        READ    canceled        NOTIFY canceledChanged)
    Q_PROPERTY(QString              syncMessage     READ    syncMessage     WRITE  setSyncMessage       NOTIFY syncMessageChanged)
    Q_PROPERTY(int                  syncProgress    READ    syncProgress    NOTIFY syncProgressChanged)
    Q_PROPERTY(int                  fileProgress    READ    fileProgress    NOTIFY fileProgressChanged)
    Q_PROPERTY(QGCFileListController* logController READ    logController   CONSTANT)
    Q_PROPERTY(QGCFileListController* mapController READ    mapController   CONSTANT)

    //-- Connect to remote node
    Q_INVOKABLE bool connectToRemote    (QString name);
    //-- Disconnect remote node
    Q_INVOKABLE void disconnectRemote   ();
    //-- Init sync state
    Q_INVOKABLE void initSync           ();
    //-- Upload current mission (Plan View)
    Q_INVOKABLE void uploadMission      (QString name, PlanMasterController* controller);
    //-- Upload all local mission files
    Q_INVOKABLE void uploadAllMissions  ();
    //-- Cancel sync thread
    Q_INVOKABLE void cancelSync         ();
    //-- Download all remote mission files
    Q_INVOKABLE void downloadAllMissions();
    //-- Download selected logs
    Q_INVOKABLE void downloadSelectedLogs(QString path);
    //-- Init log fetch state
    Q_INVOKABLE void initLogFetch       ();
    //-- Init map fetch state
    Q_INVOKABLE void initMapFetch       ();
    //-- Upload selected map tile sets
    Q_INVOKABLE void uploadSelectedTiles();
    //-- Download selected map tile sets
    Q_INVOKABLE void downloadSelectedTiles();

    QStringList remoteList              () { return _remoteNames; }
    bool        remoteReady             ();
    QString     currentRemote           () { return _currentRemote; }
    void        setCurrentRemote        (QString remote) { _currentRemote = remote; emit currentRemoteChanged(); }
    bool        sendingFiles            () { return _sendingFiles; }
    bool        syncDone                () { return _syncDone; }
    bool        canceled                () { return _cancel; }
    QString     syncMessage             () { return _syncMessage; }
    void        setSyncMessage          (QString message) { _syncMessage = message; emit syncMessageChanged(); }
    int         syncProgress            () { return _syncProgress; }
    int         fileProgress            () { return _fileProgress; }

    QGCFileListController* logController() { return &_logController; }
    QGCFileListController* mapController() { return &_mapController; }

public:
    //-------------------------------------------------------------------------
    //-- From QGCRemote

    enum SyncType {
        SyncClone = 0,
        SyncReplace = 1,
        SyncAppend = 2,
    };

    Q_ENUM(SyncType)

    Q_PROPERTY(SyncType             syncType        READ    syncType        WRITE setSyncType NOTIFY syncTypeChanged)

    SyncType    syncType                ();
    void        setSyncType             (SyncType type);

signals:
    //-- To QML
    void    remoteListChanged           ();
    void    remoteReadyChanged          ();
    void    currentRemoteChanged        ();
    void    sendingFilesChanged         ();
    void    syncDoneChanged             ();
    void    canceledChanged             ();
    void    syncMessageChanged          ();
    void    syncProgressChanged         ();
    void    fileProgressChanged         ();
    void    syncTypeChanged             ();
    void    selectedCountChanged        ();
    //-- Thread
    void    doMissionSync               (QStringList missionsToSend);
    void    doMapSync                   (QTemporaryFile* mapFile);
    void    progress                    (quint32 totalCount, quint32 curCount);
    void    completed                   ();
    void    message                     (QString errorMessage);
    void    missionToMobile             (QString name, QByteArray mission);

private slots:
    void    _stateChanged               (QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState);
    void    _nodeError                  (QRemoteObjectNode::ErrorCode errorCode);
    void    _readUDPBytes               ();
    void    _remoteMaintenance          ();
    void    _syncTypeChanged            (QGCRemoteReplica::SyncType syncType);
    void    _delayedDisconnect          ();
    void    _mapFragmentFromMobile      (QGCMapFragment fragment);
    void    _mapImportCompleted         ();
    void    _mapImportProgress          (int percentage);
    void    _mapImportError             (QGCMapTask::TaskType type, QString errorString);
    void    _missionFromMobile          (QGCNewMission mission);
    void    _logFragment                (QGCLogFragment fragment);
    void    _mapExportDone              ();
    void    _mapExportProgressChanged   (int percentage);
    void    _mapExportError             (QGCMapTask::TaskType type, QString errorString);
    //-- From Thread
    void    _setSyncProgress            (quint32 total, quint32 current);
    void    _setFileProgress            (quint32 total, quint32 current);
    void    _message                    (QString message);
    void    _completed                  ();
    bool    _missionToMobile            (QString name, QByteArray mission);
    void    _mapFragmentToMobile        (QGCMapFragment fragment);

private:
    void    _initUDPListener            ();
    bool    _prepareSync                ();
    bool    _processIncomingMission     (QString name, int count, QString& missionFile);

private:
    QGCFileListController               _logController;
    QGCFileListController               _mapController;
    QUdpSocket*                         _udpSocket;
    QSharedPointer<QGCRemoteReplica>    _remoteObject;
    QRemoteObjectNode*                  _remoteNode;
    QStringList                         _remoteNames;
    QString                             _currentRemote;
    QMap<QString, QUrl>                 _remoteURLs;
    QMap<QString, QTime>                _remoteTimer;
    bool                                _cancel;
    quint32                             _totalFiles;
    quint32                             _curFile;
    quint32                             _syncProgress;
    quint32                             _fileProgress;
    QString                             _syncMessage;
    bool                                _sendingFiles;
    bool                                _syncDone;
    bool                                _disconnecting;
    bool                                _connecting;
    QTimer                              _remoteMaintenanceTimer;
    QThread                             _workerThread;
    QString                             _logPath;
    QFile                               _currentLog;
    QTemporaryFile*                     _mapFile;
    int                                 _lastMapExportProgress;
};
