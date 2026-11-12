#include "NTRIPSettings.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPSettingsLog, "GPS.Settings.NTRIPSettings")

namespace {

// The keys of a saved caster.
constexpr QLatin1StringView kCasterName("name");
constexpr QLatin1StringView kCasterHost("host");
constexpr QLatin1StringView kCasterPort("port");
constexpr QLatin1StringView kCasterUsername("username");
constexpr QLatin1StringView kCasterPassword("password");
constexpr QLatin1StringView kCasterMountpoint("mountpoint");
constexpr QLatin1StringView kCasterUseTls("useTls");
constexpr QLatin1StringView kCasterAllowSelfSigned("allowSelfSignedCerts");
constexpr QLatin1StringView kCasterPin("pinnedCertificate");

/// Whether @a pin is the trusted certificate of @a caster: a pin starts with the host and port it was trusted for,
/// as NTRIPConnectionConfig::endpoint() writes them, with an IPv6 literal in brackets.
bool pinNamesCaster(const QString& pin, const NTRIPSavedCaster& caster)
{
    const bool literal = caster.host.contains(QLatin1Char(':')) && !caster.host.startsWith(QLatin1Char('['));
    const QString host = literal ? QLatin1Char('[') + caster.host + QLatin1Char(']') : caster.host;
    return pin.startsWith(host + QLatin1Char(':') + QString::number(caster.port) + QLatin1Char('|'));
}

/// @a pin as it is written now. One stored before IPv6 literals went in brackets starts with "fe80::1:2101": its
/// port follows its last colon, so it names one host and port, which go in brackets here. Any other pin is returned
/// as it is.
QString pinWithLiteralInBrackets(const QString& pin)
{
    const qsizetype digest = pin.lastIndexOf(QLatin1Char('|'));
    const qsizetype port = digest < 0 ? -1 : pin.lastIndexOf(QLatin1Char(':'), digest);
    if (port < 0) {
        return pin;
    }
    const QString host = pin.first(port);
    if (!host.contains(QLatin1Char(':')) || host.startsWith(QLatin1Char('['))) {
        return pin;
    }
    return QLatin1Char('[') + host + QLatin1Char(']') + pin.sliced(port);
}

bool validSavedCaster(const NTRIPSavedCaster& caster)
{
    const auto bounded = [](const QString& text) { return text.size() <= NTRIPSettings::MaxSavedCasterTextLength; };
    const auto hostCharacter = [](QChar c) { return !c.isSpace() && c.category() != QChar::Other_Control; };
    return !caster.name.isEmpty() && caster.name.size() <= NTRIPSettings::MaxSavedCasterNameLength &&
           !caster.host.isEmpty() && bounded(caster.host) && std::ranges::all_of(caster.host, hostCharacter) &&
           caster.port >= 1 && caster.port <= 65535 && bounded(caster.username) && bounded(caster.password) &&
           bounded(caster.mountpoint);
}

/// @a caster as it is saved: its name without surrounding spaces, and its pin, as it is written now, only when it
/// accepts self-signed certificates and the pin is its own.
NTRIPSavedCaster savedCasterAsStored(NTRIPSavedCaster caster)
{
    caster.name = caster.name.trimmed();
    caster.pinnedCertificate = pinWithLiteralInBrackets(caster.pinnedCertificate);
    if (!caster.allowSelfSignedCerts || !pinNamesCaster(caster.pinnedCertificate, caster)) {
        caster.pinnedCertificate.clear();
    }
    return caster;
}

/// The caster a saved entry holds; empty for an entry that is not a valid caster.
std::optional<NTRIPSavedCaster> savedCasterFromJson(const QJsonValue& entry)
{
    const QJsonObject object = entry.toObject();
    const QJsonValue name = object.value(kCasterName);
    const QJsonValue host = object.value(kCasterHost);
    const QJsonValue port = object.value(kCasterPort);
    if (!name.isString() || !host.isString() || !port.isDouble()) {
        return std::nullopt;
    }
    // A port that is not a whole number reads as 0, which is not a port.
    NTRIPSavedCaster caster;
    caster.name = name.toString();
    caster.host = host.toString();
    caster.port = port.toInt();
    // The other keys may be absent, but not of another type.
    const auto text = [&object](QLatin1StringView key, QString& value) {
        const QJsonValue saved = object.value(key);
        value = saved.toString();
        return saved.isString() || saved.isUndefined();
    };
    const auto flag = [&object](QLatin1StringView key, bool& value) {
        const QJsonValue saved = object.value(key);
        value = saved.toBool();
        return saved.isBool() || saved.isUndefined();
    };
    if (!text(kCasterUsername, caster.username) || !text(kCasterPassword, caster.password) ||
        !text(kCasterMountpoint, caster.mountpoint) || !text(kCasterPin, caster.pinnedCertificate) ||
        !flag(kCasterUseTls, caster.useTls) || !flag(kCasterAllowSelfSigned, caster.allowSelfSignedCerts)) {
        return std::nullopt;
    }
    caster = savedCasterAsStored(caster);
    return validSavedCaster(caster) ? std::optional(caster) : std::nullopt;
}

QJsonObject savedCasterToJson(const NTRIPSavedCaster& caster)
{
    return {{kCasterName, caster.name},
            {kCasterHost, caster.host},
            {kCasterPort, caster.port},
            {kCasterUsername, caster.username},
            {kCasterPassword, caster.password},
            {kCasterMountpoint, caster.mountpoint},
            {kCasterUseTls, caster.useTls},
            {kCasterAllowSelfSigned, caster.allowSelfSignedCerts},
            {kCasterPin, caster.pinnedCertificate}};
}

}  // namespace

