#ifndef APMSOFTWARECONFIG_H
#define APMSOFTWARECONFIG_H

#include <QWidget>
#include "ui_ApmSoftwareConfig.h"

class ApmSoftwareConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit ApmSoftwareConfig(QWidget *parent = 0);
    ~ApmSoftwareConfig();
    
private:
    Ui::ApmSoftwareConfig ui;
};

#endif // APMSOFTWARECONFIG_H
