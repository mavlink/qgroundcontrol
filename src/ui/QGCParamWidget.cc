/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/
/**
 * @file
 *   @brief Implementation of class QGCParamWidget
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */
#include <cmath>
#include <float.h>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>

#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTime>

#include "MainWindow.h"
#include "QGC.h"
#include "QGCParamWidget.h"
#include "UASInterface.h"
#include "UASParameterCommsMgr.h"

/**
 * @param uas MAV to set the parameters on
 * @param parent Parent widget
 */
QGCParamWidget::QGCParamWidget(UASInterface* uas, QWidget *parent) :
    QGCUASParamManager(uas, parent),
    componentItems(new QMap<int, QTreeWidgetItem*>()),
    updatingParamNameLock("")
{

}

void QGCParamWidget::init()
{
    layoutWidget();
    connectSignalsAndSlots();

    // Ensure we have a list of params
    paramCommsMgr->requestParameterListIfEmpty();

}

void QGCParamWidget::connectSignalsAndSlots()
{

    // Listen for edits to the tree UI
    connect(tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(parameterItemChanged(QTreeWidgetItem*,int)));

    // Listen to updated param signals from the data model
    connect(paramDataModel, SIGNAL(parameterUpdated(int, QString , QVariant )),
            this, SLOT(handleParameterUpdate(int,QString,QVariant)));

    connect(paramDataModel, SIGNAL(pendingParamUpdate(int , const QString&, QVariant , bool )),
            this, SLOT(handlePendingParamUpdate(int , const QString& ,  QVariant, bool )));

    // Listen for param list reload finished
    connect(paramCommsMgr, SIGNAL(parameterListUpToDate()),
            this, SLOT(handleParameterListUpToDate()));

    // Listen to communications status messages so we can display them
    connect(paramCommsMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SLOT(handleParamStatusMsgUpdate(QString , int )));
}

void QGCParamWidget::layoutWidget()
{
    // Create tree widget
    tree = new QTreeWidget(this);
    statusLabel = new QLabel();
    statusLabel->setAutoFillBackground(true);

    // Set tree widget as widget onto this component
    QGridLayout* horizontalLayout;
    //form->setAutoFillBackground(false);
    horizontalLayout = new QGridLayout(this);
    horizontalLayout->setHorizontalSpacing(6);
    horizontalLayout->setVerticalSpacing(6);
    horizontalLayout->setMargin(0);
    horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
    //horizontalLayout->setSizeConstraint( QLayout::SetFixedSize );

    // Parameter tree
    horizontalLayout->addWidget(tree, 0, 0, 1, 3);

    // Status line
    statusLabel->setText(tr("Click refresh to download parameters"));
    horizontalLayout->addWidget(statusLabel, 1, 0, 1, 3);

    // BUTTONS
    QPushButton* refreshButton = new QPushButton(tr("Get"));
    refreshButton->setToolTip(tr("Load parameters currently in non-permanent memory of aircraft."));
    refreshButton->setWhatsThis(tr("Load parameters currently in non-permanent memory of aircraft."));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(requestAllParamsUpdate()));
    horizontalLayout->addWidget(refreshButton, 2, 0);

    QPushButton* setButton = new QPushButton(tr("Set"));
    setButton->setToolTip(tr("Set current parameters in non-permanent onboard memory"));
    setButton->setWhatsThis(tr("Set current parameters in non-permanent onboard memory"));
    connect(setButton, SIGNAL(clicked()),
            this, SLOT(sendPendingParameters()));
    horizontalLayout->addWidget(setButton, 2, 1);

    QPushButton* writeButton = new QPushButton(tr("Write (ROM)"));
    writeButton->setToolTip(tr("Copy current parameters in non-permanent memory of the aircraft to permanent memory. Transmit your parameters first to write these."));
    writeButton->setWhatsThis(tr("Copy current parameters in non-permanent memory of the aircraft to permanent memory. Transmit your parameters first to write these."));
    connect(writeButton, SIGNAL(clicked()), this, SLOT(writeParameters()));
    horizontalLayout->addWidget(writeButton, 2, 2);

    QPushButton* loadFileButton = new QPushButton(tr("Load File"));
    loadFileButton->setToolTip(tr("Load parameters from a file on this computer in the view. To write them to the aircraft, use transmit after loading them."));
    loadFileButton->setWhatsThis(tr("Load parameters from a file on this computer in the view. To write them to the aircraft, use transmit after loading them."));
    connect(loadFileButton, SIGNAL(clicked()), this, SLOT(loadParametersFromFile()));
    horizontalLayout->addWidget(loadFileButton, 3, 0);

    QPushButton* saveFileButton = new QPushButton(tr("Save File"));
    saveFileButton->setToolTip(tr("Save parameters in this view to a file on this computer."));
    saveFileButton->setWhatsThis(tr("Save parameters in this view to a file on this computer."));
    connect(saveFileButton, SIGNAL(clicked()), this, SLOT(saveParametersToFile()));
    horizontalLayout->addWidget(saveFileButton, 3, 1);

    QPushButton* readButton = new QPushButton(tr("Read (ROM)"));
    readButton->setToolTip(tr("Copy parameters from permanent memory to non-permanent current memory of aircraft. DOES NOT update the parameters in this view, click refresh after copying them to get them."));
    readButton->setWhatsThis(tr("Copy parameters from permanent memory to non-permanent current memory of aircraft. DOES NOT update the parameters in this view, click refresh after copying them to get them."));
    connect(readButton, SIGNAL(clicked()), this, SLOT(readParameters()));
    horizontalLayout->addWidget(readButton, 3, 2);

    // Set correct vertical scaling
    horizontalLayout->setRowStretch(0, 100);
    horizontalLayout->setRowStretch(1, 10);
    horizontalLayout->setRowStretch(2, 10);
    horizontalLayout->setRowStretch(3, 10);

    // Set layout
    this->setLayout(horizontalLayout);

    // Set header
    QStringList headerItems;
    headerItems.append("Parameter");
    headerItems.append("Value");
    tree->setHeaderLabels(headerItems);
    tree->setColumnCount(2);
    tree->setColumnWidth(0,200);
    tree->setColumnWidth(1,120);
    tree->setExpandsOnDoubleClick(true);

    tree->setVisible(true);
}


