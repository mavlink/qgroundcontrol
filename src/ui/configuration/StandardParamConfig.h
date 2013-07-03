#ifndef STANDARDPARAMCONFIG_H
#define STANDARDPARAMCONFIG_H

#include <QWidget>
#include "ui_StandardParamConfig.h"

class StandardParamConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit StandardParamConfig(QWidget *parent = 0);
    ~StandardParamConfig();
    
private:
    Ui::StandardParamConfig ui;
};

#endif // STANDARDPARAMCONFIG_H
