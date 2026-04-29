#include "MAVLinkSigningKeys.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QLockFile>
#include <QtCore/QRandomGenerator>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <array>

#include "MAVLinkSigning.h"
#include "SigningController.h"
#include "VehicleSigningController.h"
#include "MultiVehicleManager.h"
#include "QGCKeychain.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SecureMemory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(MAVLinkSigningKeysLog, "MAVLink.SigningKeys")

namespace {

// Cross-process lock; QLockFile auto-detects stale PIDs (crash recovery).
QString _keystoreLockPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }
    QDir().mkpath(dir);
    return dir + QLatin1String("/mavlink-signing-keys.lock");
}

constexpr int kKeystoreLockTimeoutMs = 5000;

}  // namespace

MAVLinkSigningKey::MAVLinkSigningKey(const QString& name, const MAVLinkSigning::SigningKey& keyBytes, QObject* parent)
    : QObject(parent), _name(name), _keyBytes(keyBytes)
{
    qCDebug(MAVLinkSigningKeysLog) << this;
}

MAVLinkSigningKey::~MAVLinkSigningKey()
{
    QGC::secureZero(_keyBytes);

    qCDebug(MAVLinkSigningKeysLog) << this;
}

Q_APPLICATION_STATIC(MAVLinkSigningKeys, _mavlinkSigningKeysInstance);

MAVLinkSigningKeys* MAVLinkSigningKeys::instance()
{
    return _mavlinkSigningKeysInstance();
}

MAVLinkSigningKeys::MAVLinkSigningKeys(QObject* parent) : QObject(parent), _keys(new QmlObjectListModel(this))
{
    qCDebug(MAVLinkSigningKeysLog) << this;

    _load();

    auto* mvm = MultiVehicleManager::instance();
    connect(mvm, &MultiVehicleManager::vehicleAdded, this, &MAVLinkSigningKeys::_connectVehicle);
    connect(mvm, &MultiVehicleManager::vehicleRemoved, this, &MAVLinkSigningKeys::_disconnectVehicle);

    // Singleton may be created after vehicles are connected.
    for (int i = 0; i < mvm->vehicles()->count(); ++i) {
        _connectVehicle(mvm->vehicles()->value<Vehicle*>(i));
    }

    _timestampFlushTimer = new QTimer(this);
    _timestampFlushTimer->setInterval(kTimestampFlushIntervalMs);
    connect(_timestampFlushTimer, &QTimer::timeout, this, &MAVLinkSigningKeys::flushAllTimestamps);
    _timestampFlushTimer->start();
}

MAVLinkSigningKeys::~MAVLinkSigningKeys()
{
    qCDebug(MAVLinkSigningKeysLog) << this;
}

bool MAVLinkSigningKeys::isKeyInUse(const QString& name) const
{
    auto* mvm = MultiVehicleManager::instance();
    for (int i = 0; i < mvm->vehicles()->count(); ++i) {
        const auto* vehicle = mvm->vehicles()->value<Vehicle*>(i);
        if (vehicle && vehicle->signingController() &&
            vehicle->signingController()->signingStatus().keyName == name) {
            return true;
        }
    }
    return false;
}

void MAVLinkSigningKeys::_connectVehicle(Vehicle* vehicle)
{
    if (auto* sc = vehicle->signingController()) {
        connect(sc, &VehicleSigningController::signingStatusChanged, this, &MAVLinkSigningKeys::keyUsageChanged);
    }
    ++_keyUsageRevision;
    emit keyUsageChanged();
}

void MAVLinkSigningKeys::_disconnectVehicle(Vehicle* vehicle)
{
    if (auto* sc = vehicle->signingController()) {
        disconnect(sc, &VehicleSigningController::signingStatusChanged, this, &MAVLinkSigningKeys::keyUsageChanged);
    }
    ++_keyUsageRevision;
    emit keyUsageChanged();
}

MAVLinkSigningKey* MAVLinkSigningKeys::keyAt(int index) const
{
    if (index >= 0 && index < _keys->count()) {
        return _keys->value<MAVLinkSigningKey*>(index);
    }
    return nullptr;
}

std::optional<MAVLinkSigning::SigningKey> MAVLinkSigningKeys::keyBytesByName(const QString& name) const
{
    const auto it = _keyIndex.constFind(name);
    if (it == _keyIndex.constEnd()) {
        return std::nullopt;
    }
    return it.value()->keyBytes();
}

MAVLinkSigningKey* MAVLinkSigningKeys::_insertKey(const QString& name, const MAVLinkSigning::SigningKey& keyBytes)
{
    auto* key = new MAVLinkSigningKey(name, keyBytes, _keys);
    _keys->append(key);
    _keyIndex.insert(name, key);
    return key;
}

