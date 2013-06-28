#include "CameraGimbalConfig.h"
#include <QMessageBox>
#include <QDebug>
CameraGimbalConfig::CameraGimbalConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    ui.tiltChannelComboBox->addItem("Disable");
    ui.tiltChannelComboBox->addItem("RC5");
    ui.tiltChannelComboBox->addItem("RC6");
    ui.tiltChannelComboBox->addItem("RC7");
    ui.tiltChannelComboBox->addItem("RC8");
    ui.tiltChannelComboBox->addItem("RC10");
    ui.tiltChannelComboBox->addItem("RC11");

    ui.tiltInputChannelComboBox->addItem("Disable");
    ui.tiltInputChannelComboBox->addItem("RC5");
    ui.tiltInputChannelComboBox->addItem("RC6");
    ui.tiltInputChannelComboBox->addItem("RC7");
    ui.tiltInputChannelComboBox->addItem("RC8");

    ui.rollChannelComboBox->addItem("Disable");
    ui.rollChannelComboBox->addItem("RC5");
    ui.rollChannelComboBox->addItem("RC6");
    ui.rollChannelComboBox->addItem("RC7");
    ui.rollChannelComboBox->addItem("RC8");
    ui.rollChannelComboBox->addItem("RC10");
    ui.rollChannelComboBox->addItem("RC11");

    ui.rollInputChannelComboBox->addItem("Disable");
    ui.rollInputChannelComboBox->addItem("RC5");
    ui.rollInputChannelComboBox->addItem("RC6");
    ui.rollInputChannelComboBox->addItem("RC7");
    ui.rollInputChannelComboBox->addItem("RC8");


    ui.panChannelComboBox->addItem("Disable");
    ui.panChannelComboBox->addItem("RC5");
    ui.panChannelComboBox->addItem("RC6");
    ui.panChannelComboBox->addItem("RC7");
    ui.panChannelComboBox->addItem("RC8");
    ui.panChannelComboBox->addItem("RC10");
    ui.panChannelComboBox->addItem("RC11");

    ui.panInputChannelComboBox->addItem("Disable");
    ui.panInputChannelComboBox->addItem("RC5");
    ui.panInputChannelComboBox->addItem("RC6");
    ui.panInputChannelComboBox->addItem("RC7");
    ui.panInputChannelComboBox->addItem("RC8");


    ui.shutterChannelComboBox->addItem("Disable");
    ui.shutterChannelComboBox->addItem("Relay");
    ui.shutterChannelComboBox->addItem("Transistor");
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

    connect(ui.rollServoMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.rollServoMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.rollAngleMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.rollAngleMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.rollChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
    connect(ui.rollInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
    connect(ui.rollReverseCheckBox,SIGNAL(clicked(bool)),this,SLOT(updateTilt()));
    connect(ui.rollStabilizeCheckBox,SIGNAL(clicked(bool)),this,SLOT(updateTilt()));

    connect(ui.panServoMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.panServoMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.panAngleMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.panAngleMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateTilt()));
    connect(ui.panChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
    connect(ui.panInputChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTilt()));
    connect(ui.panReverseCheckBox,SIGNAL(clicked(bool)),this,SLOT(updateTilt()));
    connect(ui.panStabilizeCheckBox,SIGNAL(clicked(bool)),this,SLOT(updateTilt()));


    connect(ui.shutterServoMinSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterServoMaxSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterPushedSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterNotPushedSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterDurationSpinBox,SIGNAL(editingFinished()),this,SLOT(updateShutter()));
    connect(ui.shutterChannelComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateShutter()));


}

void CameraGimbalConfig::updateTilt()
{
    if (!m_uas)
    {
        QMessageBox::information(0,tr("Error"),tr("Please connect to a MAV before attempting to set configuration"));
        return;
    }
    if (ui.tiltChannelComboBox->currentIndex() == 0)
    {
        //Disabled
        return;
    }
    for (QMap<int,QString>::const_iterator i = m_uas->getComponents().constBegin(); i != m_uas->getComponents().constEnd();i++)
    {
        qDebug() << "Component:" << i.key() << "Name:" << i.value();
    }
    m_uas->getParamManager()->setParameter(1,ui.tiltChannelComboBox->currentText() + "_FUNCTION",7);
    m_uas->getParamManager()->setParameter(1,ui.tiltChannelComboBox->currentText() + "_MIN",ui.tiltServoMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,ui.tiltChannelComboBox->currentText() + "_MAX",ui.tiltServoMaxSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMIN_TIL",ui.tiltAngleMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMAX_TIL",ui.tiltAngleMaxSpinBox->value());
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
        QMessageBox::information(0,tr("Error"),tr("Please connect to a MAV before attempting to set configuration"));
        return;
    }
    m_uas->getParamManager()->setParameter(1,ui.rollChannelComboBox->currentText() + "_FUNCTION",8);
    m_uas->getParamManager()->setParameter(1,ui.rollChannelComboBox->currentText() + "_MIN",ui.rollServoMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,ui.rollChannelComboBox->currentText() + "_MAX",ui.rollServoMaxSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMIN_ROL",ui.rollAngleMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMAX_ROL",ui.rollAngleMaxSpinBox->value());
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
        QMessageBox::information(0,tr("Error"),tr("Please connect to a MAV before attempting to set configuration"));
        return;
    }
    m_uas->getParamManager()->setParameter(1,ui.panChannelComboBox->currentText() + "_FUNCTION",6);
    m_uas->getParamManager()->setParameter(1,ui.panChannelComboBox->currentText() + "_MIN",ui.panServoMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,ui.panChannelComboBox->currentText() + "_MAX",ui.panServoMaxSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMIN_PAN",ui.panAngleMinSpinBox->value());
    m_uas->getParamManager()->setParameter(1,"MNT_ANGMAX_PAN",ui.panAngleMaxSpinBox->value());
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
        QMessageBox::information(0,tr("Error"),tr("Please connect to a MAV before attempting to set configuration"));
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

}
