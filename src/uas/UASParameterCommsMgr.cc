#include "UASParameterCommsMgr.h"

#include <QSettings>
#include <QDebug>

#include "QGCUASParamManagerInterface.h"
#include "UASInterface.h"
#include "MAVLinkProtocol.h"
#include "MainWindow.h"

#define RC_CAL_CHAN_MAX 8

Q_LOGGING_CATEGORY(UASParameterCommsMgrLog, "UASParameterCommsMgrLog")

UASParameterCommsMgr::UASParameterCommsMgr(QObject *parent) :
    QObject(parent),
    lastReceiveTime(0),
    mav(NULL),
    maxSilenceTimeout(30000),
    paramDataModel(NULL),
    retransmitBurstLimit(5),
    silenceTimeout(1000),
    transmissionListMode(false)
{
    // We signal to ourselves to start/stop timer on our own thread
    connect(this, SIGNAL(_startSilenceTimer(void)), this, SLOT(_startSilenceTimerOnThisThread(void)));
    connect(this, SIGNAL(_stopSilenceTimer(void)), this, SLOT(_stopSilenceTimerOnThisThread(void)));
}

UASParameterCommsMgr* UASParameterCommsMgr::initWithUAS(UASInterface* uas)
{
    mav = uas;
    paramDataModel = mav->getParamManager()->dataModel();
    loadParamCommsSettings();

    //Requesting parameters one-by-one from mav
    connect(this, SIGNAL(parameterUpdateRequestedById(int,int)),
            mav, SLOT(requestParameter(int,int)));

    // Sending params to the UAS
    connect(this, SIGNAL(commitPendingParameter(int,QString,QVariant)),
            mav, SLOT(setParameter(int,QString,QVariant)));

    // Received parameter updates from UAS
    connect(mav, SIGNAL(parameterChanged(int,int,int,int,QString,QVariant)),
            this, SLOT(receivedParameterUpdate(int,int,int,int,QString,QVariant)));

    connect(&silenceTimer, SIGNAL(timeout()),
            this,SLOT(silenceTimerExpired()));

    return this;
}




void UASParameterCommsMgr::loadParamCommsSettings()
{
    QSettings settings;
    //TODO these are duplicates of MAVLinkProtocol settings...seems wrong to use them in two places
    settings.beginGroup("QGC_MAVLINK_PROTOCOL");
    bool ok;
    int val = settings.value("PARAMETER_RETRANSMISSION_TIMEOUT", 1000).toInt(&ok);
    if (ok) {
        silenceTimeout = val;
        qDebug() << "silenceTimeout" << silenceTimeout;
    }

    settings.endGroup();
}

void UASParameterCommsMgr::_sendParamRequestListMsg(void)
{
    MAVLinkProtocol* mavlink = MainWindow::instance()->getMAVLink();
    Q_ASSERT(mavlink);
    
    mavlink_message_t msg;
    mavlink_msg_param_request_list_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, mav->getUASID(), MAV_COMP_ID_ALL);
    mav->sendMessage(msg);
}

/**
 * Send a request to deliver the list of onboard parameters
 * from the MAV.
 */
void UASParameterCommsMgr::requestParameterList()
{
    if (!mav) {
        return;
    }
    

    if (!transmissionListMode) {
        qCDebug(UASParameterCommsMgrLog) << "Requesting full parameter list";
        transmissionListMode = true;//TODO eliminate?
        //we use (compId 0, paramId 0) as  indicating all params for the system
        markReadParamWaiting(0,0);
        
        _sendParamRequestListMsg();
        
        updateSilenceTimer();
    }
    else {
        qCDebug(UASParameterCommsMgrLog) << "Ignoring requestParameterList because we're receiving params list";
    }

}


void UASParameterCommsMgr::markReadParamWaiting(int compId, int paramId)
{
    if (!readsWaiting.contains(compId)) {
        readsWaiting.insert(compId, new QSet<int>());
    }

    readsWaiting.value(compId)->insert(paramId);
}

