#include "QGCPendingParamWidget.h"

#include "UASManager.h"
#include "UASParameterCommsMgr.h"


QGCPendingParamWidget::QGCPendingParamWidget(QObject *parent) :
    QGCParamWidget(UASManager::instance()->getActiveUAS(),(QWidget*)parent)
{
}


void QGCPendingParamWidget::init()
{
    //we override a lot of the super's init methods
    layoutWidget();
    connectSignalsAndSlots();

    //don't request update params here...assume that everything we need is in the data model
}

void QGCPendingParamWidget::connectSignalsAndSlots()
{
    // Listing for pending list update
    connect(paramDataModel, SIGNAL(pendingParamUpdate(int , const QString&, QVariant , bool )),
            this, SLOT(handlePendingParamUpdate(int , const QString& ,  QVariant, bool )));

    // Listen to communications status messages so we can display them
    connect(paramCommsMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SLOT(handleParamStatusMsgUpdate(QString , int )));
}


