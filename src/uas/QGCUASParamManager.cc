#include "QGCUASParamManager.h"

#include <QApplication>
#include <QDir>
#include <QDebug>

#include "UASInterface.h"
#include "QGCMessageBox.h"

QGCUASParamManager::QGCUASParamManager(QObject *parent) :
    QGCUASParamManagerInterface(parent),
    mav(NULL),
    paramDataModel(this),
    _parametersReady(false)
{


}

QGCUASParamManager* QGCUASParamManager::initWithUAS(UASInterface* uas)
{
    mav = uas;

    // Load default values and tooltips for data model
    loadParamMetaInfoCSV();

    paramDataModel.setUASID(mav->getUASID());
    paramCommsMgr.initWithUAS(uas);

    connectToModelAndComms();

    return this;
}

void QGCUASParamManager::connectToModelAndComms()
{
    // Pass along comms mgr status msgs
    connect(&paramCommsMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SIGNAL(parameterStatusMsgUpdated(QString,int)));

    connect(&paramCommsMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_parameterListUpToDate()));
    
    connect(&paramCommsMgr, SIGNAL(parameterListProgress(float)), this, SIGNAL(parameterListProgress(float)));

    // Pass along data model updates
    connect(&paramDataModel, SIGNAL(parameterUpdated(int, QString , QVariant )),
            this, SIGNAL(parameterUpdated(int, QString , QVariant )));

    connect(&paramDataModel, SIGNAL(pendingParamUpdate(int , const QString& , QVariant , bool )),
            this, SIGNAL(pendingParamUpdate(int , const QString& , QVariant , bool )));


}


void QGCUASParamManager::clearAllPendingParams()
{
    paramDataModel.clearAllPendingParams();
    emit parameterStatusMsgUpdated(tr("Cleared all pending params"), UASParameterCommsMgr::ParamCommsStatusLevel_OK);

}


int QGCUASParamManager::getDefaultComponentId()
{
    return paramDataModel.getDefaultComponentId();
}

QList<int> QGCUASParamManager::getComponentForParam(const QString& parameter) const
{
    return paramDataModel.getComponentForOnboardParam(parameter);
}


bool QGCUASParamManager::getParameterValue(int component, const QString& parameter, QVariant& value) const
{
    return paramDataModel.getOnboardParamValue(component,parameter,value);
}


void QGCUASParamManager::requestParameterUpdate(int component, const QString& parameter)
{
    if (mav) {
        paramCommsMgr.requestParameterUpdate(component,parameter);
    }
}



/**
 * Send a request to deliver the list of onboard parameters
 * to the MAV.
 */
void QGCUASParamManager::requestParameterList()
{
    if (mav) {
        emit parameterStatusMsgUpdated(tr("Requested param list.. waiting"), UASParameterCommsMgr::ParamCommsStatusLevel_OK);
        paramCommsMgr.requestParameterList();
    }
}

void QGCUASParamManager::requestParameterListIfEmpty()
{
    if (mav) {
        int totalOnboard = paramDataModel.countOnboardParams();
        if (totalOnboard < 2) { //TODO arbitrary constant, maybe 0 is OK?
            requestParameterList();
        }
    }
}


void QGCUASParamManager::setParamDescriptions(const QMap<QString,QString>& paramInfo) {
    paramDataModel.setParamDescriptions(paramInfo);
}


void QGCUASParamManager::setParameter(int compId, QString paramName, QVariant value)
{
    if ((0 == compId) || (-1 == compId)) {
        //attempt to get an actual component ID
        compId = paramDataModel.getDefaultComponentId();
    }
    paramDataModel.updatePendingParamWithValue(compId,paramName,value);
}

void QGCUASParamManager::sendPendingParameters(bool persistAfterSend, bool forceSend)
{
    paramCommsMgr.sendPendingParameters(persistAfterSend, forceSend);
}




void QGCUASParamManager::setPendingParam(int compId,  const QString& paramName,  const QVariant& value, bool forceSend)
{
    if ((0 == compId) || (-1 == compId)) {
        //attempt to get an actual component ID
        compId = paramDataModel.getDefaultComponentId();
    }
    paramDataModel.updatePendingParamWithValue(compId,paramName,value, forceSend);
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
        qDebug() << "loadParamMetaInfoCSV couldn't open:" << fileName;
        return;
    }

    QTextStream in(&paramMetaFile);
    paramDataModel.loadParamMetaInfoFromStream(in);
    paramMetaFile.close();

}


UASInterface* QGCUASParamManager::getUAS()
{
    return mav;
}

UASParameterDataModel* QGCUASParamManager::dataModel()
{
    return &paramDataModel;
}



void QGCUASParamManager::writeOnboardParamsToStream(QTextStream &stream, const QString& uasName)
{
    paramDataModel.writeOnboardParamsToStream(stream,uasName);
}

void QGCUASParamManager::readPendingParamsFromStream(QTextStream &stream)
{
    paramDataModel.readUpdateParamsFromStream(stream);
}



void QGCUASParamManager::copyVolatileParamsToPersistent()
{
    int changedParamCount = paramDataModel.countPendingParams();

    if (changedParamCount > 0) {
        QGCMessageBox::warning(tr("Warning"),
                                   tr("There are locally changed parameters. Please transmit them first (<TRANSMIT>) or update them with the onboard values (<REFRESH>) before storing onboard from RAM to ROM."));
    }
    else {
        paramCommsMgr.writeParamsToPersistentStorage();
    }
}


void QGCUASParamManager::copyPersistentParamsToVolatile()
{
    if (mav) {
        mav->readParametersFromStorage(); //TODO use comms mgr instead?
    }
}


void QGCUASParamManager::requestRcCalibrationParamsUpdate() {
    paramCommsMgr.requestRcCalibrationParamsUpdate();
}

void QGCUASParamManager::_parameterListUpToDate(void)
{
    //qDebug() << "Emitting parameters ready, count:" << paramDataModel.countOnboardParams();

    _parametersReady = true;
    emit parameterListUpToDate();
}