void QGCParamWidget::addComponentItem(int compId, QString compName)
{

    QString compLine = QString("%1 (#%2)").arg(compName).arg(compId);

    QString ptrStr = QString().sprintf("%8p", this);
    qDebug() <<  "QGCParamWidget" << ptrStr << "addComponentItem:" << compLine;

    if (componentItems->contains(compId)) {
        // Update existing component item
        componentItems->value(compId)->setData(0, Qt::DisplayRole, compLine);
        //components->value(component)->setData(1, Qt::DisplayRole, QString::number(component));
        componentItems->value(compId)->setFirstColumnSpanned(true);
    } else {
        // Add new component item
        QStringList list(compLine);
        QTreeWidgetItem* compItem = new QTreeWidgetItem(list);
        compItem->setFirstColumnSpanned(true);
        componentItems->insert(compId, compItem);
        // Create parameter grouping for this component and update maps
        paramGroups.insert(compId, new QMap<QString, QTreeWidgetItem*>());
        tree->addTopLevelItem(compItem);
        tree->update();
    }

    //TODO it seems unlikely that the UI would know about a component before the data model...
    paramDataModel->addComponent(compId);

}

void QGCParamWidget::handlePendingParamUpdate(int compId, const QString& paramName, QVariant value, bool isPending)
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
        paramItem->setBackground(0, QBrush(QColor(QGC::colorOrange)));
        paramItem->setBackground(1, QBrush(QColor(QGC::colorOrange)));
    }
    else {
        paramItem->setBackground(0, Qt::NoBrush);
        paramItem->setBackground(1, Qt::NoBrush);
    }

    updatingParamNameLock.clear();

}

void QGCParamWidget::handleParameterUpdate(int compId, const QString& paramName, QVariant value)
{
//    qDebug() << "handlePendingParamUpdate:" << paramName << "with updatingParamNameLock:" << updatingParamNameLock;
    if (paramName == updatingParamNameLock) {
        qDebug() << "handlePendingParamUpdate ignoring bounce from " << paramName;
        return;
    }
    updatingParamNameLock = paramName;
    updateParameterDisplay(compId, paramName, value);
    updatingParamNameLock.clear();
}


void QGCParamWidget::handleParameterListUpToDate()
{
    //turn off updates while we refresh the entire list
    tree->setUpdatesEnabled(false);

    //rewrite the component item tree after receiving the full list
    QMap<int, QMap<QString, QVariant>*>::iterator i;
    QMap<int, QMap<QString, QVariant>*>* onboardParams = paramDataModel->getAllOnboardParams();

    for (i = onboardParams->begin(); i != onboardParams->end(); ++i) {
        int compId = i.key();
        QMap<QString, QVariant>* paramPairs = onboardParams->value(compId);
        QMap<QString, QVariant>::iterator j;
        for (j = paramPairs->begin(); j != paramPairs->end(); j++) {
            updatingParamNameLock = j.key();
            updateParameterDisplay(compId, j.key(),j.value());
            updatingParamNameLock.clear();
        }
    }

    // Expand visual tree
    tree->expandItem(tree->topLevelItem(0));
    tree->setUpdatesEnabled(true);
    tree->update();

}

