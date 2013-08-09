#include "UASParameterCommsMgr.h"

#include <QSettings>

#include "QGCUASParamManager.h"
#include "UASInterface.h"

#define RC_CAL_CHAN_MAX 8

UASParameterCommsMgr::UASParameterCommsMgr(QObject *parent, UASInterface *uas) :
    QObject(parent),
    mav(uas),
    paramDataModel(NULL),
    transmissionListMode(false),
    transmissionActive(false),
    transmissionTimeout(0),
    retransmissionTimeout(1000),
    rewriteTimeout(1000),
    retransmissionBurstRequestSize(5)
{
    paramDataModel = mav->getParamDataModel();
    loadParamCommsSettings();


    //Requesting parameters one-by-one from mav
    connect(this, SIGNAL(parameterUpdateRequestedById(int,int)),
            mav, SLOT(requestParameter(int,int)));

    // Sending params to the UAS
    connect(this, SIGNAL(parameterChanged(int,QString,QVariant)),
            mav, SLOT(setParameter(int,QString,QVariant)));

    // Received parameter updates from UAS
    connect(mav, SIGNAL(parameterChanged(int,int,int,int,QString,QVariant)),
            this, SLOT(receivedParameterUpdate(int,int,int,int,QString,QVariant)));

    //connecto retransmissionTimer
    connect(&retransmissionTimer, SIGNAL(timeout()),
            this, SLOT(retransmissionGuardTick()));

}



void UASParameterCommsMgr::loadParamCommsSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAVLINK_PROTOCOL");
    bool ok;
    int val = settings.value("PARAMETER_RETRANSMISSION_TIMEOUT", retransmissionTimeout).toInt(&ok);
    if (ok) {
        retransmissionTimeout = val;
    }
    val = settings.value("PARAMETER_REWRITE_TIMEOUT", rewriteTimeout).toInt(&ok);
    if (ok) {
        rewriteTimeout = val;
    }
    settings.endGroup();
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

    //TODO check: no need to cause datamodel to forget params here?
//    paramDataModel->forgetAllOnboardParameters();

    if (!transmissionListMode) {
        // Clear transmission state
        receivedParamsList.clear();
        transmissionListSizeKnown.clear();

        transmissionListMode = true;
        foreach (int key, transmissionMissingPackets.keys()) {
            transmissionMissingPackets.value(key)->clear();
        }
        transmissionActive = true;

        setParameterStatusMsg(tr("Requested param list.. waiting"));
        listRecvTimeout = QGC::groundTimeMilliseconds() + 10000;
        mav->requestParameters();
        setRetransmissionGuardEnabled(true);
    }
    else {
        qDebug() << __FILE__ << __LINE__ << "Ignoring requestParameterList because we're receiving params list";
    }

}


/*
 Empty read retransmission list
 Empty write retransmission list
*/
void UASParameterCommsMgr::clearRetransmissionLists(int& missingReadCount, int& missingWriteCount )
{
    qDebug() << __FILE__ << __LINE__ << "clearRetransmissionLists";

    missingReadCount = 0;
    QList<int> readKeys = transmissionMissingPackets.keys();
    foreach (int compId, readKeys) {
        missingReadCount += transmissionMissingPackets.value(compId)->count();
        transmissionMissingPackets.value(compId)->clear();
    }

    missingWriteCount = 0;
    QList<int> writeKeys = transmissionMissingWriteAckPackets.keys();
    foreach (int compId, writeKeys) {
        missingWriteCount += transmissionMissingWriteAckPackets.value(compId)->count();
        transmissionMissingWriteAckPackets.value(compId)->clear();
    }

}


