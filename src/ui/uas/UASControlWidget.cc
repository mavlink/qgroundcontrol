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
#include <QFileDialog>
#include <QProcess>
#include <QPalette>

#include "UASControlWidget.h"
#include <UASManager.h>
#include <UAS.h>
#include "QGC.h"

static struct full_mode_s modes_list_common[] = {
    { MAV_MODE_FLAG_MANUAL_INPUT_ENABLED,
            0 },
    { (MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED),
            0 },
    { (MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED),
            0 },
    { (MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED),
            0 },
};

static struct full_mode_s modes_list_px4[] = {
    { (MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED),
            PX4_CUSTOM_MODE_MANUAL },
    { (MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED),
            PX4_CUSTOM_MODE_SEATBELT },
    { (MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED),
            PX4_CUSTOM_MODE_EASY },
    { (MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED),
            PX4_CUSTOM_MODE_AUTO },
};

UASControlWidget::UASControlWidget(QWidget *parent) : QWidget(parent),
    uasID(-1),
    modesList(NULL),
    modesNum(0),
    modeIdx(0),
    armed(false)
{
    ui.setupUi(this);

    this->setUAS(UASManager::instance()->getActiveUAS());

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    connect(ui.modeComboBox, SIGNAL(activated(int)), this, SLOT(setMode(int)));
    connect(ui.setModeButton, SIGNAL(clicked()), this, SLOT(transmitMode()));

    ui.gridLayout->setAlignment(Qt::AlignTop);
}

void UASControlWidget::updateModesList()
{
    // Detect autopilot type
    int autopilot = 0;
    if (this->uasID >= 0) {
        UASInterface *uas = UASManager::instance()->getUASForId(this->uasID);
        if (uas) {
            autopilot = UASManager::instance()->getUASForId(this->uasID)->getAutopilotType();
        }
    }

    // Use corresponding modes list
    if (autopilot == MAV_AUTOPILOT_PX4) {
        modesList = modes_list_px4;
        modesNum = sizeof(modes_list_px4) / sizeof(struct full_mode_s);
    } else {
        modesList = modes_list_common;
        modesNum = sizeof(modes_list_common) / sizeof(struct full_mode_s);
    }

    // Set combobox items
    ui.modeComboBox->clear();
    for (int i = 0; i < modesNum; i++) {
        struct full_mode_s mode = modesList[i];
        ui.modeComboBox->insertItem(i, UAS::getShortModeTextFor(mode.baseMode, mode.customMode, autopilot).remove(0, 2), i);
    }

    // Select first mode in list
    modeIdx = 0;
    ui.modeComboBox->setCurrentIndex(modeIdx);
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
            //connect(ui.setHomeButton, SIGNAL(clicked()), uas, SLOT(setLocalOriginAtCurrentGPSPosition()));
            disconnect(oldUAS, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int, QString, QString)));
            disconnect(oldUAS, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));
        }
    }

    // Connect user interface controls
    if (uas) {
        connect(ui.controlButton, SIGNAL(clicked()), this, SLOT(cycleContextButton()));
        connect(ui.liftoffButton, SIGNAL(clicked()), uas, SLOT(launch()));
        connect(ui.landButton, SIGNAL(clicked()), uas, SLOT(home()));
        connect(ui.shutdownButton, SIGNAL(clicked()), uas, SLOT(shutdown()));
        //connect(ui.setHomeButton, SIGNAL(clicked()), uas, SLOT(setLocalOriginAtCurrentGPSPosition()));
        connect(uas, SIGNAL(modeChanged(int, QString, QString)), this, SLOT(updateMode(int, QString, QString)));
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


void UASControlWidget::updateMode(int uas, QString mode, QString description)
{
    Q_UNUSED(uas);
    Q_UNUSED(mode);
    Q_UNUSED(description);
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

/**
 * Called by the button
 */
void UASControlWidget::setMode(int mode)
{
    // Adapt context button mode
    modeIdx = mode;
    ui.modeComboBox->blockSignals(true);
    ui.modeComboBox->setCurrentIndex(mode);
    ui.modeComboBox->blockSignals(false);

    emit changedMode(mode);
}

void UASControlWidget::transmitMode()
{
    UASInterface* uas = UASManager::instance()->getUASForId(this->uasID);
    if (uas) {
        if (modeIdx >= 0 && modeIdx < modesNum) {
            struct full_mode_s mode = modesList[modeIdx];
            // include armed state
            if (armed) {
                mode.baseMode |= MAV_MODE_FLAG_SAFETY_ARMED;
            } else {
                mode.baseMode &= ~MAV_MODE_FLAG_SAFETY_ARMED;
            }

            uas->setMode(mode.baseMode, mode.customMode);
            QString modeText = ui.modeComboBox->currentText();

            ui.lastActionLabel->setText(QString("Sent new mode %1 to %2").arg(modeText).arg(uas->getUASName()));
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

