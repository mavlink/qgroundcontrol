#ifndef CAMERAGIMBALCONFIG_H
#define CAMERAGIMBALCONFIG_H

#include <QWidget>
#include "ui_CameraGimbalConfig.h"

class CameraGimbalConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit CameraGimbalConfig(QWidget *parent = 0);
    ~CameraGimbalConfig();
    
private:
    Ui::CameraGimbalConfig ui;
};

#endif // CAMERAGIMBALCONFIG_H
