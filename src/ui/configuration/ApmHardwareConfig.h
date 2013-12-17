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
 *   @brief APM Hardware Configuration widget header.
 *
 *   @author Michael Carpenter <malcom2073@gmail.com>
 *
 */

#ifndef APMHARDWARECONFIG_H
#define APMHARDWARECONFIG_H

#include <QWidget>
#include "ui_ApmHardwareConfig.h"
#include <UASInterface.h>
#include <UASManager.h>
#include "FrameTypeConfig.h"
#include "CompassConfig.h"
#include "AccelCalibrationConfig.h"
#include "RadioCalibrationConfig.h"
#include "Radio3DRConfig.h"
#include "BatteryMonitorConfig.h"
#include "SonarConfig.h"
#include "AirspeedConfig.h"
#include "OpticalFlowConfig.h"
#include "OsdConfig.h"
#include "CameraGimbalConfig.h"
#include "AntennaTrackerConfig.h"
#include "ApmPlaneLevel.h"
#include "ApmFirmwareConfig.h"

class ApmHardwareConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit ApmHardwareConfig(QWidget *parent = 0);
    ~ApmHardwareConfig();
private:
    QPointer<FrameTypeConfig> m_frameConfig;
    QPointer<CompassConfig> m_compassConfig;
    QPointer<AccelCalibrationConfig> m_accelConfig;
    QPointer<RadioCalibrationConfig> m_radioConfig;

    QPointer<ApmFirmwareConfig> m_apmFirmwareConfig;
    QPointer<Radio3DRConfig> m_radio3drConfig;
    QPointer<BatteryMonitorConfig> m_batteryConfig;
    QPointer<SonarConfig> m_sonarConfig;
    QPointer<AirspeedConfig> m_airspeedConfig;
    QPointer<OpticalFlowConfig> m_opticalFlowConfig;
    QPointer<OsdConfig> m_osdConfig;
    QPointer<CameraGimbalConfig> m_cameraGimbalConfig;
    QPointer<AntennaTrackerConfig> m_antennaTrackerConfig;
    QPointer<ApmPlaneLevel> m_planeLevel;

private slots:
    void activeUASSet(UASInterface *uas);
    void activateStackedWidget();
private:
    Ui::ApmHardwareConfig ui;

    //This is a map between the buttons, and the widgets they should be displying
    QMap<QObject*,QWidget*> m_buttonToConfigWidgetMap;
};

#endif // APMHARDWARECONFIG_H
