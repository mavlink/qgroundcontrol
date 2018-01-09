/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QtRemoteObjects>
#include <QThread>
#include "QGCLoggingCategory.h"

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
    Q_PROPERTY(QString              syncMessage     READ    syncMessage     NOTIFY syncMessageChanged)
    Q_PROPERTY(int                  syncProgress    READ    syncProgress    NOTIFY syncProgressChanged)

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
    Q_INVOKABLE void downloadSelectedLogs();

    QStringList remoteList              () { return _remoteNames; }
    bool        remoteReady             ();
    QString     currentRemote           () { return _currentRemote; }
    void        setCurrentRemote        (QString remote) { _currentRemote = remote; emit currentRemoteChanged(); }
    bool        sendingFiles            () { return _sendingFiles; }
    bool        syncDone                () { return _syncDone; }
    QString     syncMessage             () { return _syncMessage; }
    int         syncProgress            () { return _syncProgress; }

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
    void    syncMessageChanged          ();
    void    syncProgressChanged         ();
    void    syncTypeChanged             ();
    //-- From Thread
    void    progress                    (quint32 totalCount, quint32 curCount);
    void    completed                   ();
    void    message                     (QString errorMessage);
    void    sendMission                 (QString name, QByteArray mission);

private slots:
    void    _stateChanged               (QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState);
    void    _nodeError                  (QRemoteObjectNode::ErrorCode errorCode);
    void    _readUDPBytes               ();
    void    _remoteMaintenance          ();
    //-- From Thread
    void    _progress                   (quint32 total, quint32 current);
    void    _message                    (QString message);
    void    _completed                  ();
    bool    _sendMission                (QString name, QByteArray mission);
    void    _syncTypeChanged            (QGCRemoteReplica::SyncType syncType);
    void    _receiveMission             (QGCNewMission mission);

private:
    void    _initUDPListener            ();
    bool    _doSync                     ();
    bool    _processIncomingMission     (QString name, int count, QString& missionFile);

private:
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
    QString                             _syncMessage;
    bool                                _sendingFiles;
    bool                                _syncDone;
    QStringList                         _missions;
    QTimer                              _remoteMaintenanceTimer;
};
