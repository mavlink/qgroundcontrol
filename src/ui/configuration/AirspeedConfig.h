#ifndef AIRSPEEDCONFIG_H
#define AIRSPEEDCONFIG_H

#include <QWidget>
#include "AP2ConfigWidget.h"
#include "ui_AirspeedConfig.h"

class AirspeedConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit AirspeedConfig(QWidget *parent = 0);
    ~AirspeedConfig();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void useCheckBoxClicked(bool checked);
    void enableCheckBoxClicked(bool checked);
private:
    Ui::AirspeedConfig ui;
};

#endif // AIRSPEEDCONFIG_H
