#include "RTKSettings.h"

#include <QtCore/QPointer>
#include <QtCore/QSignalBlocker>

#include <array>

#include "GPSReceiverConfig.h"

DECLARE_SETTINGGROUP(RTK, "RTK")
{
    QSettings settings;
    if (!settings.contains(QStringLiteral("RTK/connectionType")) &&
        settings.value(QStringLiteral("AutoConnect/autoConnectNetworkRTKGPS"), false).toBool()) {
        settings.setValue(QStringLiteral("RTK/connectionType"), static_cast<int>(Tcp));
    }
}

DECLARE_SETTINGSFACT(RTKSettings, baseReceiverManufacturers)
DECLARE_SETTINGSFACT(RTKSettings, surveyInAccuracyLimit)
DECLARE_SETTINGSFACT(RTKSettings, surveyInMinObservationDuration)
DECLARE_SETTINGSFACT(RTKSettings, useFixedBasePosition)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLatitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLongitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAltitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAccuracy)
DECLARE_SETTINGSFACT(RTKSettings, networkBaseHost)
DECLARE_SETTINGSFACT(RTKSettings, networkBasePort)
DECLARE_SETTINGSFACT(RTKSettings, udpLocalPort)
DECLARE_SETTINGSFACT(RTKSettings, networkReceiverType)

DECLARE_SETTINGSFACT(RTKSettings, useReceiverPosition)
DECLARE_SETTINGSFACT(RTKSettings, connectionType)
DECLARE_SETTINGSFACT(RTKSettings, serialDevice)

DECLARE_SETTINGSFACT(RTKSettings, receiverRole)

DECLARE_SETTINGSFACT(RTKSettings, constellationMask)
DECLARE_SETTINGSFACT(RTKSettings, dynamicModel)
DECLARE_SETTINGSFACT(RTKSettings, outputRateHz)
DECLARE_SETTINGSFACT(RTKSettings, headingOffsetDeg)

bool RTKSettings::saveFixedBasePosition(const GPSReceiverConfig& configuration)
{
    if (configuration.role != GPSReceiverConfig::Role::RTKBase || !configuration.base.useFixedBase ||
        !configuration.validationError().isEmpty()) {
        return false;
    }
    std::array<std::pair<QPointer<Fact>, QVariant>, 4> values = {{
        {fixedBasePositionLatitude(), configuration.base.fixedBaseLatitude},
        {fixedBasePositionLongitude(), configuration.base.fixedBaseLongitude},
        {fixedBasePositionAltitude(), configuration.base.fixedBaseAltitudeMeters},
        {fixedBasePositionAccuracy(), configuration.base.fixedBaseAccuracyMeters},
    }};
    for (auto& [fact, value] : values) {
        QVariant converted;
        QString error;
        if (!fact->metaData()->convertAndValidateRaw(value, false, converted, error)) {
            return false;
        }
        value = converted;
    }
    std::array<bool, 4> changed;
    for (size_t index = 0; index < values.size(); ++index) {
        changed[index] = values[index].first->rawValue() != values[index].second;
    }
    // Publish only after the entire tuple is installed, so Fact observers cannot consume a partial position.
    for (const auto& [fact, value] : values) {
        const QSignalBlocker blocker(fact);
        fact->setRawValue(value);
    }
    // Persist the whole tuple before notifications can reenter or destroy this settings object.
    QSettings storage;
    storage.beginGroup(SettingsGroup::settingsGroup());
    for (const auto& [fact, value] : values) {
        storage.setValue(fact->name(), value);
    }
    const QPointer<RTKSettings> guard(this);
    for (size_t index = 0; index < values.size(); ++index) {
        if (!changed[index]) {
            continue;
        }
        const auto& [fact, value] = values[index];
        if (!guard || !fact) {
            return false;
        }
        if (fact->rawValue() != value) {
            continue;
        }
        emit fact->valueChanged(fact->cookedValue());
        if (!guard || !fact) {
            return false;
        }
        if (fact->rawValue() != value) {
            continue;
        }
        emit fact->containerRawValueChanged(value);
        if (!guard || !fact) {
            return false;
        }
        if (fact->rawValue() == value) {
            emit fact->rawValueChanged(value);
        }
    }
    return guard;
}