void UASParameterCommsMgr::emitParameterChanged(int compId, const QString& key, QVariant& value)
{
    int paramType = (int)value.type();
    switch (paramType)
    {
    case QVariant::Char:
    {
        QVariant fixedValue(QChar((unsigned char)value.toInt()));
        emit parameterChanged(compId, key, fixedValue);
    }
        break;
    case QVariant::Int:
    {
        QVariant fixedValue(value.toInt());
        emit parameterChanged(compId, key, fixedValue);
    }
        break;
    case QVariant::UInt:
    {
        QVariant fixedValue(value.toUInt());
        emit parameterChanged(compId, key, fixedValue);
    }
        break;
    case QMetaType::Float:
    {
        QVariant fixedValue(value.toFloat());
        emit parameterChanged(compId, key, fixedValue);
    }
        break;
    default:
        qCritical() << "ABORTED PARAM SEND, NO VALID QVARIANT TYPE";
        return;
    }

    setParameterStatusMsg(tr("Requested write of: %1: %2").arg(key).arg(value.toDouble()));

}


void UASParameterCommsMgr::resendReadWriteRequests()
{
    int compId;
    QList<int> compIds;

    // Re-request at maximum retransmissionBurstRequestSize parameters at once
    // to prevent link flooding'
    int requestedReadCount = 0;
    compIds = transmissionMissingPackets.keys();
    foreach (compId, compIds) {
        // Request n parameters from this component (at maximum)
        QList<int>* missingReadParams = transmissionMissingPackets.value(compId, NULL);
        foreach (int paramId, *missingReadParams) {
            if (requestedReadCount < retransmissionBurstRequestSize) {
                //qDebug() << __FILE__ << __LINE__ << "RETRANSMISSION GUARD REQUESTS RETRANSMISSION OF PARAM #" << paramId << "FROM COMPONENT #" << compId;
                emit parameterUpdateRequestedById(compId, paramId);
                setParameterStatusMsg(tr("Requested retransmission of #%1").arg(paramId+1));
                requestedReadCount++;
            }
            else {
                qDebug() << "Throttling read retransmit requests at" << requestedReadCount;
                break;
            }
        }
    }

    // Re-request at maximum retransmissionBurstRequestSize parameters at once
    // to prevent write-request link flooding
    int requestedWriteCount = 0;
    compIds = transmissionMissingWriteAckPackets.keys();
    foreach (compId, compIds) {
        QMap <QString, QVariant>* missingParams = transmissionMissingWriteAckPackets.value(compId);
        foreach (QString key, missingParams->keys()) {
            if (requestedWriteCount < retransmissionBurstRequestSize) {
                // Re-request write operation
                QVariant value = missingParams->value(key);
                emitParameterChanged(compId, key, value);
                requestedWriteCount++;
            }
            else {
                qDebug() << "Throttling write retransmit requests at" << requestedWriteCount;
                break;
            }
        }
    }

    if ((0 == requestedWriteCount) && (0 == requestedReadCount) ) {
        qDebug() << __FILE__ << __LINE__ << "NO re-read or rewrite requests??";
        if (!transmissionListMode) {
            setRetransmissionGuardEnabled(false);
            transmissionActive = false;
        }
    }
    else {
        //restart the timer now that we've sent
        setRetransmissionGuardEnabled(true);
    }
}

void UASParameterCommsMgr::resetAfterListReceive()
{
    transmissionListMode = false;
    transmissionListSizeKnown.clear();

    //TODO we shouldn't clear missingPackets because other transactions might be using them?
    //for list reception we only clear receive packets?
//    foreach (int key, transmissionMissingPackets.keys()) {
//        transmissionMissingPackets.value(key)->clear();
//    }

}

