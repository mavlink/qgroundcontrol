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

/// GCS-side GPS receiver settings: RTK base-station configuration plus the
/// autoconnect toggles for NMEA and RTK GPS sources that used to live in
/// AutoConnectSettings.
class GcsGpsSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    /// True iff autoConnectNmeaPort is set (not empty, not "Disabled").
    Q_PROPERTY(bool nmeaActive READ nmeaActive NOTIFY nmeaActiveChanged)

public:
    GcsGpsSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    bool nmeaActive() const;

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
    DEFINE_SETTINGFACT(gnssSystems)
    DEFINE_SETTINGFACT(headingOffset)

    // Autoconnect / NMEA source (moved from AutoConnectSettings)
    DEFINE_SETTINGFACT(autoConnectRTKGPS)
    DEFINE_SETTINGFACT(autoConnectNmeaPort)
    DEFINE_SETTINGFACT(autoConnectNmeaBaud)
    DEFINE_SETTINGFACT(nmeaUdpPort)

    // Which external source the operator has selected (0=None, 1=NMEA, 2=RTK).
    DEFINE_SETTINGFACT(positionSourceType)

    /// Maps baseReceiverManufacturers combo index to the GPSManager driver type string.
    Q_INVOKABLE static QString gpsTypeForManufacturer(int manufacturer);

signals:
    void nmeaActiveChanged();
};
