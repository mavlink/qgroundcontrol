#ifndef ANTENNATRACKERCONFIG_H
#define ANTENNATRACKERCONFIG_H

#include <QWidget>
#include "ui_AntennaTrackerConfig.h"

class AntennaTrackerConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit AntennaTrackerConfig(QWidget *parent = 0);
    ~AntennaTrackerConfig();
    
private:
    Ui::AntennaTrackerConfig ui;
};

#endif // ANTENNATRACKERCONFIG_H