void UASParameterCommsMgr::retransmissionGuardTick()
{
    quint64 curTime = QGC::groundTimeMilliseconds();

    //Workaround for an apparent Qt bug that causes retransmission guard timer to fire prematurely (350ms)
    quint64 elapsed = (curTime - lastTimerReset);
    if (elapsed < transmissionTimeout) {
        qDebug() << "retransmissionGuardTick elapsed:" << (curTime - lastTimerReset);
        //reset the guard timer: it fired prematurely
        setRetransmissionGuardEnabled(true);
        return;
    }

    if (transmissionActive) {

        if (transmissionListMode && transmissionListSizeKnown.isEmpty() ) {
            //we are still waitin for the first parameter list response
            if (curTime > this->listRecvTimeout) {
                //re-request parameters
                setParameterStatusMsg(tr("TIMEOUT: Re-requesting param list"),ParamCommsStatusLevel_Warning);
                listRecvTimeout = curTime + 10000;
                mav->requestParameters();
            }
            return;
        }

        // Check for timeout
        // stop retransmission attempts on timeout
        if (curTime > transmissionTimeout) {
            setRetransmissionGuardEnabled(false);
            resetAfterListReceive();

            int missingReadCount, missingWriteCount;
            clearRetransmissionLists(missingReadCount,missingWriteCount);
            if ((missingReadCount > 0) || (missingWriteCount > 0)) {
                setParameterStatusMsg(tr("TIMEOUT! MISSING: %1 read, %2 write.").arg(missingReadCount).arg(missingWriteCount),
                                      ParamCommsStatusLevel_Warning);
            }

            return;
        }

        qDebug() << __FILE__ << __LINE__ << "RETRANSMISSION GUARD ACTIVE, CHECKING FOR DROPS..";
        resendReadWriteRequests();
    }
    else {
        qDebug() << __FILE__ << __LINE__ << "STOPPING RETRANSMISSION GUARD GRACEFULLY";
        setRetransmissionGuardEnabled(false);
    }
}



/**
 * Enabling the retransmission guard enables the parameter widget to track
 * dropped parameters and to re-request them. This works for both individual
 * parameter reads as well for whole list requests.
 *
 * @param enabled True if retransmission checking should be enabled, false else
 */

void UASParameterCommsMgr::setRetransmissionGuardEnabled(bool enabled)
{
    if (enabled) {
        retransmissionTimer.start(retransmissionTimeout);
        lastTimerReset = QGC::groundTimeMilliseconds() ;
    } else {
        retransmissionTimer.stop();
    }
}

void UASParameterCommsMgr::requestParameterUpdate(int component, const QString& parameter)
{
    if (mav) {
        mav->requestParameter(component, parameter);
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

        for (unsigned int i = 1; i < (RC_CAL_CHAN_MAX+1); ++i)  {
            qDebug() << "Request RC " << i;
            mav->requestParameter(0, minTpl.arg(i));
            QGC::SLEEP::usleep(5000);
            mav->requestParameter(0, trimTpl.arg(i));
            QGC::SLEEP::usleep(5000);
            mav->requestParameter(0, maxTpl.arg(i));
            QGC::SLEEP::usleep(5000);
            mav->requestParameter(0, revTpl.arg(i));
            QGC::SLEEP::usleep(5000);
        }
    }
    else {
        qDebug() << __FILE__ << __LINE__ << "Ignoring requestRcCalibrationParamsUpdate because we're receiving params list";
    }
}


/**
 * @param component the subsystem which has the parameter
 * @param parameterName name of the parameter, as delivered by the system
 * @param value value of the parameter
 */
void UASParameterCommsMgr::setParameter(int component, QString parameterName, QVariant value)
{
    if (parameterName.isEmpty()) {
        return;
    }

    double dblValue = value.toDouble();

    if (paramDataModel->isValueLessThanParamMin(parameterName,dblValue)) {
        setParameterStatusMsg(tr("REJ. %1, %2 < min").arg(parameterName).arg(dblValue),
                              ParamCommsStatusLevel_Error
                              );
        return;
    }
    if (paramDataModel->isValueGreaterThanParamMax(parameterName,dblValue)) {
        setParameterStatusMsg(tr("REJ. %1, %2 > max").arg(parameterName).arg(dblValue),
                              ParamCommsStatusLevel_Error
                              );
        return;
    }

    QVariant onboardVal;
    paramDataModel->getOnboardParamValue(component,parameterName,onboardVal);
    if (onboardVal == value) {
        setParameterStatusMsg(tr("REJ. %1 already %2").arg(parameterName).arg(dblValue),
                              ParamCommsStatusLevel_Warning
                              );
        return;
    }

    emitParameterChanged(component, parameterName, value);

    // Wait for parameter to be written back
    // mark it therefore as missing
    if (!transmissionMissingWriteAckPackets.contains(component)) {
        transmissionMissingWriteAckPackets.insert(component, new QMap<QString, QVariant>());
    }

    // Insert it in missing write ACK list
    transmissionMissingWriteAckPackets.value(component)->insert(parameterName, value);

    // Set timeouts
    if (transmissionActive) {
        transmissionTimeout += rewriteTimeout;
    }
    else {
        quint64 newTransmissionTimeout = QGC::groundTimeMilliseconds() + rewriteTimeout;
        if (newTransmissionTimeout > transmissionTimeout) {
            transmissionTimeout = newTransmissionTimeout;
        }
        transmissionActive = true;
    }

    // Enable guard / reset timeouts
    setRetransmissionGuardEnabled(true);
}

