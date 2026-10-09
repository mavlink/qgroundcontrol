#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"
#include <QObject>

class BaseModeDefinition : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class Mode
    {
        BaseSurveyIn = 0,
        BaseFixed = 1,
        BaseReceiverAveraging = 2,
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

    /// Persisted values of receiverRole. The storage type keeps any saved value a valid enum value.
    enum ReceiverRole : quint8
    {
        Passive = 1,
        ConfiguredBase = 2,
    };
    Q_ENUM(ReceiverRole)

    /// Persisted values of connectionType. The storage type keeps any saved value a valid enum value.
    enum ConnectionType : quint8
    {
        Serial = 0,
        Tcp = 1,
        Udp = 2,
    };
    Q_ENUM(ConnectionType)

    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(receiverRole)
    DEFINE_SETTINGFACT(forwardReceiverRtcm)
    DEFINE_SETTINGFACT(baseReceiverManufacturers)
    DEFINE_SETTINGFACT(surveyInAccuracyLimit)
    DEFINE_SETTINGFACT(surveyInMinObservationDuration)
    DEFINE_SETTINGFACT(receiverAveragingDuration)
    DEFINE_SETTINGFACT(connectionType)
    DEFINE_SETTINGFACT(tcpHost)
    DEFINE_SETTINGFACT(tcpPort)
    DEFINE_SETTINGFACT(udpPort)
    DEFINE_SETTINGFACT(autoConnect)
    DEFINE_SETTINGFACT(gcsPositionSource)
    DEFINE_SETTINGFACT(serialDevice)
    DEFINE_SETTINGFACT(serialBaudRate)
    DEFINE_SETTINGFACT(useFixedBasePosition)
    DEFINE_SETTINGFACT(fixedBasePositionLatitude)
    DEFINE_SETTINGFACT(fixedBasePositionLongitude)
    DEFINE_SETTINGFACT(fixedBasePositionAltitude)
    DEFINE_SETTINGFACT(fixedBasePositionAccuracy)
    DEFINE_SETTINGFACT(compactRtcmCorrections)
};
