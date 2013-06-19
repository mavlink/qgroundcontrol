#ifndef FLIGHTMODECONFIG_H
#define FLIGHTMODECONFIG_H

#include <QWidget>
#include "ui_FlightModeConfig.h"

class FlightModeConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit FlightModeConfig(QWidget *parent = 0);
    ~FlightModeConfig();
    
private:
    Ui::FlightModeConfig ui;
};

#endif // FLIGHTMODECONFIG_H