void UASParameterCommsMgr::setParameterStatusMsg(const QString& msg, ParamCommsStatusLevel_t level)
{
    qDebug() << "parameterStatusMsg: " << msg;
    parameterStatusMsg = msg;

    emit parameterStatusMsgUpdated(msg,level);
}


/**
 * @param uas System which has the component
 * @param component id of the component
 * @param parameterName human friendly name of the parameter
 */
void UASParameterCommsMgr::receivedParameterUpdate(int uas, int compId, int paramCount, int paramId, QString paramName, QVariant value)
{
    Q_UNUSED(uas); //this object is assigned to one UAS only

    //notify the data model that we have an updated param
    paramDataModel->handleParamUpdate(compId,paramName,value);

    // Missing packets list has to be instantiated for all components
    if (!transmissionMissingPackets.contains(compId)) {
        transmissionMissingPackets.insert(compId, new QList<int>());
    }

    QList<int>* compXmitMissing =  transmissionMissingPackets.value(compId);

    // List mode is different from single parameter transfers
    if (transmissionListMode) {
        // Only accept the list size once on the first packet from
        // each component
        if (!transmissionListSizeKnown.contains(compId)) {
            // Mark list size as known
            transmissionListSizeKnown.insert(compId, true);

            qDebug() << "Mark all parameters as missing: " << paramCount;
            for (int i = 1; i < paramCount; ++i) { //TODO check: param Id 0 is  "all parameters" and not valid ?
                if (!compXmitMissing->contains(i)) {
                    compXmitMissing->append(i);
                }
            }

            // There is only one transmission timeout for all components
            // since components do not manage their transmission,
            // the longest timeout is safe for all components.
            quint64 thisTransmissionTimeout = QGC::groundTimeMilliseconds() + ((paramCount)*retransmissionTimeout);
            if (thisTransmissionTimeout > transmissionTimeout) {
                transmissionTimeout = thisTransmissionTimeout;
            }
        }

    }

    // Mark this parameter as received in read list
    int index = compXmitMissing->indexOf(paramId);
    // If the MAV sent the parameter without request, it wont be in missing list
    if (index != -1) {
        compXmitMissing->removeAt(index);
    }

    bool justWritten = false;
    bool writeMismatch = false;

    // Mark this parameter as received in write ACK list
    QMap<QString, QVariant>* map = transmissionMissingWriteAckPackets.value(compId);
    if (map && map->contains(paramName)) {
        justWritten = true;
        QVariant newval = map->value(paramName);
        if (map->value(paramName) != value) {
            writeMismatch = true;
        }
        map->remove(paramName);
    }

    int missCount = 0;
    foreach (int key, transmissionMissingPackets.keys()) {
        missCount +=  transmissionMissingPackets.value(key)->count();
    }

    int missWriteCount = 0;
    foreach (int key, transmissionMissingWriteAckPackets.keys()) {
        missWriteCount += transmissionMissingWriteAckPackets.value(key)->count();
    }

    //TODO simplify this if-else tree
    if (justWritten && !writeMismatch && missWriteCount == 0) {
        // Just wrote one and count went to 0 - this was the last missing write parameter
        setParameterStatusMsg(tr("SUCCESS: WROTE ALL PARAMETERS"));
    }
    else if (justWritten && !writeMismatch) {
        setParameterStatusMsg(tr("SUCCESS: Wrote %2 (#%1/%4): %3").arg(paramId+1).arg(paramName).arg(value.toDouble()).arg(paramCount));
    }
    else if (justWritten && writeMismatch) {
        // Mismatch, tell user
        setParameterStatusMsg(tr("FAILURE: Wrote %1: sent %2 != onboard %3").arg(paramName).arg(map->value(paramName).toDouble()).arg(value.toDouble()),
                              ParamCommsStatusLevel_Warning);
    }
    else {
        QString val = QString("%1").arg(value.toFloat(), 5, 'f', 1, QChar(' '));
        if (missCount == 0) {
            // Transmission done
            QTime time = QTime::currentTime();
            QString timeString = time.toString();
            setParameterStatusMsg(tr("All received. (updated at %1)").arg(timeString));
        }
        else {
            // Transmission in progress
            setParameterStatusMsg(tr("OK: %1 %2 (%3/%4)").arg(paramName).arg(val).arg(paramCount-missCount).arg(paramCount),
                                  ParamCommsStatusLevel_Warning);
        }
    }

    // Check if last parameter was received
    if (missCount == 0 && missWriteCount == 0) {
        resetAfterListReceive();
        setRetransmissionGuardEnabled(false);
        //all parameters have been received, broadcast to UI
        emit parameterListUpToDate();
    }
    else {
        //reset the timeout timer since we received one
        setRetransmissionGuardEnabled(true);

        //qDebug() << "missCount:" << missCount << "missWriteCount:" << missWriteCount;
        //if (missCount < 4) {
        //    foreach (int key, transmissionMissingPackets.keys()) {
        //        QList<int>* list = transmissionMissingPackets.value(key);

        //        if (list && list->count()) {
        //            QString yazza = QString().sprintf("Component %d missing %d: ",key,list->count());

        //            for (int i = 0; i < list->count(); i++) {
        //                int val = list->at(i);
        //                yazza.append( QString().sprintf("%d,",val) );
        //            }

        //            qDebug() << yazza;
        //        }
        //        else {
        //            //very suspicious...no actual missing items??
        //            transmissionMissingPackets.remove(key);
        //            break;
        //        }
        //    }
        //}
    }
}


