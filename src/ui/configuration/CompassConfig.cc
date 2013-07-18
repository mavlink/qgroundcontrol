#include "CompassConfig.h"


CompassConfig::CompassConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    m_uas=0;
    ui.setupUi(this);
    ui.autoDecCheckBox->setEnabled(false);
    ui.enableCheckBox->setEnabled(false);
    ui.orientationComboBox->setEnabled(false);
    ui.declinationLineEdit->setEnabled(false);
    connect(ui.enableCheckBox,SIGNAL(clicked(bool)),this,SLOT(enableClicked(bool)));
    connect(ui.autoDecCheckBox,SIGNAL(clicked(bool)),this,SLOT(autoDecClicked(bool)));
    connect(ui.orientationComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(orientationComboChanged(int)));

    ui.orientationComboBox->addItem("ROTATION_NONE");
    ui.orientationComboBox->addItem("ROTATION_YAW_45");
    ui.orientationComboBox->addItem("ROTATION_YAW_90");
    ui.orientationComboBox->addItem("ROTATION_YAW_135");
    ui.orientationComboBox->addItem("ROTATION_YAW_180");
    ui.orientationComboBox->addItem("ROTATION_YAW_225");
    ui.orientationComboBox->addItem("ROTATION_YAW_270");
    ui.orientationComboBox->addItem("ROTATION_YAW_315");
    ui.orientationComboBox->addItem("ROTATION_ROLL_180");
    ui.orientationComboBox->addItem("ROTATION_ROLL_180_YAW_45");
    ui.orientationComboBox->addItem("ROTATION_ROLL_180_YAW_90");
    ui.orientationComboBox->addItem("ROTATION_ROLL_180_YAW_135");
    ui.orientationComboBox->addItem("ROTATION_PITCH_180");
    ui.orientationComboBox->addItem("ROTATION_ROLL_180_YAW_225");
    ui.orientationComboBox->addItem("ROTATION_ROLL_180_YAW_270");
    ui.orientationComboBox->addItem("ROTATION_ROLL_180_YAW_315");
    ui.orientationComboBox->addItem("ROTATION_ROLL_90");
    ui.orientationComboBox->addItem("ROTATION_ROLL_90_YAW_45");
    ui.orientationComboBox->addItem("ROTATION_ROLL_90_YAW_90");
    ui.orientationComboBox->addItem("ROTATION_ROLL_90_YAW_135");
    ui.orientationComboBox->addItem("ROTATION_ROLL_270");
    ui.orientationComboBox->addItem("ROTATION_ROLL_270_YAW_45");
    ui.orientationComboBox->addItem("ROTATION_ROLL_270_YAW_90");
    ui.orientationComboBox->addItem("ROTATION_ROLL_270_YAW_135");
    ui.orientationComboBox->addItem("ROTATION_PITCH_90");
    ui.orientationComboBox->addItem("ROTATION_PITCH_270");
    ui.orientationComboBox->addItem("ROTATION_MAX");
}
CompassConfig::~CompassConfig()
{
}
void CompassConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (parameterName == "MAG_ENABLE")
    {
        if (value.toInt() == 0)
        {
            ui.enableCheckBox->setChecked(false);
            ui.autoDecCheckBox->setEnabled(false);
            ui.declinationLineEdit->setEnabled(false);
            ui.orientationComboBox->setEnabled(false);
        }
        else
        {
            ui.enableCheckBox->setChecked(true);
            ui.autoDecCheckBox->setEnabled(true);
            ui.declinationLineEdit->setEnabled(true);
            ui.orientationComboBox->setEnabled(true);
        }
        ui.enableCheckBox->setEnabled(true);
    }
    else if (parameterName == "COMPASS_AUTODEC")
    {
        if (value.toInt() == 0)
        {
            ui.autoDecCheckBox->setChecked(false);
        }
        else
        {
            ui.autoDecCheckBox->setChecked(true);
        }
    }
    else if (parameterName == "COMPASS_DEC")
    {
        ui.declinationLineEdit->setText(QString::number(value.toDouble()));
    }
    else if (parameterName == "COMPASS_ORIENT")
    {
        disconnect(ui.orientationComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(orientationComboChanged(int)));
        ui.orientationComboBox->setCurrentIndex(value.toInt());
        connect(ui.orientationComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(orientationComboChanged(int)));
    }
}

void CompassConfig::enableClicked(bool enabled)
{
    if (m_uas)
    {
        if (enabled)
        {
            m_uas->getParamManager()->setParameter(1,"MAG_ENABLE",QVariant(1));
            ui.autoDecCheckBox->setEnabled(true);
            if (!ui.autoDecCheckBox->isChecked())
            {
                ui.declinationLineEdit->setEnabled(true);
            }
        }
        else
        {
            m_uas->getParamManager()->setParameter(1,"MAG_ENABLE",QVariant(0));
            ui.autoDecCheckBox->setEnabled(false);
            ui.declinationLineEdit->setEnabled(false);
        }
    }
}

void CompassConfig::autoDecClicked(bool enabled)
{
    if (m_uas)
    {
        if (enabled)
        {
            m_uas->getParamManager()->setParameter(1,"COMPASS_AUTODEC",QVariant(1));
        }
        else
        {
            m_uas->getParamManager()->setParameter(1,"COMPASS_AUTODEC",QVariant(0));
        }
    }
}

void CompassConfig::orientationComboChanged(int index)
{
    //COMPASS_ORIENT
    if (!m_uas)
    {
        return;
    }
    m_uas->getParamManager()->setParameter(1,"COMPASS_ORIENT",index);

}
