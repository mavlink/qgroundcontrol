#ifndef OPTICALFLOWCONFIG_H
#define OPTICALFLOWCONFIG_H

#include <QWidget>
#include "ui_OpticalFlowConfig.h"

class OpticalFlowConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit OpticalFlowConfig(QWidget *parent = 0);
    ~OpticalFlowConfig();
    
private:
    Ui::OpticalFlowConfig ui;
};

#endif // OPTICALFLOWCONFIG_H
