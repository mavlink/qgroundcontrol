#include "UASParameterCommsMgr.h"

#include "QGCUASParamManager.h"
#include "UASInterface.h"

UASParameterCommsMgr::UASParameterCommsMgr(QObject *parent, UASInterface *uas) :
    QObject(parent)
{
    mav = uas;

    // Sending params to the UAS
    connect(this, SIGNAL(parameterChanged(int,QString,QVariant)), mav, SLOT(setParameter(int,QString,QVariant)));

    // New parameters from UAS
    connect(mav, SIGNAL(parameterChanged(int,int,int,int,QString,QVariant)), this, SLOT(receivedParameterUpdate(int,int,int,int,QString,QVariant)));

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

    // Clear transmission state
    receivedParamsList.clear();
    transmissionListSizeKnown.clear();

    transmissionListMode = true;
    foreach (int key, transmissionMissingPackets.keys()) {
        transmissionMissingPackets.value(key)->clear();
    }
    transmissionActive = true;

    setParameterStatusMsg(tr("Requested param list.. waiting"));
    mav->requestParameters();

}


void UASParameterCommsMgr::retransmissionGuardTick()
{
    if (transmissionActive) {
        //qDebug() << __FILE__ << __LINE__ << "RETRANSMISSION GUARD ACTIVE, CHECKING FOR DROPS..";

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
        QMap<int, QMap<QString, QVariant>*> onboardParams = paramDataModel->getOnboardParameters();
        for (i = onboardParams.begin(); i != onboardParams.end(); ++i) {
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
                    switch ((int)onboardParams.value(component)->value(key).type())
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
        //qDebug() << __FILE__ << __LINE__ << "STOPPING RETRANSMISSION GUARD GRACEFULLY";
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
        setParameterStatusMsg(tr("REJ. %1, %2 < min").arg(parameterName).arg(dblValue));
        return;
    }
    if (paramDataModel->isValueGreaterThanParamMax(parameterName,dblValue)) {
        setParameterStatusMsg(tr("REJ. %1, %2 > max").arg(parameterName).arg(dblValue));
        return;
    }
    QVariant onboardVal;
    paramDataModel->getOnboardParameterValue(component,parameterName,onboardVal);
    if (onboardVal == value) {
        setParameterStatusMsg(tr("REJ. %1 already %2").arg(parameterName).arg(dblValue));
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
        //qDebug() << "PARAM WIDGET SENT:" << fixedValue;
    }
        break;
    case QVariant::Int:
    {
        QVariant fixedValue(value.toInt());
        emit parameterChanged(component, parameterName, fixedValue);
        //qDebug() << "PARAM WIDGET SENT:" << fixedValue;
    }
        break;
    case QVariant::UInt:
    {
        QVariant fixedValue(value.toUInt());
        emit parameterChanged(component, parameterName, fixedValue);
        //qDebug() << "PARAM WIDGET SENT:" << fixedValue;
    }
        break;
    case QMetaType::Float:
    {
        QVariant fixedValue(value.toFloat());
        emit parameterChanged(component, parameterName, fixedValue);
        //qDebug() << "PARAM WIDGET SENT:" << fixedValue;
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

void UASParameterCommsMgr::setParameterStatusMsg(const QString& msg)
{
    qDebug() << "parameterStatusMsg: " << msg;
    parameterStatusMsg = msg;
}
