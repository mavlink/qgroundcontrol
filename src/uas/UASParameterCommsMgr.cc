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
    retransmissionTimeout(350),
    rewriteTimeout(500),
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

    // New parameters from UAS
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


void UASParameterCommsMgr::retransmissionGuardTick()
{
    if (transmissionActive) {

        if (transmissionListMode && transmissionListSizeKnown.isEmpty() ) {
            //we are still waitin for the first parameter list response
            if (QGC::groundTimeMilliseconds() > this->listRecvTimeout) {
                //re-request parameters
                setParameterStatusMsg(tr("TIMEOUT: Re-requesting param list"),ParamCommsStatusLevel_Warning);
                listRecvTimeout = QGC::groundTimeMilliseconds() + 10000;
                mav->requestParameters();
            }
            return;
        }

        qDebug() << __FILE__ << __LINE__ << "RETRANSMISSION GUARD ACTIVE, CHECKING FOR DROPS..";

        // Check for timeout
        // stop retransmission attempts on timeout
        if (QGC::groundTimeMilliseconds() > transmissionTimeout) {
            setRetransmissionGuardEnabled(false);
            transmissionActive = false;

            // Empty read retransmission list
            // Empty write retransmission list
            int missingReadCount = 0;
            QList<int> readKeys = transmissionMissingPackets.keys();
            foreach (int component, readKeys) {
                missingReadCount += transmissionMissingPackets.value(component)->count();
                transmissionMissingPackets.value(component)->clear();
            }

            // Empty write retransmission list
            int missingWriteCount = 0;
            QList<int> writeKeys = transmissionMissingWriteAckPackets.keys();
            foreach (int component, writeKeys) {
                missingWriteCount += transmissionMissingWriteAckPackets.value(component)->count();
                transmissionMissingWriteAckPackets.value(component)->clear();
            }
            setParameterStatusMsg(tr("TIMEOUT! MISSING: %1 read, %2 write.").arg(missingReadCount).arg(missingWriteCount));
        }

        // Re-request at maximum retransmissionBurstRequestSize parameters at once
        // to prevent link flooding
        QMap<int, QMap<QString, QVariant>*>::iterator i;
        QMap<int, QMap<QString, QVariant>*>* onboardParams = paramDataModel->getOnboardParameters();
        for (i = onboardParams->begin(); i != onboardParams->end(); ++i) {
            // Iterate through the parameters of the component
            int component = i.key();
            // Request n parameters from this component (at maximum)
            QList<int> * paramList = transmissionMissingPackets.value(component, NULL);
            if (paramList) {
                int count = 0;
                foreach (int id, *paramList) {
                    if (count < retransmissionBurstRequestSize) {
                        //qDebug() << __FILE__ << __LINE__ << "RETRANSMISSION GUARD REQUESTS RETRANSMISSION OF PARAM #" << id << "FROM COMPONENT #" << component;
                        //TODO mavlink msg type for "request parameter set" ?
                        emit parameterUpdateRequestedById(component, id);
                        setParameterStatusMsg(tr("Requested retransmission of #%1").arg(id+1));
                        count++;
                    } else {
                        break;
                    }
                }
            }
        }

        // Re-request at maximum retransmissionBurstRequestSize parameters at once
        // to prevent write-request link flooding
        // Empty write retransmission list
        QList<int> writeKeys = transmissionMissingWriteAckPackets.keys();
        foreach (int component, writeKeys) {
            int count = 0;
            QMap <QString, QVariant>* missingParams = transmissionMissingWriteAckPackets.value(component);
            foreach (QString key, missingParams->keys()) {
                if (count < retransmissionBurstRequestSize) {
                    // Re-request write operation
                    QVariant value = missingParams->value(key);
                    switch ((int)onboardParams->value(component)->value(key).type())
                    {
                    case QVariant::Int:
                    {
                        QVariant fixedValue(value.toInt());
                        emit parameterChanged(component, key, fixedValue);
                    }
                        break;
                    case QVariant::UInt:
                    {
                        QVariant fixedValue(value.toUInt());
                        emit parameterChanged(component, key, fixedValue);
                    }
                        break;
                    case QMetaType::Float:
                    {
                        QVariant fixedValue(value.toFloat());
                        emit parameterChanged(component, key, fixedValue);
                    }
                        break;
                    default:
                        //qCritical() << "ABORTED PARAM RETRANSMISSION, NO VALID QVARIANT TYPE";
                        return;
                    }
                    setParameterStatusMsg(tr("Requested rewrite of: %1: %2").arg(key).arg(missingParams->value(key).toDouble()));
                    count++;
                } else {
                    break;
                }
            }
        }
    } else {
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
//    qDebug() << "setRetransmissionGuardEnabled: " << enabled;

    if (enabled) {
        retransmissionTimer.start(retransmissionTimeout);
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
    paramDataModel->getOnboardParameterValue(component,parameterName,onboardVal);
    if (onboardVal == value) {
        setParameterStatusMsg(tr("REJ. %1 already %2").arg(parameterName).arg(dblValue),
                              ParamCommsStatusLevel_Warning
                              );
        return;
    }

    //int paramType = (int)onboardParameters.value(component)->value(parameterName).type();
    int paramType = (int)value.type();
    switch (paramType)
    {
    case QVariant::Char:
    {
        QVariant fixedValue(QChar((unsigned char)value.toInt()));
        emit parameterChanged(component, parameterName, fixedValue);
    }
        break;
    case QVariant::Int:
    {
        QVariant fixedValue(value.toInt());
        emit parameterChanged(component, parameterName, fixedValue);
    }
        break;
    case QVariant::UInt:
    {
        QVariant fixedValue(value.toUInt());
        emit parameterChanged(component, parameterName, fixedValue);
    }
        break;
    case QMetaType::Float:
    {
        QVariant fixedValue(value.toFloat());
        emit parameterChanged(component, parameterName, fixedValue);
    }
        break;
    default:
        qCritical() << "ABORTED PARAM SEND, NO VALID QVARIANT TYPE";
        return;
    }

    // Wait for parameter to be written back
    // mark it therefore as missing
    if (!transmissionMissingWriteAckPackets.contains(component))
    {
        transmissionMissingWriteAckPackets.insert(component, new QMap<QString, QVariant>());
    }

    // Insert it in missing write ACK list
    transmissionMissingWriteAckPackets.value(component)->insert(parameterName, value);

    // Set timeouts
    if (transmissionActive)
    {
        transmissionTimeout += rewriteTimeout;
    }
    else
    {
        quint64 newTransmissionTimeout = QGC::groundTimeMilliseconds() + rewriteTimeout;
        if (newTransmissionTimeout > transmissionTimeout)
        {
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

    // Missing packets list has to be instantiated for all components
    if (!transmissionMissingPackets.contains(compId)) {
        transmissionMissingPackets.insert(compId, new QList<int>());
    }

    // List mode is different from single parameter transfers
    if (transmissionListMode) {
        // Only accept the list size once on the first packet from
        // each component
        if (!transmissionListSizeKnown.contains(compId)) {
            // Mark list size as known
            transmissionListSizeKnown.insert(compId, true);

            qDebug() << "Mark all parameters as missing";
            QList<int>* compParamList = transmissionMissingPackets.value(compId);
            for (int i = 0; i < paramCount; ++i) {
                if (!compParamList->contains(i)) {
                    if (i != paramId) {
                        compParamList->append(i);
                    }
                    else {
                        qDebug() << "Already received " << paramId;
                    }
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

        // Start retransmission guard
        // or reset timer
        setRetransmissionGuardEnabled(true); //TODO
    }

    // Mark this parameter as received in read list
    int index = transmissionMissingPackets.value(compId)->indexOf(paramId);
    // If the MAV sent the parameter without request, it wont be in missing list
    if (index != -1) {
        transmissionMissingPackets.value(compId)->removeAt(index);
    }

    bool justWritten = false;
    bool writeMismatch = false;
    //bool lastWritten = false;
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
        this->transmissionActive = false;
        this->transmissionListMode = false;
        transmissionListSizeKnown.clear();
        foreach (int key, transmissionMissingPackets.keys()) {
            transmissionMissingPackets.value(key)->clear();
        }

        setRetransmissionGuardEnabled(false);
        //all parameters have been received, broadcast to UI
        emit parameterListUpToDate();
    }
    else {
        qDebug() << "missCount:" << missCount << "missWriteCount:" << missWriteCount;
        foreach (int key, transmissionMissingPackets.keys()) {
            QList<int>* list = transmissionMissingPackets.value(key);
            qDebug() << "Component" << key << "missing numParams:" << list->count() ;
        }
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
    QMap<int, QMap<QString, QVariant>*>* changedValues = paramDataModel->getPendingParameters();
    QMap<int, QMap<QString, QVariant>*>::iterator i;
    for (i = changedValues->begin(); i != changedValues->end(); ++i) {
        // Iterate through the parameters of the component
        int compid = i.key();
        QMap<QString, QVariant>* comp = i.value();
        {
            QMap<QString, QVariant>::iterator j;
            for (j = comp->begin(); j != comp->end(); ++j) {
                //TODO mavlink command for "set parameter list" ?
                setParameter(compid, j.key(), j.value());
                parametersSent++;
            }
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

