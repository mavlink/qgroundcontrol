#include "ArduPlanePidConfig.h"


ArduPlanePidConfig::ArduPlanePidConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);

    nameToBoxMap["RLL2SRV_P"] = ui.servoRollPSpinBox;
    nameToBoxMap["RLL2SRV_I"] = ui.servoRollISpinBox;
    nameToBoxMap["RLL2SRV_D"] = ui.servoRollDSpinBox;
    nameToBoxMap["RLL2SRV_IMAX"] = ui.servoRollIMAXSpinBox;

    nameToBoxMap["PTCH2SRV_P"] = ui.servoPitchPSpinBox;
    nameToBoxMap["PTCH2SRV_I"] = ui.servoPitchISpinBox;
    nameToBoxMap["PTCH2SRV_D"] = ui.servoPitchDSpinBox;
    nameToBoxMap["PTCH2SRV_IMAX"] = ui.servoPitchIMAXSpinBox;

    nameToBoxMap["YW2SRV_P"] = ui.servoYawPSpinBox;
    nameToBoxMap["YW2SRV_I"] = ui.servoYawISpinBox;
    nameToBoxMap["YW2SRV_D"] = ui.servoYawDSpinBox;
    nameToBoxMap["YW2SRV_IMAX"] = ui.servoYawIMAXSpinBox;

    nameToBoxMap["ALT2PTCH_P"] = ui.navAltPSpinBox;
    nameToBoxMap["ALT2PTCH_I"] = ui.navAltISpinBox;
    nameToBoxMap["ALT2PTCH_D"] = ui.navAltDSpinBox;
    nameToBoxMap["ALT2PTCH_IMAX"] = ui.navAltIMAXSpinBox;

    nameToBoxMap["ARSP2PTCH_P"] = ui.navASPSpinBox;
    nameToBoxMap["ARSP2PTCH_I"] = ui.navASISpinBox;
    nameToBoxMap["ARSP2PTCH_D"] = ui.navASDSpinBox;
    nameToBoxMap["ARSP2PTCH_IMAX"] = ui.navASIMAXSpinBox;

    nameToBoxMap["ENRGY2THR_P"] = ui.energyPSpinBox;
    nameToBoxMap["ENRGY2THR_I"] = ui.energyISpinBox;
    nameToBoxMap["ENRGY2THR_D"] = ui.energyDSpinBox;
    nameToBoxMap["ENRGY2THR_IMAX"] = ui.energyIMAXSpinBox;

    nameToBoxMap["KFF_PTCH2THR"] = ui.otherPitchCompSpinBox;
    nameToBoxMap["KFF_PTCHCOMP"] = ui.otherPtTSpinBox;
    nameToBoxMap["KFF_RDDRMIX"] = ui.otherRudderMixSpinBox;

    nameToBoxMap["TRIM_THROTTLE"] = ui.throttleCruiseSpinBox;
    nameToBoxMap["THR_FS_VALUE"] = ui.throttleFSSpinBox;
    nameToBoxMap["THR_MAX"] = ui.throttleMaxSpinBox;
    nameToBoxMap["THR_MIN"] = ui.throttleMinSpinBox;

    nameToBoxMap["TRIM_ARSPD_CM"] = ui.airspeedCruiseSpinBox;
    nameToBoxMap["ARSPD_FBW_MAX"] = ui.airspeedFBWMaxSpinBox;
    nameToBoxMap["ARSPD_FBW_MIN"] = ui.airspeedFBWMinSpinBox;
    nameToBoxMap["ARSPD_RATIO"] = ui.airspeedRatioSpinBox;

    nameToBoxMap["NAVL1_DAMPING"] = ui.l1DampingSpinBox;
    nameToBoxMap["NAVL1_PERIOD"] = ui.l1PeriodSpinBox;

    nameToBoxMap["LIM_ROLL_CD"] = ui.navBankMaxSpinBox;
    nameToBoxMap["LIM_PITCH_MAX"] = ui.navPitchMaxSpinBox;
    nameToBoxMap["LIM_PITCH_MIN"] = ui.navPitchMinSpinBox;

    connect(ui.writePushButton,SIGNAL(clicked()),this,SLOT(writeButtonClicked()));
    connect(ui.refreshPushButton,SIGNAL(clicked()),this,SLOT(refreshButtonClicked()));

}

ArduPlanePidConfig::~ArduPlanePidConfig()
{
}
void ArduPlanePidConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (nameToBoxMap.contains(parameterName))
    {
        nameToBoxMap[parameterName]->setValue(value.toDouble());
    }
}
void ArduPlanePidConfig::writeButtonClicked()
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

void ArduPlanePidConfig::refreshButtonClicked()
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
