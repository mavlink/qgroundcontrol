#ifndef APMSOFTWARECONFIG_H
#define APMSOFTWARECONFIG_H

#include <QWidget>
#include "ui_ApmSoftwareConfig.h"
#include "FlightModeConfig.h"
#include "BasicPidConfig.h"
#include "StandardParamConfig.h"
#include "GeoFenceConfig.h"
#include "FailSafeConfig.h"
#include "AdvancedParamConfig.h"
#include "ArduCopterPidConfig.h"
#include "ArduPlanePidConfig.h"
#include "ArduRoverPidConfig.h"
#include "AdvParameterList.h"
#include "UASInterface.h"
#include "UASManager.h"

class ApmSoftwareConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit ApmSoftwareConfig(QWidget *parent = 0);
    ~ApmSoftwareConfig();
private slots:
    void activateStackedWidget();
    void activeUASSet(UASInterface *uas);
private:
    Ui::ApmSoftwareConfig ui;
    BasicPidConfig *basicPidConfig;
    FlightModeConfig *flightConfig;
    StandardParamConfig *standardParamConfig;
    GeoFenceConfig *geoFenceConfig;
    FailSafeConfig *failSafeConfig;
    AdvancedParamConfig *advancedParamConfig;
    ArduCopterPidConfig *arduCopterPidConfig;
    ArduPlanePidConfig *arduPlanePidConfig;
    ArduRoverPidConfig *arduRoverPidConfig;
    AdvParameterList *advParameterList;
    QMap<QObject*,QWidget*> buttonToConfigWidgetMap;
};

#endif // APMSOFTWARECONFIG_H
