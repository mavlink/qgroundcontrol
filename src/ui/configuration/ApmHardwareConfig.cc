/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2013 Michael Carpenter (malcom2073@gmail.com)

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
 *   @brief APM Hardware Configuration widget source.
 *
 *   @author Michael Carpenter <malcom2073@gmail.com>
 *
 */
#include "ApmHardwareConfig.h"


ApmHardwareConfig::ApmHardwareConfig(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    //ui.firmwareButton->setVisible(valse);
    ui.manditoryHardware->setVisible(false);
    ui.frameTypeButton->setVisible(false);
    ui.compassButton->setVisible(false);
    ui.accelCalibrateButton->setVisible(false);
    ui.radioCalibrateButton->setVisible(false);
    ui.optionalHardwareButton->setVisible(false);
    //ui.radio3DRButton->setVisible(false);
    ui.batteryMonitorButton->setVisible(false);
    ui.sonarButton->setVisible(false);
    ui.airspeedButton->setVisible(false);
    ui.opticalFlowButton->setVisible(false);
    ui.osdButton->setVisible(false);
    ui.cameraGimbalButton->setVisible(false);
    //ui.antennaTrackerButton->setVisible(false);

    connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.frameTypeButton,SLOT(setShown(bool)));
    connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.compassButton,SLOT(setShown(bool)));
    connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.accelCalibrateButton,SLOT(setShown(bool)));
    connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.radioCalibrateButton,SLOT(setShown(bool)));

    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.radio3DRButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.batteryMonitorButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.sonarButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.airspeedButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.opticalFlowButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.osdButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.cameraGimbalButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.antennaTrackerButton,SLOT(setShown(bool)));

    connect(ui.frameTypeButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));


    QWidget *widget = new QWidget(this);
    ui.stackedWidget->addWidget(widget); //Firmware placeholder.
    buttonToConfigWidgetMap[ui.firmwareButton] = widget;
    connect(ui.firmwareButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    frameConfig = new FrameTypeConfig(this);
    ui.stackedWidget->addWidget(frameConfig);
    buttonToConfigWidgetMap[ui.frameTypeButton] = frameConfig;
    connect(ui.frameTypeButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    compassConfig = new CompassConfig(this);
    ui.stackedWidget->addWidget(compassConfig);
    buttonToConfigWidgetMap[ui.compassButton] = compassConfig;
    connect(ui.compassButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    accelConfig = new AccelCalibrationConfig(this);
    ui.stackedWidget->addWidget(accelConfig);
    buttonToConfigWidgetMap[ui.accelCalibrateButton] = accelConfig;
    connect(ui.accelCalibrateButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    radioConfig = new RadioCalibrationConfig(this);
    ui.stackedWidget->addWidget(radioConfig);
    buttonToConfigWidgetMap[ui.radioCalibrateButton] = radioConfig;
    connect(ui.radioCalibrateButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));


    radio3drConfig = new Radio3DRConfig(this);
    ui.stackedWidget->addWidget(radio3drConfig);
    buttonToConfigWidgetMap[ui.radio3DRButton] = radio3drConfig;
    connect(ui.radio3DRButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    batteryConfig = new BatteryMonitorConfig(this);
    ui.stackedWidget->addWidget(batteryConfig);
    buttonToConfigWidgetMap[ui.batteryMonitorButton] = batteryConfig;
    connect(ui.batteryMonitorButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    sonarConfig = new SonarConfig(this);
    ui.stackedWidget->addWidget(sonarConfig);
    buttonToConfigWidgetMap[ui.sonarButton] = sonarConfig;
    connect(ui.sonarButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    airspeedConfig = new AirspeedConfig(this);
    ui.stackedWidget->addWidget(airspeedConfig);
    buttonToConfigWidgetMap[ui.airspeedButton] = airspeedConfig;
    connect(ui.airspeedButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    opticalFlowConfig = new OpticalFlowConfig(this);
    ui.stackedWidget->addWidget(opticalFlowConfig);
    buttonToConfigWidgetMap[ui.opticalFlowButton] = opticalFlowConfig;
    connect(ui.opticalFlowButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    osdConfig = new OsdConfig(this);
    ui.stackedWidget->addWidget(osdConfig);
    buttonToConfigWidgetMap[ui.osdButton] = osdConfig;
    connect(ui.osdButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    cameraGimbalConfig = new CameraGimbalConfig(this);
    ui.stackedWidget->addWidget(cameraGimbalConfig);
    buttonToConfigWidgetMap[ui.cameraGimbalButton] = cameraGimbalConfig;
    connect(ui.cameraGimbalButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    antennaTrackerConfig = new AntennaTrackerConfig(this);
    ui.stackedWidget->addWidget(antennaTrackerConfig);
    buttonToConfigWidgetMap[ui.antennaTrackerButton] = antennaTrackerConfig;
    connect(ui.antennaTrackerButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));



    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(activeUASSet(UASInterface*)));
    if (UASManager::instance()->getActiveUAS())
    {
        activeUASSet(UASManager::instance()->getActiveUAS());
    }
}
void ApmHardwareConfig::activateStackedWidget()
{
    if (buttonToConfigWidgetMap.contains(sender()))
    {
        ui.stackedWidget->setCurrentWidget(buttonToConfigWidgetMap[sender()]);
    }
}

ApmHardwareConfig::~ApmHardwareConfig()
{
}

void ApmHardwareConfig::activeUASSet(UASInterface *uas)
{
    if (!uas)
    {
        return;
    }
    ui.firmwareButton->setVisible(true);
    ui.manditoryHardware->setVisible(true);
    ui.manditoryHardware->setChecked(false);
    ui.optionalHardwareButton->setVisible(true);
    ui.optionalHardwareButton->setChecked(false);
    ui.radio3DRButton->setVisible(false);
    ui.antennaTrackerButton->setVisible(false);
}
