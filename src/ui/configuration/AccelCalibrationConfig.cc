#include "AccelCalibrationConfig.h"


AccelCalibrationConfig::AccelCalibrationConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    m_uas=0;
    ui.setupUi(this);
    connect(ui.calibrateAccelButton,SIGNAL(clicked()),this,SLOT(calibrateButtonClicked()));

    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(activeUASSet(UASInterface*)));
    activeUASSet(UASManager::instance()->getActiveUAS());
    m_accelAckCount=0;
}

AccelCalibrationConfig::~AccelCalibrationConfig()
{
}
void AccelCalibrationConfig::activeUASSet(UASInterface *uas)
{
    if (m_uas)
    {
        disconnect(m_uas,SIGNAL(textMessageReceived(int,int,int,QString)),this,SLOT(uasTextMessageReceived(int,int,int,QString)));
    }
    AP2ConfigWidget::activeUASSet(uas);
    if (!uas)
    {
        return;
    }
    connect(m_uas,SIGNAL(textMessageReceived(int,int,int,QString)),this,SLOT(uasTextMessageReceived(int,int,int,QString)));

}

void AccelCalibrationConfig::calibrateButtonClicked()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    if (m_accelAckCount == 0)
    {
        MAV_CMD command = MAV_CMD_PREFLIGHT_CALIBRATION;
        int confirm = 0;
        float param1 = 0.0;
        float param2 = 0.0;
        float param3 = 0.0;
        float param4 = 0.0;
        float param5 = 1.0;
        float param6 = 0.0;
        float param7 = 0.0;
        int component = 1;
        m_uas->executeCommand(command, confirm, param1, param2, param3, param4, param5, param6, param7, component);
    }
    else if (m_accelAckCount <= 5)
    {
        m_uas->executeCommandAck(m_accelAckCount++,true);
    }
    else
    {
        m_uas->executeCommandAck(m_accelAckCount++,true);
        ui.calibrateAccelButton->setText("Calibrate\nAccelerometer");
    }

}
void AccelCalibrationConfig::uasTextMessageReceived(int uasid, int componentid, int severity, QString text)
{
    //command received: " Severity 1
    //Place APM Level and press any key" severity 5
    if (severity == 5)
    {
        //This is a calibration instruction
        if (m_accelAckCount == 0)
        {
            //Calibration Sucessful\r"
            ui.calibrateAccelButton->setText("Any\nKey");
            m_accelAckCount++;
        }
        if (m_accelAckCount == 7)
        {
            //All finished
            //ui.outputLabel->setText(ui.outputLabel->text() + "\n" + text);
            ui.outputLabel->setText(text);
            m_accelAckCount++;
        }
        if (m_accelAckCount == 8)
        {
            if (text.contains("Calibration") && text.contains("successful"))
            {
                m_accelAckCount = 0;
            }
            ui.outputLabel->setText(ui.outputLabel->text() + "\n" + text);
        }
        else
        {
            ui.outputLabel->setText(text);
        }
    }

}
