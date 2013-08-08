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
    componentItems(new QMap<int, QTreeWidgetItem*>())
{

    // Create tree widget
    tree = new QTreeWidget(this);
    statusLabel = new QLabel();
    statusLabel->setAutoFillBackground(true);
    tree->setColumnWidth(70, 30);

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
    connect(setButton, SIGNAL(clicked()), this, SLOT(setParameters()));
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
    tree->setExpandsOnDoubleClick(true);

    // Connect signals/slots
//    connect(this, SIGNAL(parameterChanged(int,QString,QVariant)),
//            mav, SLOT(setParameter(int,QString,QVariant)));
    connect(tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(parameterItemChanged(QTreeWidgetItem*,int)));

//    // New parameters from UAS
//    connect(uas, SIGNAL(parameterChanged(int,int,int,int,QString,QVariant)), this, SLOT(receivedParameterUpdate(int,int,int,int,QString,QVariant)));


//    connect(&retransmissionTimer, SIGNAL(timeout()), this, SLOT(retransmissionGuardTick()));
//    connect(this, SIGNAL(requestParameter(int,QString)), uas, SLOT(requestParameter(int,QString)));
//    connect(this, SIGNAL(requestParameter(int,int)), uas, SLOT(requestParameter(int,int)));


    connect(paramDataModel, SIGNAL(parameterUpdated(int, QString , QVariant )),
            this, SLOT(handleParameterUpdate(int,QString,QVariant)));

    // Listen for param list reload finished
    connect(paramCommsMgr, SIGNAL(parameterListUpToDate()),
            this, SLOT(handleParameterListUpToDate()));

    connect(paramCommsMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SLOT(handleParamStatusMsgUpdate(QString , int )));

    // Get parameters
    if (uas) {
        requestAllParamsUpdate();
    }
}







void QGCParamWidget::addComponentItem( int compId, QString compName)
{
    if (componentItems->contains(compId)) {
        // Update existing
        componentItems->value(compId)->setData(0, Qt::DisplayRole, QString("%1 (#%2)").arg(compName).arg(compId));
        //components->value(component)->setData(1, Qt::DisplayRole, QString::number(component));
        componentItems->value(compId)->setFirstColumnSpanned(true);
    } else {
        // Add new
        QStringList list(QString("%1 (#%2)").arg(compName).arg(compId));
        QTreeWidgetItem* comp = new QTreeWidgetItem(list);
        comp->setFirstColumnSpanned(true);
        componentItems->insert(compId, comp);
        // Create grouping and update maps
        paramGroups.insert(compId, new QMap<QString, QTreeWidgetItem*>());
        tree->addTopLevelItem(comp);
        tree->update();
    }

    //TODO it seems unlikely that the UI would know about a component before the data model...
    paramDataModel->addComponent(compId);

}


void QGCParamWidget::handleParameterUpdate(int componentId, const QString& paramName, QVariant value)
{
    updateParameterDisplay(componentId, paramName, value);
}


void QGCParamWidget::handleParameterListUpToDate()
{
    // Expand visual tree
    tree->expandItem(tree->topLevelItem(0));
}


