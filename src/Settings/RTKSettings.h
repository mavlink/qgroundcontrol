#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"
#include <QObject>

class BaseModeDefinition : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class Mode {
        BaseSurveyIn = 0,
        BaseFixed    = 1,
    };
    Q_ENUM(Mode)

private:
    explicit BaseModeDefinition(QObject* parent = nullptr) : QObject(parent) {}
};

class RTKSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    RTKSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(baseReceiverManufacturers)
    DEFINE_SETTINGFACT(surveyInAccuracyLimit)
    DEFINE_SETTINGFACT(surveyInMinObservationDuration)
    DEFINE_SETTINGFACT(useFixedBasePosition)
    DEFINE_SETTINGFACT(fixedBasePositionLatitude)
    DEFINE_SETTINGFACT(fixedBasePositionLongitude)
    DEFINE_SETTINGFACT(fixedBasePositionAltitude)
    DEFINE_SETTINGFACT(fixedBasePositionAccuracy)
    DEFINE_SETTINGFACT(gpsOutputMode)
    DEFINE_SETTINGFACT(ubxMode)
    DEFINE_SETTINGFACT(ubxDynamicModel)
    DEFINE_SETTINGFACT(ubxUart2Baudrate)
    DEFINE_SETTINGFACT(ubxPpkOutput)
    DEFINE_SETTINGFACT(ubxDgnssTimeout)
    DEFINE_SETTINGFACT(ubxMinCno)
    DEFINE_SETTINGFACT(ubxMinElevation)
    DEFINE_SETTINGFACT(ubxOutputRate)
    DEFINE_SETTINGFACT(ubxJamDetSensitivityHi)
};
