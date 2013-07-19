#include <QMessageBox>
#include <QDebug>

#include "CameraGimbalConfig.h"

CameraGimbalConfig::CameraGimbalConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    ui.tiltChannelComboBox->addItem(tr("Disable"));
    ui.tiltChannelComboBox->addItem("RC5");
    ui.tiltChannelComboBox->addItem("RC6");
    ui.tiltChannelComboBox->addItem("RC7");
    ui.tiltChannelComboBox->addItem("RC8");
    ui.tiltChannelComboBox->addItem("RC10");
    ui.tiltChannelComboBox->addItem("RC11");

    ui.tiltInputChannelComboBox->addItem(tr("Disable"));
    ui.tiltInputChannelComboBox->addItem("RC5");
    ui.tiltInputChannelComboBox->addItem("RC6");
    ui.tiltInputChannelComboBox->addItem("RC7");
    ui.tiltInputChannelComboBox->addItem("RC8");

    ui.rollChannelComboBox->addItem(tr("Disable"));
    ui.rollChannelComboBox->addItem("RC5");
    ui.rollChannelComboBox->addItem("RC6");
    ui.rollChannelComboBox->addItem("RC7");
    ui.rollChannelComboBox->addItem("RC8");
    ui.rollChannelComboBox->addItem("RC10");
    ui.rollChannelComboBox->addItem("RC11");

    ui.rollInputChannelComboBox->addItem(tr("Disable"));
    ui.rollInputChannelComboBox->addItem("RC5");
    ui.rollInputChannelComboBox->addItem("RC6");
    ui.rollInputChannelComboBox->addItem("RC7");
    ui.rollInputChannelComboBox->addItem("RC8");


    ui.panChannelComboBox->addItem(tr("Disable"));
    ui.panChannelComboBox->addItem("RC5");
    ui.panChannelComboBox->addItem("RC6");
    ui.panChannelComboBox->addItem("RC7");
    ui.panChannelComboBox->addItem("RC8");
    ui.panChannelComboBox->addItem("RC10");
    ui.panChannelComboBox->addItem("RC11");

    ui.panInputChannelComboBox->addItem(tr("Disable"));
    ui.panInputChannelComboBox->addItem("RC5");
    ui.panInputChannelComboBox->addItem("RC6");
    ui.panInputChannelComboBox->addItem("RC7");
    ui.panInputChannelComboBox->addItem("RC8");


    ui.shutterChannelComboBox->addItem(tr("Disable"));
    ui.shutterChannelComboBox->addItem(tr("Relay"));
    ui.shutterChannelComboBox->addItem(tr("Transistor"));
    ui.shutterChannelComboBox->addItem("RC5");
    ui.shutterChannelComboBox->addItem("RC6");
    ui.shutterChannelComboBox->addItem("RC7");
    ui.shutterChannelComboBox->addItem("RC8");
    ui.shutterChannelComboBox->addItem("RC10");
    ui.shutterChannelComboBox->addItem("RC11");

    connect(ui.tiltServoMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.tiltServoMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.tiltAngleMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.tiltAngleMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
    connect(ui.tiltInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
    connect(ui.tiltReverseCheckBox,SIGNAL(clicked(bool)),this,SLOT(updateTilt()));
    connect(ui.tiltStabilizeCheckBox,SIGNAL(clicked(bool)),this,SLOT(updateTilt()));

    connect(ui.rollServoMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateRoll()));
    connect(ui.rollServoMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateRoll()));
    connect(ui.rollAngleMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateRoll()));
    connect(ui.rollAngleMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateRoll()));
    connect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
    connect(ui.rollInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
    connect(ui.rollReverseCheckBox,SIGNAL(clicked(bool)),this,SLOT(updateRoll()));
    connect(ui.rollStabilizeCheckBox,SIGNAL(clicked(bool)),this,SLOT(updateRoll()));

    connect(ui.panServoMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updatePan()));
    connect(ui.panServoMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updatePan()));
    connect(ui.panAngleMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updatePan()));
    connect(ui.panAngleMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updatePan()));
    connect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
    connect(ui.panInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
    connect(ui.panReverseCheckBox,SIGNAL(clicked(bool)),this,SLOT(updatePan()));
    connect(ui.panStabilizeCheckBox,SIGNAL(clicked(bool)),this,SLOT(updatePan()));


    connect(ui.shutterServoMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterServoMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterPushedSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterNotPushedSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterDurationSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));

    connect(ui.retractXSpinBox,SIGNAL(editingFinished()),this,SLOT(updateRetractAngles()));
    connect(ui.retractYSpinBox,SIGNAL(editingFinished()),this,SLOT(updateRetractAngles()));
    connect(ui.retractZSpinBox,SIGNAL(editingFinished()),this,SLOT(updateRetractAngles()));

    connect(ui.controlXSpinBox,SIGNAL(editingFinished()),this,SLOT(updateControlAngles()));
    connect(ui.controlYSpinBox,SIGNAL(editingFinished()),this,SLOT(updateControlAngles()));
    connect(ui.controlZSpinBox,SIGNAL(editingFinished()),this,SLOT(updateControlAngles()));

    connect(ui.neutralXSpinBox,SIGNAL(editingFinished()),this,SLOT(updateNeutralAngles()));
    connect(ui.neutralYSpinBox,SIGNAL(editingFinished()),this,SLOT(updateNeutralAngles()));
    connect(ui.neutralZSpinBox,SIGNAL(editingFinished()),this,SLOT(updateNeutralAngles()));


}
void CameraGimbalConfig::updateRetractAngles()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    m_uas->getParamManager()->setParameter(1,"MNT_RETRACT_X",ui.retractXSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_RETRACT_Y",ui.retractYSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_RETRACT_Z",ui.retractZSpinBox->value());
}

