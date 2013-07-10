#ifndef ARDUCOPTERPIDCONFIG_H
#define ARDUCOPTERPIDCONFIG_H

#include <QWidget>
#include "ui_ArduCopterPidConfig.h"

#include "AP2ConfigWidget.h"

class ArduCopterPidConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit ArduCopterPidConfig(QWidget *parent = 0);
    ~ArduCopterPidConfig();
    
private:
    Ui::ArduCopterPidConfig ui;
};

#endif // ARDUCOPTERPIDCONFIG_H
