#ifndef OPTICALFLOWCONFIG_H
#define OPTICALFLOWCONFIG_H

#include <QWidget>
#include "AP2ConfigWidget.h"
#include "ui_OpticalFlowConfig.h"

class OpticalFlowConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit OpticalFlowConfig(QWidget *parent = 0);
    ~OpticalFlowConfig();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void enableCheckBoxClicked(bool checked);
private:
    Ui::OpticalFlowConfig ui;
};

#endif // OPTICALFLOWCONFIG_H