void UASParameterCommsMgr::writeParamsToPersistentStorage()
{
    if (mav) {
        mav->writeParametersToStorage(); //TODO track timeout, retransmit etc?
    }
}


void UASParameterCommsMgr::sendPendingParameters()
{
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
            //TODO mavlink command for "set parameter list" ?
            setParameter(compId, j.key(), j.value());
            parametersSent++;
        }
    }

    // Change transmission status if necessary
    if (parametersSent == 0) {
        setParameterStatusMsg(tr("No transmission: No changed values."),ParamCommsStatusLevel_Warning);
    } else {
        setParameterStatusMsg(tr("Transmitting %1 parameters.").arg(parametersSent));
        // Set timeouts
        if (transmissionActive) {
            transmissionTimeout += parametersSent*rewriteTimeout;
        }
        else {
            transmissionActive = true;
            quint64 newTransmissionTimeout = QGC::groundTimeMilliseconds() + parametersSent*rewriteTimeout;
            if (newTransmissionTimeout > transmissionTimeout) {
                transmissionTimeout = newTransmissionTimeout;
            }
        }
        // Enable guard
        setRetransmissionGuardEnabled(true);
    }
}

UASParameterCommsMgr::~UASParameterCommsMgr()
{
    setRetransmissionGuardEnabled(false);

    QString ptrStr;
    ptrStr.sprintf("%8p", this);
    qDebug() <<  "UASParameterCommsMgr destructor: " << ptrStr ;

}

