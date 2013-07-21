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


    ui.manditoryHardware->setVisible(false);
    ui.frameTypeButton->setVisible(false);
    ui.compassButton->setVisible(false);
    ui.accelCalibrateButton->setVisible(false);
    ui.arduPlaneLevelButton->setVisible(false);
    ui.radioCalibrateButton->setVisible(false);
    ui.optionalHardwareButton->setVisible(false);
    ui.batteryMonitorButton->setVisible(false);
    ui.sonarButton->setVisible(false);
    ui.airspeedButton->setVisible(false);
    ui.opticalFlowButton->setVisible(false);
    ui.osdButton->setVisible(false);
    ui.cameraGimbalButton->setVisible(false);

    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.radio3DRButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.batteryMonitorButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.sonarButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.opticalFlowButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.osdButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.cameraGimbalButton,SLOT(setShown(bool)));
    connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.antennaTrackerButton,SLOT(setShown(bool)));

    connect(ui.frameTypeButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));


    QWidget *widget = new QWidget(this);
    ui.stackedWidget->addWidget(widget); //Firmware placeholder.
    m_buttonToConfigWidgetMap[ui.firmwareButton] = widget;
    connect(ui.firmwareButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_frameConfig = new FrameTypeConfig(this);
    ui.stackedWidget->addWidget(m_frameConfig);
    m_buttonToConfigWidgetMap[ui.frameTypeButton] = m_frameConfig;
    connect(ui.frameTypeButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_compassConfig = new CompassConfig(this);
    ui.stackedWidget->addWidget(m_compassConfig);
    m_buttonToConfigWidgetMap[ui.compassButton] = m_compassConfig;
    connect(ui.compassButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_accelConfig = new AccelCalibrationConfig(this);
    ui.stackedWidget->addWidget(m_accelConfig);
    m_buttonToConfigWidgetMap[ui.accelCalibrateButton] = m_accelConfig;
    connect(ui.accelCalibrateButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_planeLevel = new ApmPlaneLevel(this);
    ui.stackedWidget->addWidget(m_planeLevel);
    m_buttonToConfigWidgetMap[ui.arduPlaneLevelButton] = m_planeLevel;
    connect(ui.arduPlaneLevelButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_radioConfig = new RadioCalibrationConfig(this);
    ui.stackedWidget->addWidget(m_radioConfig);
    m_buttonToConfigWidgetMap[ui.radioCalibrateButton] = m_radioConfig;
    connect(ui.radioCalibrateButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));


    m_radio3drConfig = new Radio3DRConfig(this);
    ui.stackedWidget->addWidget(m_radio3drConfig);
    m_buttonToConfigWidgetMap[ui.radio3DRButton] = m_radio3drConfig;
    connect(ui.radio3DRButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_batteryConfig = new BatteryMonitorConfig(this);
    ui.stackedWidget->addWidget(m_batteryConfig);
    m_buttonToConfigWidgetMap[ui.batteryMonitorButton] = m_batteryConfig;
    connect(ui.batteryMonitorButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_sonarConfig = new SonarConfig(this);
    ui.stackedWidget->addWidget(m_sonarConfig);
    m_buttonToConfigWidgetMap[ui.sonarButton] = m_sonarConfig;
    connect(ui.sonarButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_airspeedConfig = new AirspeedConfig(this);
    ui.stackedWidget->addWidget(m_airspeedConfig);
    m_buttonToConfigWidgetMap[ui.airspeedButton] = m_airspeedConfig;
    connect(ui.airspeedButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_opticalFlowConfig = new OpticalFlowConfig(this);
    ui.stackedWidget->addWidget(m_opticalFlowConfig);
    m_buttonToConfigWidgetMap[ui.opticalFlowButton] = m_opticalFlowConfig;
    connect(ui.opticalFlowButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_osdConfig = new OsdConfig(this);
    ui.stackedWidget->addWidget(m_osdConfig);
    m_buttonToConfigWidgetMap[ui.osdButton] = m_osdConfig;
    connect(ui.osdButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_cameraGimbalConfig = new CameraGimbalConfig(this);
    ui.stackedWidget->addWidget(m_cameraGimbalConfig);
    m_buttonToConfigWidgetMap[ui.cameraGimbalButton] = m_cameraGimbalConfig;
    connect(ui.cameraGimbalButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    m_antennaTrackerConfig = new AntennaTrackerConfig(this);
    ui.stackedWidget->addWidget(m_antennaTrackerConfig);
    m_buttonToConfigWidgetMap[ui.antennaTrackerButton] = m_antennaTrackerConfig;
    connect(ui.antennaTrackerButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(activeUASSet(UASInterface*)));
    if (UASManager::instance()->getActiveUAS())
    {
        activeUASSet(UASManager::instance()->getActiveUAS());
    }
}
void ApmHardwareConfig::activateStackedWidget()
{
    if (m_buttonToConfigWidgetMap.contains(sender()))
    {
        ui.stackedWidget->setCurrentWidget(m_buttonToConfigWidgetMap[sender()]);
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
    if (uas->getSystemType() == MAV_TYPE_FIXED_WING)
    {
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.compassButton,SLOT(setShown(bool)));
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.arduPlaneLevelButton,SLOT(setShown(bool)));
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.radioCalibrateButton,SLOT(setShown(bool)));
        connect(ui.optionalHardwareButton,SIGNAL(toggled(bool)),ui.airspeedButton,SLOT(setShown(bool)));
    }
    else if (uas->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.frameTypeButton,SLOT(setShown(bool)));
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.compassButton,SLOT(setShown(bool)));
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.accelCalibrateButton,SLOT(setShown(bool)));
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.radioCalibrateButton,SLOT(setShown(bool)));
    }
    else if (uas->getSystemType() == MAV_TYPE_GROUND_ROVER)
    {
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.compassButton,SLOT(setShown(bool)));
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.radioCalibrateButton,SLOT(setShown(bool)));
    }
    else
    {
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.compassButton,SLOT(setShown(bool)));
        connect(ui.manditoryHardware,SIGNAL(toggled(bool)),ui.radioCalibrateButton,SLOT(setShown(bool)));
    }
    ui.firmwareButton->setVisible(true);
    ui.manditoryHardware->setVisible(true);
    ui.manditoryHardware->setChecked(true);
    ui.optionalHardwareButton->setVisible(true);
    ui.optionalHardwareButton->setChecked(true);
}