void UASParameterCommsMgr::markWriteParamWaiting(int compId, QString paramName, QVariant value)
{
    //ensure we have a map for this compId
    if (!writesWaiting.contains(compId)) {
        writesWaiting.insert(compId, new QMap<QString, QVariant>());
    }

    // Insert it in missing write ACK list
    writesWaiting.value(compId)->insert(paramName, value);
}

/*
 Empty read retransmission list
 Empty write retransmission list
*/
void UASParameterCommsMgr::clearRetransmissionLists(int& missingReadCount, int& missingWriteCount )
{
    qCDebug(UASParameterCommsMgrLog) << "Clearing re-transmission lists";

    missingReadCount = 0;
    QList<int> compIds = readsWaiting.keys();
    foreach (int compId, compIds) {
        missingReadCount += readsWaiting.value(compId)->count();
        readsWaiting.value(compId)->clear();
    }

    missingWriteCount = 0;
    compIds = writesWaiting.keys();
    foreach (int compId, compIds) {
        missingWriteCount += writesWaiting.value(compId)->count();
        writesWaiting.value(compId)->clear();
    }

}


void UASParameterCommsMgr::emitPendingParameterCommit(int compId, const QString& key, QVariant& value)
{
    int paramType = (int)value.type();
    switch (paramType)
    {
    case QVariant::Char:
    {
        QVariant fixedValue(QChar((unsigned char)value.toInt()));
        emit commitPendingParameter(compId, key, fixedValue);
    }
        break;
    case QVariant::Int:
    {
        QVariant fixedValue(value.toInt());
        emit commitPendingParameter(compId, key, fixedValue);
    }
        break;
    case QVariant::UInt:
    {
        QVariant fixedValue(value.toUInt());
        emit commitPendingParameter(compId, key, fixedValue);
    }
        break;
    case QMetaType::Float:
    {
        QVariant fixedValue(value.toFloat());
        emit commitPendingParameter(compId, key, fixedValue);
    }
        break;
    default:
        qCritical() << "ABORTED PARAM SEND, INVALID QVARIANT TYPE" << paramType;
        return;
    }

    setParameterStatusMsg(tr("Writing %1: %2 for comp. %3").arg(key).arg(value.toDouble()).arg(compId));

}


void UASParameterCommsMgr::resendReadWriteRequests()
{
    int compId;
    QList<int> compIds;

    // Re-request at maximum retransmitBurstLimit parameters at once
    // to prevent link flooding'
    int requestedReadCount = 0;
    compIds = readsWaiting.keys();
    foreach (compId, compIds) {
        // Request n parameters from this component (at maximum)
        QSet<int>* missingReadParams = readsWaiting.value(compId, NULL);
        qDebug() << "compId " << compId << "readsWaiting:" << missingReadParams->count();
        foreach (int paramId, *missingReadParams) {
            if (0 == paramId && 0 == compId) {
                _sendParamRequestListMsg();
                //don't request any other params individually for this component
                break;
            }
            if (requestedReadCount < retransmitBurstLimit) {
                //qDebug() << __FILE__ << __LINE__ << "RETRANSMISSION GUARD REQUESTS RETRANSMISSION OF PARAM #" << paramId << "FROM COMPONENT #" << compId;
                emit parameterUpdateRequestedById(compId, paramId);
                setParameterStatusMsg(tr("Requested retransmission of #%1").arg(paramId+1));
                requestedReadCount++;
            }
            else {
                qCDebug(UASParameterCommsMgrLog) << "Throttling read retransmit requests at" << requestedReadCount;
                break;
            }
        }
    }

    // Re-request at maximum retransmitBurstLimit parameters at once
    // to prevent write-request link flooding
    int requestedWriteCount = 0;
    compIds = writesWaiting.keys();
    foreach (compId, compIds) {
        QMap <QString, QVariant>* missingWriteParams = writesWaiting.value(compId);
        foreach (QString key, missingWriteParams->keys()) {
            if (requestedWriteCount < retransmitBurstLimit) {
                // Re-request write operation
                QVariant value = missingWriteParams->value(key);
                emitPendingParameterCommit(compId, key, value);
                requestedWriteCount++;
            }
            else {
                qCDebug(UASParameterCommsMgrLog) << "Throttling write retransmit requests at" << requestedWriteCount;
                break;
            }
        }
    }

    updateSilenceTimer();

}

