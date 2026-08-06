#include "RemoteIDSettings.h"

#include <QtCore/QSettings>

#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(RemoteIDSettingsLog, "Settings.RemoteIDSettings")

DECLARE_SETTINGGROUP(RemoteID, "RemoteID")
{
    migrateLegacyOperatorID();

    // Reject UI writes of invalid EU operator IDs so a stored EU ID is always valid by construction
    operatorIDEU()->metaData()->setCustomCookedValidator(&RemoteIDSettings::_euOperatorIDCookedValidator);

    connect(operatorIDEU(), &Fact::rawValueChanged, this, &RemoteIDSettings::_operatorIDEUChanged);
    connect(operatorIDFAA(), &Fact::rawValueChanged, this, &RemoteIDSettings::_updateOperatorIDValidForRegion);
    connect(region(), &Fact::rawValueChanged, this, &RemoteIDSettings::_regionChanged);
    connect(basicID(), &Fact::rawValueChanged, this, &RemoteIDSettings::_updateBasicIDValidity);
    connect(basicIDType(), &Fact::rawValueChanged, this, &RemoteIDSettings::_updateBasicIDValidity);
    connect(basicIDUaType(), &Fact::rawValueChanged, this, &RemoteIDSettings::_updateBasicIDValidity);

    _updateOperatorIDValidForRegion();
    _updateBasicIDValidity();
}

void RemoteIDSettings::migrateLegacyOperatorID()
{
    QSettings qSettings;
    qSettings.beginGroup(settingsGroup);

    // Chain of legacy keys: operatorIDValid (shipped v4.4.0 - v5.0.x) was renamed to
    // operatorIDEUValid (never in a release), which was then replaced (along with the single
    // operatorID fact, shipped through v5.0.x) by the per-region facts.
    const QString legacyOperatorIDKey = QStringLiteral("operatorID");
    const QString legacyValidKey = QStringLiteral("operatorIDValid");
    const QString legacyEUValidKey = QStringLiteral("operatorIDEUValid");

    if (qSettings.contains(legacyOperatorIDKey)) {
        const QString legacyOperatorID = qSettings.value(legacyOperatorIDKey).toString();
        const bool legacyEUValid = qSettings.value(legacyEUValidKey, qSettings.value(legacyValidKey, false)).toBool();

        if (!legacyOperatorID.isEmpty()) {
            if (legacyEUValid) {
                // Keep only the 16-char public part: legacy stores could hold the full ID
                // including the secret chars, and the sanitize handler isn't connected yet
                operatorIDEU()->setRawValue(legacyOperatorID.left(16));
            } else {
                operatorIDFAA()->setRawValue(legacyOperatorID);
            }
        }
        qSettings.remove(legacyOperatorIDKey);
    }
    qSettings.remove(legacyValidKey);
    qSettings.remove(legacyEUValidKey);

    qSettings.endGroup();
}

QString RemoteIDSettings::_euOperatorIDCookedValidator(const QVariant& cookedValue)
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    const QString candidate = cookedValue.toString();

    // Clearing and no-op re-commits of the stored (sanitized) value are always legal
    if (candidate.isEmpty() || (candidate == settings->operatorIDEU()->rawValue().toString())) {
        return QString();
    }

    if (_isEUOperatorIDValid(candidate)) {
        return QString();
    }
    return tr("Invalid Operator ID. Enter the full 19 or 20 character ID including the 3 secret characters.");
}

void RemoteIDSettings::_operatorIDEUChanged()
{
    if (_updatingOperatorID) {
        return;
    }

    // A full EU operator ID passed validation on write. Keep only the public 16-character part:
    // the 3 secret characters must not be stored or broadcast.
    const QString currentOperatorID = operatorIDEU()->rawValue().toString();
    if ((currentOperatorID.length() > 16) && _isEUOperatorIDValid(currentOperatorID)) {
        _updatingOperatorID = true;
        operatorIDEU()->setRawValue(currentOperatorID.sliced(0, 16));
        _updatingOperatorID = false;
    }

    _updateOperatorIDValidForRegion();
}

void RemoteIDSettings::_regionChanged()
{
    const int newRegion = region()->rawValue().toInt();

    if (newRegion == static_cast<int>(RegionOperation::EU)) {
        // EU regulation requires broadcasting the operator ID
        sendOperatorID()->setRawValue(true);
    } else if (newRegion == static_cast<int>(RegionOperation::FAA)) {
        // FAA requires the live operator position, not a fixed location
        locationType()->setRawValue(static_cast<int>(LocationType::LIVE));
    }

    _updateOperatorIDValidForRegion();
}

void RemoteIDSettings::_updateOperatorIDValidForRegion()
{
    const bool isEURegion = (region()->rawValue().toInt() == static_cast<int>(RegionOperation::EU));
    const QString currentOperatorID = (isEURegion ? operatorIDEU() : operatorIDFAA())->rawValue().toString();

    // EU IDs are valid by construction (write-gated + sanitized), FAA IDs have no published
    // spec to validate against, so presence is the only requirement for either region.
    const bool validForRegion = !currentOperatorID.isEmpty();

    if (_operatorIDValidForRegion != validForRegion) {
        _operatorIDValidForRegion = validForRegion;
        emit operatorIDValidForRegionChanged();
    }
}

