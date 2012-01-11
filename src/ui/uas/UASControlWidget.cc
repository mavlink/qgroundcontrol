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
#include <QDebug>
#include <QProcess>
#include <QPalette>

#include "UASControlWidget.h"
#include <UASManager.h>
#include <UAS.h>
#include "QGC.h"

UASControlWidget::UASControlWidget(QWidget *parent) : QWidget(parent),
    uas(0),
    uasMode(0),
    engineOn(false)
{
    ui.setupUi(this);

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    ui.modeComboBox->clear();
    ui.modeComboBox->insertItem(0, UAS::getShortModeTextFor(MAV_MODE_PREFLIGHT), MAV_MODE_PREFLIGHT);
    ui.modeComboBox->insertItem(1, UAS::getShortModeTextFor((MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED)), (MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED));
    ui.modeComboBox->insertItem(2, UAS::getShortModeTextFor(MAV_MODE_FLAG_MANUAL_INPUT_ENABLED), MAV_MODE_FLAG_MANUAL_INPUT_ENABLED);
    ui.modeComboBox->insertItem(3, UAS::getShortModeTextFor((MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED)), (MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED));
    ui.modeComboBox->insertItem(4, UAS::getShortModeTextFor((MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED)), (MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED));
    ui.modeComboBox->insertItem(5, UAS::getShortModeTextFor((MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_TEST_ENABLED)), (MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_TEST_ENABLED));
    connect(ui.modeComboBox, SIGNAL(activated(int)), this, SLOT(setMode(int)));
    connect(ui.setModeButton, SIGNAL(clicked()), this, SLOT(transmitMode()));

    uasMode = ui.modeComboBox->itemData(ui.modeComboBox->currentIndex()).toInt();

    ui.modeComboBox->setCurrentIndex(0);

    ui.gridLayout->setAlignment(Qt::AlignTop);

}

void UASControlWidget::setUAS(UASInterface* uas)
{
    if (this->uas != 0)
    {
        UASInterface* oldUAS = UASManager::instance()->getUASForId(this->uas);
        disconnect(ui.controlButton, SIGNAL(clicked()), oldUAS, SLOT(armSystem()));
        disconnect(ui.liftoffButton, SIGNAL(clicked()), oldUAS, SLOT(launch()));
        disconnect(ui.landButton, SIGNAL(clicked()), oldUAS, SLOT(home()));
        disconnect(ui.shutdownButton, SIGNAL(clicked()), oldUAS, SLOT(shutdown()));
        //connect(ui.setHomeButton, SIGNAL(clicked()), uas, SLOT(setLocalOriginAtCurrentGPSPosition()));
        disconnect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        disconnect(uas, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));
    }

    // Connect user interface controls
    connect(ui.controlButton, SIGNAL(clicked()), this, SLOT(cycleContextButton()));
    connect(ui.liftoffButton, SIGNAL(clicked()), uas, SLOT(launch()));
    connect(ui.landButton, SIGNAL(clicked()), uas, SLOT(home()));
    connect(ui.shutdownButton, SIGNAL(clicked()), uas, SLOT(shutdown()));
    //connect(ui.setHomeButton, SIGNAL(clicked()), uas, SLOT(setLocalOriginAtCurrentGPSPosition()));
    connect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
    connect(uas, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));

    ui.controlStatusLabel->setText(tr("Connected to ") + uas->getUASName());

    this->uas = uas->getUASID();
    setBackgroundColor(uas->getColor());
}

UASControlWidget::~UASControlWidget()
{

}

void UASControlWidget::updateStatemachine()
{

    if (engineOn)
    {
        ui.controlButton->setText(tr("DISARM SYSTEM"));
    }
    else
    {
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
    uasColor = uasColor.darker(900);
    colorstyle = colorstyle.sprintf("QLabel { border-radius: 3px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X; border: 0px solid %s; }",
                                    uasColor.red(), uasColor.green(), uasColor.blue(), borderColor.toStdString().c_str());
    setStyleSheet(colorstyle);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, QBrush(uasColor));
    setPalette(palette);
    setAutoFillBackground(true);
}


void UASControlWidget::updateMode(int uas,QString mode,QString description)
{
    Q_UNUSED(uas);
    Q_UNUSED(mode);
    Q_UNUSED(description);
}

void UASControlWidget::updateState(int state)
{
    switch (state)
    {
    case (int)MAV_STATE_ACTIVE:
        engineOn = true;
        ui.controlButton->setText(tr("DISARM SYSTEM"));
        break;
    case (int)MAV_STATE_STANDBY:
        engineOn = false;
        ui.controlButton->setText(tr("ARM SYSTEM"));
        break;
    }
}

/**
 * Called by the button
 */
void UASControlWidget::setMode(int mode)
{
    // Adapt context button mode
    uasMode = ui.modeComboBox->itemData(mode).toInt();
    ui.modeComboBox->blockSignals(true);
    ui.modeComboBox->setCurrentIndex(mode);
    ui.modeComboBox->blockSignals(false);

    emit changedMode(mode);
}

void UASControlWidget::transmitMode()
{
    UASInterface* mav = UASManager::instance()->getUASForId(this->uas);
    if (mav)
    {
        mav->setMode(uasMode);
        QString mode = ui.modeComboBox->currentText();

        ui.lastActionLabel->setText(QString("Sent new mode %1 to %2").arg(mode).arg(mav->getUASName()));
    }
}

void UASControlWidget::cycleContextButton()
{
    UAS* mav = dynamic_cast<UAS*>(UASManager::instance()->getUASForId(this->uas));
    if (mav)
    {

        if (!engineOn)
        {
            mav->armSystem();
            ui.lastActionLabel->setText(QString("Enabled motors on %1").arg(mav->getUASName()));
        } else {
            mav->disarmSystem();
            ui.lastActionLabel->setText(QString("Disabled motors on %1").arg(mav->getUASName()));
        }
        // Update state now and in several intervals when MAV might have changed state
        updateStatemachine();

        QTimer::singleShot(50, this, SLOT(updateStatemachine()));
        QTimer::singleShot(200, this, SLOT(updateStatemachine()));

    }

}

