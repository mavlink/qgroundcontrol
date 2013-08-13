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

void QGCPendingParamWidget::handlePendingParamUpdate(int compId, const QString& paramName, QVariant value, bool isPending)
{
   // qDebug() << "handlePendingParamUpdate:" << paramName << "with updatingParamNameLock:" << updatingParamNameLock;

    if (updatingParamNameLock == paramName) {
        //qDebug() << "ignoring bounce from " << paramName;
        return;
    }
    else {
        updatingParamNameLock = paramName;
    }

    QTreeWidgetItem* paramItem = updateParameterDisplay(compId,paramName,value);

    if (isPending) {
        QTreeWidgetItem* paramItem = updateParameterDisplay(compId,paramName,value);
        paramItem->setFlags(paramItem->flags() & ~Qt::ItemIsEditable); //disallow editing
        paramItem->setBackground(0, QBrush(QColor(QGC::colorOrange)));
        paramItem->setBackground(1, QBrush(QColor(QGC::colorOrange)));
        tree->expandAll();
    }
    else {
        //we don't display non-pending items
        paramItem->parent()->removeChild(paramItem);
    }

    updatingParamNameLock.clear();

}