void UASParameterCommsMgr::resetAfterListReceive()
{
    transmissionListMode = false;
    knownParamListSize.clear();
}

void UASParameterCommsMgr::silenceTimerExpired()
{
    quint64 curTime = QGC::groundTimeMilliseconds();
    int elapsed = (int)(curTime - lastSilenceTimerReset);
    qCDebug(UASParameterCommsMgrLog) << "silenceTimerExpired elapsed:" << elapsed;

    if (elapsed < silenceTimeout) {
        //reset the guard timer: it fired prematurely
        updateSilenceTimer();
        return;
    }

    int totalElapsed = (int)(curTime - lastReceiveTime);
    if (totalElapsed > maxSilenceTimeout) {
        qCDebug(UASParameterCommsMgrLog) << "maxSilenceTimeout exceeded: " << totalElapsed;
        int missingReads, missingWrites;
        clearRetransmissionLists(missingReads,missingWrites);
        emit _stopSilenceTimer(); // Stop timer on our thread;
        lastReceiveTime = 0;
        lastSilenceTimerReset = curTime;
        setParameterStatusMsg(tr("TIMEOUT: Abandoning %1 reads %2 writes after %3 seconds").arg(missingReads).arg(missingWrites).arg(totalElapsed/1000));
    }
    else {
        resendReadWriteRequests();
    }
}


void UASParameterCommsMgr::requestParameterUpdate(int compId, const QString& paramName)
{
    if (mav) {
        mav->requestParameter(compId, paramName);
        //TODO track these read requests with a paramName but no param ID  : use index in getOnboardParamsForComponent?
        //ensure we keep track of every single read request
    }
}

void UASParameterCommsMgr::requestRcCalibrationParamsUpdate()
{
    if (!transmissionListMode) {
        QString minTpl("RC%1_MIN");
        QString maxTpl("RC%1_MAX");
        QString trimTpl("RC%1_TRIM");
        QString revTpl("RC%1_REV");

        // Do not request the RC type, as these values depend on this
        // active onboard parameter


        int defCompId = paramDataModel->getDefaultComponentId();
        for (unsigned int i = 1; i < (RC_CAL_CHAN_MAX+1); ++i)  {
            qDebug() << "Request RC " << i;
            requestParameterUpdate(defCompId, minTpl.arg(i));
            requestParameterUpdate(defCompId, trimTpl.arg(i));
            requestParameterUpdate(defCompId, maxTpl.arg(i));
            requestParameterUpdate(defCompId, revTpl.arg(i));
            QGC::SLEEP::usleep(5000);
        }
    }
    else {
        qCDebug(UASParameterCommsMgrLog) << "Ignoring requestRcCalibrationParamsUpdate because we're receiving params list";
    }
}


/**
 * @param component the subsystem which has the parameter
 * @param parameterName name of the parameter, as delivered by the system
 * @param value value of the parameter
 */
void UASParameterCommsMgr::setParameter(int compId, QString paramName, QVariant value, bool forceSend)
{
    if (paramName.isEmpty()) {
        return;
    }

    double dblValue = value.toDouble();

    if (paramDataModel->isValueLessThanParamMin(paramName,dblValue)) {
        setParameterStatusMsg(tr("REJ. %1, %2 < min").arg(paramName).arg(dblValue),
                              ParamCommsStatusLevel_Error
                              );
        return;
    }
    if (paramDataModel->isValueGreaterThanParamMax(paramName,dblValue)) {
        setParameterStatusMsg(tr("REJ. %1, %2 > max").arg(paramName).arg(dblValue),
                              ParamCommsStatusLevel_Error
                              );
        return;
    }

	if (!forceSend) {
		QVariant onboardVal;
		paramDataModel->getOnboardParamValue(compId,paramName,onboardVal);
		if (onboardVal == value) {
			setParameterStatusMsg(tr("REJ. %1 already %2").arg(paramName).arg(dblValue),
				ParamCommsStatusLevel_Warning
				);
			return;
		}
	}

    emitPendingParameterCommit(compId, paramName, value);

    //Add this request to list of writes not yet ack'd

    markWriteParamWaiting( compId,  paramName,  value);
    updateSilenceTimer();


}

