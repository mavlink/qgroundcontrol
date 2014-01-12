#include "AccelCalibrationConfig.h"


AccelCalibrationConfig::AccelCalibrationConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    connect(ui.calibrateAccelButton,SIGNAL(clicked()),this,SLOT(calibrateButtonClicked()));

    m_accelAckCount=0;
    initConnections();
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
        if (m_accelAckCount > 8)
        {
            //We've clicked too many times! Reset.
            for (int i=0;i<8;i++)
            {
                m_uas->executeCommandAck(i,true);
            }
            m_accelAckCount = 0;
        }
    }

}
void AccelCalibrationConfig::hideEvent(QHideEvent *evt)
{
    Q_UNUSED(evt);
    
    if (!m_uas || !m_accelAckCount)
    {
        return;
    }
    for (int i=m_accelAckCount;i<8;i++)
    {
        m_uas->executeCommandAck(i,true); //Clear out extra commands.
    }
}
void AccelCalibrationConfig::uasTextMessageReceived(int uasid, int componentid, int severity, QString text)
{
    Q_UNUSED(uasid);
    Q_UNUSED(componentid);
    
    //command received: " Severity 1
    //Place APM Level and press any key" severity 5
    if (severity == 5)
    {
        //This is a calibration instruction
        if (m_accelAckCount == 0)
        {
            //Calibration Sucessful\r"
            ui.calibrateAccelButton->setText("Continue");
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
            else if (text.contains("Calibration") && text.contains("FAILED")) //Failure
            {
                m_accelAckCount = 0;
            }
            ui.outputLabel->setText(ui.outputLabel->text() + "\n" + text);
        }
        else
        {
            ui.outputLabel->setText(text.replace("press any key","click Continue below"));
            if (!this->isVisible())
            {
                //Clear out!
                m_uas->executeCommandAck(m_accelAckCount++,true);
                ui.calibrateAccelButton->setText("Calibrate\nAccelerometer");
            }
        }
    }

}
