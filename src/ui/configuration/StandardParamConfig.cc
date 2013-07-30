#include "StandardParamConfig.h"
#include "ParamWidget.h"
StandardParamConfig::StandardParamConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    initConnections();
}
StandardParamConfig::~StandardParamConfig()
{
}
void StandardParamConfig::addRange(QString title,QString description,QString param,double min,double max)
{
    ParamWidget *widget = new ParamWidget(ui.scrollAreaWidgetContents);
    paramToWidgetMap[param] = widget;
    widget->setupDouble(title + "(" + param + ")",description,0,min,max);
    ui.verticalLayout->addWidget(widget);
    widget->show();
}

void StandardParamConfig::addCombo(QString title,QString description,QString param,QList<QPair<int,QString> > valuelist)
{
    ParamWidget *widget = new ParamWidget(ui.scrollAreaWidgetContents);
    paramToWidgetMap[param] = widget;
    widget->setupCombo(title + "(" + param + ")",description,valuelist);
    ui.verticalLayout->addWidget(widget);
    widget->show();
}
void StandardParamConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (paramToWidgetMap.contains(parameterName))
    {
        paramToWidgetMap[parameterName]->setValue(value.toDouble());
    }
}