void CameraGimbalConfig::updateNeutralAngles()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    m_uas->getParamManager()->setParameter(1,"MNT_NEUTRAL_X",ui.neutralXSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_NEUTRAL_Y",ui.neutralYSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_NEUTRAL_Z",ui.neutralZSpinBox->value());
}

void CameraGimbalConfig::updateControlAngles()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    m_uas->getParamManager()->setParameter(1,"MNT_CONTROL_X",ui.controlXSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_CONTROL_Y",ui.controlYSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_CONTROL_Z",ui.controlZSpinBox->value());
}

void CameraGimbalConfig::updateTilt()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    if (!m_tiltPrefix.isEmpty())
    {
        //We need to set this to 0 for disabled.
        m_uas->getParamManager()->setParameter(1,m_tiltPrefix + "FUNCTION",0);
    }
    if (ui.tiltChannelComboBox->currentIndex() == 0)
    {
        //Disabled
        return;
    }

    m_uas->getParamManager()->setParameter(1,ui.tiltChannelComboBox->currentText() + "_FUNCTION",7);
    m_uas->getParamManager()->setParameter(1,ui.tiltChannelComboBox->currentText() + "_MIN",ui.tiltServoMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,ui.tiltChannelComboBox->currentText() + "_MAX",ui.tiltServoMaxSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMIN_TIL",ui.tiltAngleMinSpinBox->value() * 100);
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMAX_TIL",ui.tiltAngleMaxSpinBox->value() * 100);
    m_uas->getParamManager()->setParameter(1,ui.tiltChannelComboBox->currentText() + "_REV",(ui.tiltReverseCheckBox->isChecked() ? 1 : 0));
    if (ui.tiltInputChannelComboBox->currentIndex() == 0)
    {
        m_uas->getParamManager()->setParameter(1,"MNT_RC_IN_TILT",0);
    }
    else
    {
        m_uas->getParamManager()->setParameter(1,"MNT_RC_IN_TILT",ui.tiltInputChannelComboBox->currentIndex()+4);
    }
}

