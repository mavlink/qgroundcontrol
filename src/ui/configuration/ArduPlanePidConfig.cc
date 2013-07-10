#include "ArduPlanePidConfig.h"


ArduPlanePidConfig::ArduPlanePidConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
}

ArduPlanePidConfig::~ArduPlanePidConfig()
{
}
void ArduPlanePidconfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (parameterName == "")
    {

    }
}
