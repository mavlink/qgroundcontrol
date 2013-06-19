#ifndef SONARCONFIG_H
#define SONARCONFIG_H

#include <QWidget>
#include "ui_SonarConfig.h"

class SonarConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit SonarConfig(QWidget *parent = 0);
    ~SonarConfig();
    
private:
    Ui::SonarConfig ui;
};

#endif // SONARCONFIG_H
