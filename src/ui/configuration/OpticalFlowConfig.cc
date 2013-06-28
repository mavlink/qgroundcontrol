#include "OpticalFlowConfig.h"
#include <QMessageBox>

OpticalFlowConfig::OpticalFlowConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    connect(ui.enableCheckBox,SIGNAL(clicked(bool)),this,SLOT(enableCheckBoxClicked(bool)));
}

OpticalFlowConfig::~OpticalFlowConfig()
{
}
void OpticalFlowConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
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
        QMessageBox::information(0,tr("Error"),tr("Please connect to a MAV before attempting to set configuration"));
        return;
    }
    m_uas->setParameter(0,"FLOW_ENABLE",checked ? 1 : 0);
}