bool MAVLinkSigningKeys::_validateNewKey(const QString& name) const
{
    if (name.isEmpty()) {
        qCWarning(MAVLinkSigningKeysLog) << "Key name must not be empty";
        return false;
    }
    if (_keys->count() >= kMaxKeys) {
        qCWarning(MAVLinkSigningKeysLog) << "Maximum key count reached:" << kMaxKeys;
        return false;
    }
    if (_keyIndex.contains(name)) {
        qCWarning(MAVLinkSigningKeysLog) << "Key with name already exists:" << name;
        return false;
    }
    return true;
}

bool MAVLinkSigningKeys::addKey(const QString& name, const QString& passphrase)
{
    if (passphrase.isEmpty()) {
        qCWarning(MAVLinkSigningKeysLog) << "Passphrase must not be empty";
        return false;
    }
    if (!_validateNewKey(name)) {
        return false;
    }

    QByteArray utf8 = passphrase.toUtf8();
    const QByteArray hash = QCryptographicHash::hash(utf8, QCryptographicHash::Sha256);
    QGC::secureZero(utf8);
    const auto key = MAVLinkSigning::makeSigningKey(hash);
    if (!key) {
        return false;
    }
    _insertKey(name, *key);
    _save();
    emit keysChanged();
    return true;
}

bool MAVLinkSigningKeys::addRawKey(const QString& name, const QString& hexKey)
{
    if (hexKey.isEmpty()) {
        qCWarning(MAVLinkSigningKeysLog) << "Hex key must not be empty";
        return false;
    }
    if (!_validateNewKey(name)) {
        return false;
    }

    const QByteArray keyBytes = QByteArray::fromHex(hexKey.toLatin1());
    const auto key = MAVLinkSigning::makeSigningKey(keyBytes);
    if (!key) {
        qCWarning(MAVLinkSigningKeysLog) << "Raw key must be exactly 32 bytes (64 hex chars), got" << keyBytes.size();
        return false;
    }

    _insertKey(name, *key);
    _save();
    emit keysChanged();
    return true;
}

QString MAVLinkSigningKeys::generateRandomHexKey()
{
    // Generate into aligned buffer then copy — QByteArray::data() may not be 4-byte aligned
    std::array<quint32, 8> aligned{};
    QRandomGenerator::system()->fillRange(aligned.data(), aligned.size());
    const QByteArray bytes(reinterpret_cast<const char*>(aligned.data()), sizeof(aligned));
    return QString::fromLatin1(bytes.toHex());
}

QString MAVLinkSigningKeys::keyHexByName(const QString& name) const
{
    const auto key = keyBytesByName(name);
    if (!key) {
        return {};
    }
    const QByteArray bytes(reinterpret_cast<const char*>(key->data()), key->size());
    return QString::fromLatin1(bytes.toHex());
}

void MAVLinkSigningKeys::removeKey(const QString& name)
{
    const auto it = _keyIndex.constFind(name);
    if (it == _keyIndex.constEnd()) {
        return;
    }
    MAVLinkSigningKey* const entry = it.value();
    _keyIndex.erase(it);
    if (auto* removed = _keys->removeOne(entry)) {
        removed->deleteLater();
    }
    _save();
    emit keysChanged();
}

void MAVLinkSigningKeys::removeAllKeys()
{
    if (_keys->count() == 0) {
        return;
    }

    _keyIndex.clear();
    _keys->clearAndDeleteContents();
    _save();
    emit keysChanged();
}

void MAVLinkSigningKeys::_save()
{
    QLockFile lock(_keystoreLockPath());
    if (!lock.tryLock(kKeystoreLockTimeoutMs)) {
        // Best-effort: a torn write is better than silently dropping key data.
        qCWarning(MAVLinkSigningKeysLog) << "Could not acquire keystore lock; proceeding without it";
    }

    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    const QStringList previousNames = settings.value(kManifestKey).toStringList();
    settings.endGroup();

    // Manifest only records names whose keychain write succeeded; otherwise _load() would drop the key silently.
    QStringList currentNames;
    for (int i = 0; i < _keys->count(); ++i) {
        const auto* key = _keys->value<MAVLinkSigningKey*>(i);
        const auto& bytes = key->keyBytes();
        QByteArray serialized(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));
        const bool ok = QGCKeychain::writeBinary(kKeychainKeyPrefix + key->name(), serialized);
        QGC::secureZero(serialized);
        if (ok) {
            currentNames.append(key->name());
        } else {
            qCWarning(MAVLinkSigningKeysLog) << "Failed to save key to keychain:" << key->name();
        }
    }

    const QSet<QString> currentNameSet(currentNames.constBegin(), currentNames.constEnd());
    for (const QString& oldName : previousNames) {
        if (!currentNameSet.contains(oldName)) {
            QGCKeychain::remove(kKeychainKeyPrefix + oldName);
        }
    }

    // Manifest contains names only; secrets live in keychain.
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kManifestKey, currentNames);

    for (const QString& oldName : previousNames) {
        if (!currentNameSet.contains(oldName)) {
            settings.remove(QString("%1/%2").arg(kTimestampSubgroup, oldName));
        }
    }
    settings.endGroup();
    // Explicit sync: crash between keychain writes and QSettings destruction would orphan entries.
    settings.sync();
}

