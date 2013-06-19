#ifndef ACCELCALIBRATIONCONFIG_H
#define ACCELCALIBRATIONCONFIG_H

#include <QWidget>
#include "ui_AccelCalibrationConfig.h"

class AccelCalibrationConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit AccelCalibrationConfig(QWidget *parent = 0);
    ~AccelCalibrationConfig();
    
private:
    Ui::AccelCalibrationConfig ui;
};

#endif // ACCELCALIBRATIONCONFIG_H
