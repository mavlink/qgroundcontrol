#include "QGCPendingParamWidget.h"

#include <QGridLayout>
#include <QPushButton>

#include "UASManager.h"
#include "UASParameterCommsMgr.h"


QGCPendingParamWidget::QGCPendingParamWidget(QObject *parent) :
    QGCParamWidget((QWidget*)parent)
{
}


void QGCPendingParamWidget::connectToParamManager()
{
    paramMgr = mav->getParamManager();

    // Listen to updated param signals from the data model
    connect(paramMgr, SIGNAL(pendingParamUpdate(int , const QString&, QVariant , bool )),
            this, SLOT(handlePendingParamUpdate(int , const QString& ,  QVariant, bool )));

    // Listen to communications status messages so we can display them
    connect(paramMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SLOT(handleParamStatusMsgUpdate(QString , int )));
}


void QGCPendingParamWidget::disconnectFromParamManager()
{
    // Listen to updated param signals from the data model
    disconnect(paramMgr, SIGNAL(pendingParamUpdate(int , const QString&, QVariant , bool )),
            this, SLOT(handlePendingParamUpdate(int , const QString& ,  QVariant, bool )));

    // Listen to communications status messages so we can display them
    disconnect(paramMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SLOT(handleParamStatusMsgUpdate(QString , int )));

    paramMgr = NULL;
}


void QGCPendingParamWidget::disconnectViewSignalsAndSlots()
{
    //we ignore edits from the tree view
}


void QGCPendingParamWidget::connectViewSignalsAndSlots()
{
    //we ignore edits from the tree view
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
        QTreeWidgetItem* groupItem = paramItem->parent();
        if (NULL != groupItem) {
            tree->setUpdatesEnabled(false);
            QTreeWidgetItem* componentItem = NULL;
            if (1 == groupItem->childCount()) {
                componentItem = groupItem->parent();
            }

            //always remove the actual paramItem from its parent
            groupItem->removeChild(paramItem);

            //now we may need to remove the groupItem if it has no more children
            if (NULL != componentItem) {
                //remove the group from our internal data structures
                QMap<QString, QTreeWidgetItem*>* compParamGroups = paramGroups.value(compId);
                QString groupStr = paramName.section("_", 0, 0, QString::SectionSkipEmpty);
                compParamGroups->remove(groupStr);
                //remove the group item from componentItems
                componentItems->value(compId)->removeChild(groupItem);
                // remove the group item from the tree widget itself
                componentItem->removeChild(groupItem);

                if (0 == componentItem->childCount()) {
                    //the component itself no longer has any pending changes: remove it
                    paramGroups.remove(compId);
                    componentItems->remove(compId);
                    QTreeWidgetItem* compTop = tree->takeTopLevelItem(tree->indexOfTopLevelItem(componentItem));
                    delete compTop; //we own it after take
                }
            }
            tree->setUpdatesEnabled(true);
            tree->update();

        }
    }

    updatingParamNameLock.clear();

}

void QGCPendingParamWidget::addActionButtonsToLayout(QGridLayout* layout)
{

    QPushButton* setButton = new QPushButton(tr("Set"));
    setButton->setToolTip(tr("Send pending parameters to volatile onboard memory"));
    setButton->setWhatsThis(tr("Send pending parameters to volatile onboard memory"));
    connect(setButton, SIGNAL(clicked()),
            paramMgr, SLOT(sendPendingParameters()));
    layout->addWidget(setButton, 2, 0);

    QPushButton* clearButton = new QPushButton(tr("Clear"));
    clearButton->setToolTip(tr("Clear pending parameters without sending"));
    clearButton->setWhatsThis(tr("Clear pending parameters without sending"));
    connect(clearButton, SIGNAL(clicked()),
            paramMgr, SLOT(clearAllPendingParams()));
    layout->addWidget(clearButton, 2, 1);


}