void UASParameterCommsMgr::updateSilenceTimer()
{
    //if there are pending reads or writes, ensure we timeout in a little while
    //if we hear nothing but silence from our partner

    int missReadCount = 0;
    foreach (int key, readsWaiting.keys()) {
        missReadCount +=  readsWaiting.value(key)->count();
    }

    int missWriteCount = 0;
    foreach (int key, writesWaiting.keys()) {
        missWriteCount += writesWaiting.value(key)->count();
    }


    if (missReadCount > 0 || missWriteCount > 0) {
        lastSilenceTimerReset = QGC::groundTimeMilliseconds();
        if (0 == lastReceiveTime) {
            lastReceiveTime = lastSilenceTimerReset;
        }
        // We signal this to ourselves so timer is started on the right thread
        emit _startSilenceTimer();
    }
    else {
        //all parameters have been received, broadcast to UI
        qCDebug(UASParameterCommsMgrLog) << "emitting parameterListUpToDate";
        emit parameterListUpToDate();
        resetAfterListReceive();
        emit _stopSilenceTimer(); // Stop timer on our thread;
        lastReceiveTime = 0;
    }



}


void UASParameterCommsMgr::setParameterStatusMsg(const QString& msg, ParamCommsStatusLevel_t level)
{
    //qDebug() << "parameterStatusMsg: " << msg;
    emit parameterStatusMsgUpdated(msg,level);
}

void UASParameterCommsMgr::receivedParameterUpdate(int uas, int compId, int paramCount, int paramId, QString paramName, QVariant value)
{
    qCDebug(UASParameterCommsMgrLog) << QString("Received parameter update for: name(%1) count(%2) index(%3)").arg(paramName).arg(paramCount).arg(paramId);

    Q_UNUSED(uas); //this object is assigned to one UAS only
    lastReceiveTime = QGC::groundTimeMilliseconds();
    // qDebug() << "compId" << compId << "receivedParameterUpdate:" << paramName;

    //notify the data model that we have an updated param
    paramDataModel->handleParamUpdate(compId,paramName,value);


    // Ensure we have missing read/write lists for this compId
    if (!readsWaiting.contains(compId)) {
        readsWaiting.insert(compId, new QSet<int>());
    }
    if (!writesWaiting.contains(compId) ) {
        writesWaiting.insert(compId,new QMap<QString,QVariant>());
    }

    QSet<int>* compMissingReads =  readsWaiting.value(compId);
    // List mode is different from single parameter transfers
    if (transmissionListMode) {
        // Only accept the list size once on the first packet from each component
        if (!knownParamListSize.contains(compId)) {
            // Mark list size as known
            knownParamListSize.insert(compId,paramCount);

            //remove our placeholder read request for all params
            readsWaiting.value(0)->remove(0);

            qCDebug(UASParameterCommsMgrLog) << "receivedParameterUpdate: Mark all parameters as missing: " << paramCount;
            for (int i = 1; i < paramCount; ++i) { //param Id 0 is  "all parameters" and not valid
                compMissingReads->insert(i);
            }
        }
    }


    // Mark this parameter as received in read list
    compMissingReads->remove(paramId);


    bool justWritten = false;
    bool writeMismatch = false;

    // Mark this parameter as received in write ACK list
    QMap<QString, QVariant>* compMissingWrites = writesWaiting.value(compId);
    if (!compMissingWrites) {
        //we sometimes send a write request on compId 0 and get a response on a nonzero compId eg 50
        compMissingWrites = writesWaiting.value(0);
    }
    if (compMissingWrites && compMissingWrites->contains(paramName)) {
        justWritten = true;
        if (compMissingWrites->value(paramName) != value) {
            writeMismatch = true;
        }
        compMissingWrites->remove(paramName);
    }


    if (justWritten) {
        int waitingWritesCount = compMissingWrites->count();
        if (!writeMismatch) {
            setParameterStatusMsg(tr("SUCCESS: Wrote %2 (#%1): %3").arg(paramId+1).arg(paramName).arg(value.toDouble()));
        }

        if (!writeMismatch) {
            if (0 == waitingWritesCount) {
                setParameterStatusMsg(tr("SUCCESS: Wrote all params for component %1").arg(compId));
                if (persistParamsAfterSend) {
                    writeParamsToPersistentStorage();
                    persistParamsAfterSend = false;
                }
            }
        }
        else  {
            // Mismatch, tell user
            setParameterStatusMsg(tr("FAILURE: Wrote %1: sent %2 != onboard %3").arg(paramName).arg(compMissingWrites->value(paramName).toDouble()).arg(value.toDouble()),
                                  ParamCommsStatusLevel_Warning);
        }
    }
    else {
        int waitingReadsCount = compMissingReads->count();

        if (0 == waitingReadsCount) {
            // Transmission done
            QTime time = QTime::currentTime();
            QString timeString = time.toString();
            setParameterStatusMsg(tr("All received. (updated at %1)").arg(timeString));
        }
        else {
            // Waiting to receive more
            QString val = QString("%1").arg(value.toFloat(), 5, 'f', 1, QChar(' '));
            setParameterStatusMsg(tr("OK: %1 %2 (%3/%4)").arg(paramName).arg(val).arg(paramCount-waitingReadsCount).arg(paramCount),
                                  ParamCommsStatusLevel_OK);
        }
    }

    updateSilenceTimer();


}


