#include "FailSafeConfig.h"


FailSafeConfig::FailSafeConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    ui.radio1In->setName("Radio 1");
    ui.radio1In->setMin(800);
    ui.radio1In->setMax(2200);
    ui.radio1In->setOrientation(Qt::Horizontal);
    ui.radio2In->setName("Radio 2");
    ui.radio2In->setMin(800);
    ui.radio2In->setMax(2200);
    ui.radio2In->setOrientation(Qt::Horizontal);
    ui.radio3In->setName("Radio 3");
    ui.radio3In->setMin(800);
    ui.radio3In->setMax(2200);
    ui.radio3In->setOrientation(Qt::Horizontal);
    ui.radio4In->setName("Radio 4");
    ui.radio4In->setMin(800);
    ui.radio4In->setMax(2200);
    ui.radio4In->setOrientation(Qt::Horizontal);
    ui.radio5In->setName("Radio 5");
    ui.radio5In->setMin(800);
    ui.radio5In->setMax(2200);
    ui.radio5In->setOrientation(Qt::Horizontal);
    ui.radio6In->setName("Radio 6");
    ui.radio6In->setMin(800);
    ui.radio6In->setMax(2200);
    ui.radio6In->setOrientation(Qt::Horizontal);
    ui.radio7In->setName("Radio 7");
    ui.radio7In->setMin(800);
    ui.radio7In->setMax(2200);
    ui.radio7In->setOrientation(Qt::Horizontal);
    ui.radio8In->setName("Radio 8");
    ui.radio8In->setMin(800);
    ui.radio8In->setMax(2200);
    ui.radio8In->setOrientation(Qt::Horizontal);

    ui.radio1Out->setName("Radio 1");
    ui.radio1Out->setMin(800);
    ui.radio1Out->setMax(2200);
    ui.radio1Out->setOrientation(Qt::Horizontal);
    ui.radio2Out->setName("Radio 2");
    ui.radio2Out->setMin(800);
    ui.radio2Out->setMax(2200);
    ui.radio2Out->setOrientation(Qt::Horizontal);
    ui.radio3Out->setName("Radio 3");
    ui.radio3Out->setMin(800);
    ui.radio3Out->setMax(2200);
    ui.radio3Out->setOrientation(Qt::Horizontal);
    ui.radio4Out->setName("Radio 4");
    ui.radio4Out->setMin(800);
    ui.radio4Out->setMax(2200);
    ui.radio4Out->setOrientation(Qt::Horizontal);
    ui.radio5Out->setName("Radio 5");
    ui.radio5Out->setMin(800);
    ui.radio5Out->setMax(2200);
    ui.radio5Out->setOrientation(Qt::Horizontal);
    ui.radio6Out->setName("Radio 6");
    ui.radio6Out->setMin(800);
    ui.radio6Out->setMax(2200);
    ui.radio6Out->setOrientation(Qt::Horizontal);
    ui.radio7Out->setName("Radio 7");
    ui.radio7Out->setMin(800);
    ui.radio7Out->setMax(2200);
    ui.radio7Out->setOrientation(Qt::Horizontal);
    ui.radio8Out->setName("Radio 8");
    ui.radio8Out->setMin(800);
    ui.radio8Out->setMax(2200);
    ui.radio8Out->setOrientation(Qt::Horizontal);

    ui.throttleFailSafeComboBox->addItem("Disable");
    ui.throttleFailSafeComboBox->addItem("Enabled - Always TRL");
    ui.throttleFailSafeComboBox->addItem("Enabled - Continue in auto");

    connect(ui.batteryFailCheckBox,SIGNAL(clicked(bool)),this,SLOT(batteryFailChecked(bool)));
    connect(ui.fsLongCheckBox,SIGNAL(clicked(bool)),this,SLOT(fsLongClicked(bool)));
    connect(ui.fsShortCheckBox,SIGNAL(clicked(bool)),this,SLOT(fsShortClicked(bool)));
    connect(ui.gcsCheckBox,SIGNAL(clicked(bool)),this,SLOT(gcsChecked(bool)));
    connect(ui.throttleActionCheckBox,SIGNAL(clicked(bool)),this,SLOT(throttleActionChecked(bool)));
    connect(ui.throttleCheckBox,SIGNAL(clicked(bool)),this,SLOT(throttleChecked(bool)));
    connect(ui.throttlePwmSpinBox,SIGNAL(editingFinished()),this,SLOT(throttlePwmChanged()));
    connect(ui.throttleFailSafeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(throttleFailSafeChanged(int)));
}
void FailSafeConfig::gcsChecked(bool checked)
{
    if (!m_uas)
    {
        return;
    }
    if (checked)
    {
        m_uas->setParameter(1,"FS_GCS_ENABL",1);
    }
    else
    {
        m_uas->setParameter(1,"FS_GCS_ENABL",0);
    }
}

