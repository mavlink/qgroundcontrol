#ifndef ADVANCEDPARAMCONFIG_H
#define ADVANCEDPARAMCONFIG_H

#include <QWidget>
#include "ui_AdvancedParamConfig.h"

class AdvancedParamConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit AdvancedParamConfig(QWidget *parent = 0);
    ~AdvancedParamConfig();
    
private:
    Ui::AdvancedParamConfig ui;
};

#endif // ADVANCEDPARAMCONFIG_H