QTreeWidgetItem* QGCParamWidget::findChildWidgetItemForParam(QTreeWidgetItem* parentItem, const QString& paramName)
{
    QTreeWidgetItem* childItem = NULL;

    for (int i = 0; i < parentItem->childCount(); i++) {
        QTreeWidgetItem* child = parentItem->child(i);
        QString key = child->data(0, Qt::DisplayRole).toString();
        if (key == paramName)  {
            childItem = child;
            break;
        }
    }

    return childItem;
}

QTreeWidgetItem* QGCParamWidget::getParentWidgetItemForParam(int compId, const QString& paramName)
{
    QTreeWidgetItem* parentItem = componentItems->value(compId);

    QString splitToken = "_";
    // Check if auto-grouping can work
    if (paramName.contains(splitToken)) {
        QString parentStr = paramName.section(splitToken, 0, 0, QString::SectionSkipEmpty);
        QMap<QString, QTreeWidgetItem*>* compParamGroups = paramGroups.value(compId);
        if (!compParamGroups->contains(parentStr)) {
            // Insert group item
            QStringList glist;
            glist.append(parentStr);
            QTreeWidgetItem* groupItem = new QTreeWidgetItem(glist);

            compParamGroups->insert(parentStr, groupItem);

            // insert new group alphabetized
            QList<QString> groupKeys = compParamGroups->uniqueKeys();
            int insertIdx = groupKeys.indexOf(parentStr);
            componentItems->value(compId)->insertChild(insertIdx,groupItem);
        }

        //parent item for this parameter item will be a group widget item
        parentItem = compParamGroups->value(parentStr);
    }
    else  {
        //parent item for this parameter will be the top level (component) widget item
        parentItem = componentItems->value(compId);
    }

    return parentItem;
}

QTreeWidgetItem* QGCParamWidget::updateParameterDisplay(int compId, QString parameterName, QVariant value)
{
//    qDebug() << "QGCParamWidget::updateParameterDisplay" << parameterName;

    // Reference to item in tree
    QTreeWidgetItem* paramItem = NULL;

    // Add component item if necessary
    if (!componentItems->contains(compId)) {
        QString componentName = tr("Component #%1").arg(compId);
        addComponentItem(compId, componentName);
    }

    //default parent item for this parameter widget item will be the top level component item
    QTreeWidgetItem* parentItem = getParentWidgetItemForParam(compId,parameterName);
    if (parentItem) {
        paramItem = findChildWidgetItemForParam(parentItem,parameterName);
        if (!paramItem) {
            // Insert parameter into map
            QStringList plist;
            plist.append(parameterName);
            // CREATE PARAMETER ITEM
            paramItem = new QTreeWidgetItem(plist);
            // CONFIGURE PARAMETER ITEM
            if (value.type() == QVariant::Char) {
                paramItem->setData(1, Qt::DisplayRole, value.toUInt());
            }
            else {
                paramItem->setData(1, Qt::DisplayRole, value);
            }
            paramItem->setFlags(paramItem->flags() | Qt::ItemIsEditable);

            //TODO insert alphabetically
            parentItem->addChild(paramItem);

            //only add the tooltip when the parameter item is first added
            QString paramDesc = paramDataModel->getParamDescription(parameterName);
            if (!paramDesc.isEmpty()) {
                QString tooltipFormat;
                if (paramDataModel->isParamDefaultKnown(parameterName)) {
                    tooltipFormat = tr("Default: %1, %2");
                    double paramDefValue = paramDataModel->getParamDefault(parameterName);
                    tooltipFormat = tooltipFormat.arg(paramDefValue).arg(paramDesc);
                }
                else {
                    tooltipFormat = paramDesc;
                }
                paramItem->setToolTip(0, tooltipFormat);
                paramItem->setToolTip(1, tooltipFormat);
            }
        }

        //update the parameterItem's data
        if (value.type() == QVariant::Char) {
            paramItem->setData(1, Qt::DisplayRole, value.toUInt());
        }
        else {
            paramItem->setData(1, Qt::DisplayRole, value);
        }
    }

    if (paramItem) {
        // Reset background color
        paramItem->setBackground(0, Qt::NoBrush);
        paramItem->setBackground(1, Qt::NoBrush);

        paramItem->setTextColor(0, QGC::colorDarkWhite);
        paramItem->setTextColor(1, QGC::colorDarkWhite);

        if (paramItem == tree->currentItem()) {
            //need to unset current item to clear highlighting (green by default)
            tree->setCurrentItem(NULL); //clear the selected line
        }

    }
    return paramItem;

}



