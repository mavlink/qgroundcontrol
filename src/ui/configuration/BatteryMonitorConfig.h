#ifndef BATTERYMONITORCONFIG_H
#define BATTERYMONITORCONFIG_H

#include <QWidget>
#include "AP2ConfigWidget.h"
#include "ui_BatteryMonitorConfig.h"

class BatteryMonitorConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit BatteryMonitorConfig(QWidget *parent = 0);
    ~BatteryMonitorConfig();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void monitorCurrentIndexChanged(int index);
    void sensorCurrentIndexChanged(int index);
    void apmVerCurrentIndexChanged(int index);
    void calcDividerSet();
    void ampsPerVoltSet();
    void batteryCapacitySet();
    void activeUASSet(UASInterface *uas);
    void batteryChanged(UASInterface* uas, double voltage, double current, double percent, int seconds);
private:
    Ui::BatteryMonitorConfig ui;
};

#endif // BATTERYMONITORCONFIG_H
