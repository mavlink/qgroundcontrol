#include "QGCUASParamManager.h"
#include "UASInterface.h"

#include "UASParameterCommsMgr.h"

QGCUASParamManager::QGCUASParamManager(UASInterface* uas, QWidget *parent) :
    QWidget(parent),
    mav(uas)
{
    paramDataModel = uas->getParamDataModel();
    paramCommsMgr = uas->getParamCommsMgr();
    mav->setParamManager(this);

    // Load default values and tooltips
    loadParamMetaInfoCSV();


    // Connect retransmission guard
    connect(this, SIGNAL(parameterUpdateRequested(int,QString)),
            this, SLOT(requestParameterUpdate(int,QString)));

//    //TODO connect in paramCommsMgr instead
//    connect(this, SIGNAL(parameterUpdateRequestedById(int,int)),
//            mav, SLOT(requestParameter(int,int)));

    // New parameters from UAS

    void parameterUpdated(int compId, int paramId, QString paramName, QVariant value);

    connect(paramDataModel, SIGNAL(parameterUpdated(int, int, QString , QVariant )),
            this, SLOT(handleParameterUpdate(int,int,int,QString,QVariant));

//    connect(uas, SIGNAL(parameterChanged(int,int,int,int,QString,QVariant)),
//            this, SLOT(receivedParameterUpdate(int,int,int,int,QString,QVariant)));


    // Listen for param list reload finished
    connect(paramCommsMgr, SIGNAL(parameterListUpToDate(int)),
            this, SLOT(handleParameterListUpToDate(int))));



}


bool QGCUASParamManager::getParameterValue(int component, const QString& parameter, QVariant& value) const
{
    return paramDataModel->getOnboardParameterValue(component,parameter,value);
}


void QGCUASParamManager::requestParameterUpdate(int component, const QString& parameter)
{
    if (mav) {
        paramCommsMgr->requestParameterUpdate(component,parameter);
    }
}



/**
 * Send a request to deliver the list of onboard parameters
 * to the MAV.
 */
void QGCUASParamManager::requestParameterList()
{
    if (!mav) {
        return;
    }
    //paramDataModel->forgetAllOnboardParameters(); //TODO really??
    setParameterStatusMsg(tr("Requested param list.. waiting"));
    paramCommsMgr->requestParameterList();
}

//void QGCUASParamManager::retransmissionGuardTick()
//{
//    if (transmissionActive) {
//        //qDebug() << __FILE__ << __LINE__ << "RETRANSMISSION GUARD ACTIVE, CHECKING FOR DROPS..";

//        // Check for timeout
//        // stop retransmission attempts on timeout
//        if (QGC::groundTimeMilliseconds() > transmissionTimeout) {
//            setRetransmissionGuardEnabled(false);
//            transmissionActive = false;

//            // Empty read retransmission list
//            // Empty write retransmission list
//            int missingReadCount = 0;
//            QList<int> readKeys = transmissionMissingPackets.keys();
//            foreach (int component, readKeys) {
//                missingReadCount += transmissionMissingPackets.value(component)->count();
//                transmissionMissingPackets.value(component)->clear();
//            }

//            // Empty write retransmission list
//            int missingWriteCount = 0;
//            QList<int> writeKeys = transmissionMissingWriteAckPackets.keys();
//            foreach (int component, writeKeys) {
//                missingWriteCount += transmissionMissingWriteAckPackets.value(component)->count();
//                transmissionMissingWriteAckPackets.value(component)->clear();
//            }
//            setParameterStatusMsg(tr("TIMEOUT! MISSING: %1 read, %2 write.").arg(missingReadCount).arg(missingWriteCount));
//        }

//        // Re-request at maximum retransmissionBurstRequestSize parameters at once
//        // to prevent link flooding
//        QMap<int, QMap<QString, QVariant>*>::iterator i;
//        QMap<int, QMap<QString, QVariant>*> onboardParams = paramDataModel->getOnboardParameters();
//        for (i = onboardParams.begin(); i != onboardParams.end(); ++i) {
//            // Iterate through the parameters of the component
//            int component = i.key();
//            // Request n parameters from this component (at maximum)
//            QList<int> * paramList = transmissionMissingPackets.value(component, NULL);
//            if (paramList) {
//                int count = 0;
//                foreach (int id, *paramList) {
//                    if (count < retransmissionBurstRequestSize) {
//                        //qDebug() << __FILE__ << __LINE__ << "RETRANSMISSION GUARD REQUESTS RETRANSMISSION OF PARAM #" << id << "FROM COMPONENT #" << component;
//                        //TODO mavlink msg type for "request parameter set" ?
//                        emit parameterUpdateRequestedById(component, id);
//                        setParameterStatusMsg(tr("Requested retransmission of #%1").arg(id+1));
//                        count++;
//                    } else {
//                        break;
//                    }
//                }
//            }
//        }

