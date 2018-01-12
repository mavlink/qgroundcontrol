/*!
 *   @brief Desktop/Mobile RPC
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QtRemoteObjects>
#include "QGCLoggingCategory.h"
#include "QGCMapEngineManager.h"

//-- This is built at compile time from QGCRemote.rep
#include "rep_QGCRemote_source.h"

Q_DECLARE_LOGGING_CATEGORY(QGCSyncFiles)

class QGCSyncFilesMobile;

//-----------------------------------------------------------------------------
class QGCLogUploadWorker : public QObject
{
    Q_OBJECT
public:
    QGCLogUploadWorker(QGCSyncFilesMobile* parent) : _pSync(parent) {}
public slots:
    void doLogSync      (QStringList logsToSend);
signals:
    void logFragment(QGCLogFragment fragment);
    void done           ();
private:
    QGCSyncFilesMobile* _pSync;
};

//-----------------------------------------------------------------------------
class QGCMapUploadWorker : public QObject
{
    Q_OBJECT
public:
    QGCMapUploadWorker(QGCSyncFilesMobile* parent) : _pSync(parent) {}
public slots:
    void doMapSync      (QTemporaryFile* mapFile);
signals:
    void mapFragment(QGCMapFragment fragment);
    void done           ();
private:
    QGCSyncFilesMobile* _pSync;
};

//-----------------------------------------------------------------------------
class QGCSyncFilesMobile : public QGCRemoteSimpleSource
{
    Q_OBJECT
public:
    QGCSyncFilesMobile                  (QObject* parent = NULL);
    virtual ~QGCSyncFilesMobile         ();

    Q_PROPERTY(QString remoteIdentifier READ remoteIdentifier NOTIFY remoteIdentifierChanged)

    QString remoteIdentifier            () { return _remoteIdentifier; }

public slots:
    void    missionToMobile             (QGCNewMission mission);
    void    mapToMobile                 (bool importReplace);
    void    mapFragmentToMobile         (QGCMapFragment fragment);
    void    pruneExtraMissionsOnMobile  (QStringList allMissions);
    void    requestMissionsFromMobile   (QStringList missions);
    void    requestLogsFromMobile       (QStringList logs);
    void    requestMapTilesFromMobile   (QStringList sets);

private slots:
    void    _broadcastPresence          ();
    void    _logFragment                (QGCLogFragment fragment);
    void    _mapFragment                (QGCMapFragment fragment);
    void    _canceled                   (bool cancel);
    void    _logWorkerDone              ();
    void    _mapWorkerDone              ();
    void    _tileSetsChanged            ();
    void    _mapExportDone              ();
    void    _mapExportProgressChanged   (int percentage);
    void    _mapExportError             (QGCMapTask::TaskType type, QString errorString);
    void    _mapImportCompleted         ();
    void    _mapImportError             (QGCMapTask::TaskType type, QString errorString);

signals:
    void    remoteIdentifierChanged     ();
    void    doLogSync                   (QStringList logsToSend);
    void    doMapSync                   (QTemporaryFile* mapFile);
    void    cancelFromDesktop           ();

private:
    bool    _processIncomingMission     (QString name, int count, QString& missionFile);
    void    _updateMissionsOnMobile     ();
    void    _updateLogEntriesOnMobile   ();

private:
    QTimer                  _broadcastTimer;
    QString                 _remoteIdentifier;
    QUdpSocket*             _udpSocket;
    QRemoteObjectHost*      _remoteObject;
    QThread                 _workerThread;
    QGCLogUploadWorker*     _logWorker;
    QGCMapUploadWorker*     _mapWorker;
    QTemporaryFile*         _mapFile;
    int                     _lastMapExportProgress;
};
