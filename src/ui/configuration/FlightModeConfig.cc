#include "FlightModeConfig.h"


FlightModeConfig::FlightModeConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    connect(ui.savePushButton,SIGNAL(clicked()),this,SLOT(saveButtonClicked()));
    initConnections();
}

FlightModeConfig::~FlightModeConfig()
{
}
void FlightModeConfig::activeUASSet(UASInterface *uas)
{
    if (m_uas)
    {
        disconnect(m_uas,SIGNAL(modeChanged(int,QString,QString)),this,SLOT(modeChanged(int,QString,QString)));
        disconnect(m_uas,SIGNAL(remoteControlChannelRawChanged(int,float)),this,SLOT(remoteControlChannelRawChanged(int,float)));
    }
    AP2ConfigWidget::activeUASSet(uas);
    if (!uas) return;
    connect(m_uas,SIGNAL(modeChanged(int,QString,QString)),this,SLOT(modeChanged(int,QString,QString)));
    connect(m_uas,SIGNAL(remoteControlChannelRawChanged(int,float)),this,SLOT(remoteControlChannelRawChanged(int,float)));
    QStringList itemlist;
    if (m_uas->getSystemType() == MAV_TYPE_FIXED_WING)
    {
        itemlist << "Manual";
        itemlist << "Circle";
        itemlist << "Stabilize";
        itemlist << "Training";
        itemlist << "FBW A";
        itemlist << "FBW B";
        itemlist << "Auto";
        itemlist << "RTL";
        itemlist << "Loiter";
        itemlist << "Guided";

        planeModeIndexToUiIndex[0] = 0;
        planeModeUiIndexToIndex[0] = 0;

        planeModeIndexToUiIndex[1] = 1;
        planeModeUiIndexToIndex[1] = 1;

        planeModeIndexToUiIndex[2] = 2;
        planeModeUiIndexToIndex[2] = 2;

        planeModeIndexToUiIndex[3] = 3;
        planeModeUiIndexToIndex[3] = 3;

        planeModeIndexToUiIndex[5] = 4;
        planeModeUiIndexToIndex[4] = 5;

        planeModeIndexToUiIndex[6] = 5;
        planeModeUiIndexToIndex[5] = 6;

        planeModeIndexToUiIndex[10] = 6;
        planeModeUiIndexToIndex[6] = 10;

        planeModeIndexToUiIndex[11] = 7;
        planeModeUiIndexToIndex[7] = 11;

        planeModeIndexToUiIndex[12] = 8;
        planeModeUiIndexToIndex[8] = 12;

        planeModeIndexToUiIndex[15] = 9;
        planeModeUiIndexToIndex[9] = 15;

        ui.mode6ComboBox->setEnabled(true);
    }
    else if (m_uas->getSystemType() == MAV_TYPE_GROUND_ROVER)
    {
        itemlist << "Manual";
        itemlist << "Learning";
        itemlist << "Steering";
        itemlist << "Hold";
        itemlist << "Auto";
        itemlist << "RTL";
        itemlist << "Guided";
        itemlist << "Initialising";
        ui.mode6ComboBox->setEnabled(false);
        roverModeIndexToUiIndex[0] = 0;
        roverModeUiIndexToIndex[0] = 0;

        roverModeIndexToUiIndex[2] = 1;
        roverModeUiIndexToIndex[1] = 2;

        roverModeIndexToUiIndex[3] = 2;
        roverModeUiIndexToIndex[2] = 3;

        roverModeIndexToUiIndex[4] = 3;
        roverModeUiIndexToIndex[3] = 4;

        roverModeIndexToUiIndex[10] = 5;
        roverModeUiIndexToIndex[5] = 10;

        roverModeIndexToUiIndex[11] = 6;
        roverModeUiIndexToIndex[6] = 11;

        roverModeIndexToUiIndex[15] = 7;
        roverModeUiIndexToIndex[7] = 15;

        roverModeIndexToUiIndex[16] = 8;
        roverModeUiIndexToIndex[8] = 16;


    }
    else if (m_uas->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        itemlist << "Stabilize";
        itemlist << "Acro";
        itemlist << "Alt Hold";
        itemlist << "Auto";
        itemlist << "Guided";
        itemlist << "Loiter";
        itemlist << "RTL";
        itemlist << "Circle";
        itemlist << "Pos Hold";
        itemlist << "Land";
        itemlist << "OF_LOITER";
        itemlist << "Toy";
        ui.mode6ComboBox->setEnabled(true);
    }
    ui.mode1ComboBox->addItems(itemlist);
    ui.mode2ComboBox->addItems(itemlist);
    ui.mode3ComboBox->addItems(itemlist);
    ui.mode4ComboBox->addItems(itemlist);
    ui.mode5ComboBox->addItems(itemlist);
    ui.mode6ComboBox->addItems(itemlist);
}
void FlightModeConfig::modeChanged(int sysId, QString status, QString description)
{
    //Unused?
}
void FlightModeConfig::saveButtonClicked()
{
    if (m_uas->getSystemType() == MAV_TYPE_FIXED_WING)
    {
        m_uas->getParamManager()->setParameter(1,"FLTMODE1",(char)planeModeUiIndexToIndex[ui.mode1ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"FLTMODE2",(char)planeModeUiIndexToIndex[ui.mode2ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"FLTMODE3",(char)planeModeUiIndexToIndex[ui.mode3ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"FLTMODE4",(char)planeModeUiIndexToIndex[ui.mode4ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"FLTMODE5",(char)planeModeUiIndexToIndex[ui.mode5ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"FLTMODE6",(char)planeModeUiIndexToIndex[ui.mode6ComboBox->currentIndex()]);
    }
    else if (m_uas->getSystemType() == MAV_TYPE_GROUND_ROVER)
    {
        m_uas->getParamManager()->setParameter(1,"MODE1",(char)roverModeUiIndexToIndex[ui.mode1ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"MODE2",(char)roverModeUiIndexToIndex[ui.mode2ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"MODE3",(char)roverModeUiIndexToIndex[ui.mode3ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"MODE4",(char)roverModeUiIndexToIndex[ui.mode4ComboBox->currentIndex()]);
        m_uas->getParamManager()->setParameter(1,"MODE5",(char)roverModeUiIndexToIndex[ui.mode5ComboBox->currentIndex()]);
    }
    else if (m_uas->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        m_uas->getParamManager()->setParameter(1,"FLTMODE1",(char)ui.mode1ComboBox->currentIndex());
        m_uas->getParamManager()->setParameter(1,"FLTMODE2",(char)ui.mode2ComboBox->currentIndex());
        m_uas->getParamManager()->setParameter(1,"FLTMODE3",(char)ui.mode3ComboBox->currentIndex());
        m_uas->getParamManager()->setParameter(1,"FLTMODE4",(char)ui.mode4ComboBox->currentIndex());
        m_uas->getParamManager()->setParameter(1,"FLTMODE5",(char)ui.mode5ComboBox->currentIndex());
        m_uas->getParamManager()->setParameter(1,"FLTMODE6",(char)ui.mode6ComboBox->currentIndex());
    }
}

void FlightModeConfig::remoteControlChannelRawChanged(int chan,float val)
{
    if (chan == 4)
    {
        //Channel 5 (0 array) is the mode switch.
        ///TODO: Make this configurable
        if (val <= 1230)
        {
            ui.mode1Label->setStyleSheet("background-color: rgb(0, 255, 0);color: rgb(0, 0, 0);");
            ui.mode2Label->setStyleSheet("");
            ui.mode3Label->setStyleSheet("");
            ui.mode4Label->setStyleSheet("");
            ui.mode5Label->setStyleSheet("");
            ui.mode6Label->setStyleSheet("");
        }
        else if (val <= 1360)
        {
            ui.mode1Label->setStyleSheet("");
            ui.mode2Label->setStyleSheet("background-color: rgb(0, 255, 0);color: rgb(0, 0, 0);");
            ui.mode3Label->setStyleSheet("");
            ui.mode4Label->setStyleSheet("");
            ui.mode5Label->setStyleSheet("");
            ui.mode6Label->setStyleSheet("");
        }
        else if (val <= 1490)
        {
            ui.mode1Label->setStyleSheet("");
            ui.mode2Label->setStyleSheet("");
            ui.mode3Label->setStyleSheet("background-color: rgb(0, 255, 0);color: rgb(0, 0, 0);");
            ui.mode4Label->setStyleSheet("");
            ui.mode5Label->setStyleSheet("");
            ui.mode6Label->setStyleSheet("");
        }
        else if (val <=1620)
        {
            ui.mode1Label->setStyleSheet("");
            ui.mode2Label->setStyleSheet("");
            ui.mode3Label->setStyleSheet("");
            ui.mode4Label->setStyleSheet("background-color: rgb(0, 255, 0);color: rgb(0, 0, 0);");
            ui.mode5Label->setStyleSheet("");
            ui.mode6Label->setStyleSheet("");
        }
        else if (val <=1749)
        {
            ui.mode1Label->setStyleSheet("");
            ui.mode2Label->setStyleSheet("");
            ui.mode3Label->setStyleSheet("");
            ui.mode4Label->setStyleSheet("");
            ui.mode5Label->setStyleSheet("background-color: rgb(0, 255, 0);color: rgb(0, 0, 0);");
            ui.mode6Label->setStyleSheet("");
        }
        else
        {
            ui.mode1Label->setStyleSheet("");
            ui.mode2Label->setStyleSheet("");
            ui.mode3Label->setStyleSheet("");
            ui.mode4Label->setStyleSheet("");
            ui.mode5Label->setStyleSheet("");
            ui.mode6Label->setStyleSheet("background-color: rgb(0, 255, 0);color: rgb(0, 0, 0);");
        }
    }
}

void FlightModeConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (m_uas->getSystemType() == MAV_TYPE_FIXED_WING)
    {
        if (parameterName == "FLTMODE1")
        {
            ui.mode1ComboBox->setCurrentIndex(planeModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "FLTMODE2")
        {
            ui.mode2ComboBox->setCurrentIndex(planeModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "FLTMODE3")
        {
            ui.mode3ComboBox->setCurrentIndex(planeModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "FLTMODE4")
        {
            ui.mode4ComboBox->setCurrentIndex(planeModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "FLTMODE5")
        {
            ui.mode5ComboBox->setCurrentIndex(planeModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "FLTMODE6")
        {
            ui.mode6ComboBox->setCurrentIndex(planeModeIndexToUiIndex[value.toInt()]);
        }
    }
    else if (m_uas->getSystemType() == MAV_TYPE_GROUND_ROVER)
    {
        if (parameterName == "MODE1")
        {
            ui.mode1ComboBox->setCurrentIndex(roverModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "MODE2")
        {
            ui.mode2ComboBox->setCurrentIndex(roverModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "MODE3")
        {
            ui.mode3ComboBox->setCurrentIndex(roverModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "MODE4")
        {
            ui.mode4ComboBox->setCurrentIndex(roverModeIndexToUiIndex[value.toInt()]);
        }
        else if (parameterName == "MODE5")
        {
            ui.mode5ComboBox->setCurrentIndex(roverModeIndexToUiIndex[value.toInt()]);
        }
    }
    else if (m_uas->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        if (parameterName == "FLTMODE1")
        {
            ui.mode1ComboBox->setCurrentIndex(value.toInt());
        }
        else if (parameterName == "FLTMODE2")
        {
            ui.mode2ComboBox->setCurrentIndex(value.toInt());
        }
        else if (parameterName == "FLTMODE3")
        {
            ui.mode3ComboBox->setCurrentIndex(value.toInt());
        }
        else if (parameterName == "FLTMODE4")
        {
            ui.mode4ComboBox->setCurrentIndex(value.toInt());
        }
        else if (parameterName == "FLTMODE5")
        {
            ui.mode5ComboBox->setCurrentIndex(value.toInt());
        }
        else if (parameterName == "FLTMODE6")
        {
            ui.mode6ComboBox->setCurrentIndex(value.toInt());
        }
    }
}
