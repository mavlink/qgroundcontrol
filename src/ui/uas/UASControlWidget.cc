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

#include <MG.h>
#include "UASControlWidget.h"
#include <UASManager.h>
#include <UAS.h>
#include "QGC.h"

#define CONTROL_MODE_LOCKED "MODE LOCKED"
#define CONTROL_MODE_MANUAL "MODE MANUAL"
#define CONTROL_MODE_GUIDED "MODE GUIDED"
#define CONTROL_MODE_AUTO   "MODE AUTO"
#define CONTROL_MODE_TEST1  "MODE TEST1"
#define CONTROL_MODE_TEST2  "MODE TEST2"
#define CONTROL_MODE_TEST3  "MODE TEST3"
#define CONTROL_MODE_READY  "MODE TEST3"
#define CONTROL_MODE_RC_TRAINING  "RC SIMULATION"

#define CONTROL_MODE_LOCKED_INDEX 1
#define CONTROL_MODE_MANUAL_INDEX 2
#define CONTROL_MODE_GUIDED_INDEX 3
#define CONTROL_MODE_AUTO_INDEX   4
#define CONTROL_MODE_TEST1_INDEX  5
#define CONTROL_MODE_TEST2_INDEX  6
#define CONTROL_MODE_TEST3_INDEX  7
#define CONTROL_MODE_READY_INDEX  8
#define CONTROL_MODE_RC_TRAINING_INDEX  9

UASControlWidget::UASControlWidget(QWidget *parent) : QWidget(parent),
uas(0),
engineOn(false)
{
    ui.setupUi(this);

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    ui.modeComboBox->clear();
    ui.modeComboBox->insertItem(0, "Select..");
    ui.modeComboBox->insertItem(CONTROL_MODE_LOCKED_INDEX, CONTROL_MODE_LOCKED);
    ui.modeComboBox->insertItem(CONTROL_MODE_MANUAL_INDEX, CONTROL_MODE_MANUAL);
    ui.modeComboBox->insertItem(CONTROL_MODE_GUIDED_INDEX, CONTROL_MODE_GUIDED);
    ui.modeComboBox->insertItem(CONTROL_MODE_AUTO_INDEX, CONTROL_MODE_AUTO);
    ui.modeComboBox->insertItem(CONTROL_MODE_TEST1_INDEX, CONTROL_MODE_TEST1);
    ui.modeComboBox->insertItem(CONTROL_MODE_TEST2_INDEX, CONTROL_MODE_TEST2);
    ui.modeComboBox->insertItem(CONTROL_MODE_TEST3_INDEX, CONTROL_MODE_TEST3);
    ui.modeComboBox->insertItem(CONTROL_MODE_READY_INDEX, CONTROL_MODE_READY);
    ui.modeComboBox->insertItem(CONTROL_MODE_RC_TRAINING_INDEX, CONTROL_MODE_RC_TRAINING);
    connect(ui.modeComboBox, SIGNAL(activated(int)), this, SLOT(setMode(int)));
    connect(ui.setModeButton, SIGNAL(clicked()), this, SLOT(transmitMode()));

    ui.modeComboBox->setCurrentIndex(0);

    ui.gridLayout->setAlignment(Qt::AlignTop);
}

void UASControlWidget::setUAS(UASInterface* uas)
{
    if (this->uas != 0)
    {
        UASInterface* oldUAS = UASManager::instance()->getUASForId(this->uas);
        disconnect(ui.controlButton, SIGNAL(clicked()), oldUAS, SLOT(enable_motors()));
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

//    // Check if additional controls should be loaded
//    UAS* mav = dynamic_cast<UAS*>(uas);
//    if (mav)
//    {
//        QPushButton* startRecButton = new QPushButton(tr("Record"));
//        connect(startRecButton, SIGNAL(clicked()), mav, SLOT(startDataRecording()));
//        ui.gridLayout->addWidget(startRecButton, 7, 1);

//        QPushButton* pauseRecButton = new QPushButton(tr("Pause"));
//        connect(pauseRecButton, SIGNAL(clicked()), mav, SLOT(pauseDataRecording()));
//        ui.gridLayout->addWidget(pauseRecButton, 7, 3);

//        QPushButton* stopRecButton = new QPushButton(tr("Stop"));
//        connect(stopRecButton, SIGNAL(clicked()), mav, SLOT(stopDataRecording()));
//        ui.gridLayout->addWidget(stopRecButton, 7, 4);
//    }


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
        ui.controlButton->setText(tr("Stop Engine"));
    }
    else
    {
        ui.controlButton->setText(tr("Activate Engine"));
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
        ui.controlButton->setText(tr("Stop Engine"));
        break;
    case (int)MAV_STATE_STANDBY:
        engineOn = false;
        ui.controlButton->setText(tr("Activate Engine"));
        break;
    }
}

