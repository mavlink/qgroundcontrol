#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
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

/// \brief A single named signing key entry.
///
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
    MAVLinkSigning::SigningKey _keyBytes;  // 32-byte derived key — secureZero on destruction
    uint64_t _lastTimestamp = 0;
};

/// \brief Bag of named MAVLink signing keys; correct key per vehicle is auto-detected from incoming signed packets.
///
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

    /// Derives a 32-byte key via PBKDF2-HMAC-SHA256 with a fixed app salt — deterministic across installs so
    /// the same passphrase yields the same key on multiple GCS stations sharing one vehicle.
    /// Returns false on duplicate name, passphrase below kMinPassphraseLength, or capacity limit.
    Q_INVOKABLE bool addKey(const QString& name, const QString& passphrase);

    /// Add a key from raw 32-byte hex string (e.g. from a key file or shared secret).
    /// Returns false on duplicate name, malformed hex, or capacity limit.
    Q_INVOKABLE bool addRawKey(const QString& name, const QString& hexKey);

    /// Cryptographically random 64-char hex string (32 bytes).
    Q_INVOKABLE static QString generateRandomHexKey();

    Q_INVOKABLE void removeKey(const QString& name);

    /// Used by tests and full reset.
    Q_INVOKABLE void removeAllKeys();

    /// True if any connected vehicle is using the key with the given name.
    Q_INVOKABLE bool isKeyInUse(const QString& name) const;

    /// Hex-encoded key bytes for export (empty if not found).
    Q_INVOKABLE QString keyHexByName(const QString& name) const;

    /// Key entry at the given index, or nullptr if invalid.
    MAVLinkSigningKey* keyAt(int index) const;

    /// Key bytes for the key with the given name, or nullopt if not found.
    std::optional<MAVLinkSigning::SigningKey> keyBytesByName(const QString& name) const;

    /// Last persisted signing timestamp for `name`, or 0 if unknown / no entry.
    uint64_t lastTimestamp(const QString& name) const;

    /// Update in-memory + persisted last-timestamp for `name`. Monotonic — older values are dropped.
    void recordTimestamp(const QString& name, uint64_t ts);

    /// Batch update with single QSettings + sync; per-entry monotonic guard still applies.
    void recordTimestamps(const QHash<QString, uint64_t>& batch);

    /// Walk every signing channel and persist its current timestamp under the active key's name.
    void flushAllTimestamps();

    /// Try every stored key against `message`'s signature; on match, configures `channel` and returns the key name.
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
    QHash<QString, uint64_t> _snapshotAllTimestamps() const;

    QmlObjectListModel* _keys = nullptr;
    QHash<QString, MAVLinkSigningKey*> _keyIndex;  // O(1) name lookups alongside QML model
    int _keyUsageRevision = 0;
    QTimer* _timestampFlushTimer = nullptr;

    static constexpr QLatin1StringView kSettingsGroup = QLatin1StringView("MAVLinkSigningKeys");
    static constexpr QLatin1StringView kManifestKey = QLatin1StringView("manifest");  // key names list (no secrets)
    static constexpr QLatin1StringView kKeySubgroup = QLatin1StringView("keys");  // <kSettingsGroup>/keys/<name> = key bytes
    static constexpr QLatin1StringView kTimestampSubgroup = QLatin1StringView("timestamps");  // <kSettingsGroup>/timestamps/<name> = quint64
    static constexpr int kTimestampFlushIntervalMs = 5000;

    /// Fixed app-wide salt — keeps passphrase→key derivation deterministic across installs (portable shared secret).
    /// Bumping the version suffix invalidates all stored passphrase keys; coordinate with deployment.
    static constexpr QByteArrayView kPbkdf2Salt = QByteArrayView("QGroundControl-MAVLink-Signing-v1");
    /// OWASP 2023+ floor for PBKDF2-HMAC-SHA256; ~250ms per addKey on a 2024 laptop.
    static constexpr int kPbkdf2Iterations = 600'000;

    static constexpr int kSigningKeySize = MAVLinkSigning::kSigningKeySize;
    static constexpr int kMaxKeys = 64;
    /// Mirrored in QML acceptButtonEnabled — keep in sync.
    static constexpr int kMinPassphraseLength = 8;

public:
    /// Test-only override for PBKDF2 iteration count; 0 restores production default.
    static void setPbkdf2IterationsForTesting(int iterations) { _pbkdf2IterationsOverride = iterations; }

private:
    static int _effectivePbkdf2Iterations() {
        return _pbkdf2IterationsOverride > 0 ? _pbkdf2IterationsOverride : kPbkdf2Iterations;
    }
    static inline int _pbkdf2IterationsOverride = 0;
};
