#include "OpticalFlowConfig.h"
#include <QMessageBox>

OpticalFlowConfig::OpticalFlowConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    connect(ui.enableCheckBox,SIGNAL(clicked(bool)),this,SLOT(enableCheckBoxClicked(bool)));
    initConnections();
}

OpticalFlowConfig::~OpticalFlowConfig()
{
}
void OpticalFlowConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);
    
    if (parameterName == "FLOW_ENABLE")
    {
        if (value.toInt() == 0)
        {
            ui.enableCheckBox->setChecked(false);
        }
        else
        {
            ui.enableCheckBox->setChecked(true);
        }
    }
}

void OpticalFlowConfig::enableCheckBoxClicked(bool checked)
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    m_uas->getParamManager()->setParameter(1,"FLOW_ENABLE",checked ? 1 : 0);
}
