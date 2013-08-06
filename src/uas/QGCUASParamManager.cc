#include "QGCUASParamManager.h"
#include "UASInterface.h"

QGCUASParamManager::QGCUASParamManager(UASInterface* uas, QWidget *parent) :
    QWidget(parent),
    mav(uas),
    transmissionListMode(false),
    transmissionActive(false),
    transmissionTimeout(0),
    retransmissionTimeout(350),
    rewriteTimeout(500),
    retransmissionBurstRequestSize(5)
{
    uas->setParamManager(this);
    paramDataModel = uas->getParamDataModel();
}


/**
 * The .. signal is emitted
 */
void QGCUASParamManager::requestParameterListUpdate(int component)
{
	Q_UNUSED(component);
}

bool QGCUASParamManager::getParameterValue(int component, const QString& parameter, QVariant& value) const {
    return paramDataModel->getOnboardParameterValue(component,parameter,value);
}

