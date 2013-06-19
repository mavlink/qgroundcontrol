#ifndef BATTERYMONITORCONFIG_H
#define BATTERYMONITORCONFIG_H

#include <QWidget>
#include "ui_BatteryMonitorConfig.h"

class BatteryMonitorConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit BatteryMonitorConfig(QWidget *parent = 0);
    ~BatteryMonitorConfig();
    
private:
    Ui::BatteryMonitorConfig ui;
};

#endif // BATTERYMONITORCONFIG_H
