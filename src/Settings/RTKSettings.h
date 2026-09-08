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

    enum ConnectionType
    {
        Serial = 0,
        Tcp,
        Udp
    };
    Q_ENUM(ConnectionType)
    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(baseReceiverManufacturers)
    DEFINE_SETTINGFACT(surveyInAccuracyLimit)
    DEFINE_SETTINGFACT(surveyInMinObservationDuration)
    DEFINE_SETTINGFACT(useFixedBasePosition)
    DEFINE_SETTINGFACT(fixedBasePositionLatitude)
    DEFINE_SETTINGFACT(fixedBasePositionLongitude)
    DEFINE_SETTINGFACT(fixedBasePositionAltitude)
    DEFINE_SETTINGFACT(fixedBasePositionAccuracy)
    DEFINE_SETTINGFACT(connectionType)
    DEFINE_SETTINGFACT(serialDevice)
    DEFINE_SETTINGFACT(networkBaseHost)
    DEFINE_SETTINGFACT(networkBasePort)
    DEFINE_SETTINGFACT(udpLocalPort)
    DEFINE_SETTINGFACT(networkReceiverType)
};