void QGCParamWidget::updateParameterDisplay(int componentId, QString parameterName, QVariant value)
{

//    QString ptrStr;
//    ptrStr.sprintf("%8p", this);
//    qDebug() <<  "QGCParamWidget " << ptrStr << " got param" <<  parameterName;

    // Reference to item in tree
    QTreeWidgetItem* parameterItem = NULL;

    // Get component
    if (!componentItems->contains(componentId)) {
        QString componentName = tr("Component #%1").arg(componentId);
        addComponentItem(componentId, componentName);
    }

    //TODO this should be bubbling up from the model, not vice-versa, right?
//    // Replace value in data model
//    paramDataModel->handleParameterUpdate(componentId,parameterName,value);


    QString splitToken = "_";
    // Check if auto-grouping can work
    if (parameterName.contains(splitToken))
    {
        QString parent = parameterName.section(splitToken, 0, 0, QString::SectionSkipEmpty);
        QMap<QString, QTreeWidgetItem*>* compParamGroups = paramGroups.value(componentId);
        if (!compParamGroups->contains(parent))
        {
            // Insert group item
            QStringList glist;
            glist.append(parent);
            QTreeWidgetItem* item = new QTreeWidgetItem(glist);
            compParamGroups->insert(parent, item);
            componentItems->value(componentId)->addChild(item);
        }

        // Append child to group
        bool found = false;
        QTreeWidgetItem* parentItem = compParamGroups->value(parent);
        for (int i = 0; i < parentItem->childCount(); i++) {
            QTreeWidgetItem* child = parentItem->child(i);
            QString key = child->data(0, Qt::DisplayRole).toString();
            if (key == parameterName)  {
                //qDebug() << "UPDATED CHILD";
                parameterItem = child;
                if (value.type() == QVariant::Char) {
                    parameterItem->setData(1, Qt::DisplayRole, value.toUInt());
                }
                else {
                    parameterItem->setData(1, Qt::DisplayRole, value);
                }
                found = true;
            }
        }

        if (!found) {
            // Insert parameter into map
            QStringList plist;
            plist.append(parameterName);
            // CREATE PARAMETER ITEM
            parameterItem = new QTreeWidgetItem(plist);
            // CONFIGURE PARAMETER ITEM
            if (value.type() == QVariant::Char) {
                parameterItem->setData(1, Qt::DisplayRole, value.toUInt());
            }
            else {
                parameterItem->setData(1, Qt::DisplayRole, value);
            }

            compParamGroups->value(parent)->addChild(parameterItem);
            parameterItem->setFlags(parameterItem->flags() | Qt::ItemIsEditable);
        }
    }
    else  {
        bool found = false;
        QTreeWidgetItem* parent = componentItems->value(componentId);
        for (int i = 0; i < parent->childCount(); i++) {
            QTreeWidgetItem* child = parent->child(i);
            QString key = child->data(0, Qt::DisplayRole).toString();
            if (key == parameterName) {
                //qDebug() << "UPDATED CHILD";
                parameterItem = child;
                parameterItem->setData(1, Qt::DisplayRole, value);
                found = true;
            }
        }

        if (!found) {
            // Insert parameter into map
            QStringList plist;
            plist.append(parameterName);
            // CREATE PARAMETER ITEM
            parameterItem = new QTreeWidgetItem(plist);
            // CONFIGURE PARAMETER ITEM
            parameterItem->setData(1, Qt::DisplayRole, value);

            componentItems->value(componentId)->addChild(parameterItem);
            parameterItem->setFlags(parameterItem->flags() | Qt::ItemIsEditable);
        }
        //tree->expandAll();
    }
    // Reset background color
    parameterItem->setBackground(0, Qt::NoBrush);
    parameterItem->setBackground(1, Qt::NoBrush);
    // Add tooltip
    QString paramDesc = paramDataModel->getParamDescription(parameterName);
    QString tooltipFormat;
    if (paramDataModel->isParamDefaultKnown(parameterName)) {
        tooltipFormat = tr("Default: %1, %2");
        double paramDefValue = paramDataModel->getParamDefault(parameterName);
        tooltipFormat = tooltipFormat.arg(paramDefValue).arg(paramDesc);
    }
    else {
        tooltipFormat = paramDesc;
    }
    parameterItem->setToolTip(0, tooltipFormat);
    parameterItem->setToolTip(1, tooltipFormat);

    //paramDataModel->handleParameterUpdate(componentId,parameterName,value);

}