void UASControlWidget::setMode(int mode)
{
    // Adapt context button mode
    if (mode == CONTROL_MODE_LOCKED_INDEX)
    {
        uasMode = (unsigned int)MAV_MODE_LOCKED;
        ui.modeComboBox->setCurrentIndex(mode);
    }
    else if (mode == CONTROL_MODE_MANUAL_INDEX)
    {
        uasMode = (unsigned int)MAV_MODE_MANUAL;
        ui.modeComboBox->setCurrentIndex(mode);
    }
    else if (mode == CONTROL_MODE_GUIDED_INDEX)
    {
        uasMode = (unsigned int)MAV_MODE_GUIDED;
        ui.modeComboBox->setCurrentIndex(mode);
    }
    else if (mode == CONTROL_MODE_AUTO_INDEX)
    {
        uasMode = (unsigned int)MAV_MODE_AUTO;
        ui.modeComboBox->setCurrentIndex(mode);
    }
    else if (mode == CONTROL_MODE_TEST1_INDEX)
    {
        uasMode = (unsigned int)MAV_MODE_TEST1;
        ui.modeComboBox->setCurrentIndex(mode);
    }
    else if (mode == CONTROL_MODE_TEST2_INDEX)
    {
        uasMode = (unsigned int)MAV_MODE_TEST2;
        ui.modeComboBox->setCurrentIndex(mode);
    }
    else if (mode == CONTROL_MODE_TEST3_INDEX)
    {
        uasMode = (unsigned int)MAV_MODE_TEST3;
        ui.modeComboBox->setCurrentIndex(mode);
    }
    else if (mode == CONTROL_MODE_RC_TRAINING_INDEX)
    {
        uasMode = (unsigned int)MAV_MODE_RC_TRAINING;
        ui.modeComboBox->setCurrentIndex(mode);
    }
    else
    {
        qDebug() << "ERROR! MODE NOT FOUND";
        uasMode = 0;
    }


    qDebug() << "SET MODE REQUESTED" << uasMode;

    emit changedMode(mode);
}

void UASControlWidget::transmitMode()
{
    if (uasMode != 0)
    {
        UASInterface* mav = UASManager::instance()->getUASForId(this->uas);
        if (mav)
        {
            mav->setMode(uasMode);
            ui.lastActionLabel->setText(QString("Set new mode for system %1").arg(mav->getUASName()));
        }
    }
}

void UASControlWidget::cycleContextButton()
{
    UAS* mav = dynamic_cast<UAS*>(UASManager::instance()->getUASForId(this->uas));
    if (mav)
    {

        if (!engineOn)
        {
            mav->enable_motors();
            ui.lastActionLabel->setText(QString("Enabled motors on %1").arg(mav->getUASName()));
        }
        else
        {
            mav->disable_motors();
            ui.lastActionLabel->setText(QString("Disabled motors on %1").arg(mav->getUASName()));
        }
        // Update state now and in several intervals when MAV might have changed state
        updateStatemachine();

        QTimer::singleShot(50, this, SLOT(updateStatemachine()));
        QTimer::singleShot(200, this, SLOT(updateStatemachine()));

        //ui.controlButton->setText(tr("Force Landing"));
        //ui.controlButton->setText(tr("KILL VEHICLE"));
    }

}

