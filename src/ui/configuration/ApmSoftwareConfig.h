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
    QPointer<BasicPidConfig> m_basicPidConfig;
    QPointer<FlightModeConfig> m_flightConfig;
    QPointer<StandardParamConfig> m_standardParamConfig;
    QPointer<GeoFenceConfig> m_geoFenceConfig;
    QPointer<FailSafeConfig> m_failSafeConfig;
    QPointer<AdvancedParamConfig> m_advancedParamConfig;
    QPointer<ArduCopterPidConfig> m_arduCopterPidConfig;
    QPointer<ArduPlanePidConfig> m_arduPlanePidConfig;
    QPointer<ArduRoverPidConfig> m_arduRoverPidConfig;
    QPointer<AdvParameterList> m_advParameterList;
    QMap<QObject*,QWidget*> m_buttonToConfigWidgetMap;
};

#endif // APMSOFTWARECONFIG_H