void QGCParamWidget::parameterItemChanged(QTreeWidgetItem* paramItem, int column)
{

    if (paramItem && column > 0) {

        QString key = paramItem->data(0, Qt::DisplayRole).toString();
        //qDebug() << "parameterItemChanged:" << key << "with updatingParamNameLock:" << updatingParamNameLock;

        if (key == updatingParamNameLock) {
            //qDebug() << "parameterItemChanged ignoring parameterItemChanged" << key;
            return;
        }
        else {
            updatingParamNameLock = key;
        }

        QTreeWidgetItem* parent = paramItem->parent();
        while (parent->parent() != NULL) {
            parent = parent->parent();
        }
        // Parent is now top-level component
        int componentId = componentItems->key(parent);
        QVariant value = paramItem->data(1, Qt::DisplayRole);


        bool pending = paramDataModel->updatePendingParamWithValue(componentId,key,value);

        // If the value will result in an update
        if (pending) {
            // Set parameter on changed list to be transmitted to MAV
            statusLabel->setText(tr("Pending: %1:%2: %3").arg(componentId).arg(key).arg(value.toFloat(), 5, 'f', 1, QChar(' ')));

            paramItem->setBackground(0, QBrush(QColor(QGC::colorOrange)));
            paramItem->setBackground(1, QBrush(QColor(QGC::colorOrange)));
        }
        else {
            QMap<QString , QVariant>* pendingParams = paramDataModel->getPendingParamsForComponent(componentId);
            int pendingCount = pendingParams->count();
            statusLabel->setText(tr("Pending items: %1").arg(pendingCount));
            paramItem->setBackground(0, Qt::NoBrush);
            paramItem->setBackground(1, Qt::NoBrush);
        }


        if (paramItem == tree->currentItem()) {
            //need to unset current item to clear highlighting (green by default)
            tree->setCurrentItem(NULL); //clear the selected line
        }

        updatingParamNameLock.clear();
    }
}



void QGCParamWidget::saveParametersToFile()
{
    if (!mav) return;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "./parameters.txt", tr("Parameter File (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream outstream(&file);
    paramDataModel->writeOnboardParamsToStream(outstream,mav->getUASName());
    file.close();
}


void QGCParamWidget::loadParametersFromFile()
{
    if (!mav) return;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), ".", tr("Parameter file (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    paramDataModel->readUpdateParamsFromStream(in);
    file.close();
}

void QGCParamWidget::setParameterStatusMsg(const QString& msg)
{
    statusLabel->setText(msg);
}

void QGCParamWidget::requestAllParamsUpdate()
{
    if (!mav) {
        return;
    }

    // Clear view and request param list
    clear();
    //paramDataModel->forgetAllOnboardParameters(); //TODO really??

    requestParameterList();
}



/**
 * Write the current onboard parameters from RAM into
 * permanent storage, e.g. EEPROM or harddisk
 */
void QGCParamWidget::writeParameters()
{
    int changedParamCount = paramDataModel->countPendingParams();

    if (changedParamCount > 0) {
        QMessageBox msgBox;
        msgBox.setText(tr("There are locally changed parameters. Please transmit them first (<TRANSMIT>) or update them with the onboard values (<REFRESH>) before storing onboard from RAM to ROM."));
        msgBox.exec();
    }
    else {
        paramCommsMgr->writeParamsToPersistentStorage();
    }
}


void QGCParamWidget::readParameters()
{
    if (!mav) return;
    mav->readParametersFromStorage(); //TODO use data model / mgr instead?
}

/**
 * Clear all data in the parameter widget
 */
void QGCParamWidget::clear()
{
    tree->clear();
    componentItems->clear();
}

void QGCParamWidget::handleParamStatusMsgUpdate(QString msg, int level)
{
    QColor bgColor = QGC::colorGreen;
    if ((int)UASParameterCommsMgr::ParamCommsStatusLevel_Warning == level) {
        bgColor = QGC::colorOrange;
    }
    else if ((int)UASParameterCommsMgr::ParamCommsStatusLevel_Error == level) {
        bgColor =  QGC::colorRed;
    }

    QPalette pal = statusLabel->palette();
    pal.setColor(backgroundRole(), bgColor);
    statusLabel->setPalette(pal);
    statusLabel->setText(msg);

}