void UASParameterCommsMgr::writeParamsToPersistentStorage()
{
    if (mav) {
        mav->writeParametersToStorage(); //TODO track timeout, retransmit etc?
        persistParamsAfterSend = false; //done
    }
}


void UASParameterCommsMgr::sendPendingParameters(bool copyToPersistent, bool forceSend)
{
    persistParamsAfterSend |= copyToPersistent;

    // Iterate through all components, through all pending parameters and send them to UAS
    int parametersSent = 0;
    QMap<int, QMap<QString, QVariant>*>* changedValues = paramDataModel->getAllPendingParams();
    QMap<int, QMap<QString, QVariant>*>::iterator i;
    for (i = changedValues->begin(); i != changedValues->end(); ++i) {
        // Iterate through the parameters of the component
        int compId = i.key();
        QMap<QString, QVariant>* paramList = i.value();
        QMap<QString, QVariant>::iterator j;
        setParameterStatusMsg(tr("%1 pending params for component %2").arg(paramList->count()).arg(compId));

        for (j = paramList->begin(); j != paramList->end(); ++j) {
            setParameter(compId, j.key(), j.value(), forceSend);
            parametersSent++;
        }
    }

    // Change transmission status if necessary
    if (0 == parametersSent) {
        setParameterStatusMsg(tr("No transmission: No changed values."),ParamCommsStatusLevel_Warning);
    }
    else {
        setParameterStatusMsg(tr("Transmitting %1 parameters.").arg(parametersSent));
        qCDebug(UASParameterCommsMgrLog) << "Pending parameters now:" << paramDataModel->countPendingParams();
    }


    updateSilenceTimer();
}

UASParameterCommsMgr::~UASParameterCommsMgr()
{
    silenceTimer.stop();

    QString ptrStr;
    ptrStr.sprintf("%8p", this);
    qCDebug(UASParameterCommsMgrLog) <<  "UASParameterCommsMgr destructor: " << ptrStr ;

}

void UASParameterCommsMgr::_startSilenceTimerOnThisThread(void)
{
    silenceTimer.start(silenceTimeout);
}

void UASParameterCommsMgr::_stopSilenceTimerOnThisThread(void)
{
    silenceTimer.stop();
}
