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
}


/**
 * The .. signal is emitted
 */
void QGCUASParamManager::requestParameterListUpdate(int component)
{
	Q_UNUSED(component);
}