void QGCParamWidget::parameterItemChanged(QTreeWidgetItem* current, int column)
{
    if (current && column > 0) {
        QTreeWidgetItem* parent = current->parent();
        while (parent->parent() != NULL) {
            parent = parent->parent();
        }
        // Parent is now top-level component
        int componentId = componentItems->key(parent);

        QString key = current->data(0, Qt::DisplayRole).toString();
        QVariant value = current->data(1, Qt::DisplayRole);
        // Set parameter on changed list to be transmitted to MAV
        QPalette pal = statusLabel->palette();
        pal.setColor(backgroundRole(), QGC::colorOrange);
        statusLabel->setPalette(pal);
        statusLabel->setText(tr("Transmit pend. %1:%2: %3").arg(componentId).arg(key).arg(value.toFloat(), 5, 'f', 1, QChar(' ')));
        //qDebug() << "PARAM CHANGED: COMP:" << key << "KEY:" << str << "VALUE:" << value;
        // Changed values list

        bool changed = paramDataModel->addPendingIfParameterChanged(componentId,key,value);

        // If the value was numerically changed, display it differently
        if (changed) {
            current->setBackground(0, QBrush(QColor(QGC::colorOrange)));
            current->setBackground(1, QBrush(QColor(QGC::colorOrange)));

            //TODO this seems incorrect-- we're pre-updating the onboard value before we've received confirmation
            //paramDataModel->setOnboardParameterWithType(componentId,key,value);
        }

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
    paramDataModel->writeOnboardParametersToStream(outstream,mav->getUASName());
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
    paramDataModel->readUpdateParametersFromStream(in);
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

    requestParameterList();
}



/**
 * Set all parameter in the parameter tree on the MAV
 */
void QGCParamWidget::setParameters()
{
    paramCommsMgr->sendPendingParameters();
}

/**
 * Write the current onboard parameters from RAM into
 * permanent storage, e.g. EEPROM or harddisk
 */
void QGCParamWidget::writeParameters()
{
    int changedParamCount = 0;

    QMap<int, QMap<QString, QVariant>*>::iterator i;
    QMap<int, QMap<QString, QVariant>*> changedValues = paramDataModel->getPendingParameters();

    for (i = changedValues.begin(); i != changedValues.end() , (0 == changedParamCount);  ++i) {
        // Iterate through the parameters of the component
        QMap<QString, QVariant>* compPending = i.value();
        changedParamCount += compPending->count();
    }

    if (changedParamCount > 0) {
        QMessageBox msgBox;
        msgBox.setText(tr("There are locally changed parameters. Please transmit them first (<TRANSMIT>) or update them with the onboard values (<REFRESH>) before storing onboard from RAM to ROM."));
        msgBox.exec();
    }
    else {
        paramCommsMgr->writeParamsToPersistentStorage();
    }
}

/**
 * @param component the subsystem which has the parameter
 * @param parameterName name of the parameter, as delivered by the system
 * @param value value of the parameter
 */
//void QGCParamWidget::setParameter(int component, QString parameterName, QVariant value)
//{

//    if (parameterName.isEmpty()) {
//        return;
//    }

//    double dblValue = value.toDouble();

//    if (paramDataModel->isValueLessThanParamMin(parameterName,dblValue)) {
//        statusLabel->setText(tr("REJ. %1, %2 < min").arg(parameterName).arg(dblValue));
//        return;
//    }
//    if (paramDataModel->isValueGreaterThanParamMax(parameterName,dblValue)) {
//        statusLabel->setText(tr("REJ. %1, %2 > max").arg(parameterName).arg(dblValue));
//        return;
//    }
//    QVariant onboardVal;
//    paramDataModel->getOnboardParameterValue(component,parameterName,onboardVal);
//    if (onboardVal == value) {
//        statusLabel->setText(tr("REJ. %1 already %2").arg(parameterName).arg(dblValue));
//        return;
//    }

//    //int paramType = (int)onboardParameters.value(component)->value(parameterName).type();
//    int paramType = (int)value.type();
//    switch (paramType)
//    {
//    case QVariant::Char:
//    {
//        QVariant fixedValue(QChar((unsigned char)value.toInt()));
//        emit parameterChanged(component, parameterName, fixedValue);
//        //qDebug() << "PARAM WIDGET SENT:" << fixedValue;
//    }
//        break;
//    case QVariant::Int:
//    {
//        QVariant fixedValue(value.toInt());
//        emit parameterChanged(component, parameterName, fixedValue);
//        //qDebug() << "PARAM WIDGET SENT:" << fixedValue;
//    }
//        break;
//    case QVariant::UInt:
//    {
//        QVariant fixedValue(value.toUInt());
//        emit parameterChanged(component, parameterName, fixedValue);
//        //qDebug() << "PARAM WIDGET SENT:" << fixedValue;
//    }
//        break;
//    case QMetaType::Float:
//    {
//        QVariant fixedValue(value.toFloat());
//        emit parameterChanged(component, parameterName, fixedValue);
//        //qDebug() << "PARAM WIDGET SENT:" << fixedValue;
//    }
//        break;
//    default:
//        qCritical() << "ABORTED PARAM SEND, NO VALID QVARIANT TYPE";
//        return;
//    }

//    // Wait for parameter to be written back
//    // mark it therefore as missing
//    if (!transmissionMissingWriteAckPackets.contains(component))
//    {
//        transmissionMissingWriteAckPackets.insert(component, new QMap<QString, QVariant>());
//    }

//    // Insert it in missing write ACK list
//    transmissionMissingWriteAckPackets.value(component)->insert(parameterName, value);

//    // Set timeouts
//    if (transmissionActive)
//    {
//        transmissionTimeout += rewriteTimeout;
//    }
//    else
//    {
//        quint64 newTransmissionTimeout = QGC::groundTimeMilliseconds() + rewriteTimeout;
//        if (newTransmissionTimeout > transmissionTimeout)
//        {
//            transmissionTimeout = newTransmissionTimeout;
//        }
//        transmissionActive = true;
//    }

//    // Enable guard / reset timeouts
//    paramCommsMgr->setRetransmissionGuardEnabled(true); //TODO
//}

void QGCParamWidget::readParameters()
{
    if (!mav) return;
    mav->readParametersFromStorage();
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