void FailSafeConfig::throttleActionChecked(bool checked)
{
    if (!m_uas)
    {
        return;
    }
    if (checked)
    {
        m_uas->setParameter(1,"THR_FS_ACTION",1);
    }
    else
    {
        m_uas->setParameter(1,"THR_FS_ACTION",0);
    }
}

void FailSafeConfig::throttleChecked(bool checked)
{
    if (!m_uas)
    {
        return;
    }
    if (checked)
    {
        m_uas->setParameter(1,"THR_FAILSAFE",1);
    }
    else
    {
        m_uas->setParameter(1,"THR_FAILSAFE",0);
    }
}

void FailSafeConfig::throttlePwmChanged()
{
    if (!m_uas)
    {
        return;
    }
    m_uas->setParameter(1,"THR_FS_VALUE",ui.throttlePwmSpinBox->value());
}

void FailSafeConfig::throttleFailSafeChanged(int index)
{
    if (!m_uas)
    {
        return;
    }
    m_uas->setParameter(1,"FS_THR_ENABLE",index);
}

void FailSafeConfig::fsLongClicked(bool checked)
{
    if (!m_uas)
    {
        return;
    }
    if (checked)
    {
        m_uas->setParameter(1,"FS_LONG_ACTN",1);
    }
    else
    {
        m_uas->setParameter(1,"FS_LONG_ACTN",0);
    }
}

void FailSafeConfig::fsShortClicked(bool checked)
{
    if (!m_uas)
    {
        return;
    }
    if (checked)
    {
        m_uas->setParameter(1,"FS_SHORT_ACTN",1);
    }
    else
    {
        m_uas->setParameter(1,"FS_SHORT_ACTN",0);
    }
}

void FailSafeConfig::batteryFailChecked(bool checked)
{
    if (!m_uas)
    {
        return;
    }
    if (checked)
    {
        m_uas->setParameter(1,"FS_BATT_ENABLE",1);
    }
    else
    {
        m_uas->setParameter(1,"FS_BATT_ENABLE",0);
    }
}

