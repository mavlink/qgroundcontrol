/*!
 *   @brief Desktop/Mobile RPC
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QtRemoteObjects>

#if defined(__planner__)
#include "rep_QGCRemote_replica.h"
class TyphoonHQuickInterface;
//-----------------------------------------------------------------------------
class YuneecRPCPlannerSide : public QObject
{
    Q_OBJECT
public:
    YuneecRPCPlannerSide                (TyphoonHQuickInterface* parent) : _pIface(parent) {}
    virtual ~YuneecRPCPlannerSide       () {}
    bool    connectToNode               (QUrl url);
    bool    sendMission                 (QString name, QByteArray mission);
    bool    ready                       ();
private slots:
    void    _stateChanged               (QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState);
    void    _nodeError                  (QRemoteObjectNode::ErrorCode errorCode);

private:
    QSharedPointer<QGCRemoteReplica>    _remoteObject;
    QRemoteObjectNode                   _repNode;
    TyphoonHQuickInterface*             _pIface;
};
#else
#include "rep_QGCRemote_source.h"
//-----------------------------------------------------------------------------
class YuneecRPCST16Side : public QGCRemoteSimpleSource
{
    Q_OBJECT
public:
    YuneecRPCST16Side                   (QObject* parent = NULL) : QGCRemoteSimpleSource(parent) {}
    virtual ~YuneecRPCST16Side          () {}
public slots:
    virtual void sendMission            (QGCNewMission mission);
};
#endif

