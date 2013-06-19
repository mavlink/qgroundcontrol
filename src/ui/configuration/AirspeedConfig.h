#ifndef AIRSPEEDCONFIG_H
#define AIRSPEEDCONFIG_H

#include <QWidget>
#include "ui_AirspeedConfig.h"

class AirspeedConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit AirspeedConfig(QWidget *parent = 0);
    ~AirspeedConfig();
    
private:
    Ui::AirspeedConfig ui;
};

#endif // AIRSPEEDCONFIG_H
