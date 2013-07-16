#include "ArduCopterPidConfig.h"

ArduCopterPidConfig::ArduCopterPidConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    nameToBoxMap["STB_RLL_P"] = ui.stabilPitchPSpinBox;
    nameToBoxMap["STB_PIT_P"] = ui.stabilRollPSpinBox;
    nameToBoxMap["STB_YAW_P"] = ui.stabilYawPSpinBox;
    nameToBoxMap["HLD_LAT_P"] = ui.loiterPidPSpinBox;

    nameToBoxMap["RATE_RLL_P"] = ui.rateRollPSpinBox;
    nameToBoxMap["RATE_RLL_I"] = ui.rateRollISpinBox;
    nameToBoxMap["RATE_RLL_D"] = ui.rateRollDSpinBox;
    nameToBoxMap["RATE_RLL_IMAX"] = ui.rateRollIMAXSpinBox;

    nameToBoxMap["RATE_PIT_P"] = ui.ratePitchPSpinBox;
    nameToBoxMap["RATE_PIT_I"] = ui.ratePitchISpinBox;
    nameToBoxMap["RATE_PIT_D"] = ui.ratePitchDSpinBox;
    nameToBoxMap["RATE_PIT_IMAX"] = ui.ratePitchIMAXSpinBox;

    nameToBoxMap["RATE_YAW_P"] = ui.rateYawPSpinBox;
    nameToBoxMap["RATE_YAW_I"] = ui.rateYawISpinBox;
    nameToBoxMap["RATE_YAW_D"] = ui.rateYawDSpinBox;
    nameToBoxMap["RATE_YAW_IMAX"] = ui.rateYawIMAXSpinBox;

    nameToBoxMap["LOITER_LAT_P"] = ui.rateLoiterPSpinBox;
    nameToBoxMap["LOITER_LAT_I"] = ui.rateLoiterISpinBox;
    nameToBoxMap["LOITER_LAT_D"] = ui.rateLoiterDSpinBox;
    nameToBoxMap["LOITER_LAT_IMAX"] = ui.rateLoiterIMAXSpinBox;

    nameToBoxMap["THR_ACCEL_P"] = ui.throttleAccelPSpinBox;
    nameToBoxMap["THR_ACCEL_I"] = ui.throttleAccelISpinBox;
    nameToBoxMap["THR_ACCEL_D"] = ui.throttleAccelDSpinBox;
    nameToBoxMap["THR_ACCEL_IMAX"] = ui.throttleAccelIMAXSpinBox;

    nameToBoxMap["THR_RATE_P"] = ui.throttleRatePSpinBox;
    nameToBoxMap["THR_RATE_D"] = ui.throttleDateDSpinBox;

    nameToBoxMap["THR_ALT_P"] = ui.altitudeHoldPSpinBox;

    nameToBoxMap["WPNAV_SPEED"] = ui.wpNavLoiterSpeedSpinBox;
    nameToBoxMap["WPNAV_RADIUS"] = ui.wpNavRadiusSpinBox;
    nameToBoxMap["WPNAV_SPEED_DN"] = ui.wpNavSpeedDownSpinBox;
    nameToBoxMap["WPNAV_LOIT_SPEED"] = ui.wpNavSpeedSpinBox;
    nameToBoxMap["WPNAV_SPEED_UP"] = ui.wpNavSpeedUpSpinBox;

    nameToBoxMap["TUNE_HIGH"] = ui.ch6MaxSpinBox;
    nameToBoxMap["TUNE_LOW"] = ui.ch6MinSpinBox;

    connect(ui.writePushButton,SIGNAL(clicked()),this,SLOT(writeButtonClicked()));
    connect(ui.refreshPushButton,SIGNAL(clicked()),this,SLOT(refreshButtonClicked()));



    ch6ValueToTextList.append(QPair<int,QString>(0,"CH6_NONE"));
    ch6ValueToTextList.append(QPair<int,QString>(1,"CH6_STABILIZE_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(2,"CH6_STABILIZE_KI"));
    ch6ValueToTextList.append(QPair<int,QString>(3,"CH6_YAW_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(24,"CH6_YAW_KI"));
    ch6ValueToTextList.append(QPair<int,QString>(4,"CH6_RATE_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(5,"CH6_RATE_KI"));
    ch6ValueToTextList.append(QPair<int,QString>(6,"CH6_YAW_RATE_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(26,"CH6_YAW_RATE_KD"));
    ch6ValueToTextList.append(QPair<int,QString>(7,"CH6_THROTTLE_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(9,"CH6_RELAY"));
    ch6ValueToTextList.append(QPair<int,QString>(10,"CH6_WP_SPEED"));
    ch6ValueToTextList.append(QPair<int,QString>(12,"CH6_LOITER_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(13,"CH6_HELI_EXTERNAL_GYRO"));
    ch6ValueToTextList.append(QPair<int,QString>(14,"CH6_THR_HOLD_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(17,"CH6_OPTFLOW_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(18,"CH6_OPTFLOW_KI"));
    ch6ValueToTextList.append(QPair<int,QString>(19,"CH6_OPTFLOW_KD"));
    ch6ValueToTextList.append(QPair<int,QString>(21,"CH6_RATE_KD"));
    ch6ValueToTextList.append(QPair<int,QString>(22,"CH6_LOITER_RATE_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(23,"CH6_LOITER_RATE_KD"));
    ch6ValueToTextList.append(QPair<int,QString>(25,"CH6_ACRO_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(27,"CH6_LOITER_KI"));
    ch6ValueToTextList.append(QPair<int,QString>(28,"CH6_LOITER_RATE_KI"));
    ch6ValueToTextList.append(QPair<int,QString>(29,"CH6_STABILIZE_KD"));
    ch6ValueToTextList.append(QPair<int,QString>(30,"CH6_AHRS_YAW_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(31,"CH6_AHRS_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(32,"CH6_INAV_TC"));
    ch6ValueToTextList.append(QPair<int,QString>(33,"CH6_THROTTLE_KI"));
    ch6ValueToTextList.append(QPair<int,QString>(34,"CH6_THR_ACCEL_KP"));
    ch6ValueToTextList.append(QPair<int,QString>(35,"CH6_THR_ACCEL_KI"));
    ch6ValueToTextList.append(QPair<int,QString>(36,"CH6_THR_ACCEL_KD"));
    ch6ValueToTextList.append(QPair<int,QString>(38,"CH6_DECLINATION"));
    ch6ValueToTextList.append(QPair<int,QString>(39,"CH6_CIRCLE_RATE"));
    for (int i=0;i<ch6ValueToTextList.size();i++)
    {
        ui.ch6OptComboBox->addItem(ch6ValueToTextList[i].second);
    }

    ch7ValueToTextList.append(QPair<int,QString>(0,"Do nothing"));
    ch7ValueToTextList.append(QPair<int,QString>(2,"Flip"));
    ch7ValueToTextList.append(QPair<int,QString>(3,"Simple mode"));
    ch7ValueToTextList.append(QPair<int,QString>(4,"RTL"));
    ch7ValueToTextList.append(QPair<int,QString>(5,"Save Trim"));
    ch7ValueToTextList.append(QPair<int,QString>(7,"Save WP"));
    ch7ValueToTextList.append(QPair<int,QString>(8,"Multi Mode"));
    ch7ValueToTextList.append(QPair<int,QString>(9,"Camera Trigger"));
    ch7ValueToTextList.append(QPair<int,QString>(10,"Sonar"));
    ch7ValueToTextList.append(QPair<int,QString>(11,"Fence"));
    ch7ValueToTextList.append(QPair<int,QString>(12,"ResetToArmedYaw"));
    for (int i=0;i<ch7ValueToTextList.size();i++)
    {
        ui.ch7OptComboBox->addItem(ch7ValueToTextList[i].second);
    }

    ch8ValueToTextList.append(QPair<int,QString>(0,"Do nothing"));
    ch8ValueToTextList.append(QPair<int,QString>(2,"Flip"));
    ch8ValueToTextList.append(QPair<int,QString>(3,"Simple mode"));
    ch8ValueToTextList.append(QPair<int,QString>(4,"RTL"));
    ch8ValueToTextList.append(QPair<int,QString>(5,"Save Trim"));
    ch8ValueToTextList.append(QPair<int,QString>(7,"Save WP"));
    ch8ValueToTextList.append(QPair<int,QString>(8,"Multi Mode"));
    ch8ValueToTextList.append(QPair<int,QString>(9,"Camera Trigger"));
    ch8ValueToTextList.append(QPair<int,QString>(10,"Sonar"));
    ch8ValueToTextList.append(QPair<int,QString>(11,"Fence"));
    ch8ValueToTextList.append(QPair<int,QString>(12,"ResetToArmedYaw"));
    for (int i=0;i<ch8ValueToTextList.size();i++)
    {
        ui.ch8OptComboBox->addItem(ch8ValueToTextList[i].second);
    }

}

ArduCopterPidConfig::~ArduCopterPidConfig()
{
}
void ArduCopterPidConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (nameToBoxMap.contains(parameterName))
    {
        nameToBoxMap[parameterName]->setValue(value.toDouble());
    }
    else if (parameterName == "TUNE")
    {
        for (int i=0;i<ch6ValueToTextList.size();i++)
        {
            if (ch6ValueToTextList[i].first == value.toInt())
            {
                ui.ch6OptComboBox->setCurrentIndex(i);
            }
        }
    }
    else if (parameterName == "CH7_OPT")
    {
        for (int i=0;i<ch7ValueToTextList.size();i++)
        {
            if (ch7ValueToTextList[i].first == value.toInt())
            {
                ui.ch7OptComboBox->setCurrentIndex(i);
            }
        }
    }
    else if (parameterName == "CH8_OPT")
    {
        for (int i=0;i<ch8ValueToTextList.size();i++)
        {
            if (ch8ValueToTextList[i].first == value.toInt())
            {
                ui.ch8OptComboBox->setCurrentIndex(i);
            }
        }
    }
}
void ArduCopterPidConfig::writeButtonClicked()
{
    if (!m_uas)
    {
        return;
    }
    for (QMap<QString,QDoubleSpinBox*>::const_iterator i=nameToBoxMap.constBegin();i!=nameToBoxMap.constEnd();i++)
    {
        m_uas->getParamManager()->setParameter(1,i.key(),i.value()->value());
    }
    m_uas->getParamManager()->setParameter(1,"TUNE",ch8ValueToTextList[ui.ch6OptComboBox->currentIndex()].first);
    m_uas->getParamManager()->setParameter(1,"CH7_OPT",ch7ValueToTextList[ui.ch7OptComboBox->currentIndex()].first);
    m_uas->getParamManager()->setParameter(1,"CH8_OPT",ch8ValueToTextList[ui.ch8OptComboBox->currentIndex()].first);
}

void ArduCopterPidConfig::refreshButtonClicked()
{
    if (!m_uas)
    {
        return;
    }
    for (QMap<QString,QDoubleSpinBox*>::const_iterator i=nameToBoxMap.constBegin();i!=nameToBoxMap.constEnd();i++)
    {
        m_uas->getParamManager()->requestParameterUpdate(1,i.key());
    }
    m_uas->getParamManager()->requestParameterUpdate(1,"TUNE");
    m_uas->getParamManager()->requestParameterUpdate(1,"CH7_OPT");
    m_uas->getParamManager()->requestParameterUpdate(1,"CH8_OPT");
}
