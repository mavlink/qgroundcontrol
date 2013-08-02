#ifndef ADVANCEDPARAMCONFIG_H
#define ADVANCEDPARAMCONFIG_H

#include <QWidget>
#include "ui_AdvancedParamConfig.h"
#include "AP2ConfigWidget.h"
#include "ParamWidget.h"
class AdvancedParamConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit AdvancedParamConfig(QWidget *parent = 0);
    ~AdvancedParamConfig();
    void addRange(QString title,QString description,QString param,double min,double max);
    void addCombo(QString title,QString description,QString param,QList<QPair<int,QString> > valuelist);
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void doubleValueChanged(QString param,double value);
    void intValueChanged(QString param,int value);
private:
    QMap<QString,ParamWidget*> m_paramToWidgetMap;
    Ui::AdvancedParamConfig ui;
};

#endif // ADVANCEDPARAMCONFIG_H
