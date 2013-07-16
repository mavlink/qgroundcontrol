#include "AccelCalibrationConfig.h"


AccelCalibrationConfig::AccelCalibrationConfig(QWidget *parent) : QWidget(parent)
{
    m_uas=0;
    ui.setupUi(this);
    connect(ui.calibrateAccelButton,SIGNAL(clicked()),this,SLOT(calibrateButtonClicked()));

    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(activeUASSet(UASInterface*)));
    activeUASSet(UASManager::instance()->getActiveUAS());
    accelAckCount=0;
}

AccelCalibrationConfig::~AccelCalibrationConfig()
{
}
void AccelCalibrationConfig::activeUASSet(UASInterface *uas)
{
    if (!uas)
    {
        return;
    }
    if (m_uas)
    {
    }
    m_uas = uas;
    connect(m_uas,SIGNAL(textMessageReceived(int,int,int,QString)),this,SLOT(uasTextMessageReceived(int,int,int,QString)));
}

void AccelCalibrationConfig::calibrateButtonClicked()
{
    if (accelAckCount == 0)
    {
        MAV_CMD command = MAV_CMD_PREFLIGHT_CALIBRATION;
        int confirm = 0;
        float param1 = 0;
        float param2 = 0;
        float param3 = 0;
        float param4 = 0;
        float param5 = 1;
        float param6 = 0;
        float param7 = 0;
        int component = 1;
        m_uas->executeCommand(command, confirm, param1, param2, param3, param4, param5, param6, param7, component);
    }
    else if (accelAckCount <= 5)
    {
        m_uas->executeCommandAck(accelAckCount++,true);
    }
    else
    {
        m_uas->executeCommandAck(accelAckCount++,true);
        ui.calibrateAccelButton->setText("Calibrate\nAccelerometer");
        //accelAckCount = 0;
    }

}
void AccelCalibrationConfig::uasTextMessageReceived(int uasid, int componentid, int severity, QString text)
{
    //command received: " Severity 1
    //Place APM Level and press any key" severity 5
    if (severity == 5)
    {
        //This is a calibration instruction
        if (accelAckCount == 0)
        {
            //Calibration Sucessful\r"
            ui.calibrateAccelButton->setText("Any\nKey");
            accelAckCount++;
        }
        if (accelAckCount == 7)
        {
            //All finished
            //ui.outputLabel->setText(ui.outputLabel->text() + "\n" + text);
            ui.outputLabel->setText(text);
            accelAckCount++;
        }
        if (accelAckCount == 8)
        {
            if (text.contains("Calibration") && text.contains("successful"))
            {
                accelAckCount = 0;
            }
            ui.outputLabel->setText(ui.outputLabel->text() + "\n" + text);
        }
        else
        {
            ui.outputLabel->setText(text);
        }
    }

}
