#include "QGCUASParamManager.h"

#include <QApplication>>
#include <QDir>

#include "UASInterface.h"
#include "UASParameterCommsMgr.h"

QGCUASParamManager::QGCUASParamManager(UASInterface* uas, QWidget *parent) :
    QWidget(parent),
    mav(uas),
    paramDataModel(NULL),
    paramCommsMgr(NULL)
{
    paramDataModel = uas->getParamDataModel();
    paramCommsMgr = uas->getParamCommsMgr();
    mav->setParamManager(this);

    // Load default values and tooltips
    loadParamMetaInfoCSV();



}


bool QGCUASParamManager::getParameterValue(int component, const QString& parameter, QVariant& value) const
{
    return paramDataModel->getOnboardParamValue(component,parameter,value);
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


void QGCUASParamManager::setParameterStatusMsg(const QString& msg)
{
    qDebug() << "parameterStatusMsg: " << msg;
    parameterStatusMsg = msg;
}

void QGCUASParamManager::setParamDescriptions(const QMap<QString,QString>& paramInfo) {
    paramDataModel->setParamDescriptions(paramInfo);
}


void QGCUASParamManager::setParameter(int component, QString parameterName, QVariant value)
{
    paramCommsMgr->setParameter(component,parameterName,value);
}

void QGCUASParamManager::sendPendingParameters()
{
    paramCommsMgr->sendPendingParameters();
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
