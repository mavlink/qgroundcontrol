/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "QGCFileListController.h"
#include "QGCLoggingCategory.h"

#include <QtRemoteObjects>
#include <QThread>

//-- This is built at compile time from QGCRemote.rep (full of unused variable warnings)
#include "rep_QGCRemote_replica.h"

Q_DECLARE_LOGGING_CATEGORY(QGCSyncFiles)

class PlanMasterController;

//-----------------------------------------------------------------------------
class QGCSyncFilesDesktop : public QThread
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
    Q_PROPERTY(QString              syncMessage     READ    syncMessage     NOTIFY syncMessageChanged)
    Q_PROPERTY(int                  syncProgress    READ    syncProgress    NOTIFY syncProgressChanged)
    Q_PROPERTY(int                  fileProgress    READ    fileProgress    NOTIFY fileProgressChanged)
    Q_PROPERTY(QGCFileListController* logController READ    logController   CONSTANT)

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

    QStringList remoteList              () { return _remoteNames; }
    bool        remoteReady             ();
    QString     currentRemote           () { return _currentRemote; }
    void        setCurrentRemote        (QString remote) { _currentRemote = remote; emit currentRemoteChanged(); }
    bool        sendingFiles            () { return _sendingFiles; }
    bool        syncDone                () { return _syncDone; }
    bool        canceled                () { return _cancel; }
    QString     syncMessage             () { return _syncMessage; }
    int         syncProgress            () { return _syncProgress; }
    int         fileProgress            () { return _fileProgress; }

    QGCFileListController* logController() { return &_logController; }

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

protected:
    void    run                         ();

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
    //-- From Thread
    void    progress                    (quint32 totalCount, quint32 curCount);
    void    completed                   ();
    void    message                     (QString errorMessage);
    void    sendMission                 (QString name, QByteArray mission);
    void    selectedCountChanged        ();

private slots:
    void    _stateChanged               (QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState);
    void    _nodeError                  (QRemoteObjectNode::ErrorCode errorCode);
    void    _readUDPBytes               ();
    void    _remoteMaintenance          ();
    //-- From Thread
    void    _setSyncProgress            (quint32 total, quint32 current);
    void    _setFileProgress            (quint32 total, quint32 current);
    void    _message                    (QString message);
    void    _completed                  ();
    bool    _sendMission                (QString name, QByteArray mission);
    void    _syncTypeChanged            (QGCRemoteReplica::SyncType syncType);
    void    _receiveMission             (QGCNewMission mission);
    void    _sendLogFragment            (QGCLogFragment fragment);
    void    _delayedDisconnect          ();

private:
    void    _initUDPListener            ();
    bool    _doSync                     ();
    bool    _processIncomingMission     (QString name, int count, QString& missionFile);

private:
    QGCFileListController               _logController;
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
    QStringList                         _missions;
    QTimer                              _remoteMaintenanceTimer;
    //-- Fetch Logs
    QString                             _logPath;
    QFile                               _currentLog;
};
