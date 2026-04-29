#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QLatin1StringView>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>
#include <optional>

#include "MAVLinkSigning.h"

class QmlObjectListModel;
class SigningController;
class Vehicle;

/// A single named signing key entry
class MAVLinkSigningKey : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString name READ name CONSTANT)

public:
    explicit MAVLinkSigningKey(const QString& name, const MAVLinkSigning::SigningKey& keyBytes,
                               QObject* parent = nullptr);
    ~MAVLinkSigningKey() override;

    const QString& name() const { return _name; }

    const MAVLinkSigning::SigningKey& keyBytes() const { return _keyBytes; }

    /// 10µs ticks since 2015-01-01; persisted for forward-progress across restarts with skewed clock.
    uint64_t lastTimestamp() const { return _lastTimestamp; }
    void setLastTimestamp(uint64_t ts) { _lastTimestamp = ts; }

private:
    const QString _name;
    MAVLinkSigning::SigningKey _keyBytes;  // 32-byte SHA-256 hash — secureZero on destruction
    uint64_t _lastTimestamp = 0;
};

/// Manages the collection of named MAVLink signing keys.
/// There is no "active key" concept. Keys are stored as a bag. The correct key
/// for each vehicle is auto-detected from incoming signed packets.
class MAVLinkSigningKeys : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QmlObjectListModel* keys READ keys CONSTANT)
    Q_PROPERTY(int keyUsageRevision READ keyUsageRevision NOTIFY keyUsageChanged)

    friend class SigningTest;

public:
    static MAVLinkSigningKeys* instance();

    explicit MAVLinkSigningKeys(QObject* parent = nullptr);
    ~MAVLinkSigningKeys() override;

    /// Add a new named key. The passphrase is SHA-256 hashed; only the hash is stored.
    /// Returns false on duplicate name, empty passphrase, or capacity limit; QML uses this to keep the dialog open.
    Q_INVOKABLE bool addKey(const QString& name, const QString& passphrase);

    /// Add a key from raw 32-byte hex string (e.g. from a key file or shared secret)
    /// Returns false on duplicate name, malformed hex, or capacity limit.
    Q_INVOKABLE bool addRawKey(const QString& name, const QString& hexKey);

    /// Generate a cryptographically random 64-char hex string (32 bytes)
    Q_INVOKABLE static QString generateRandomHexKey();

    /// Remove a key by name
    Q_INVOKABLE void removeKey(const QString& name);

    /// Remove all keys (used by tests and full reset)
    Q_INVOKABLE void removeAllKeys();

    /// Returns true if any connected vehicle is using the key with the given name
    Q_INVOKABLE bool isKeyInUse(const QString& name) const;

    /// Returns the hex-encoded key bytes for export (empty if not found)
    Q_INVOKABLE QString keyHexByName(const QString& name) const;

    /// Returns the key entry at the given index, or nullptr if invalid
    MAVLinkSigningKey* keyAt(int index) const;

    /// Returns the key bytes for the key with the given name, or nullopt if not found.
    std::optional<MAVLinkSigning::SigningKey> keyBytesByName(const QString& name) const;

    /// Last persisted signing timestamp for `name`, or 0 if unknown / no entry.
    uint64_t lastTimestamp(const QString& name) const;

    /// Update the in-memory + persisted last-timestamp for `name`. Monotonic — older values are dropped.
    void recordTimestamp(const QString& name, uint64_t ts);

    /// Batch update with single QSettings + sync; per-entry monotonic guard still applies.
    void recordTimestamps(const QHash<QString, uint64_t>& batch);

    /// Walk every signing channel and persist its current timestamp under the active key's name.
    void flushAllTimestamps();

    /// Try every stored key against `message`'s signature on `channel`. On match, configures
    /// the channel via initSigning. Returns the matching key name, or empty string if no match.
    QString tryDetectKey(SigningController* controller, const mavlink_message_t& message);

    QmlObjectListModel* keys() const { return _keys; }

    int keyUsageRevision() const { return _keyUsageRevision; }

signals:
    void keysChanged();
    void keyUsageChanged();

private:
    void _save();
    void _load();
    bool _validateNewKey(const QString& name) const;
    MAVLinkSigningKey* _insertKey(const QString& name, const MAVLinkSigning::SigningKey& keyBytes);
    void _connectVehicle(Vehicle* vehicle);
    void _disconnectVehicle(Vehicle* vehicle);

    QmlObjectListModel* _keys = nullptr;
    QHash<QString, MAVLinkSigningKey*> _keyIndex;  // O(1) name lookups alongside QML model
    int _keyUsageRevision = 0;
    QTimer* _timestampFlushTimer = nullptr;

    static constexpr QLatin1StringView kSettingsGroup = QLatin1StringView("MAVLinkSigningKeys");
    static constexpr QLatin1StringView kManifestKey = QLatin1StringView("manifest");  // key names list (no secrets)
    static constexpr QLatin1StringView kKeychainKeyPrefix = QLatin1StringView("MAVLinkSigningKeys/");
    static constexpr QLatin1StringView kTimestampSubgroup = QLatin1StringView("timestamps");  // <kSettingsGroup>/timestamps/<name> = quint64 (10µs ticks since 2015)
    static constexpr int kTimestampFlushIntervalMs = 5000;

    static constexpr int kSigningKeySize = MAVLinkSigning::kSigningKeySize;
    static constexpr int kMaxKeys = 64;
};
