#include "RTKSettings.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>

#include "AutoConnectSettings.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTKSettingsLog, "GPS.Settings.RTKSettings")

namespace {
// Values persisted before receivers had roles.
constexpr int kLegacyPassiveManufacturer = 7;
constexpr QLatin1StringView kManufacturerMigrated("manufacturerMigrated");

enum LegacyNmeaSource
{
    NmeaDisabled = 0,
    NmeaUdp = 1,
    NmeaSerial = 2,
};

/// Before 5.1 autoConnectNmeaPort held the selected combo label or a serial device. Only the pseudo-locale ever
/// translated those labels, and their QML contexts are gone, so the source labels are the ones recognized.
LegacyNmeaSource legacyNmeaSourceFromPort(const QString& port)
{
    if (port.isEmpty() || port == QLatin1String("Disabled") || port == QLatin1String("Serial <none available>")) {
        return NmeaDisabled;
    }
    return port == QLatin1String("UDP Port") ? NmeaUdp : NmeaSerial;
}

/// Converts the keys of receivers without roles and of the separate NMEA GPS input.
void migrateLegacySettings()
{
    QSettings settings;
    settings.beginGroup(RTKSettings::settingsGroup);
    // Before receivers had roles, the manufacturer only filtered the settings shown and auto-connect chose the family
    // by board name, so a saved value falls back to Detect automatically. Old and new values share the key, so a marker
    // keeps a manufacturer chosen later. Only the legacy passive choice stores a role; otherwise the default applies.
    if (!settings.contains(kManufacturerMigrated)) {
        if (!settings.contains(RTKSettings::receiverRoleName)) {
            if (settings.value(RTKSettings::baseReceiverManufacturersName).toInt() == kLegacyPassiveManufacturer) {
                settings.setValue(RTKSettings::receiverRoleName, RTKSettings::Passive);
            }
            settings.remove(RTKSettings::baseReceiverManufacturersName);
        }
        settings.setValue(kManufacturerMigrated, true);
    }
    const bool receiverConfigured = !settings.value(RTKSettings::serialDeviceName).toString().trimmed().isEmpty();
    settings.endGroup();

    // Receiver auto-connect moved here from the AutoConnect group.
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    const QVariant autoConnect = settings.value(QStringLiteral("autoConnectRTKGPS"));
    settings.remove(QStringLiteral("autoConnectRTKGPS"));
    settings.endGroup();
    settings.beginGroup(RTKSettings::settingsGroup);
    if (autoConnect.isValid() && !settings.contains(RTKSettings::autoConnectName)) {
        settings.setValue(RTKSettings::autoConnectName, autoConnect);
    }
    settings.endGroup();

    // The separate NMEA GPS input became a passive receiver that forwards no RTCM. Only one receiver remains: the input
    // replaces neither a configured receiver nor RTK auto-connect the user turned on, since that setting is saved only
    // when changed. Left at its default, the input the user chose wins over a base that may not exist.
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    const QString nmeaPort = settings.value(QStringLiteral("autoConnectNmeaPort")).toString().trimmed();
    const int nmeaSource = settings.contains(QStringLiteral("nmeaSource"))
                               ? settings.value(QStringLiteral("nmeaSource")).toInt()
                               : legacyNmeaSourceFromPort(nmeaPort);
    const QVariant nmeaBaud = settings.value(QStringLiteral("autoConnectNmeaBaud"), 4800);
    const QVariant nmeaUdpPort = settings.value(QStringLiteral("nmeaUdpPort"));
    for (const auto* key : {"nmeaSource", "autoConnectNmeaPort", "autoConnectNmeaBaud", "nmeaUdpPort"}) {
        settings.remove(QLatin1String(key));
    }
    settings.endGroup();
    const bool nmeaInput = nmeaSource == NmeaUdp || (nmeaSource == NmeaSerial && !nmeaPort.isEmpty());
    const bool baseChosen = autoConnect.isValid() && autoConnect.toBool();
    if (receiverConfigured || baseChosen || !nmeaInput) {
        return;
    }
    settings.beginGroup(RTKSettings::settingsGroup);
    settings.setValue(RTKSettings::receiverRoleName, RTKSettings::Passive);
    settings.setValue(RTKSettings::forwardReceiverRtcmName, false);
    // The NMEA input always connected at startup.
    settings.setValue(RTKSettings::autoConnectName, true);
    if (nmeaSource == NmeaUdp) {
        settings.setValue(RTKSettings::connectionTypeName, RTKSettings::Udp);
        if (nmeaUdpPort.isValid()) {
            settings.setValue(RTKSettings::udpPortName, nmeaUdpPort);
        }
    } else {
        settings.setValue(RTKSettings::connectionTypeName, RTKSettings::Serial);
        settings.setValue(RTKSettings::serialDeviceName, nmeaPort);
        settings.setValue(RTKSettings::serialBaudRateName, nmeaBaud);
    }
    settings.endGroup();
}

// The keys of a saved base site.
constexpr QLatin1StringView kSiteName("name");
constexpr QLatin1StringView kSiteLatitude("latitude");
constexpr QLatin1StringView kSiteLongitude("longitude");
constexpr QLatin1StringView kSiteAltitude("altitude");
constexpr QLatin1StringView kSiteAccuracy("accuracy");

bool validBaseSite(const RTKBaseSite& site)
{
    return !site.name.isEmpty() && site.name.size() <= RTKSettings::MaxBaseSiteNameLength &&
           std::isfinite(site.latitude) && std::abs(site.latitude) <= 90. && std::isfinite(site.longitude) &&
           std::abs(site.longitude) <= 180. && std::isfinite(site.altitude) && std::isfinite(site.accuracy);
}

/// The site a saved entry holds; empty for an entry that is not a valid site.
std::optional<RTKBaseSite> baseSiteFromJson(const QJsonValue& entry)
{
    const QJsonObject object = entry.toObject();
    const QJsonValue name = object.value(kSiteName);
    const QJsonValue latitude = object.value(kSiteLatitude);
    const QJsonValue longitude = object.value(kSiteLongitude);
    const QJsonValue altitude = object.value(kSiteAltitude);
    const QJsonValue accuracy = object.value(kSiteAccuracy);
    if (!name.isString() || !latitude.isDouble() || !longitude.isDouble() || !altitude.isDouble() ||
        !accuracy.isDouble()) {
        return std::nullopt;
    }
    const RTKBaseSite site{.name = name.toString().trimmed(),
                           .latitude = latitude.toDouble(),
                           .longitude = longitude.toDouble(),
                           .altitude = altitude.toDouble(),
                           .accuracy = accuracy.toDouble()};
    return validBaseSite(site) ? std::optional(site) : std::nullopt;
}

QJsonObject baseSiteToJson(const RTKBaseSite& site)
{
    return {{kSiteName, site.name},
            {kSiteLatitude, site.latitude},
            {kSiteLongitude, site.longitude},
            {kSiteAltitude, site.altitude},
            {kSiteAccuracy, site.accuracy}};
}

}  // namespace

