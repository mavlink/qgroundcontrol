#ifndef SONARCONFIG_H
#define SONARCONFIG_H

#include <QWidget>
#include "AP2ConfigWidget.h"
#include "ui_SonarConfig.h"

class SonarConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit SonarConfig(QWidget *parent = 0);
    ~SonarConfig();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void checkBoxToggled(bool enabled);
    void sonarTypeChanged(int index);
private:
    Ui::SonarConfig ui;
};

#endif // SONARCONFIG_H