void CameraGimbalConfig::updateRoll()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    m_uas->getParamManager()->setParameter(1,ui.rollChannelComboBox->currentText() + "_FUNCTION",8);
    m_uas->getParamManager()->setParameter(1,ui.rollChannelComboBox->currentText() + "_MIN",ui.rollServoMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,ui.rollChannelComboBox->currentText() + "_MAX",ui.rollServoMaxSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMIN_ROL",ui.rollAngleMinSpinBox->value() * 100);
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMAX_ROL",ui.rollAngleMaxSpinBox->value() * 100);
    m_uas->getParamManager()->setParameter(1,ui.rollChannelComboBox->currentText() + "_REV",(ui.rollReverseCheckBox->isChecked() ? 1 : 0));
    if (ui.rollInputChannelComboBox->currentIndex() == 0)
    {
        m_uas->getParamManager()->setParameter(1,"MNT_RC_IN_ROLL",0);
    }
    else
    {
        m_uas->getParamManager()->setParameter(1,"MNT_RC_IN_ROLL",ui.rollInputChannelComboBox->currentIndex()+4);
    }
}

void CameraGimbalConfig::updatePan()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    m_uas->getParamManager()->setParameter(1,ui.panChannelComboBox->currentText() + "_FUNCTION",6);
    m_uas->getParamManager()->setParameter(1,ui.panChannelComboBox->currentText() + "_MIN",ui.panServoMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,ui.panChannelComboBox->currentText() + "_MAX",ui.panServoMaxSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMIN_PAN",ui.panAngleMinSpinBox->value() * 100);
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMAX_PAN",ui.panAngleMaxSpinBox->value() * 100);
    m_uas->getParamManager()->setParameter(1,ui.panChannelComboBox->currentText() + "_REV",(ui.panReverseCheckBox->isChecked() ? 1 : 0));
    if (ui.panInputChannelComboBox->currentIndex() == 0)
    {
        m_uas->getParamManager()->setParameter(1,"MNT_RC_IN_PAN",0);
    }
    else
    {
        m_uas->getParamManager()->setParameter(1,"MNT_RC_IN_PAN",ui.panInputChannelComboBox->currentIndex()+4);
    }
}

void CameraGimbalConfig::updateShutter()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    if (ui.shutterChannelComboBox->currentIndex() == 0) //Disabled
    {
        m_uas->getParamManager()->setParameter(1,"CAM_TRIGG_TYPE",0);
    }
    else if (ui.shutterChannelComboBox->currentIndex() == 1) //Relay
    {
        m_uas->getParamManager()->setParameter(1,"CAM_TRIGG_TYPE",1);
    }
    else if (ui.shutterChannelComboBox->currentIndex() == 2) //Transistor
    {
        m_uas->getParamManager()->setParameter(1,"CAM_TRIGG_TYPE",4);
    }
    else
    {
        m_uas->getParamManager()->setParameter(1,ui.shutterChannelComboBox->currentText() + "_FUNCTION",10);
        m_uas->getParamManager()->setParameter(1,"CAM_TRIGG_TYPE",0);
    }
    m_uas->getParamManager()->setParameter(1,ui.shutterChannelComboBox->currentText() + "_MIN",ui.shutterServoMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,ui.shutterChannelComboBox->currentText() + "_MAX",ui.shutterServoMaxSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"CAM_SERVO_ON",ui.shutterPushedSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"CAM_SERVO_OFF",ui.shutterNotPushedSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"CAM_DURATION",ui.shutterDurationSpinBox->value());


}


CameraGimbalConfig::~CameraGimbalConfig()
{
}

void CameraGimbalConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (parameterName == "MNT_ANGMIN_TIL") //TILT
    {
        ui.tiltAngleMinSpinBox->setValue(value.toInt() / 100.0);
    }
    else if (parameterName == "MNT_ANGMAX_TIL")
    {
        ui.tiltAngleMaxSpinBox->setValue(value.toInt() / 100.0);
    }
    else if (parameterName == "MNT_RC_IN_TILT")
    {
        disconnect(ui.tiltInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
        if (value.toInt() == 0)
        {
            ui.tiltInputChannelComboBox->setCurrentIndex(0);
        }
        else
        {
            ui.tiltInputChannelComboBox->setCurrentIndex(value.toInt()-4);
        }
        connect(ui.tiltInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
    }
    else if (parameterName == "MNT_ANGMIN_ROL") //ROLL
    {
        ui.rollAngleMinSpinBox->setValue(value.toInt() / 100.0);
    }
    else if (parameterName == "MNT_ANGMAX_ROL")
    {
        ui.rollAngleMaxSpinBox->setValue(value.toInt() / 100.0);
    }
    else if (parameterName == "MNT_RC_IN_ROLL")
    {
        disconnect(ui.rollInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
        if (value.toInt() == 0)
        {
            ui.rollInputChannelComboBox->setCurrentIndex(0);
        }
        else
        {
            ui.rollInputChannelComboBox->setCurrentIndex(value.toInt()-4);
        }
        connect(ui.rollInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
    }
    else if (parameterName == "MNT_ANGMIN_PAN") //PAN
    {
        ui.panAngleMinSpinBox->setValue(value.toInt() / 100.0);
    }
    else if (parameterName == "MNT_ANGMAX_PAN")
    {
        ui.panAngleMaxSpinBox->setValue(value.toInt() / 100.0);
    }
    else if (parameterName == "MNT_RC_IN_PAN")
    {
        disconnect(ui.panInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
        if (value.toInt() == 0)
        {
            ui.panInputChannelComboBox->setCurrentIndex(0);
        }
        else
        {
            ui.panInputChannelComboBox->setCurrentIndex(value.toInt()-4);
        }
        connect(ui.panInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
    }
    if (parameterName == "CAM_DURATION")
    {
        ui.shutterDurationSpinBox->setValue(value.toInt());
    }
    else if (parameterName == "CAM_SERVO_OFF")
    {
        ui.shutterNotPushedSpinBox->setValue(value.toInt());
    }
    else if (parameterName == "CAM_SERVO_ON")
    {
        ui.shutterPushedSpinBox->setValue(value.toInt());
    }
    else if (parameterName == "CAM_TRIGG_TYPE")
    {
        disconnect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
        if (value.toInt() == 0) //Disabled
        {
            ui.shutterChannelComboBox->setCurrentIndex(0);
            ///TODO: Request all _FUNCTIONs here to find out if shutter is actually disabled.
        }
        else if (value.toInt() == 1) // Relay
        {
            ui.shutterChannelComboBox->setCurrentIndex(1);
        }
        else if (value.toInt() == 4) //Transistor
        {
            ui.shutterChannelComboBox->setCurrentIndex(2);
        }
        connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
    }
    if (parameterName.startsWith(m_shutterPrefix) && !m_shutterPrefix.isEmpty())
    {
        if (parameterName.endsWith("MIN"))
        {
            ui.shutterServoMinSpinBox->setValue(value.toInt());
        }
        else if (parameterName.endsWith("MAX"))
        {
            ui.shutterServoMaxSpinBox->setValue(value.toInt());
        }
    }
    else if (parameterName.startsWith(m_tiltPrefix) && !m_tiltPrefix.isEmpty())
    {
        if (parameterName.endsWith("MIN"))
        {
            ui.tiltServoMinSpinBox->setValue(value.toInt());
        }
        else if (parameterName.endsWith("MAX"))
        {
            ui.tiltServoMaxSpinBox->setValue(value.toInt());
        }
        else if (parameterName.endsWith("REV"))
        {
            if (value.toInt() == 0)
            {
                ui.tiltReverseCheckBox->setChecked(false);
            }
            else
            {
                ui.tiltReverseCheckBox->setChecked(true);
            }
        }
    }
    else if (parameterName.startsWith(m_rollPrefix) && !m_rollPrefix.isEmpty())
    {
        if (parameterName.endsWith("MIN"))
        {
            ui.rollServoMinSpinBox->setValue(value.toInt());
        }
        else if (parameterName.endsWith("MAX"))
        {
            ui.rollServoMaxSpinBox->setValue(value.toInt());
        }
        else if (parameterName.endsWith("REV"))
        {
            if (value.toInt() == 0)
            {
                ui.rollReverseCheckBox->setChecked(false);
            }
            else
            {
                ui.rollReverseCheckBox->setChecked(true);
            }
        }
    }
    else if (parameterName.startsWith(m_panPrefix) && !m_panPrefix.isEmpty())
    {
        if (parameterName.endsWith("MIN"))
        {
            ui.panServoMinSpinBox->setValue(value.toInt());
        }
        else if (parameterName.endsWith("MAX"))
        {
            ui.panServoMaxSpinBox->setValue(value.toInt());
        }
        else if (parameterName.endsWith("REV"))
        {
            if (value.toInt() == 0)
            {
                ui.panReverseCheckBox->setChecked(false);
            }
            else
            {
                ui.panReverseCheckBox->setChecked(true);
            }
        }
    }
    else if (parameterName == "RC5_FUNCTION")
    {
        if (value.toInt() == 10)
        {
            //RC5 is shutter.
            disconnect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            ui.shutterChannelComboBox->setCurrentIndex(3);
            connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            m_shutterPrefix = "RC5_";
        }
        else if (value.toInt() == 8)
        {
            //RC5 is roll
            disconnect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            ui.rollChannelComboBox->setCurrentIndex(1);
            connect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            m_rollPrefix = "RC5_";
        }
        else if (value.toInt() == 7)
        {
            //RC5 is tilt
            disconnect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            ui.tiltChannelComboBox->setCurrentIndex(1);
            connect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            m_tiltPrefix = "RC5_";
        }
        else if (value.toInt() == 6)
        {
            //RC5 is pan
            disconnect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            ui.panChannelComboBox->setCurrentIndex(1);
            connect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            m_panPrefix = "RC5_";
        }
        m_uas->getParamManager()->requestParameterUpdate(1,"RC5_MIN");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC5_MAX");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC5_REV");
    }
    else if (parameterName == "RC6_FUNCTION")
    {
        if (value.toInt() == 10)
        {
            //RC6 is shutter.
            disconnect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            ui.shutterChannelComboBox->setCurrentIndex(4);
            connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            m_shutterPrefix = "RC6_";
        }
        else if (value.toInt() == 8)
        {
            //RC6 is roll
            disconnect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            ui.rollChannelComboBox->setCurrentIndex(2);
            connect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            m_rollPrefix = "RC6_";
        }
        else if (value.toInt() == 7)
        {
            //RC6 is tilt
            disconnect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            ui.tiltChannelComboBox->setCurrentIndex(2);
            connect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            m_tiltPrefix = "RC6_";
        }
        else if (value.toInt() == 6)
        {
            //RC6 is pan
            disconnect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            ui.panChannelComboBox->setCurrentIndex(2);
            connect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            m_panPrefix = "RC6_";
        }
        m_uas->getParamManager()->requestParameterUpdate(1,"RC6_MIN");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC6_MAX");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC6_REV");
    }
    else if (parameterName == "RC7_FUNCTION")
    {
        if (value.toInt() == 10)
        {
            //RC7 is shutter.
            disconnect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            ui.shutterChannelComboBox->setCurrentIndex(5);
            connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            m_shutterPrefix = "RC7_";
        }
        else if (value.toInt() == 8)
        {
            //RC7 is roll
            disconnect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            ui.rollChannelComboBox->setCurrentIndex(3);
            connect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            m_rollPrefix = "RC7_";
        }
        else if (value.toInt() == 7)
        {
            //RC7 is tilt
            disconnect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            ui.tiltChannelComboBox->setCurrentIndex(3);
            connect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            m_tiltPrefix = "RC7_";
        }
        else if (value.toInt() == 6)
        {
            //RC7 is pan
            disconnect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            ui.panChannelComboBox->setCurrentIndex(3);
            connect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            m_panPrefix = "RC7_";
        }
        m_uas->getParamManager()->requestParameterUpdate(1,"RC7_MIN");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC7_MAX");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC7_REV");
    }
    else if (parameterName == "RC8_FUNCTION")
    {
        if (value.toInt() == 10)
        {
            //RC8 is shutter.
            disconnect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            ui.shutterChannelComboBox->setCurrentIndex(6);
            connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            m_shutterPrefix = "RC8_";
        }
        else if (value.toInt() == 8)
        {
            //RC8 is roll
            disconnect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            ui.rollChannelComboBox->setCurrentIndex(4);
            connect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            m_rollPrefix = "RC8_";
        }
        else if (value.toInt() == 7)
        {
            //RC8 is tilt
            disconnect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            ui.tiltChannelComboBox->setCurrentIndex(4);
            connect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            m_tiltPrefix = "RC8_";
        }
        else if (value.toInt() == 6)
        {
            //RC8 is pan
            disconnect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            ui.panChannelComboBox->setCurrentIndex(4);
            connect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            m_panPrefix = "RC8_";
        }
        m_uas->getParamManager()->requestParameterUpdate(1,"RC8_MIN");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC8_MAX");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC8_REV");
    }
    else if (parameterName == "RC10_FUNCTION")
    {
        if (value.toInt() == 10)
        {
            //RC10 is shutter.
            disconnect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            ui.shutterChannelComboBox->setCurrentIndex(7);
            connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            m_shutterPrefix = "RC10_";
        }
        else if (value.toInt() == 8)
        {
            //RC10 is roll
            disconnect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            ui.rollChannelComboBox->setCurrentIndex(5);
            connect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            m_rollPrefix = "RC10_";
        }
        else if (value.toInt() == 7)
        {
            //RC10 is tilt
            disconnect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            ui.tiltChannelComboBox->setCurrentIndex(5);
            connect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            m_tiltPrefix = "RC10_";
        }
        else if (value.toInt() == 6)
        {
            //RC10 is pan
            disconnect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            ui.panChannelComboBox->setCurrentIndex(5);
            connect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            m_panPrefix = "RC10_";
        }
        m_uas->getParamManager()->requestParameterUpdate(1,"RC10_MIN");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC10_MAX");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC10_REV");
    }
    else if (parameterName == "RC11_FUNCTION")
    {
        if (value.toInt() == 10)
        {
            //RC11 is shutter.
            disconnect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            ui.shutterChannelComboBox->setCurrentIndex(8);
            connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));
            m_shutterPrefix = "RC11_";
        }
        else if (value.toInt() == 8)
        {
            //RC11 is roll
            disconnect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            ui.rollChannelComboBox->setCurrentIndex(6);
            connect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateRoll()));
            m_rollPrefix = "RC11_";
        }
        else if (value.toInt() == 7)
        {
            //RC11 is tilt
            disconnect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            ui.tiltChannelComboBox->setCurrentIndex(6);
            connect(ui.tiltChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
            m_tiltPrefix = "RC11_";
        }
        else if (value.toInt() == 6)
        {
            //RC11 is pan
            disconnect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            ui.panChannelComboBox->setCurrentIndex(6);
            connect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updatePan()));
            m_panPrefix = "RC11_";
        }
        m_uas->getParamManager()->requestParameterUpdate(1,"RC11_MIN");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC11_MAX");
        m_uas->getParamManager()->requestParameterUpdate(1,"RC11_REV");
    }
}
