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
 *   @brief Definition of class ParameterInterface
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTreeWidget>

#include "ParameterInterface.h"
#include "UASManager.h"
#include "ui_ParameterInterface.h"
#include "QGCSensorSettingsWidget.h"

#include <QDebug>

ParameterInterface::ParameterInterface(QWidget *parent) :
    QWidget(parent),
    paramWidgets(new QMap<int, QGCParamWidget*>()),
    curr(-1),
    m_ui(new Ui::parameterWidget)
{
    m_ui->setupUi(this);

    // Get current MAV list
    QList<UASInterface*> systems = UASManager::instance()->getUASList();

    // Add each of them
    foreach (UASInterface* sys, systems) {
        addUAS(sys);
    }

    // Setup MAV connections
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(activeUASSetListIndex(int)), this, SLOT(selectUAS(int)));
    this->setVisible(false);
}

ParameterInterface::~ParameterInterface()
{
    delete m_ui;
}

void ParameterInterface::selectUAS(int index)
{
    m_ui->stackedWidget->setCurrentIndex(index);
    m_ui->sensorSettings->setCurrentIndex(index);
    curr = index;
}

/**
 *
 * @param uas System to add to list
 */
void ParameterInterface::addUAS(UASInterface* uas)
{
    QGCParamWidget* param = new QGCParamWidget(uas, this);
    paramWidgets->insert(uas->getUASID(), param);
    m_ui->stackedWidget->addWidget(param);


    QGCSensorSettingsWidget* sensor = new QGCSensorSettingsWidget(uas, this);
    m_ui->sensorSettings->addWidget(sensor);

    // Set widgets as default
    if (curr == -1) {
        // Clear
        m_ui->sensorSettings->setCurrentWidget(sensor);
        m_ui->stackedWidget->setCurrentWidget(param);
        curr = 0;
    }
}

void ParameterInterface::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
