#include "QGCUASParamManager.h"

#include <QApplication>
#include <QDir>
#include <QMessageBox>

#include "UASInterface.h"
#include "UASParameterCommsMgr.h"

QGCUASParamManager::QGCUASParamManager(QObject *parent) :
    QObject(parent),
    mav(NULL),
    paramDataModel(this),
    paramCommsMgr(NULL),
    defaultComponentId(-1)
{


}

QGCUASParamManager* QGCUASParamManager::initWithUAS(UASInterface* uas)
{
    mav = uas;

    // Load default values and tooltips for data model
    loadParamMetaInfoCSV();

    paramCommsMgr = new UASParameterCommsMgr(this);
    paramCommsMgr->initWithUAS(uas);

    connectToModelAndComms();

    return this;
}

void QGCUASParamManager::connectToModelAndComms()
{
    // Pass along comms mgr status msgs
    connect(paramCommsMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SIGNAL(parameterStatusMsgUpdated(QString,int)));

    connect(paramCommsMgr, SIGNAL(parameterListUpToDate()),
            this, SIGNAL(parameterListUpToDate()));

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
    int result = 0;

    if (-1 != defaultComponentId)
        return defaultComponentId;

    QList<int> components = getComponentForParam("SYS_AUTOSTART");//TODO is this the best way to find the right component?

    // Guard against multiple components responding - this will never show in practice
    if (1 == components.count()) {
        result = components.first();
        defaultComponentId = result;
    }

    qDebug() << "Default compId: " << result;

    return result;
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
        paramCommsMgr->requestParameterUpdate(component,parameter);
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
        paramCommsMgr->requestParameterList();
    }
}

void QGCUASParamManager::requestParameterListIfEmpty()
{
    if (mav) {
        int totalOnboard = paramDataModel.countOnboardParams();
        if (totalOnboard < 2) { //TODO arbitrary constant, maybe 0 is OK?
            defaultComponentId = -1; //reset this ...we have no idea what the default component ID is
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
        compId = getDefaultComponentId();
    }
    paramDataModel.updatePendingParamWithValue(compId,paramName,value);
}

void QGCUASParamManager::sendPendingParameters(bool persistAfterSend)
{
    paramCommsMgr->sendPendingParameters(persistAfterSend);
}




void QGCUASParamManager::setPendingParam(int compId,  const QString& paramName,  const QVariant& value)
{
    if ((0 == compId) || (-1 == compId)) {
        //attempt to get an actual component ID
        compId = getDefaultComponentId();
    }
    paramDataModel.updatePendingParamWithValue(compId,paramName,value);
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
        QMessageBox msgBox;
        msgBox.setText(tr("There are locally changed parameters. Please transmit them first (<TRANSMIT>) or update them with the onboard values (<REFRESH>) before storing onboard from RAM to ROM."));
        msgBox.exec();
    }
    else {
        paramCommsMgr->writeParamsToPersistentStorage();
    }
}


void QGCUASParamManager::copyPersistentParamsToVolatile()
{
    if (mav) {
        mav->readParametersFromStorage(); //TODO use comms mgr instead?
    }
}


void QGCUASParamManager::requestRcCalibrationParamsUpdate() {
    paramCommsMgr->requestRcCalibrationParamsUpdate();
}
