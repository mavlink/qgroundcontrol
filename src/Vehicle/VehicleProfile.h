#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(VehicleProfileLog)

/// VehicleProfile manages vehicle-specific configuration and settings profiles.
/// This allows users to save and restore vehicle configurations, preferences,
/// and custom settings for different vehicles or flight scenarios.
class VehicleProfile : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString  name            READ name           WRITE setName       NOTIFY nameChanged)
    Q_PROPERTY(QString  description     READ description    WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(bool     isActive        READ isActive       NOTIFY isActiveChanged)
    Q_PROPERTY(Vehicle* vehicle         READ vehicle        CONSTANT)

public:
    explicit VehicleProfile(Vehicle* vehicle, QObject* parent = nullptr);
    ~VehicleProfile() override;

    /// Load profile from settings
    Q_INVOKABLE bool loadProfile(const QString& profileName);

    /// Save current profile to settings
    Q_INVOKABLE bool saveProfile();

    /// Delete a saved profile
    Q_INVOKABLE bool deleteProfile(const QString& profileName);

    /// Get list of available profile names
    [[nodiscard]] Q_INVOKABLE QStringList availableProfiles() const;

    /// Apply this profile to the vehicle
    Q_INVOKABLE void applyProfile();

    // Property accessors
    [[nodiscard]] QString name() const { return _name; }
    void setName(const QString& name);

    [[nodiscard]] QString description() const { return _description; }
    void setDescription(const QString& description);

    [[nodiscard]] bool isActive() const { return _isActive; }

    [[nodiscard]] Vehicle* vehicle() const { return _vehicle; }

signals:
    void nameChanged();
    void descriptionChanged();
    void isActiveChanged();
    void profileLoaded(const QString& profileName);
    void profileSaved(const QString& profileName);
    void profileDeleted(const QString& profileName);

private:
    void _setIsActive(bool active);

    QString     _name;
    QString     _description;
    bool        _isActive       = false;
    Vehicle*    _vehicle        = nullptr;
};