DECLARE_SETTINGGROUP(NTRIP, "NTRIP")
{
    (void) connect(ntripSavedCasters(), &Fact::rawValueChanged, this, &NTRIPSettings::_loadSavedCasters);
    _loadSavedCasters();
    (void) connect(ntripPinnedCertificate(), &Fact::rawValueChanged, this, &NTRIPSettings::_bracketPinnedLiteral);
    _bracketPinnedLiteral();
}

bool NTRIPSettings::saveCaster(const NTRIPSavedCaster& caster)
{
    const NTRIPSavedCaster saved = savedCasterAsStored(caster);
    if (!validSavedCaster(saved)) {
        return false;
    }
    QList<NTRIPSavedCaster> casters = _savedCasters;
    const auto existing = std::ranges::find(casters, saved.name, &NTRIPSavedCaster::name);
    if (existing != casters.end()) {
        *existing = saved;
    } else if (casters.size() < MaxSavedCasters) {
        casters.append(saved);
    } else {
        return false;
    }
    _storeSavedCasters(casters);
    return true;
}

bool NTRIPSettings::saveConnectionAsCaster(const QString& casterName)
{
    return saveCaster({.name = casterName,
                       .host = ntripServerHostAddress()->rawValue().toString(),
                       .port = ntripServerPort()->rawValue().toInt(),
                       .username = ntripUsername()->rawValue().toString(),
                       .password = ntripPassword()->rawValue().toString(),
                       .mountpoint = ntripMountpoint()->rawValue().toString(),
                       .useTls = ntripUseTls()->rawValue().toBool(),
                       .allowSelfSignedCerts = ntripAllowSelfSignedCerts()->rawValue().toBool(),
                       .pinnedCertificate = ntripPinnedCertificate()->rawValue().toString()});
}

bool NTRIPSettings::useSavedCaster(const QString& casterName)
{
    const auto saved = std::ranges::find(std::as_const(_savedCasters), casterName, &NTRIPSavedCaster::name);
    if (saved == _savedCasters.cend() || ntripServerConnectEnabled()->rawValue().toBool()) {
        return false;
    }
    // A copy: an observer of the connection Facts may change the casters.
    const NTRIPSavedCaster caster = *saved;
    QString pin;
    if (caster.allowSelfSignedCerts) {
        const QString trusted = ntripPinnedCertificate()->rawValue().toString();
        pin = pinNamesCaster(trusted, caster) ? trusted : caster.pinnedCertificate;
    }
    // The caster and whether its certificate may be self-signed come before the pin, so that no reader of the
    // settings sees the pin beside a caster or a choice it does not belong to.
    ntripServerHostAddress()->setRawValue(caster.host);
    ntripServerPort()->setRawValue(caster.port);
    ntripUseTls()->setRawValue(caster.useTls);
    ntripAllowSelfSignedCerts()->setRawValue(caster.allowSelfSignedCerts);
    ntripUsername()->setRawValue(caster.username);
    ntripPassword()->setRawValue(caster.password);
    ntripMountpoint()->setRawValue(caster.mountpoint);
    ntripPinnedCertificate()->setRawValue(pin);
    return true;
}

bool NTRIPSettings::removeSavedCaster(const QString& casterName)
{
    QList<NTRIPSavedCaster> casters = _savedCasters;
    if (casters.removeIf([&casterName](const NTRIPSavedCaster& caster) { return caster.name == casterName; }) == 0) {
        return false;
    }
    _storeSavedCasters(casters);
    return true;
}

void NTRIPSettings::_bracketPinnedLiteral()
{
    const QString pin = ntripPinnedCertificate()->rawValue().toString();
    if (const QString current = pinWithLiteralInBrackets(pin); current != pin) {
        ntripPinnedCertificate()->setRawValue(current);
    }
}

void NTRIPSettings::_loadSavedCasters()
{
    const QString saved = ntripSavedCasters()->rawValue().toString();
    const QJsonDocument document = QJsonDocument::fromJson(saved.toUtf8());
    const QJsonArray entries = document.array();
    QList<NTRIPSavedCaster> casters;
    for (const QJsonValue& entry : entries) {
        const std::optional<NTRIPSavedCaster> caster = savedCasterFromJson(entry);
        if (caster && casters.size() < MaxSavedCasters &&
            std::ranges::find(casters, caster->name, &NTRIPSavedCaster::name) == casters.cend()) {
            casters.append(*caster);
        }
    }
    // The entries hold passwords, so the log only counts them.
    if (!saved.isEmpty() && !document.isArray()) {
        qCWarning(NTRIPSettingsLog) << "The saved casters are not a JSON array; none are used";
    } else if (casters.size() != entries.size()) {
        qCWarning(NTRIPSettingsLog) << "Dropped" << entries.size() - casters.size() << "of" << entries.size()
                                    << "saved casters: not a valid caster, a repeated name, or more than"
                                    << static_cast<int>(MaxSavedCasters);
    }
    if (casters != _savedCasters) {
        _savedCasters = casters;
        emit savedCastersChanged();
    }
}

void NTRIPSettings::_storeSavedCasters(const QList<NTRIPSavedCaster>& casters)
{
    QJsonArray entries;
    for (const NTRIPSavedCaster& caster : casters) {
        entries.append(savedCasterToJson(caster));
    }
    // The Fact's change loads the casters back.
    ntripSavedCasters()->setRawValue(
        casters.isEmpty() ? QString() : QString::fromUtf8(QJsonDocument(entries).toJson(QJsonDocument::Compact)));
}

DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerConnectEnabled)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerHostAddress)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerPort)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUsername)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripPassword)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripMountpoint)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUseTls)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripAllowSelfSignedCerts)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripPinnedCertificate)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripGgaPositionSource)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripGgaIntervalSec)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripVersion1Casters)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripSavedCasters)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripFailoverEnabled)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripRequireVehicle)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripDataCapMB)
