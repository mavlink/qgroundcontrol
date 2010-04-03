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

#include <MG.h>
#include "UASControlWidget.h"
#include <UASManager.h>
#include <UAS.h>
//#include <mavlink.h>

UASControlWidget::UASControlWidget(QWidget *parent) : QWidget(parent),
        uas(NULL)
{
    ui.setupUi(this);

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    ui.modeComboBox->insertItem(MAV_MODE_LOCKED, "MODE LOCKED");
    ui.modeComboBox->insertItem(MAV_MODE_MANUAL, "MODE MANUAL");
    ui.modeComboBox->insertItem(MAV_MODE_GUIDED, "MODE GUIDED");
    ui.modeComboBox->insertItem(MAV_MODE_AUTO, "MODE AUTO");
    ui.modeComboBox->insertItem(MAV_MODE_TEST1, "MODE TEST1");
    ui.modeComboBox->insertItem(MAV_MODE_TEST2, "MODE TEST2");
    ui.modeComboBox->insertItem(MAV_MODE_TEST3, "MODE TEST3");
}

void UASControlWidget::setUAS(UASInterface* uas)
{
    if (this->uas != NULL)
    {
        disconnect(ui.controlButton, SIGNAL(clicked()), uas, SLOT(enable_motors()));
        disconnect(ui.liftoffButton, SIGNAL(clicked()), uas, SLOT(launch()));
        disconnect(ui.landButton, SIGNAL(clicked()), uas, SLOT(home()));
        //disconnect(ui.haltButton, SIGNAL(clicked()), uas, SLOT(halt()));
        //disconnect(ui.continueButton, SIGNAL(clicked()), uas, SLOT(go()));
        //disconnect(ui.forceLandButton, SIGNAL(clicked()), uas, SLOT(emergencySTOP()));
        disconnect(ui.shutdownButton, SIGNAL(clicked()), uas, SLOT(shutdown()));
        //disconnect(uas, SIGNAL(autoModeChanged(bool)), ui.autoButton, SLOT(setChecked(bool)));
        //disconnect(ui.autoButton, SIGNAL(clicked(bool)), uas, SLOT(setAutoMode(bool)));
        //disconnect(ui.motorsStopButton, SIGNAL(clicked()), uas, SLOT(disable_motors()));
    }
    else
    {
        // Connect user interface controls
        connect(ui.controlButton, SIGNAL(clicked()), this, SLOT(cycleContextButton()));
        connect(ui.liftoffButton, SIGNAL(clicked()), uas, SLOT(launch()));
        connect(ui.landButton, SIGNAL(clicked()), uas, SLOT(home()));
        //connect(ui.haltButton, SIGNAL(clicked()), uas, SLOT(halt()));
        //connect(ui.continueButton, SIGNAL(clicked()), uas, SLOT(go()));
        //connect(ui.forceLandButton, SIGNAL(clicked()), uas, SLOT(emergencySTOP()));
        connect(ui.shutdownButton, SIGNAL(clicked()), uas, SLOT(shutdown()));
        //connect(ui.autoButton, SIGNAL(clicked(bool)), uas, SLOT(setAutoMode(bool)));
        //connect(uas, SIGNAL(autoModeChanged(bool)), ui.autoButton, SLOT(setChecked(bool)));
        //connect(ui.motorsStopButton, SIGNAL(clicked()), uas, SLOT(disable_motors()));
        connect(ui.modeComboBox, SIGNAL(activated(int)), this, SLOT(setMode(int)));
        ui.modeComboBox->insertItem(0, "Select..");

        ui.controlStatusLabel->setText(tr("Connected to ") + uas->getUASName());

        this->uas = uas;
    }
}

UASControlWidget::~UASControlWidget() {

}

void UASControlWidget::setMode(int mode)
{
    switch (mode)
    {
        case MAV_MODE_LOCKED:
        break;
        case MAV_MODE_MANUAL:
        break;
        case MAV_MODE_GUIDED:
        break;
        case MAV_MODE_AUTO:
        break;
        case MAV_MODE_TEST1:
        break;
        case MAV_MODE_TEST2:
        break;
        case MAV_MODE_TEST3:
        break;
    }
}

void UASControlWidget::cycleContextButton()
{
    //switch(uas->getMode());
    static int state = 0;

    UAS* mav = dynamic_cast<UAS*>(this->uas);
    if (mav)
    {

        switch (state)
        {
        case 0:
            ui.controlButton->setText(tr("Stop Engine"));
            mav->setMode(MAV_MODE_MANUAL);
            mav->enable_motors();
            state++;
            break;
        case 1:
            ui.controlButton->setText(tr("Activate Engine"));
            mav->setMode(MAV_MODE_LOCKED);
            mav->disable_motors();
            state = 0;
            break;
        case 2:
            //ui.controlButton->setText(tr("Force Landing"));
            ui.controlButton->setText(tr("KILL VEHICLE"));
            break;
        }
    }

}

