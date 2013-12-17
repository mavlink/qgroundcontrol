#ifndef ARDUROVERPIDCONFIG_H
#define ARDUROVERPIDCONFIG_H

#include <QWidget>
#include "ui_ArduRoverPidConfig.h"
#include "AP2ConfigWidget.h"
class ArduRoverPidConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit ArduRoverPidConfig(QWidget *parent = 0);
    ~ArduRoverPidConfig();
private slots:
    void writeButtonClicked();
    void refreshButtonClicked();
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
private:
    QMap<QString,QDoubleSpinBox*> nameToBoxMap;
    Ui::ArduRoverPidConfig ui;
};

#endif // ARDUROVERPIDCONFIG_H