DECLARE_SETTINGGROUP(RTK, "RTK")
{
    migrateLegacySettings();
    (void) connect(savedBaseSites(), &Fact::rawValueChanged, this, &RTKSettings::_loadBaseSites);
    _loadBaseSites();
}

bool RTKSettings::saveBaseSite(const RTKBaseSite& site)
{
    RTKBaseSite saved = site;
    saved.name = site.name.trimmed();
    if (!validBaseSite(saved)) {
        return false;
    }
    QList<RTKBaseSite> sites = _baseSites;
    const auto existing = std::ranges::find(sites, saved.name, &RTKBaseSite::name);
    if (existing != sites.end()) {
        *existing = saved;
    } else if (sites.size() < MaxBaseSites) {
        sites.append(saved);
    } else {
        return false;
    }
    _storeBaseSites(sites);
    return true;
}

bool RTKSettings::saveFixedBasePositionAsSite(const QString& siteName)
{
    return saveBaseSite({.name = siteName,
                         .latitude = fixedBasePositionLatitude()->rawValue().toDouble(),
                         .longitude = fixedBasePositionLongitude()->rawValue().toDouble(),
                         .altitude = fixedBasePositionAltitude()->rawValue().toDouble(),
                         .accuracy = fixedBasePositionAccuracy()->rawValue().toDouble()});
}

