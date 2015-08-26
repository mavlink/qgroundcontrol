/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of widget controlling one MAV
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QString>
#include <QTimer>
#include <QLabel>
#include <QProcess>
#include <QPalette>

#include "UASControlWidget.h"
#include <UASManager.h>
#include <UAS.h>
#include "QGC.h"
#include "AutoPilotPluginManager.h"
#include "FirmwarePluginManager.h"

UASControlWidget::UASControlWidget(QWidget *parent) : QWidget(parent),
    uasID(-1),
    armed(false)
{
    ui.setupUi(this);

    this->setUAS(UASManager::instance()->getActiveUAS());

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    connect(ui.setModeButton, SIGNAL(clicked()), this, SLOT(transmitMode()));

    ui.liftoffButton->hide();
    ui.landButton->hide();
    ui.shutdownButton->hide();

    ui.gridLayout->setAlignment(Qt::AlignTop);
}

void UASControlWidget::updateModesList()
{
    if (this->uasID == 0) {
        return;
    }
    
    UASInterface*uas = UASManager::instance()->getUASForId(this->uasID);
    Q_ASSERT(uas);
    
    _modeList = FirmwarePluginManager::instance()->firmwarePluginForAutopilot((MAV_AUTOPILOT)uas->getAutopilotType())->flightModes();
    
    // Set combobox items
    ui.modeComboBox->clear();
    foreach (QString flightMode, _modeList) {
        ui.modeComboBox->addItem(flightMode);
    }

    // Select first mode in list
    ui.modeComboBox->setCurrentIndex(0);
    ui.modeComboBox->update();
}

void UASControlWidget::setUAS(UASInterface* uas)
{
    if (this->uasID > 0) {
        UASInterface* oldUAS = UASManager::instance()->getUASForId(this->uasID);
        if (oldUAS) {
            disconnect(ui.controlButton, SIGNAL(clicked()), oldUAS, SLOT(armSystem()));
            disconnect(ui.liftoffButton, SIGNAL(clicked()), oldUAS, SLOT(launch()));
            disconnect(ui.landButton, SIGNAL(clicked()), oldUAS, SLOT(home()));
            disconnect(ui.shutdownButton, SIGNAL(clicked()), oldUAS, SLOT(shutdown()));
            disconnect(oldUAS, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));
        }
    }

    // Connect user interface controls
    if (uas) {
        connect(ui.controlButton, SIGNAL(clicked()), this, SLOT(cycleContextButton()));
        connect(ui.liftoffButton, SIGNAL(clicked()), uas, SLOT(launch()));
        connect(ui.landButton, SIGNAL(clicked()), uas, SLOT(home()));
        connect(ui.shutdownButton, SIGNAL(clicked()), uas, SLOT(shutdown()));
        connect(uas, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));

        ui.controlStatusLabel->setText(tr("Connected to ") + uas->getUASName());

        this->uasID = uas->getUASID();
        setBackgroundColor(uas->getColor());

        this->updateModesList();
        this->updateArmText();

    } else {
        this->uasID = -1;
    }
}

UASControlWidget::~UASControlWidget()
{

}

void UASControlWidget::updateArmText()
{
    if (armed) {
        ui.controlButton->setText(tr("DISARM SYSTEM"));
    } else {
        ui.controlButton->setText(tr("ARM SYSTEM"));
    }
}

/**
 * Set the background color based on the MAV color. If the MAV is selected as the
 * currently actively controlled system, the frame color is highlighted
 */
void UASControlWidget::setBackgroundColor(QColor color)
{
    // UAS color
    QColor uasColor = color;
    QString colorstyle;
    QString borderColor = "#4A4A4F";
    borderColor = "#FA4A4F";
    uasColor = uasColor.darker(400);
    colorstyle = colorstyle.sprintf("QLabel { border-radius: 3px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X; border: 0px solid %s; }",
                                    uasColor.red(), uasColor.green(), uasColor.blue(), borderColor.toStdString().c_str());
    setStyleSheet(colorstyle);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, QBrush(uasColor));
    setPalette(palette);
    setAutoFillBackground(true);
}


void UASControlWidget::updateState(int state)
{
    switch (state) {
    case (int)MAV_STATE_ACTIVE:
        armed = true;
        break;
    case (int)MAV_STATE_STANDBY:
        armed = false;
        break;
    }
    this->updateArmText();
}

void UASControlWidget::transmitMode()
{
    UAS* uas = dynamic_cast<UAS*>(UASManager::instance()->getUASForId(this->uasID));
    if (uas) {
        uint8_t     base_mode;
        uint32_t    custom_mode;
        QString     flightMode = ui.modeComboBox->itemText(ui.modeComboBox->currentIndex());
        
        if (FirmwarePluginManager::instance()->firmwarePluginForAutopilot((MAV_AUTOPILOT)uas->getAutopilotType())->setFlightMode(flightMode, &base_mode, &custom_mode)) {
            if (armed) {
                base_mode |= MAV_MODE_FLAG_SAFETY_ARMED;
            }
            
            if (uas->isHilEnabled() || uas->isHilActive()) {
                base_mode |= MAV_MODE_FLAG_HIL_ENABLED;
            }
            
            uas->setMode(base_mode, custom_mode);
            QString modeText = ui.modeComboBox->currentText();
            
            ui.lastActionLabel->setText(QString("Sent new mode %1 to %2").arg(flightMode).arg(uas->getUASName()));
        }
    }
}

void UASControlWidget::cycleContextButton()
{
    UAS* uas = dynamic_cast<UAS*>(UASManager::instance()->getUASForId(this->uasID));
    if (uas) {
        if (!armed) {
            uas->armSystem();
            ui.lastActionLabel->setText(QString("Arm %1").arg(uas->getUASName()));
        } else {
            uas->disarmSystem();
            ui.lastActionLabel->setText(QString("Disarm %1").arg(uas->getUASName()));
        }
    }
}

