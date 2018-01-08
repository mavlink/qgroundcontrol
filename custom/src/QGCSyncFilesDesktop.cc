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

//-----------------------------------------------------------------------------
QGCSyncFilesDesktop::QGCSyncFilesDesktop(QObject* parent)
    : QThread(parent)
    , _udpSocket(NULL)
    , _remoteNode(NULL)
    , _cancel(false)
    , _totalFiles(0)
    , _curFile(0)
    , _syncProgress(0)
    , _sendingFiles(false)
    , _syncDone(false)
{
    qmlRegisterUncreatableType<QGCSyncFilesDesktop>("QGroundControl", 1, 0, "QGCSyncFilesDesktop", "Reference only");
    //-- Start UDP listener
    _initUDPListener();
    connect(this, &QGCSyncFilesDesktop::completed,  this, &QGCSyncFilesDesktop::_completed);
    connect(this, &QGCSyncFilesDesktop::progress,   this, &QGCSyncFilesDesktop::_progress);
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
        _currentRemote = name;
        emit currentRemoteChanged();
        emit remoteReadyChanged();
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
        _progress(100, 50);
        QJsonDocument saveDoc(controller->saveToJson());
        _sendMission(name, saveDoc.toJson());
        //-- Should we do this?
        controller->setDirty(false);
        _message(QString(tr("%1 sent.")).arg(name));
        _progress(100, 100);
        _completed();
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::initSync()
{
    _missions.clear();
    _sendingFiles   = false;
    _syncDone       = false;
    emit sendingFilesChanged();
    emit syncDoneChanged();
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesDesktop::uploadAllMissions()
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
    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    QDirIterator it(missionPath, QStringList() << "*", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext()) {
        QFileInfo fi(it.next());
        _missions << fi.filePath();
    }
    if(!_missions.size()) {
        _message(QString(tr("No missions to send")));
        _completed();
        return;
    }
    _doSync();
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
QGCSyncFilesDesktop::_progress(quint32 total, quint32 current)
{
    _syncProgress = (int)((double)current / (double)total * 100.0);
    emit syncProgressChanged();
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
            emit sendMission(fi.baseName(), bytes);
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
