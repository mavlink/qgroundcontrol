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
}
