#include "ArduCopterPidConfig.h"

ArduCopterPidConfig::ArduCopterPidConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    m_nameToBoxMap["STB_RLL_P"] = ui.stabilPitchPSpinBox;
    m_nameToBoxMap["STB_PIT_P"] = ui.stabilRollPSpinBox;
    m_nameToBoxMap["STB_YAW_P"] = ui.stabilYawPSpinBox;
    m_nameToBoxMap["HLD_LAT_P"] = ui.loiterPidPSpinBox;

    m_nameToBoxMap["RATE_RLL_P"] = ui.rateRollPSpinBox;
    m_nameToBoxMap["RATE_RLL_I"] = ui.rateRollISpinBox;
    m_nameToBoxMap["RATE_RLL_D"] = ui.rateRollDSpinBox;
    m_nameToBoxMap["RATE_RLL_IMAX"] = ui.rateRollIMAXSpinBox;

    m_nameToBoxMap["RATE_PIT_P"] = ui.ratePitchPSpinBox;
    m_nameToBoxMap["RATE_PIT_I"] = ui.ratePitchISpinBox;
    m_nameToBoxMap["RATE_PIT_D"] = ui.ratePitchDSpinBox;
    m_nameToBoxMap["RATE_PIT_IMAX"] = ui.ratePitchIMAXSpinBox;

    m_nameToBoxMap["RATE_YAW_P"] = ui.rateYawPSpinBox;
    m_nameToBoxMap["RATE_YAW_I"] = ui.rateYawISpinBox;
    m_nameToBoxMap["RATE_YAW_D"] = ui.rateYawDSpinBox;
    m_nameToBoxMap["RATE_YAW_IMAX"] = ui.rateYawIMAXSpinBox;

    m_nameToBoxMap["LOITER_LAT_P"] = ui.rateLoiterPSpinBox;
    m_nameToBoxMap["LOITER_LAT_I"] = ui.rateLoiterISpinBox;
    m_nameToBoxMap["LOITER_LAT_D"] = ui.rateLoiterDSpinBox;
    m_nameToBoxMap["LOITER_LAT_IMAX"] = ui.rateLoiterIMAXSpinBox;

    m_nameToBoxMap["THR_ACCEL_P"] = ui.throttleAccelPSpinBox;
    m_nameToBoxMap["THR_ACCEL_I"] = ui.throttleAccelISpinBox;
    m_nameToBoxMap["THR_ACCEL_D"] = ui.throttleAccelDSpinBox;
    m_nameToBoxMap["THR_ACCEL_IMAX"] = ui.throttleAccelIMAXSpinBox;

    m_nameToBoxMap["THR_RATE_P"] = ui.throttleRatePSpinBox;
    m_nameToBoxMap["THR_RATE_D"] = ui.throttleDateDSpinBox;

    m_nameToBoxMap["THR_ALT_P"] = ui.altitudeHoldPSpinBox;

    m_nameToBoxMap["WPNAV_SPEED"] = ui.wpNavLoiterSpeedSpinBox;
    m_nameToBoxMap["WPNAV_RADIUS"] = ui.wpNavRadiusSpinBox;
    m_nameToBoxMap["WPNAV_SPEED_DN"] = ui.wpNavSpeedDownSpinBox;
    m_nameToBoxMap["WPNAV_LOIT_SPEED"] = ui.wpNavSpeedSpinBox;
    m_nameToBoxMap["WPNAV_SPEED_UP"] = ui.wpNavSpeedUpSpinBox;

    m_nameToBoxMap["TUNE_HIGH"] = ui.ch6MaxSpinBox;
    m_nameToBoxMap["TUNE_LOW"] = ui.ch6MinSpinBox;

    connect(ui.writePushButton,SIGNAL(clicked()),this,SLOT(writeButtonClicked()));
    connect(ui.refreshPushButton,SIGNAL(clicked()),this,SLOT(refreshButtonClicked()));



    m_ch6ValueToTextList.append(QPair<int,QString>(0,"CH6_NONE"));
    m_ch6ValueToTextList.append(QPair<int,QString>(1,"CH6_STABILIZE_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(2,"CH6_STABILIZE_KI"));
    m_ch6ValueToTextList.append(QPair<int,QString>(3,"CH6_YAW_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(24,"CH6_YAW_KI"));
    m_ch6ValueToTextList.append(QPair<int,QString>(4,"CH6_RATE_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(5,"CH6_RATE_KI"));
    m_ch6ValueToTextList.append(QPair<int,QString>(6,"CH6_YAW_RATE_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(26,"CH6_YAW_RATE_KD"));
    m_ch6ValueToTextList.append(QPair<int,QString>(7,"CH6_THROTTLE_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(9,"CH6_RELAY"));
    m_ch6ValueToTextList.append(QPair<int,QString>(10,"CH6_WP_SPEED"));
    m_ch6ValueToTextList.append(QPair<int,QString>(12,"CH6_LOITER_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(13,"CH6_HELI_EXTERNAL_GYRO"));
    m_ch6ValueToTextList.append(QPair<int,QString>(14,"CH6_THR_HOLD_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(17,"CH6_OPTFLOW_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(18,"CH6_OPTFLOW_KI"));
    m_ch6ValueToTextList.append(QPair<int,QString>(19,"CH6_OPTFLOW_KD"));
    m_ch6ValueToTextList.append(QPair<int,QString>(21,"CH6_RATE_KD"));
    m_ch6ValueToTextList.append(QPair<int,QString>(22,"CH6_LOITER_RATE_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(23,"CH6_LOITER_RATE_KD"));
    m_ch6ValueToTextList.append(QPair<int,QString>(25,"CH6_ACRO_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(27,"CH6_LOITER_KI"));
    m_ch6ValueToTextList.append(QPair<int,QString>(28,"CH6_LOITER_RATE_KI"));
    m_ch6ValueToTextList.append(QPair<int,QString>(29,"CH6_STABILIZE_KD"));
    m_ch6ValueToTextList.append(QPair<int,QString>(30,"CH6_AHRS_YAW_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(31,"CH6_AHRS_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(32,"CH6_INAV_TC"));
    m_ch6ValueToTextList.append(QPair<int,QString>(33,"CH6_THROTTLE_KI"));
    m_ch6ValueToTextList.append(QPair<int,QString>(34,"CH6_THR_ACCEL_KP"));
    m_ch6ValueToTextList.append(QPair<int,QString>(35,"CH6_THR_ACCEL_KI"));
    m_ch6ValueToTextList.append(QPair<int,QString>(36,"CH6_THR_ACCEL_KD"));
    m_ch6ValueToTextList.append(QPair<int,QString>(38,"CH6_DECLINATION"));
    m_ch6ValueToTextList.append(QPair<int,QString>(39,"CH6_CIRCLE_RATE"));
    for (int i=0;i<m_ch6ValueToTextList.size();i++)
    {
        ui.ch6OptComboBox->addItem(m_ch6ValueToTextList[i].second);
    }

    m_ch78ValueToTextList.append(QPair<int,QString>(0,"Do nothing"));
    m_ch78ValueToTextList.append(QPair<int,QString>(2,"Flip"));
    m_ch78ValueToTextList.append(QPair<int,QString>(3,"Simple mode"));
    m_ch78ValueToTextList.append(QPair<int,QString>(4,"RTL"));
    m_ch78ValueToTextList.append(QPair<int,QString>(5,"Save Trim"));
    m_ch78ValueToTextList.append(QPair<int,QString>(7,"Save WP"));
    m_ch78ValueToTextList.append(QPair<int,QString>(8,"Multi Mode"));
    m_ch78ValueToTextList.append(QPair<int,QString>(9,"Camera Trigger"));
    m_ch78ValueToTextList.append(QPair<int,QString>(10,"Sonar"));
    m_ch78ValueToTextList.append(QPair<int,QString>(11,"Fence"));
    m_ch78ValueToTextList.append(QPair<int,QString>(12,"ResetToArmedYaw"));
    for (int i=0;i<m_ch78ValueToTextList.size();i++)
    {
        ui.ch7OptComboBox->addItem(m_ch78ValueToTextList[i].second);
        ui.ch8OptComboBox->addItem(m_ch78ValueToTextList[i].second);
    }
}

ArduCopterPidConfig::~ArduCopterPidConfig()
{
}
void ArduCopterPidConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (m_nameToBoxMap.contains(parameterName))
    {
        m_nameToBoxMap[parameterName]->setValue(value.toDouble());
    }
    else if (parameterName == "TUNE")
    {
        for (int i=0;i<m_ch6ValueToTextList.size();i++)
        {
            if (m_ch6ValueToTextList[i].first == value.toInt())
            {
                ui.ch6OptComboBox->setCurrentIndex(i);
            }
        }
    }
    else if (parameterName == "CH7_OPT")
    {
        for (int i=0;i<m_ch78ValueToTextList.size();i++)
        {
            if (m_ch78ValueToTextList[i].first == value.toInt())
            {
                ui.ch7OptComboBox->setCurrentIndex(i);
            }
        }
    }
    else if (parameterName == "CH8_OPT")
    {
        for (int i=0;i<m_ch78ValueToTextList.size();i++)
        {
            if (m_ch78ValueToTextList[i].first == value.toInt())
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
        showNullMAVErrorMessageBox();
        return;
    }
    for (QMap<QString,QDoubleSpinBox*>::const_iterator i=m_nameToBoxMap.constBegin();i!=m_nameToBoxMap.constEnd();i++)
    {
        m_uas->getParamManager()->setParameter(1,i.key(),i.value()->value());
    }
    m_uas->getParamManager()->setParameter(1,"TUNE",m_ch78ValueToTextList[ui.ch6OptComboBox->currentIndex()].first);
    m_uas->getParamManager()->setParameter(1,"CH7_OPT",m_ch78ValueToTextList[ui.ch7OptComboBox->currentIndex()].first);
    m_uas->getParamManager()->setParameter(1,"CH8_OPT",m_ch78ValueToTextList[ui.ch8OptComboBox->currentIndex()].first);
}

void ArduCopterPidConfig::refreshButtonClicked()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    for (QMap<QString,QDoubleSpinBox*>::const_iterator i=m_nameToBoxMap.constBegin();i!=m_nameToBoxMap.constEnd();i++)
    {
        m_uas->getParamManager()->requestParameterUpdate(1,i.key());
    }
    m_uas->getParamManager()->requestParameterUpdate(1,"TUNE");
    m_uas->getParamManager()->requestParameterUpdate(1,"CH7_OPT");
    m_uas->getParamManager()->requestParameterUpdate(1,"CH8_OPT");
}
