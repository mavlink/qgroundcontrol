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
    void sendLogFragment(QGCLogFragment fragment);
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
    void sendMapFragment(QGCMapFragment fragment);
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

    Q_PROPERTY(QString      macAddress      READ    macAddress      NOTIFY macAddressChanged)

    QString macAddress                  () { return _macAddress; }

public slots:
    void    sendMission                 (QGCNewMission mission);
    void    sendMap                     (QGCLogFragment fragment);
    void    pruneExtraMissions          (QStringList allMissions);
    void    requestMissions             (QStringList missions);
    void    requestLogs                 (QStringList logs);
    void    requestMapTiles             (QStringList sets);

private slots:
    void    _broadcastPresence          ();
    void    _sendLogFragment            (QGCLogFragment fragment);
    void    _sendMapFragment            (QGCMapFragment fragment);
    void    _canceled                   (bool cancel);
    void    _logWorkerDone              ();
    void    _mapWorkerDone              ();
    void    _tileSetsChanged            ();
    void    _mapExportDone              ();
    void    _mapExportProgressChanged   (int percentage);
    void    _mapExportError             (QGCMapTask::TaskType type, QString errorString);

signals:
    void    macAddressChanged           ();
    void    doLogSync                   (QStringList logsToSend);
    void    doMapSync                   (QTemporaryFile* mapFile);
    void    cancelFromDesktop           ();

private:
    bool    _processIncomingMission     (QString name, int count, QString& missionFile);
    void    _updateMissionList          ();
    void    _updateLogEntries           ();

private:
    QTimer                  _broadcastTimer;
    QString                 _macAddress;
    QUdpSocket*             _udpSocket;
    QRemoteObjectHost*      _remoteObject;
    QThread                 _workerThread;
    QGCLogUploadWorker*     _logWorker;
    QGCMapUploadWorker*     _mapWorker;
    QTemporaryFile*         _mapFile;
    int                     _lastMapExportProgress;
};
