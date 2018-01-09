/*!
 *   @brief Desktop/Mobile RPC
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QtRemoteObjects>
#include "QGCLoggingCategory.h"

//-- This is built at compile time from QGCRemote.rep
#include "rep_QGCRemote_source.h"

Q_DECLARE_LOGGING_CATEGORY(QGCSyncFiles)

class QGCSyncFilesMobile;

//-----------------------------------------------------------------------------
class QGCLogUploadWorker : public QObject
{
    Q_OBJECT
public:
    QGCLogUploadWorker() : _cancel(false) {}
public slots:
    void doLogSync      (QStringList logsToSend);
    void cancel         (bool cancel)
    {
        qCDebug(QGCSyncFiles) << "Canceled from remote";
        _cancel = cancel;
    }
signals:
    void sendLogFragment(QGCLogFragment fragment);
private:
    bool _cancel;
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
    void    pruneExtraMissions          (QStringList allMissions);
    void    requestMissions             (QStringList missions);
    void    requestLogs                 (QStringList logs);

private slots:
    void    _broadcastPresence          ();
    void    _sendLogFragment            (QGCLogFragment fragment);

signals:
    void    macAddressChanged           ();
    void    doLogSync                   (QStringList logsToSend);

private:
    bool    _processIncomingMission     (QString name, int count, QString& missionFile);
    void    _updateMissionList          ();
    void    _updateLogEntries           ();

private:
    QTimer                  _broadcastTimer;
    QString                 _macAddress;
    QUdpSocket*             _udpSocket;
    QRemoteObjectHost*      _remoteObject;
    QThread                 _logThread;
};