uint64_t MAVLinkSigningKeys::lastTimestamp(const QString& name) const
{
    const auto it = _keyIndex.constFind(name);
    return it == _keyIndex.constEnd() ? 0 : it.value()->lastTimestamp();
}

void MAVLinkSigningKeys::recordTimestamps(const QHash<QString, uint64_t>& batch)
{
    if (batch.isEmpty()) {
        return;
    }
    QLockFile lock(_keystoreLockPath());
    if (!lock.tryLock(kKeystoreLockTimeoutMs)) {
        qCWarning(MAVLinkSigningKeysLog) << "Could not acquire keystore lock; proceeding without it";
    }
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    bool wroteAnything = false;
    for (auto it = batch.constBegin(); it != batch.constEnd(); ++it) {
        const auto keyIt = _keyIndex.constFind(it.key());
        if (keyIt == _keyIndex.constEnd()) {
            continue;
        }
        // Monotonic guard — protects against a stale snapshot from a slower flush path.
        if (it.value() <= keyIt.value()->lastTimestamp()) {
            continue;
        }
        keyIt.value()->setLastTimestamp(it.value());
        settings.setValue(QString("%1/%2").arg(kTimestampSubgroup, it.key()), QVariant::fromValue<quint64>(it.value()));
        wroteAnything = true;
    }
    settings.endGroup();
    if (wroteAnything) {
        settings.sync();
    }
}

void MAVLinkSigningKeys::recordTimestamp(const QString& name, uint64_t ts)
{
    recordTimestamps({{name, ts}});
}

void MAVLinkSigningKeys::flushAllTimestamps()
{
    recordTimestamps(MAVLinkSigning::snapshotAllTimestamps());
}

QString MAVLinkSigningKeys::tryDetectKey(SigningController* controller, const mavlink_message_t& message)
{
    if (!controller || !MAVLinkSigning::isMessageSigned(message)) {
        return QString();
    }
    // If signing is already configured on this controller, the C library already verified it.
    if (controller->isEnabled()) {
        return QString();
    }

    // Single-lock snapshot avoids TOCTOU vs MockLink's thread.
    const auto snap = controller->detectSnapshot();
    if (snap.autoDetectSuspended || snap.inCooldown) {
        return QString();
    }

    // Permissive: we can't know which components on the link actually sign; user can tighten explicitly.
    const auto callback = MAVLinkSigning::insecureConnectionAcceptUnsignedCallback;

    const QString& hintName = snap.keyHint;
    if (!hintName.isEmpty()) {
        if (const auto hintKey = keyBytesByName(hintName);
            hintKey && MAVLinkSigning::verifySignature(*hintKey, message)) {
            const QByteArrayView kv(reinterpret_cast<const char*>(hintKey->data()), hintKey->size());
            if (controller->initSigningImmediate(kv, callback, hintName)) {
                controller->clearDetectCooldown();
                qCInfo(MAVLinkSigningKeysLog)
                    << "Auto-detected signing key" << hintName << "(cached hint)";
                return hintName;
            }
        }
    }

    const int keyCount = _keys->count();
    for (int i = 0; i < keyCount; ++i) {
        const auto* entry = keyAt(i);
        if (!entry || entry->name() == hintName) {
            continue;
        }
        const auto& keyBytes = entry->keyBytes();
        if (MAVLinkSigning::verifySignature(keyBytes, message)) {
            const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
            if (controller->initSigningImmediate(kv, callback, entry->name())) {
                controller->clearDetectCooldown();
                qCInfo(MAVLinkSigningKeysLog)
                    << "Auto-detected signing key" << entry->name();
                return entry->name();
            }
        }
    }

    controller->recordDetectMiss();
    return QString();
}

void MAVLinkSigningKeys::_load()
{
    _keys->clearAndDeleteContents();
    _keyIndex.clear();

    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    const QStringList manifest = settings.value(kManifestKey).toStringList();
    for (const QString& name : manifest) {
        QByteArray keyBytes = QGCKeychain::readBinary(kKeychainKeyPrefix + name);
        if (const auto key = MAVLinkSigning::makeSigningKey(keyBytes); !name.isEmpty() && key) {
            auto* inserted = _insertKey(name, *key);
            const uint64_t ts = settings.value(QString("%1/%2").arg(kTimestampSubgroup, name)).toULongLong();
            if (ts > 0) {
                inserted->setLastTimestamp(ts);
            }
        } else if (!keyBytes.isEmpty()) {
            qCWarning(MAVLinkSigningKeysLog) << "Skipping malformed keychain entry:" << name;
        }
        QGC::secureZero(keyBytes);
    }
    settings.endGroup();
}
