#include "GcsGpsSettings.h"
#include "SettingsFact.h"

DECLARE_SETTINGGROUP(GcsGps, "GcsGps") {}

// Index order must match GcsGps.SettingsGroup.json combo values.
QString GcsGpsSettings::gpsTypeForManufacturer(int manufacturer)
{
    static const QString kTypes[] = {
        QStringLiteral("nmea"),
        QStringLiteral("trimble"),
        QStringLiteral("septentrio"),
        QStringLiteral("femtomes"),
        QStringLiteral("blox"),       // u-blox
        QStringLiteral("nmea"),
        QStringLiteral("nmea"),
        QStringLiteral("nmea"),
    };
    constexpr int kCount = static_cast<int>(sizeof(kTypes) / sizeof(kTypes[0]));
    if (manufacturer < 0 || manufacturer >= kCount) {
        return QStringLiteral("blox");
    }
    return kTypes[manufacturer];
}

DECLARE_SETTINGSFACT(GcsGpsSettings, baseReceiverManufacturers)
DECLARE_SETTINGSFACT(GcsGpsSettings, surveyInAccuracyLimit)
DECLARE_SETTINGSFACT(GcsGpsSettings, surveyInMinObservationDuration)
DECLARE_SETTINGSFACT(GcsGpsSettings, useFixedBasePosition)
DECLARE_SETTINGSFACT(GcsGpsSettings, fixedBasePositionLatitude)
DECLARE_SETTINGSFACT(GcsGpsSettings, fixedBasePositionLongitude)
DECLARE_SETTINGSFACT(GcsGpsSettings, fixedBasePositionAltitude)
DECLARE_SETTINGSFACT(GcsGpsSettings, fixedBasePositionAccuracy)
DECLARE_SETTINGSFACT(GcsGpsSettings, gpsOutputMode)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxMode)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxDynamicModel)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxUart2Baudrate)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxPpkOutput)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxDgnssTimeout)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxMinCno)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxMinElevation)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxOutputRate)
DECLARE_SETTINGSFACT(GcsGpsSettings, ubxJamDetSensitivityHi)
DECLARE_SETTINGSFACT(GcsGpsSettings, gnssSystems)
DECLARE_SETTINGSFACT(GcsGpsSettings, headingOffset)
DECLARE_SETTINGSFACT(GcsGpsSettings, nmeaUdpPort)
DECLARE_SETTINGSFACT(GcsGpsSettings, positionSourceType)

DECLARE_SETTINGSFACT_NO_FUNC(GcsGpsSettings, autoConnectNmeaBaud)
{
    if (!_autoConnectNmeaBaudFact) {
        _autoConnectNmeaBaudFact = _createSettingsFact(autoConnectNmeaBaudName);
#ifdef Q_OS_IOS
        _autoConnectNmeaBaudFact->setUserVisible(false);
#endif
    }
    return _autoConnectNmeaBaudFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(GcsGpsSettings, autoConnectRTKGPS)
{
    if (!_autoConnectRTKGPSFact) {
        _autoConnectRTKGPSFact = _createSettingsFact(autoConnectRTKGPSName);
#ifdef Q_OS_IOS
        _autoConnectRTKGPSFact->setUserVisible(false);
#endif
    }
    return _autoConnectRTKGPSFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(GcsGpsSettings, autoConnectNmeaPort)
{
    if (!_autoConnectNmeaPortFact) {
        _autoConnectNmeaPortFact = _createSettingsFact(autoConnectNmeaPortName);
#ifdef Q_OS_IOS
        _autoConnectNmeaPortFact->setUserVisible(false);
#endif
        (void) connect(_autoConnectNmeaPortFact, &Fact::rawValueChanged,
                       this, &GcsGpsSettings::nmeaActiveChanged);
    }
    return _autoConnectNmeaPortFact;
}

bool GcsGpsSettings::nmeaActive() const
{
    // const_cast: autoConnectNmeaPort() lazily instantiates the fact.
    auto *port = const_cast<GcsGpsSettings *>(this)->autoConnectNmeaPort();
    if (!port) {
        return false;
    }
    const QString value = port->rawValue().toString();
    return !value.isEmpty() && value != QLatin1String("Disabled");
}
