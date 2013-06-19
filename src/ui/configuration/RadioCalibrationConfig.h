#ifndef RADIOCALIBRATIONCONFIG_H
#define RADIOCALIBRATIONCONFIG_H

#include <QWidget>
#include "ui_RadioCalibrationConfig.h"

class RadioCalibrationConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit RadioCalibrationConfig(QWidget *parent = 0);
    ~RadioCalibrationConfig();
    
private:
    Ui::RadioCalibrationConfig ui;
};

#endif // RADIOCALIBRATIONCONFIG_H
