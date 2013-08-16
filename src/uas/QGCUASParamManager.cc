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
    paramCommsMgr(NULL)
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
}


void QGCUASParamManager::clearAllPendingParams()
{
    paramDataModel.clearAllPendingParams();
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
            requestParameterList();
        }
    }
}


void QGCUASParamManager::setParamDescriptions(const QMap<QString,QString>& paramInfo) {
    paramDataModel.setParamDescriptions(paramInfo);
}


void QGCUASParamManager::setParameter(int compId, QString paramName, QVariant value)
{
    paramDataModel.updatePendingParamWithValue(compId,paramName,value);
}

void QGCUASParamManager::sendPendingParameters()
{
    paramCommsMgr->sendPendingParameters();
}

void QGCUASParamManager::setPendingParam(int compId,  QString& paramName,  const QVariant& value)
{
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