//        // Re-request at maximum retransmissionBurstRequestSize parameters at once
//        // to prevent write-request link flooding
//        // Empty write retransmission list
//        QList<int> writeKeys = transmissionMissingWriteAckPackets.keys();
//        foreach (int component, writeKeys) {
//            int count = 0;
//            QMap <QString, QVariant>* missingParams = transmissionMissingWriteAckPackets.value(component);
//            foreach (QString key, missingParams->keys()) {
//                if (count < retransmissionBurstRequestSize) {
//                    // Re-request write operation
//                    QVariant value = missingParams->value(key);
//                    switch ((int)onboardParams.value(component)->value(key).type())
//                    {
//                    case QVariant::Int:
//                    {
//                        QVariant fixedValue(value.toInt());
//                        emit parameterChanged(component, key, fixedValue);
//                    }
//                        break;
//                    case QVariant::UInt:
//                    {
//                        QVariant fixedValue(value.toUInt());
//                        emit parameterChanged(component, key, fixedValue);
//                    }
//                        break;
//                    case QMetaType::Float:
//                    {
//                        QVariant fixedValue(value.toFloat());
//                        emit parameterChanged(component, key, fixedValue);
//                    }
//                        break;
//                    default:
//                        //qCritical() << "ABORTED PARAM RETRANSMISSION, NO VALID QVARIANT TYPE";
//                        return;
//                    }
//                    setParameterStatusMsg(tr("Requested rewrite of: %1: %2").arg(key).arg(missingParams->value(key).toDouble()));
//                    count++;
//                } else {
//                    break;
//                }
//            }
//        }
//    } else {
//        //qDebug() << __FILE__ << __LINE__ << "STOPPING RETRANSMISSION GUARD GRACEFULLY";
//        setRetransmissionGuardEnabled(false);
//    }
//}



//*
// * Enabling the retransmission guard enables the parameter widget to track
// * dropped parameters and to re-request them. This works for both individual
// * parameter reads as well for whole list requests.
// *
// * @param enabled True if retransmission checking should be enabled, false else

//void QGCUASParamManager::setRetransmissionGuardEnabled(bool enabled)
//{
//    qDebug() << "setRetransmissionGuardEnabled: " << enabled;
//
//    if (enabled) {
//        retransmissionTimer.start(retransmissionTimeout);
//    } else {
//        retransmissionTimer.stop();
//    }
//}

void QGCUASParamManager::setParameterStatusMsg(const QString& msg)
{
    qDebug() << "parameterStatusMsg: " << msg;
    parameterStatusMsg = msg;
}

void QGCUASParamManager::setParamDescriptions(const QMap<QString,QString>& paramInfo) {
    paramDataModel->setParamDescriptions(paramInfo);
}




void QGCUASParamManager::loadParamMetaInfoCSV()
{
    // Load default values and tooltips
    QString autopilot(mav->getAutopilotTypeName());

    QDir appDir = QApplication::applicationDirPath();
    appDir.cd("files");
    QString fileName = QString("%1/%2/parameter_tooltips/tooltips.txt").arg(appDir.canonicalPath()).arg(autopilot.toLower());
    QFile paramMetaFile(fileName);

    qDebug() << "loadParamMetaInfoCSV for autopilot: " << autopilot << " from file: " << fileName;

    if (!paramMetaFile.open(QIODevice::ReadOnly | QIODevice::Text))  {
        qWarning() << "loadParamMetaInfoCSV couldn't open:" << fileName;
        return;
    }

    QTextStream in(&paramMetaFile);
    paramDataModel->loadParamMetaInfoFromStream(in);
    paramMetaFile.close();

}

/**
 * @return The MAV of this mgr. Unless the MAV object has been destroyed, this
 *         pointer is never zero.
 */
UASInterface* QGCUASParamManager::getUAS()
{
    return mav;
}
