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
#include "FlightModeConfig.h"
#include "Radio3DRConfig.h"
#include "BatteryMonitorConfig.h"
#include "SonarConfig.h"
#include "AirspeedConfig.h"
#include "OpticalFlowConfig.h"
#include "OsdConfig.h"
#include "CameraGimbalConfig.h"
#include "AntennaTrackerConfig.h"

class ApmHardwareConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit ApmHardwareConfig(QWidget *parent = 0);
    ~ApmHardwareConfig();
private:
    FrameTypeConfig *frameConfig;
    CompassConfig *compassConfig;
    AccelCalibrationConfig *accelConfig;
    RadioCalibrationConfig *radioConfig;
    FlightModeConfig *flightConfig;

    Radio3DRConfig *radio3drConfig;
    BatteryMonitorConfig *batteryConfig;
    SonarConfig *sonarConfig;
    AirspeedConfig *airspeedConfig;
    OpticalFlowConfig *opticalFlowConfig;
    OsdConfig *osdConfig;
    CameraGimbalConfig *cameraGimbalConfig;
    AntennaTrackerConfig *antennaTrackerConfig;
private slots:
    void activeUASSet(UASInterface *uas);
    void activateStackedWidget();
private:
    Ui::ApmHardwareConfig ui;

    //This is a map between the buttons, and the widgets they should be displying
    QMap<QObject*,QWidget*> buttonToConfigWidgetMap;
};

#endif // APMHARDWARECONFIG_H
