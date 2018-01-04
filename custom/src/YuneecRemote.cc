/*!
 *   @brief Desktop/Mobile RPC
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "YuneecRemote.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "TyphoonHQuickInterface.h"

#if defined(__planner__)
//-----------------------------------------------------------------------------
bool
YuneecRPCPlannerSide::connectToNode(QUrl url)
{
    qDebug() << "Connect to" << url.toString();
    connect(&_repNode, &QRemoteObjectNode::error, this, &YuneecRPCPlannerSide::_nodeError);
    if(_repNode.connectToNode(url)) {
        _remoteObject.reset(_repNode.acquire<QGCRemoteReplica>());
        if(_remoteObject.isNull()) {
            return false;
        }
        connect(_remoteObject.data(), &QRemoteObjectReplica::stateChanged, this, &YuneecRPCPlannerSide::_stateChanged);
        emit _pIface->clientReadyChanged();
    }
    return false;
}

//-----------------------------------------------------------------------------
void
YuneecRPCPlannerSide::_stateChanged(QRemoteObjectReplica::State state, QRemoteObjectReplica::State)
{
    qDebug() << "State changed to" << state;
    emit _pIface->clientReadyChanged();
}

//-----------------------------------------------------------------------------
void
YuneecRPCPlannerSide::_nodeError(QRemoteObjectNode::ErrorCode errorCode)
{
    qDebug() << "Node error:" << errorCode;
}

//-----------------------------------------------------------------------------
bool
YuneecRPCPlannerSide::ready()
{
    bool nodeReady = !_remoteObject.isNull() && _remoteObject->state() == QRemoteObjectReplica::Valid;
    qDebug() << "State:" << nodeReady;
    return nodeReady;
}

//-----------------------------------------------------------------------------
bool
YuneecRPCPlannerSide::sendMission(QString name, QByteArray mission)
{
    if(_remoteObject.isNull()) {
        return false;
    }
    _remoteObject->sendMission(QGCNewMission(name, mission));
    return true;
}
#else
//-----------------------------------------------------------------------------
void
YuneecRPCST16Side::sendMission(QGCNewMission mission)
{
    QString missionFile = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    if(!missionFile.endsWith("/")) missionFile += "/";
    missionFile += mission.name();
    if(!missionFile.endsWith(".plan")) missionFile += ".plan";
    QFile file(missionFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(mission.mission());
    }
}
#endif
