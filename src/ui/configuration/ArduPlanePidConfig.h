#ifndef ARDUPLANEPIDCONFIG_H
#define ARDUPLANEPIDCONFIG_H

#include <QWidget>
#include "ui_ArduPlanePidConfig.h"
#include "AP2ConfigWidget.h"

class ArduPlanePidConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit ArduPlanePidConfig(QWidget *parent = 0);
    ~ArduPlanePidConfig();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void writeButtonClicked();
    void refreshButtonClicked();
private:
    QMap<QString,QDoubleSpinBox*> nameToBoxMap;
    Ui::ArduPlanePidConfig ui;
};

#endif // ARDUPLANEPIDCONFIG_H
