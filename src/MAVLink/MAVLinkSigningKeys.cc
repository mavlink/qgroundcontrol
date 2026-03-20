#include "MAVLinkSigningKeys.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(MAVLinkSigningKeysLog, "MAVLink.SigningKeys")

Q_APPLICATION_STATIC(MAVLinkSigningKeys, _mavlinkSigningKeysInstance);

// ── MAVLinkSigningKey ──────────────────────────────────────────────────────

MAVLinkSigningKey::MAVLinkSigningKey(const QString& name, const QByteArray& keyBytes, QObject* parent)
    : QObject(parent)
    , _name(name)
    , _keyBytes(keyBytes)
{
}

// ── MAVLinkSigningKeys ─────────────────────────────────────────────────────

MAVLinkSigningKeys* MAVLinkSigningKeys::instance()
{
    return _mavlinkSigningKeysInstance();
}

MAVLinkSigningKeys::MAVLinkSigningKeys(QObject* parent)
    : QObject(parent)
    , _keys(new QmlObjectListModel(this))
{
    _load();

    auto* mvm = MultiVehicleManager::instance();
    connect(mvm, &MultiVehicleManager::vehicleAdded, this, &MAVLinkSigningKeys::_connectVehicle);
    connect(mvm, &MultiVehicleManager::vehicleRemoved, this, &MAVLinkSigningKeys::_disconnectVehicle);

    // Wire any vehicles that already exist (in case singleton was created late)
    for (int i = 0; i < mvm->vehicles()->count(); ++i) {
        _connectVehicle(mvm->vehicles()->value<Vehicle*>(i));
    }
}

MAVLinkSigningKeys::~MAVLinkSigningKeys()
{
}

bool MAVLinkSigningKeys::isKeyInUse(const QString& name) const
{
    auto* mvm = MultiVehicleManager::instance();
    for (int i = 0; i < mvm->vehicles()->count(); ++i) {
        const auto* vehicle = mvm->vehicles()->value<Vehicle*>(i);
        if (vehicle && vehicle->mavlinkSigningKeyName() == name) {
            return true;
        }
    }
    return false;
}

void MAVLinkSigningKeys::_connectVehicle(Vehicle* vehicle)
{
    connect(vehicle, &Vehicle::mavlinkSigningChanged, this, &MAVLinkSigningKeys::keyUsageChanged);
    emit keyUsageChanged();
}

void MAVLinkSigningKeys::_disconnectVehicle(Vehicle* vehicle)
{
    disconnect(vehicle, &Vehicle::mavlinkSigningChanged, this, &MAVLinkSigningKeys::keyUsageChanged);
    emit keyUsageChanged();
}

QByteArray MAVLinkSigningKeys::keyBytesAt(int index) const
{
    if (index >= 0 && index < _keys->count()) {
        return _keys->value<MAVLinkSigningKey*>(index)->keyBytes();
    }
    return QByteArray();
}

QString MAVLinkSigningKeys::keyNameAt(int index) const
{
    if (index >= 0 && index < _keys->count()) {
        return _keys->value<MAVLinkSigningKey*>(index)->name();
    }
    return QString();
}

QByteArray MAVLinkSigningKeys::keyBytesByName(const QString& name) const
{
    for (int i = 0; i < _keys->count(); ++i) {
        const auto* key = _keys->value<MAVLinkSigningKey*>(i);
        if (key->name() == name) {
            return key->keyBytes();
        }
    }
    return QByteArray();
}

bool MAVLinkSigningKeys::_keyExists(const QString& name) const
{
    for (int i = 0; i < _keys->count(); ++i) {
        if (_keys->value<MAVLinkSigningKey*>(i)->name() == name) {
            return true;
        }
    }
    return false;
}

void MAVLinkSigningKeys::addKey(const QString& name, const QString& passphrase)
{
    if (name.isEmpty() || passphrase.isEmpty()) {
        qCWarning(MAVLinkSigningKeysLog) << "Name and passphrase must not be empty";
        return;
    }

    // Check for duplicate name
    for (int i = 0; i < _keys->count(); ++i) {
        if (_keys->value<MAVLinkSigningKey*>(i)->name() == name) {
            qCWarning(MAVLinkSigningKeysLog) << "Key with name already exists:" << name;
            return;
        }
    }

    const QByteArray hash = QCryptographicHash::hash(passphrase.toUtf8(), QCryptographicHash::Sha256);

    auto* key = new MAVLinkSigningKey(name, hash, _keys);
    _keys->append(key);

    _save();
    emit keysChanged();
}

void MAVLinkSigningKeys::removeKey(int index)
{
    if (index < 0 || index >= _keys->count()) {
        return;
    }

    _keys->removeAt(index)->deleteLater();
    _save();
    emit keysChanged();
}

void MAVLinkSigningKeys::_save()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    settings.beginWriteArray(kKeysArrayKey);
    for (int i = 0; i < _keys->count(); ++i) {
        settings.setArrayIndex(i);
        const auto* key = _keys->value<MAVLinkSigningKey*>(i);
        settings.setValue(kNameKey, key->name());
        settings.setValue(kKeyBytesKey, key->keyBytes().toHex());
    }
    settings.endArray();

    settings.endGroup();
}

void MAVLinkSigningKeys::_load()
{
    _keys->clearAndDeleteContents();

    bool migrated = false;
    QSettings settings;

    // Load existing named keys first so _keyExists() sees them during migration.
    settings.beginGroup(kSettingsGroup);

    const int count = settings.beginReadArray(kKeysArrayKey);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        const QString name = settings.value(kNameKey).toString();
        const QByteArray keyBytes = QByteArray::fromHex(settings.value(kKeyBytesKey).toByteArray());
        if (!name.isEmpty() && keyBytes.size() == 32) {
            _keys->append(new MAVLinkSigningKey(name, keyBytes, _keys));
        }
    }
    settings.endArray();

    settings.endGroup();

    // Migrate old single signing key from the pre-named-keys Fact system.
    // The old "mavlink2SigningKey" was stored as a raw passphrase at the QSettings root level.
    if (settings.contains(kOldSigningKeySettingsKey)) {
        const QString oldPassphrase = settings.value(kOldSigningKeySettingsKey).toString();
        settings.remove(kOldSigningKeySettingsKey);
        if (!oldPassphrase.isEmpty()) {
            QString migratedName = QStringLiteral("Migrated Key");
            int suffix = 1;
            while (_keyExists(migratedName)) {
                migratedName = QStringLiteral("Migrated Key %1").arg(suffix++);
            }
            const QByteArray keyBytes = QCryptographicHash::hash(oldPassphrase.toUtf8(), QCryptographicHash::Sha256);
            _keys->append(new MAVLinkSigningKey(migratedName, keyBytes, _keys));
            migrated = true;
            qCDebug(MAVLinkSigningKeysLog) << "Migrated legacy signing key to named key system";
        }
        settings.sync();
    }

    if (migrated) {
        _save();
    }
}