FailSafeConfig::~FailSafeConfig()
{
}
void FailSafeConfig::activeUASSet(UASInterface *uas)
{
    AP2ConfigWidget::activeUASSet(uas);
    connect(uas,SIGNAL(remoteControlChannelRawChanged(int,float)),this,SLOT(remoteControlChannelRawChanges(int,float)));
    connect(uas,SIGNAL(hilActuatorsChanged(uint64_t,float,float,float,float,float,float,float,float)),this,SLOT(hilActuatorsChanged(uint64_t,float,float,float,float,float,float,float,float)));
    connect(uas,SIGNAL(armingChanged(bool)),this,SLOT(armingChanged(bool)));
    if (uas->getSystemType() == MAV_TYPE_FIXED_WING)
    {
        ui.batteryFailCheckBox->setVisible(false);
        ui.throttleFailSafeComboBox->setVisible(false);
        ui.batteryVoltSpinBox->setVisible(false);
        ui.label_6->setVisible(false);

        ui.throttlePwmSpinBox->setVisible(true); //Both

        ui.throttleCheckBox->setVisible(true);
        ui.throttleActionCheckBox->setVisible(true);
        ui.gcsCheckBox->setVisible(true);
        ui.fsLongCheckBox->setVisible(true);
        ui.fsShortCheckBox->setVisible(true);
    }
    else if (uas->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        ui.batteryFailCheckBox->setVisible(true);
        ui.throttleFailSafeComboBox->setVisible(true);
        ui.batteryVoltSpinBox->setVisible(true);
        ui.label_6->setVisible(true);

        ui.throttlePwmSpinBox->setVisible(true); //Both

        ui.throttleCheckBox->setVisible(false);
        ui.throttleActionCheckBox->setVisible(false);
        ui.gcsCheckBox->setVisible(false);
        ui.fsLongCheckBox->setVisible(false);
        ui.fsShortCheckBox->setVisible(false);
    }
    else
    {
        //Show all, just in case
        ui.batteryFailCheckBox->setVisible(true);
        ui.throttleFailSafeComboBox->setVisible(true);
        ui.batteryVoltSpinBox->setVisible(true);
        ui.throttlePwmSpinBox->setVisible(true); //Both
        ui.throttleCheckBox->setVisible(true);
        ui.throttleActionCheckBox->setVisible(true);
        ui.gcsCheckBox->setVisible(true);
        ui.fsLongCheckBox->setVisible(true);
        ui.fsShortCheckBox->setVisible(true);
    }

}
void FailSafeConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    //Arducopter
    if (parameterName == "FS_THR_ENABLE")
    {
        ui.throttleFailSafeComboBox->setCurrentIndex(value.toInt());
    }
    else if (parameterName == "FS_THR_VALUE")
    {
        ui.throttlePwmSpinBox->setValue(value.toFloat());
    }
    else if (parameterName == "FS_BATT_ENABLE")
    {
        if (value.toInt() == 0)
        {
            ui.batteryFailCheckBox->setChecked(false);
        }
        else
        {
            ui.batteryFailCheckBox->setChecked(true);
        }
    }
    else if (parameterName == "LOW_VOLT")
    {
        ui.batteryVoltSpinBox->setValue(value.toFloat());
    }
    //Arduplane
    else if (parameterName == "THR_FAILSAFE")
    {
        if (value.toInt() == 0)
        {
            ui.throttleCheckBox->setChecked(false);
        }
        else
        {
            ui.throttleCheckBox->setChecked(true);
        }
    }
    else if (parameterName == "THR_FS_VALUE")
    {
        ui.throttlePwmSpinBox->setValue(value.toFloat());
    }
    else if (parameterName == "THR_FS_ACTION")
    {
        if (value.toInt() == 0)
        {
            ui.throttleActionCheckBox->setChecked(false);
        }
        else
        {
            ui.throttleActionCheckBox->setChecked(true);
        }
    }
    else if (parameterName == "FS_GCS_ENABL")
    {
        if (value.toInt() == 0)
        {
            ui.gcsCheckBox->setChecked(false);
        }
        else
        {
            ui.gcsCheckBox->setChecked(true);
        }
    }
    else if (parameterName == "FS_SHORT_ACTN")
    {
        if (value.toInt() == 0)
        {
            ui.fsShortCheckBox->setChecked(false);
        }
        else
        {
            ui.fsShortCheckBox->setChecked(true);
        }
    }
    else if (parameterName == "FS_LONG_ACTN")
    {
        if (value.toInt() == 0)
        {
            ui.fsLongCheckBox->setChecked(false);
        }
        else
        {
            ui.fsLongCheckBox->setChecked(true);
        }
    }

}

void FailSafeConfig::armingChanged(bool armed)
{
    if (armed)
    {
        ui.armedLabel->setText("<h1>ARMED</h1>");
    }
    else
    {
        ui.armedLabel->setText("<h1>DISARMED</h1>");
    }
}

void FailSafeConfig::remoteControlChannelRawChanges(int chan,float value)
{
    if (chan == 0)
    {
        ui.radio1In->setValue(value);
    }
    else if (chan == 1)
    {
        ui.radio2In->setValue(value);
    }
    else if (chan == 2)
    {
        ui.radio3In->setValue(value);
    }
    else if (chan == 3)
    {
        ui.radio4In->setValue(value);
    }
    else if (chan == 4)
    {
        ui.radio5In->setValue(value);
    }
    else if (chan == 5)
    {
        ui.radio6In->setValue(value);
    }
    else if (chan == 6)
    {
        ui.radio7In->setValue(value);
    }
    else if (chan == 7)
    {
        ui.radio8In->setValue(value);
    }
}
void FailSafeConfig::hilActuatorsChanged(uint64_t time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8)
{
    ui.radio1Out->setValue(act1);
    ui.radio2Out->setValue(act2);
    ui.radio3Out->setValue(act3);
    ui.radio4Out->setValue(act4);
    ui.radio5Out->setValue(act5);
    ui.radio6Out->setValue(act6);
    ui.radio7Out->setValue(act7);
    ui.radio8Out->setValue(act8);
}
