#include "ApmPlaneLevel.h"
#include <QMessageBox>

ApmPlaneLevel::ApmPlaneLevel(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    connect(ui.levelPushButton,SIGNAL(clicked()),this,SLOT(levelClicked()));
    connect(ui.manualLevelCheckBox,SIGNAL(toggled(bool)),this,SLOT(manualCheckBoxToggled(bool)));
    initConnections();
}

ApmPlaneLevel::~ApmPlaneLevel()
{
}
void ApmPlaneLevel::levelClicked()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    QMessageBox::information(0,"Warning","Be sure the plane is completly level, then click ok");
    MAV_CMD command = MAV_CMD_PREFLIGHT_CALIBRATION;
    int confirm = 0;
    float param1 = 1.0;
    float param2 = 0.0;
    float param3 = 1.0;
    float param4 = 0.0;
    float param5 = 0.0;
    float param6 = 0.0;
    float param7 = 0.0;
    int component = 1;
    m_uas->executeCommand(command, confirm, param1, param2, param3, param4, param5, param6, param7, component);
    QMessageBox::information(0,"Warning","Leveling completed");
}

void ApmPlaneLevel::manualCheckBoxToggled(bool checked)
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    if (checked)
    {
        m_uas->getParamManager()->setParameter(1,"MANUAL_LEVEL",1);
    }
    else
    {
        m_uas->getParamManager()->setParameter(1,"MANUAL_LEVEL",0);
    }
}
void ApmPlaneLevel::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);
    
    if (parameterName == "MANUAL_LEVEL")
    {
        if (value.toInt() == 1)
        {
            ui.manualLevelCheckBox->setChecked(true);
        }
        else
        {
            ui.manualLevelCheckBox->setChecked(false);
        }
    }
}
