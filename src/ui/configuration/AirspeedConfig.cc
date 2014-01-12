#include "AirspeedConfig.h"
#include <QMessageBox>

AirspeedConfig::AirspeedConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    connect(ui.enableCheckBox,SIGNAL(toggled(bool)),this,SLOT(enableCheckBoxClicked(bool)));
    connect(ui.useAirspeedCheckBox,SIGNAL(toggled(bool)),this,SLOT(useCheckBoxClicked(bool)));
    initConnections();
}

AirspeedConfig::~AirspeedConfig()
{
}
void AirspeedConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);
    
    if (parameterName == "ARSPD_ENABLE")
    {
        if (value.toInt() == 0)
        {
            disconnect(ui.enableCheckBox,SIGNAL(toggled(bool)),this,SLOT(enableCheckBoxClicked(bool)));
            ui.enableCheckBox->setChecked(false);
            connect(ui.enableCheckBox,SIGNAL(toggled(bool)),this,SLOT(enableCheckBoxClicked(bool)));
            ui.useAirspeedCheckBox->setEnabled(false);
        }
        else
        {
            disconnect(ui.enableCheckBox,SIGNAL(toggled(bool)),this,SLOT(enableCheckBoxClicked(bool)));
            ui.enableCheckBox->setChecked(true);
            connect(ui.enableCheckBox,SIGNAL(toggled(bool)),this,SLOT(enableCheckBoxClicked(bool)));
            ui.useAirspeedCheckBox->setEnabled(true);
        }
    }
    else if (parameterName == "ARSPD_USE")
    {
        if (value.toInt() == 0)
        {
            disconnect(ui.useAirspeedCheckBox,SIGNAL(toggled(bool)),this,SLOT(useCheckBoxClicked(bool)));
            ui.useAirspeedCheckBox->setChecked(false);
            connect(ui.useAirspeedCheckBox,SIGNAL(toggled(bool)),this,SLOT(useCheckBoxClicked(bool)));
        }
        else
        {
            disconnect(ui.useAirspeedCheckBox,SIGNAL(toggled(bool)),this,SLOT(useCheckBoxClicked(bool)));
            ui.useAirspeedCheckBox->setChecked(true);
            connect(ui.useAirspeedCheckBox,SIGNAL(toggled(bool)),this,SLOT(useCheckBoxClicked(bool)));
        }
    }
}

void AirspeedConfig::useCheckBoxClicked(bool checked)
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    if (checked)
    {
        m_uas->getParamManager()->setParameter(1,"ARSPD_USE",1);
    }
    else
    {
        m_uas->getParamManager()->setParameter(1,"ARSPD_USE",0);
    }
}

void AirspeedConfig::enableCheckBoxClicked(bool checked)
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    if (checked)
    {
        m_uas->getParamManager()->setParameter(1,"ARSPD_ENABLE",1);
    }
    else
    {
        m_uas->getParamManager()->setParameter(1,"ARSPD_ENABLE",0);
    }
}
