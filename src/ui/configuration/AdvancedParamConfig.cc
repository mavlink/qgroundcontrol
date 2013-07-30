#include "AdvancedParamConfig.h"


AdvancedParamConfig::AdvancedParamConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    initConnections();
}

AdvancedParamConfig::~AdvancedParamConfig()
{
}
void AdvancedParamConfig::addRange(QString title,QString description,QString param,double min,double max)
{
    ParamWidget *widget = new ParamWidget(ui.scrollAreaWidgetContents);
    m_paramToWidgetMap[param] = widget;
    widget->setupDouble(title + "(" + param + ")",description,0,min,max);
    ui.verticalLayout->addWidget(widget);
    widget->show();
}

void AdvancedParamConfig::addCombo(QString title,QString description,QString param,QList<QPair<int,QString> > valuelist)
{
    ParamWidget *widget = new ParamWidget(ui.scrollAreaWidgetContents);
    m_paramToWidgetMap[param] = widget;
    widget->setupCombo(title + "(" + param + ")",description,valuelist);
    ui.verticalLayout->addWidget(widget);
    widget->show();
}
void AdvancedParamConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (m_paramToWidgetMap.contains(parameterName))
    {
        m_paramToWidgetMap[parameterName]->setValue(value.toDouble());
    }
}
