#include "FlightModeConfig.h"


FlightModeConfig::FlightModeConfig(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(setActiveUAS(UASInterface*)));


}

FlightModeConfig::~FlightModeConfig()
{
}
void FlightModeConfig::setActiveUAS(UASInterface *uas)
{
    if (!uas) return;
    if (m_uas)
    {
    }
    m_uas = uas;
    connect(m_uas,SIGNAL(modeChanged(int,QString,QString)),this,SLOT(modeChanged(int,QString,QString)));
    connect(m_uas,SIGNAL(parameterChanged(int,int,QString,QVariant)),this,SLOT(parameterChanged(int,int,QString,QVariant)));
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
        planeModeIndexToUiIndex[1] = 1;
        planeModeIndexToUiIndex[2] = 2;
        planeModeIndexToUiIndex[3] = 3;
        planeModeIndexToUiIndex[5] = 4;
        planeModeIndexToUiIndex[6] = 5;
        planeModeIndexToUiIndex[10] = 6;
        planeModeIndexToUiIndex[11] = 7;
        planeModeIndexToUiIndex[12] = 8;
        planeModeIndexToUiIndex[15] = 9;
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
        roverModeIndexToUiIndex[2] = 1;
        roverModeIndexToUiIndex[3] = 2;
        roverModeIndexToUiIndex[4] = 3;
        roverModeIndexToUiIndex[10] = 5;
        roverModeIndexToUiIndex[11] = 6;
        roverModeIndexToUiIndex[15] = 7;
        roverModeIndexToUiIndex[16] = 8;

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
void FlightModeConfig::remoteControlChannelRawChanged(int chan,float val)
{
    if (chan == 5)
    {
        //Channel 5 is the mode switch.
        ///TODO: Make this configurable
        if (val <= 1230)
        {
            ui.mode1ComboBox->setBackgroundRole(QPalette::Highlight);
            ui.mode2ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode3ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode4ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode5ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode6ComboBox->setBackgroundRole(QPalette::Background);
        }
        else if (val <= 1360)
        {
            ui.mode1ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode2ComboBox->setBackgroundRole(QPalette::Highlight);
            ui.mode3ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode4ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode5ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode6ComboBox->setBackgroundRole(QPalette::Background);
        }
        else if (val <= 1490)
        {
            ui.mode1ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode2ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode3ComboBox->setBackgroundRole(QPalette::Highlight);
            ui.mode4ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode5ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode6ComboBox->setBackgroundRole(QPalette::Background);
        }
        else if (val <=1620)
        {
            ui.mode1ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode2ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode3ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode4ComboBox->setBackgroundRole(QPalette::Highlight);
            ui.mode5ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode6ComboBox->setBackgroundRole(QPalette::Background);
        }
        else if (val <=1749)
        {
            ui.mode1ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode2ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode3ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode4ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode5ComboBox->setBackgroundRole(QPalette::Highlight);
            ui.mode6ComboBox->setBackgroundRole(QPalette::Background);
        }
        else
        {
            ui.mode1ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode2ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode3ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode4ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode5ComboBox->setBackgroundRole(QPalette::Background);
            ui.mode6ComboBox->setBackgroundRole(QPalette::Highlight);
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
            ui.mode1ComboBox->setCurrentIndex(value.toInt()-1);
        }
        else if (parameterName == "FLTMODE2")
        {
            ui.mode2ComboBox->setCurrentIndex(value.toInt()-1);
        }
        else if (parameterName == "FLTMODE3")
        {
            ui.mode3ComboBox->setCurrentIndex(value.toInt()-1);
        }
        else if (parameterName == "FLTMODE4")
        {
            ui.mode4ComboBox->setCurrentIndex(value.toInt()-1);
        }
        else if (parameterName == "FLTMODE5")
        {
            ui.mode5ComboBox->setCurrentIndex(value.toInt()-1);
        }
        else if (parameterName == "FLTMODE6")
        {
            ui.mode6ComboBox->setCurrentIndex(value.toInt()-1);
        }
    }
}
