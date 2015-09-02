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
#include "MultiVehicleManager.h"
#include "UAS.h"
#include "QGC.h"
#include "AutoPilotPluginManager.h"
#include "FirmwarePluginManager.h"

UASControlWidget::UASControlWidget(QWidget *parent) : QWidget(parent),
    _uas(NULL),
    armed(false)
{
    ui.setupUi(this);

    _activeVehicleChanged(MultiVehicleManager::instance()->activeVehicle());
    
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &UASControlWidget::_activeVehicleChanged);
    connect(ui.setModeButton, SIGNAL(clicked()), this, SLOT(transmitMode()));

    ui.liftoffButton->hide();
    ui.landButton->hide();
    ui.shutdownButton->hide();

    ui.gridLayout->setAlignment(Qt::AlignTop);
}

void UASControlWidget::updateModesList()
{
    if (!_uas) {
        return;
    }
    _modeList = FirmwarePluginManager::instance()->firmwarePluginForAutopilot((MAV_AUTOPILOT)_uas->getAutopilotType())->flightModes();
    
    // Set combobox items
    ui.modeComboBox->clear();
    foreach (QString flightMode, _modeList) {
        ui.modeComboBox->addItem(flightMode);
    }

    // Select first mode in list
    ui.modeComboBox->setCurrentIndex(0);
    ui.modeComboBox->update();
}

void UASControlWidget::_activeVehicleChanged(Vehicle* vehicle)
{
    if (_uas) {
        disconnect(ui.controlButton, SIGNAL(clicked()), _uas, SLOT(armSystem()));
        disconnect(ui.liftoffButton, SIGNAL(clicked()), _uas, SLOT(launch()));
        disconnect(ui.landButton, SIGNAL(clicked()), _uas, SLOT(home()));
        disconnect(ui.shutdownButton, SIGNAL(clicked()), _uas, SLOT(shutdown()));
        disconnect(_uas, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));
        _uas = NULL;
    }

    // Connect user interface controls
    if (vehicle) {
        _uas = vehicle->uas();
        connect(ui.controlButton, SIGNAL(clicked()), this, SLOT(cycleContextButton()));
        connect(ui.liftoffButton, SIGNAL(clicked()), _uas, SLOT(launch()));
        connect(ui.landButton, SIGNAL(clicked()), _uas, SLOT(home()));
        connect(ui.shutdownButton, SIGNAL(clicked()), _uas, SLOT(shutdown()));
        connect(_uas, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));

        ui.controlStatusLabel->setText(tr("Connected to ") + _uas->getUASName());

        setBackgroundColor(_uas->getColor());

        this->updateModesList();
        this->updateArmText();
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
    if (_uas) {
        uint8_t     base_mode;
        uint32_t    custom_mode;
        QString     flightMode = ui.modeComboBox->itemText(ui.modeComboBox->currentIndex());
        
        if (FirmwarePluginManager::instance()->firmwarePluginForAutopilot((MAV_AUTOPILOT)_uas->getAutopilotType())->setFlightMode(flightMode, &base_mode, &custom_mode)) {
            if (armed) {
                base_mode |= MAV_MODE_FLAG_SAFETY_ARMED;
            }
            
            if (_uas->isHilEnabled() || _uas->isHilActive()) {
                base_mode |= MAV_MODE_FLAG_HIL_ENABLED;
            }
            
            _uas->setMode(base_mode, custom_mode);
            QString modeText = ui.modeComboBox->currentText();
            
            ui.lastActionLabel->setText(QString("Sent new mode %1 to %2").arg(flightMode).arg(_uas->getUASName()));
        }
    }
}

void UASControlWidget::cycleContextButton()
{
    if (_uas) {
        if (!armed) {
            _uas->armSystem();
            ui.lastActionLabel->setText(QString("Arm %1").arg(_uas->getUASName()));
        } else {
            _uas->disarmSystem();
            ui.lastActionLabel->setText(QString("Disarm %1").arg(_uas->getUASName()));
        }
    }
}
