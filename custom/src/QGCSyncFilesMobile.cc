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
{
    connect(&_broadcastTimer, &QTimer::timeout, this, &QGCSyncFilesMobile::_broadcastPresence);
    _broadcastTimer.setSingleShot(false);
    //-- Start UDP broadcast
    _broadcastTimer.start(5000);
    _updateMissionList();
    _updateLogEntries();
    //-- TODO: Connect to vehicle and check when it's disarmed. Update log entries.
    //-- TODO: Need to switch interface when switching WiFi APs
}

//-----------------------------------------------------------------------------
QGCSyncFilesMobile::~QGCSyncFilesMobile()
{
    if(_udpSocket) {
        _udpSocket->deleteLater();
    }
    _logThread.quit();
    _logThread.wait();
    if(_remoteObject) {
        delete _remoteObject;
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
//-- Slot for Desktop sendMission
void
QGCSyncFilesMobile::sendMission(QGCNewMission mission)
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
        _updateMissionList();
    } else {
        qWarning() << "Error writing" << missionFile;
    }
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop pruneMission (Clone)
void
QGCSyncFilesMobile::pruneExtraMissions(QStringList allMissions)
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
    _updateMissionList();
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop mission request
void
QGCSyncFilesMobile::requestMissions(QStringList missions)
{
    QStringList missionsToSend;
    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    QDirIterator it(missionPath, QStringList() << kMissionWildCard, QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        if(missions.contains(fi.fileName())) {
            missionsToSend << fi.filePath();
        }
    }
    foreach(QString missionFile, missionsToSend) {
        qCDebug(QGCSyncFiles) << "Sending mission:" << missionFile;
        QFileInfo fi(missionFile);
        QFile f(missionFile);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open file" << missionFile;
            QGCNewMission mission(fi.fileName(), QByteArray());
            emit receiveMission(mission);
        } else {
            QByteArray bytes = f.readAll();
            f.close();
            QGCNewMission mission(fi.fileName(), bytes);
            emit receiveMission(mission);
        }
    }
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop log request
void
QGCSyncFilesMobile::requestLogs(QStringList logs)
{
    QStringList logsToSend;
    QString logPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    QDirIterator it(logPath, QStringList() << "*.tlog", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        if(logs.contains(fi.fileName())) {
            logsToSend << fi.filePath();
        }
    }
    //-- Start Worker Thread
    QGCLogUploadWorker *worker = new QGCLogUploadWorker;
    worker->moveToThread(&_logThread);
    connect(&_logThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &QGCSyncFilesMobile::doLogSync, worker, &QGCLogUploadWorker::doLogSync);
    connect(worker, &QGCLogUploadWorker::sendLogFragment, this, &QGCSyncFilesMobile::_sendLogFragment);
    _logThread.start();
    emit doLogSync(logsToSend);
}

//-----------------------------------------------------------------------------
//-- Send log fragment on main thread
void
QGCSyncFilesMobile::_sendLogFragment(QGCLogFragment fragment)
{
    emit sendLogFragment(fragment);
}

//-----------------------------------------------------------------------------
//-- Log upload thread
void
QGCLogUploadWorker::doLogSync(QStringList logsToSend)
{
    foreach(QString logFile, logsToSend) {
        qCDebug(QGCSyncFiles) << "Sending log:" << logFile;
        QFileInfo fi(logFile);
        QFile f(logFile);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open file" << logFile;
            QGCLogFragment logFrag(fi.fileName(), 0, 0, QByteArray());
            emit sendLogFragment(logFrag);
            break;
        } else {
            quint64 sofar = 0;
            quint64 total = fi.size();
            while(true) {
                //-- Send in 1M chuncks
                QByteArray bytes = f.read(1024 * 1024);
                if(bytes.size() != 0) {
                    sofar += bytes.size();
                    QGCLogFragment logFrag(fi.fileName(), sofar, total, bytes);
                    emit sendLogFragment(logFrag);
                    qCDebug(QGCSyncFiles) << "Sent:" << sofar << "bytes";
                }
                if(sofar >= total || bytes.size() == 0) {
                    break;
                }
            }
        }
    }
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
                //-- Get an URL to this host
                foreach (const QHostAddress &address, interface.allAddresses()) {
                    if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
                        if(!address.toString().startsWith("127")) {
                            url.setHost(address.toString());
                            url.setPort(QGC_RPC_PORT);
                            url.setScheme("tcp");
                            qCDebug(QGCSyncFiles) << "Remote Object URL:" << url.toString();
                            break;
                        }
                    }
                }
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
        //-- Initialize Remote Object
        _remoteObject = new QRemoteObjectHost(url);
        _remoteObject->enableRemoting(this);
    }
    _udpSocket->writeDatagram(_macAddress.toLocal8Bit(), QHostAddress::Broadcast, QGC_UDP_BROADCAST_PORT);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_updateMissionList()
{
    QStringList missions;
    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    QDirIterator it(missionPath, QStringList() << kMissionWildCard, QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        missions << fi.fileName();
    }
    setCurrentMissions(missions);
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_updateLogEntries()
{
    QList<QGCRemoteLogEntry> logs;
    QString logPath = qgcApp()->toolbox()->settingsManager()->appSettings()->telemetrySavePath();
    QDirIterator it(logPath, QStringList() << "*.tlog", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        QGCRemoteLogEntry l(fi.baseName(), fi.size());
        logs.append(l);
    }
    setLogEntries(logs);
}