void RemoteIDSettings::_updateBasicIDValidity()
{
    // Type facts are uint8 enums, so any value is structurally valid: presence of the ID is the only requirement
    const bool valid = !basicID()->rawValue().toString().isEmpty();

    if (_basicIDValid != valid) {
        _basicIDValid = valid;
        emit basicIDValidChanged();
    }
}

bool RemoteIDSettings::_isEUOperatorIDValid(const QString& operatorID)
{
    // EN 4709-002 format: 3-letter country code + 12 chars [0-9a-z] + 1 checksum char,
    // optionally followed by '-' and 3 secret chars [0-9a-z]
    const bool containsDash = operatorID.contains('-');
    if (!(operatorID.length() == 20 && containsDash) && !(operatorID.length() == 19 && !containsDash)) {
        qCDebug(RemoteIDSettingsLog) << "OperatorID format mismatch: expected 20 chars with dash or 19 chars without";
        return false;
    }
    if (containsDash && (operatorID.at(16) != '-')) {
        qCDebug(RemoteIDSettingsLog) << "OperatorID dash separator not at expected position";
        return false;
    }

    const QString countryCode = operatorID.sliced(0, 3);
    for (const QChar& c : countryCode) {
        // ISO 3166-1 alpha-3 country codes are strictly ASCII A-Z; QChar::isUpper would
        // also accept Unicode uppercase, which the checksum doesn't cover
        if (c < QLatin1Char('A') || c > QLatin1Char('Z')) {
            qCDebug(RemoteIDSettingsLog) << "OperatorID country code must be 3 uppercase ASCII letters";
            return false;
        }
    }

    const QString number = operatorID.sliced(3, 12);
    const QChar checksum = operatorID.at(15);
    const QString secret = containsDash ? operatorID.sliced(17, 3) : operatorID.sliced(16, 3);
    const QString combination = number + secret + checksum;

    const QString alphabet = QStringLiteral("0123456789abcdefghijklmnopqrstuvwxyz");
    for (const QChar& c : combination) {
        if (!alphabet.contains(c)) {
            qCDebug(RemoteIDSettingsLog) << "OperatorID contains characters outside [0-9a-z]";
            return false;
        }
    }

    const QChar result = _calculateLuhnMod36(number + secret);

    const bool valid = (result == checksum);
    qCDebug(RemoteIDSettingsLog) << "Operator ID checksum" << (valid ? "valid" : "invalid");
    return valid;
}

QChar RemoteIDSettings::_calculateLuhnMod36(const QString& input)
{
    const int n = 36;
    const QString alphabet = QStringLiteral("0123456789abcdefghijklmnopqrstuvwxyz");

    int sum = 0;
    int factor = 2;

    for (int i = input.length() - 1; i >= 0; i--) {
        const int codePoint = alphabet.indexOf(input.at(i));
        int addend = factor * codePoint;
        factor = (factor == 2) ? 1 : 2;
        addend = (addend / n) + (addend % n);
        sum += addend;
    }

    const int remainder = sum % n;
    const int checkCodePoint = (n - remainder) % n;
    return alphabet.at(checkCodePoint);
}

DECLARE_SETTINGSFACT(RemoteIDSettings,  operatorIDEU)
DECLARE_SETTINGSFACT(RemoteIDSettings,  operatorIDFAA)
DECLARE_SETTINGSFACT(RemoteIDSettings,  operatorIDType)
DECLARE_SETTINGSFACT(RemoteIDSettings,  sendOperatorID)
DECLARE_SETTINGSFACT(RemoteIDSettings,  selfIDFree)
DECLARE_SETTINGSFACT(RemoteIDSettings,  selfIDEmergency)
DECLARE_SETTINGSFACT(RemoteIDSettings,  selfIDExtended)
DECLARE_SETTINGSFACT(RemoteIDSettings,  selfIDType)
DECLARE_SETTINGSFACT(RemoteIDSettings,  sendSelfID)
DECLARE_SETTINGSFACT(RemoteIDSettings,  basicID)
DECLARE_SETTINGSFACT(RemoteIDSettings,  basicIDType)
DECLARE_SETTINGSFACT(RemoteIDSettings,  basicIDUaType)
DECLARE_SETTINGSFACT(RemoteIDSettings,  sendBasicID)
DECLARE_SETTINGSFACT(RemoteIDSettings,  region)
DECLARE_SETTINGSFACT(RemoteIDSettings,  locationType)
DECLARE_SETTINGSFACT(RemoteIDSettings,  latitudeFixed)
DECLARE_SETTINGSFACT(RemoteIDSettings,  longitudeFixed)
DECLARE_SETTINGSFACT(RemoteIDSettings,  altitudeFixed)
DECLARE_SETTINGSFACT(RemoteIDSettings,  classificationType)
DECLARE_SETTINGSFACT(RemoteIDSettings,  categoryEU)
DECLARE_SETTINGSFACT(RemoteIDSettings,  classEU)
