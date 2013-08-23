#ifndef STANDARDPARAMCONFIG_H
#define STANDARDPARAMCONFIG_H

#include <QWidget>
#include "ui_StandardParamConfig.h"
#include "AP2ConfigWidget.h"
#include "ParamWidget.h"
class StandardParamConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit StandardParamConfig(QWidget *parent = 0);
    ~StandardParamConfig();
    void addRange(QString title,QString description,QString param,double min,double max);
    void addCombo(QString title,QString description,QString param,QList<QPair<int,QString> > valuelist);
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void doubleValueChanged(QString param,double value);
    void intValueChanged(QString param,int value);
private:
    QMap<QString,ParamWidget*> paramToWidgetMap;
    Ui::StandardParamConfig ui;
};

#endif // STANDARDPARAMCONFIG_H
