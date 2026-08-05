#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class RemoteIDSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class RegionOperation {
        FAA,
        EU
    };
    Q_ENUM(RegionOperation)

    enum class LocationType {
        TAKEOFF,
        LIVE,
        FIXED
    };
    Q_ENUM(LocationType)

    enum class ClassificationType {
        UNDEFINED,
        EU
    };
    Q_ENUM(ClassificationType)

    RemoteIDSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    /// true: the current region's operator ID is set. EU IDs are valid by construction:
    /// writes are gated by an EN 4709-002 validator and sanitized to the 16-char public part.
    Q_PROPERTY(bool operatorIDValidForRegion READ operatorIDValidForRegion NOTIFY operatorIDValidForRegionChanged)
    bool operatorIDValidForRegion() const { return _operatorIDValidForRegion; }

    /// true: the basic ID entered in settings is complete enough to broadcast
    Q_PROPERTY(bool basicIDValid READ basicIDValid NOTIFY basicIDValidChanged)
    bool basicIDValid() const { return _basicIDValid; }

    /// Migrates legacy single-fact operator ID storage to the per-region facts.
    /// Required when upgrading from QGC v5.0.x or earlier (the single operatorID fact shipped
    /// through v5.0.x; operatorIDValid shipped in v4.4.0 - v5.0.x). Removable once upgrades
    /// from those versions no longer need to be supported.
    /// Idempotent: legacy QSettings keys are removed after migration.
    void migrateLegacyOperatorID();

    DEFINE_SETTINGFACT(operatorIDEU)
    DEFINE_SETTINGFACT(operatorIDFAA)
    DEFINE_SETTINGFACT(operatorIDType)
    DEFINE_SETTINGFACT(sendOperatorID)
    DEFINE_SETTINGFACT(selfIDFree)
    DEFINE_SETTINGFACT(selfIDEmergency)
    DEFINE_SETTINGFACT(selfIDExtended)
    DEFINE_SETTINGFACT(selfIDType)
    DEFINE_SETTINGFACT(sendSelfID)
    DEFINE_SETTINGFACT(basicID)
    DEFINE_SETTINGFACT(basicIDType)
    DEFINE_SETTINGFACT(basicIDUaType)
    DEFINE_SETTINGFACT(sendBasicID)
    DEFINE_SETTINGFACT(region)
    DEFINE_SETTINGFACT(locationType)
    DEFINE_SETTINGFACT(latitudeFixed)
    DEFINE_SETTINGFACT(longitudeFixed)
    DEFINE_SETTINGFACT(altitudeFixed)
    DEFINE_SETTINGFACT(classificationType)
    DEFINE_SETTINGFACT(categoryEU)
    DEFINE_SETTINGFACT(classEU)

signals:
    void operatorIDValidForRegionChanged();
    void basicIDValidChanged();

private:
    void _operatorIDEUChanged();
    void _regionChanged();
    void _updateOperatorIDValidForRegion();
    void _updateBasicIDValidity();

    /// CustomCookedValidator for operatorIDEU: rejects writes which fail EN 4709-002 validation.
    /// Empty values and no-op re-commits of the stored (sanitized) value are always accepted.
    static QString _euOperatorIDCookedValidator(const QVariant& cookedValue);

    /// Validates an EU operator ID per ASD-STAN EN 4709-002 (format + Luhn mod-36 checksum)
    static bool _isEUOperatorIDValid(const QString& operatorID);
    static QChar _calculateLuhnMod36(const QString& input);

    bool _operatorIDValidForRegion = false;
    bool _basicIDValid = false;
    bool _updatingOperatorID = false;
};
