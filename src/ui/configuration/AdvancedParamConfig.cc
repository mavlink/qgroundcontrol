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
    ParamWidget *widget = new ParamWidget(param,ui.scrollAreaWidgetContents);
    connect(widget,SIGNAL(doubleValueChanged(QString,double)),this,SLOT(doubleValueChanged(QString,double)));
    connect(widget,SIGNAL(intValueChanged(QString,int)),this,SLOT(intValueChanged(QString,int)));
    m_paramToWidgetMap[param] = widget;
    widget->setupDouble(title + "(" + param + ")",description,0,min,max);
    ui.verticalLayout->addWidget(widget);
    widget->show();
}

void AdvancedParamConfig::addCombo(QString title,QString description,QString param,QList<QPair<int,QString> > valuelist)
{
    ParamWidget *widget = new ParamWidget(param,ui.scrollAreaWidgetContents);
    connect(widget,SIGNAL(doubleValueChanged(QString,double)),this,SLOT(doubleValueChanged(QString,double)));
    connect(widget,SIGNAL(intValueChanged(QString,int)),this,SLOT(intValueChanged(QString,int)));
    m_paramToWidgetMap[param] = widget;
    widget->setupCombo(title + "(" + param + ")",description,valuelist);
    ui.verticalLayout->addWidget(widget);
    widget->show();
}
void AdvancedParamConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);
    
    if (m_paramToWidgetMap.contains(parameterName))
    {
        if (value.type() == QVariant::Double)
        {
            m_paramToWidgetMap[parameterName]->setValue(value.toDouble());
        }
        else
        {
            m_paramToWidgetMap[parameterName]->setValue(value.toInt());
        }
    }
}
void AdvancedParamConfig::doubleValueChanged(QString param,double value)
{
    if (!m_uas)
    {
        this->showNullMAVErrorMessageBox();
    }
    m_uas->getParamManager()->setParameter(1,param,value);
}

void AdvancedParamConfig::intValueChanged(QString param,int value)
{
    if (!m_uas)
    {
        this->showNullMAVErrorMessageBox();
    }
    m_uas->getParamManager()->setParameter(1,param,value);
}
