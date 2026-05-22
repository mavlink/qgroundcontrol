#include "MAVLinkSigningKeys.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QRandomGenerator>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtNetwork/QPasswordDigestor>
#include <array>

#include "LinkInterface.h"
#include "LinkManager.h"
#include "MAVLinkSigning.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SecureMemory.h"
#include "SigningController.h"
#include "Vehicle.h"
#include "VehicleSigningController.h"

QGC_LOGGING_CATEGORY(MAVLinkSigningKeysLog, "MAVLink.SigningKeys")

MAVLinkSigningKey::MAVLinkSigningKey(const QString& name, const MAVLinkSigning::SigningKey& keyBytes, QObject* parent)
    : QObject(parent), _name(name), _keyBytes(keyBytes)
{
    qCDebug(MAVLinkSigningKeysLog) << "MAVLinkSigningKey ctor:" << _name;
}

MAVLinkSigningKey::~MAVLinkSigningKey()
{
    QGC::secureZero(_keyBytes);
    qCDebug(MAVLinkSigningKeysLog) << "MAVLinkSigningKey dtor:" << _name;
}

Q_APPLICATION_STATIC(MAVLinkSigningKeys, _mavlinkSigningKeysInstance);

MAVLinkSigningKeys* MAVLinkSigningKeys::instance()
{
    return _mavlinkSigningKeysInstance();
}

MAVLinkSigningKeys::MAVLinkSigningKeys(QObject* parent) : QObject(parent), _keys(new QmlObjectListModel(this))
{
    qCDebug(MAVLinkSigningKeysLog) << "MAVLinkSigningKeys ctor";

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

    // Q_APPLICATION_STATIC destruction order is undefined relative to LinkManager — flush before either gets torn down.
    if (auto* app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this, &MAVLinkSigningKeys::flushAllTimestamps);
    }
}

MAVLinkSigningKeys::~MAVLinkSigningKeys()
{
    qCDebug(MAVLinkSigningKeysLog) << "MAVLinkSigningKeys dtor";
}

bool MAVLinkSigningKeys::isKeyInUse(const QString& name) const
{
    auto* mvm = MultiVehicleManager::instance();
    for (int i = 0; i < mvm->vehicles()->count(); ++i) {
        const auto* vehicle = mvm->vehicles()->value<Vehicle*>(i);
        if (vehicle && vehicle->signingController() && vehicle->signingController()->signingStatus().keyName == name) {
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
    if (passphrase.size() < kMinPassphraseLength) {
        qCWarning(MAVLinkSigningKeysLog) << "Passphrase must be at least" << kMinPassphraseLength << "characters";
        return false;
    }
    if (!_validateNewKey(name)) {
        return false;
    }

    // Fixed app salt → deterministic across installs: same passphrase yields same key on every GCS sharing the vehicle.
    const QByteArray salt(kPbkdf2Salt.constData(), kPbkdf2Salt.size());
    QByteArray utf8 = passphrase.toUtf8();
    QByteArray derived = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, utf8, salt,
                                                            _effectivePbkdf2Iterations(), kSigningKeySize);
    QGC::secureZero(utf8);
    const auto key = MAVLinkSigning::makeSigningKey(derived);
    QGC::secureZero(derived);
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
    QByteArray bytes(reinterpret_cast<const char*>(key->data()), key->size());
    QByteArray hex = bytes.toHex();
    QString result = QString::fromLatin1(hex);
    // Wipe both intermediates; toHex() output also contains the key in encoded form.
    QGC::secureZero(bytes);
    QGC::secureZero(hex);
    return result;
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
    // No cross-process lock: RunGuard enforces single-instance, so the keystore has no concurrent writer.
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    const QStringList previousNames = settings.value(kManifestKey).toStringList();

    QStringList manifestNames;
    for (int i = 0; i < _keys->count(); ++i) {
        const auto* key = _keys->value<MAVLinkSigningKey*>(i);
        const auto& bytes = key->keyBytes();
        QByteArray serialized(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));
        settings.setValue(QString("%1/%2").arg(kKeySubgroup, key->name()), serialized);
        QGC::secureZero(serialized);
        manifestNames.append(key->name());
    }

    const QSet<QString> liveNameSet(manifestNames.constBegin(), manifestNames.constEnd());
    for (const QString& oldName : previousNames) {
        if (!liveNameSet.contains(oldName)) {
            settings.remove(QString("%1/%2").arg(kKeySubgroup, oldName));
            settings.remove(QString("%1/%2").arg(kTimestampSubgroup, oldName));
        }
    }

    settings.setValue(kManifestKey, manifestNames);
    settings.endGroup();
    // Explicit sync: persist before a crash between here and QSettings destruction.
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
    recordTimestamps(_snapshotAllTimestamps());
}

QHash<QString, uint64_t> MAVLinkSigningKeys::_snapshotAllTimestamps() const
{
    QHash<QString, uint64_t> batch;
    const auto links = LinkManager::instance()->links();
    for (const auto& link : links) {
        if (!link) {
            continue;
        }
        const SigningController* const ctrl = link->signing();
        if (!ctrl) {
            continue;
        }
        const auto snap = ctrl->channel().currentTimestampAndName();
        if (snap.keyName.isEmpty() || snap.timestamp == 0) {
            continue;
        }
        auto& slot = batch[snap.keyName];
        if (snap.timestamp > slot) {
            slot = snap.timestamp;
        }
    }
    return batch;
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
    const auto snap = controller->channel().detectSnapshot();
    if (snap.autoDetectSuspended || snap.inCooldown) {
        return QString();
    }

    // Strict matches explicit-enable; Permissive here would silently accept all unsigned and defeat enforcement.
    constexpr auto kPolicy = MAVLinkSigning::UnsignedAcceptancePolicy::Strict;

    const QString& hintName = snap.keyHint;
    if (!hintName.isEmpty()) {
        if (const auto hintKey = keyBytesByName(hintName);
            hintKey && MAVLinkSigning::verifySignature(*hintKey, message)) {
            const QByteArrayView kv(reinterpret_cast<const char*>(hintKey->data()), hintKey->size());
            if (controller->initSigningImmediate(kv, kPolicy, hintName)) {
                controller->clearDetectCooldown();
                qCDebug(MAVLinkSigningKeysLog) << "Auto-detected signing key" << hintName << "(cached hint)";
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
            if (controller->initSigningImmediate(kv, kPolicy, entry->name())) {
                controller->clearDetectCooldown();
                qCDebug(MAVLinkSigningKeysLog) << "Auto-detected signing key" << entry->name();
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
        QByteArray keyBytes = settings.value(QString("%1/%2").arg(kKeySubgroup, name)).toByteArray();
        if (const auto key = MAVLinkSigning::makeSigningKey(keyBytes); !name.isEmpty() && key) {
            auto* inserted = _insertKey(name, *key);
            const uint64_t ts = settings.value(QString("%1/%2").arg(kTimestampSubgroup, name)).toULongLong();
            if (ts > 0) {
                inserted->setLastTimestamp(ts);
            }
        } else if (!keyBytes.isEmpty()) {
            qCWarning(MAVLinkSigningKeysLog) << "Skipping malformed key entry:" << name;
        }
        QGC::secureZero(keyBytes);
    }
    settings.endGroup();
}
