#include "ArduRoverPidConfig.h"


ArduRoverPidConfig::ArduRoverPidConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    nameToBoxMap["STEER2SRV_P"] = ui.steer2ServoPSpinBox;
    nameToBoxMap["STEER2SRV_I"] = ui.steer2ServoISpinBox;
    nameToBoxMap["STEER2SRV_D"] = ui.steer2ServoDSpinBox;
    nameToBoxMap["STEER2SRV_IMAX"] = ui.steer2ServoIMAXSpinBox;

    nameToBoxMap["XTRK_ANGLE_CD"] = ui.xtrackEntryAngleSpinBox;
    nameToBoxMap["XTRK_GAIN_SC"] = ui.xtrackGainSpinBox;

    nameToBoxMap["CRUISE_THROTTLE"] = ui.throttleCruiseSpinBox;
    nameToBoxMap["THR_MIN"] = ui.throttleMinSpinBox;
    nameToBoxMap["THR_MAX"] = ui.throttleMaxSpinBox;
    nameToBoxMap["FS_THR_VALUE"] = ui.throttleFSSpinBox;

    nameToBoxMap["HDNG2STEER_P"] = ui.heading2SteerPSpinBox;
    nameToBoxMap["HDNG2STEER_I"] = ui.heading2SteerISpinBox;
    nameToBoxMap["HDNG2STEER_D"] = ui.heading2SteerDSpinBox;
    nameToBoxMap["HDNG2STEER_IMAX"] = ui.heading2SteerIMAXSpinBox;

    nameToBoxMap["SPEED2THR_P"] = ui.speed2ThrottlePSpinBox;
    nameToBoxMap["SPEED2THR_I"] = ui.speed2ThrottleISpinBox;
    nameToBoxMap["SPEED2THR_D"] = ui.speed2ThrottleDSpinBox;
    nameToBoxMap["SPEED2THR_IMAX"] = ui.speed2ThrottleIMAXSpinBox;

    nameToBoxMap["CRUISE_SPEED"] = ui.roverCruiseSpinBox;
    nameToBoxMap["SPEED_TURN_GAIN"] = ui.roverTurnSpeedSpinBox;
    nameToBoxMap["SPEED_TURN_DIST"] = ui.roverTurnDistSpinBox;
    nameToBoxMap["WP_RADIUS"] = ui.roverWPRadiusSpinBox;

    nameToBoxMap["SONAR_TRIGGER_CM"] = ui.sonarTriggerSpinBox;
    nameToBoxMap["SONAR_TURN_ANGLE"] = ui.sonarTurnAngleSpinBox;
    nameToBoxMap["SONAR_TURN_TIME"] = ui.sonarTurnTimeSpinBox;
    nameToBoxMap["SONAR_DEBOUNCE"] = ui.sonaeDebounceSpinBox;

    connect(ui.writePushButton,SIGNAL(clicked()),this,SLOT(writeButtonClicked()));
    connect(ui.refreshPushButton,SIGNAL(clicked()),this,SLOT(refreshButtonClicked()));
    initConnections();
}

ArduRoverPidConfig::~ArduRoverPidConfig()
{
}
void ArduRoverPidConfig::writeButtonClicked()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    for (QMap<QString,QDoubleSpinBox*>::const_iterator i=nameToBoxMap.constBegin();i!=nameToBoxMap.constEnd();i++)
    {
        m_uas->getParamManager()->setParameter(1,i.key(),i.value()->value());
    }
}

void ArduRoverPidConfig::refreshButtonClicked()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    for (QMap<QString,QDoubleSpinBox*>::const_iterator i=nameToBoxMap.constBegin();i!=nameToBoxMap.constEnd();i++)
    {
        m_uas->getParamManager()->requestParameterUpdate(1,i.key());
    }
}

void ArduRoverPidConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (nameToBoxMap.contains(parameterName))
    {
        nameToBoxMap[parameterName]->setValue(value.toFloat());
    }
}
