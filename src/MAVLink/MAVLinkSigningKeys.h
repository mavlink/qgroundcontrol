#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class QmlObjectListModel;
class Vehicle;

/// A single named signing key entry
class MAVLinkSigningKey : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString name     READ name     CONSTANT)

public:
    explicit MAVLinkSigningKey(const QString& name, const QByteArray& keyBytes, QObject* parent = nullptr);

    QString    name()     const { return _name; }
    QByteArray keyBytes() const { return _keyBytes; }

private:
    QString    _name;
    QByteArray _keyBytes; // 32-byte SHA-256 hash
};

/// Manages the collection of named MAVLink signing keys.
/// There is no "active key" concept. Keys are stored as a bag. The correct key
/// for each vehicle is auto-detected from incoming signed packets.
class MAVLinkSigningKeys : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QmlObjectListModel* keys READ keys CONSTANT)

public:
    static MAVLinkSigningKeys* instance();

    explicit MAVLinkSigningKeys(QObject* parent = nullptr);
    ~MAVLinkSigningKeys() override;

    QmlObjectListModel* keys() const { return _keys; }

    /// Add a new named key. The passphrase is SHA-256 hashed; only the hash is stored.
    Q_INVOKABLE void addKey(const QString& name, const QString& passphrase);

    /// Remove a key by index
    Q_INVOKABLE void removeKey(int index);

    /// Returns true if any connected vehicle is using the key with the given name
    Q_INVOKABLE bool isKeyInUse(const QString& name) const;

    /// Returns the key bytes for a key at the given index, or empty if invalid
    QByteArray keyBytesAt(int index) const;

    /// Returns the key bytes for the key with the given name, or empty if not found
    QByteArray keyBytesByName(const QString& name) const;

    /// Returns the key name at the given index, or empty if invalid
    QString keyNameAt(int index) const;

signals:
    void keysChanged();
    void keyUsageChanged();

private:
    friend class SigningTest;

    void _save();
    void _load();
    bool _keyExists(const QString& name) const;
    void _connectVehicle(Vehicle* vehicle);
    void _disconnectVehicle(Vehicle* vehicle);

    QmlObjectListModel* _keys = nullptr;

    static constexpr const char* kSettingsGroup = "MAVLinkSigningKeys";
    static constexpr const char* kKeysArrayKey  = "keys";
    static constexpr const char* kNameKey       = "name";
    static constexpr const char* kKeyBytesKey   = "keyBytes";

    // Old Fact-based signing key (pre-named-keys system)
    static constexpr const char* kOldSigningKeySettingsKey = "mavlink2SigningKey";
};