bool RTKSettings::useBaseSite(const QString& siteName)
{
    const auto saved = std::ranges::find(std::as_const(_baseSites), siteName, &RTKBaseSite::name);
    if (saved == _baseSites.cend()) {
        return false;
    }
    // A copy: an observer of the position Facts may change the sites.
    const RTKBaseSite site = *saved;
    fixedBasePositionLatitude()->setRawValue(site.latitude);
    fixedBasePositionLongitude()->setRawValue(site.longitude);
    fixedBasePositionAltitude()->setRawValue(site.altitude);
    fixedBasePositionAccuracy()->setRawValue(site.accuracy);
    return true;
}

bool RTKSettings::removeBaseSite(const QString& siteName)
{
    QList<RTKBaseSite> sites = _baseSites;
    if (sites.removeIf([&siteName](const RTKBaseSite& site) { return site.name == siteName; }) == 0) {
        return false;
    }
    _storeBaseSites(sites);
    return true;
}

void RTKSettings::_loadBaseSites()
{
    const QString saved = savedBaseSites()->rawValue().toString();
    const QJsonDocument document = QJsonDocument::fromJson(saved.toUtf8());
    const QJsonArray entries = document.array();
    QList<RTKBaseSite> sites;
    for (const QJsonValue& entry : entries) {
        const std::optional<RTKBaseSite> site = baseSiteFromJson(entry);
        if (site && sites.size() < MaxBaseSites &&
            std::ranges::find(sites, site->name, &RTKBaseSite::name) == sites.cend()) {
            sites.append(*site);
        }
    }
    // The entries hold the base station's location, so the log only counts them.
    if (!saved.isEmpty() && !document.isArray()) {
        qCWarning(RTKSettingsLog) << "The saved base sites are not a JSON array; none are used";
    } else if (sites.size() != entries.size()) {
        qCWarning(RTKSettingsLog) << "Dropped" << entries.size() - sites.size() << "of" << entries.size()
                                  << "saved base sites: not a valid site, a repeated name, or more than"
                                  << static_cast<int>(MaxBaseSites);
    }
    if (sites != _baseSites) {
        _baseSites = sites;
        emit baseSitesChanged();
    }
}

void RTKSettings::_storeBaseSites(const QList<RTKBaseSite>& sites)
{
    QJsonArray entries;
    for (const RTKBaseSite& site : sites) {
        entries.append(baseSiteToJson(site));
    }
    // The Fact's change loads the sites back.
    savedBaseSites()->setRawValue(
        sites.isEmpty() ? QString() : QString::fromUtf8(QJsonDocument(entries).toJson(QJsonDocument::Compact)));
}

DECLARE_SETTINGSFACT(RTKSettings, receiverRole)
DECLARE_SETTINGSFACT(RTKSettings, forwardReceiverRtcm)
DECLARE_SETTINGSFACT(RTKSettings, baseReceiverManufacturers)
DECLARE_SETTINGSFACT(RTKSettings, surveyInAccuracyLimit)
DECLARE_SETTINGSFACT(RTKSettings, surveyInMinObservationDuration)
DECLARE_SETTINGSFACT(RTKSettings, receiverAveragingDuration)
DECLARE_SETTINGSFACT(RTKSettings, connectionType)
DECLARE_SETTINGSFACT(RTKSettings, tcpHost)
DECLARE_SETTINGSFACT(RTKSettings, tcpPort)
DECLARE_SETTINGSFACT(RTKSettings, udpPort)
DECLARE_SETTINGSFACT(RTKSettings, autoConnect)
DECLARE_SETTINGSFACT(RTKSettings, gcsPositionSource)
DECLARE_SETTINGSFACT(RTKSettings, serialDevice)
DECLARE_SETTINGSFACT(RTKSettings, serialBaudRate)
DECLARE_SETTINGSFACT(RTKSettings, useFixedBasePosition)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLatitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLongitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAltitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAccuracy)
DECLARE_SETTINGSFACT(RTKSettings, compactRtcmCorrections)
DECLARE_SETTINGSFACT(RTKSettings, savedBaseSites)
