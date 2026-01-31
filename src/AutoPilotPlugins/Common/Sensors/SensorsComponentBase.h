#pragma once

#include "VehicleComponent.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(SensorsComponentBaseLog)

/// Base class for APM and PX4 Sensors components.
/// Provides common functionality for sensor calibration status checking.
class SensorsComponentBase : public VehicleComponent
{
    Q_OBJECT

public:
    explicit SensorsComponentBase(Vehicle* vehicle, AutoPilotPlugin* autopilot,
                                  AutoPilotPlugin::KnownVehicleComponent knownComponent,
                                  QObject* parent = nullptr);
    ~SensorsComponentBase() override = default;

    // Common VehicleComponent overrides
    QString name() const override { return _name; }
    QString description() const override;
    QString iconResource() const override { return QStringLiteral("/qmlimages/SensorsComponentIcon.png"); }
    bool requiresSetup() const override { return true; }

protected:
    /// Helper to check if a parameter exists and has non-zero value
    bool parameterNonZero(const QString& param) const;

    /// Helper to check if a parameter exists and equals a specific value
    bool parameterEquals(const QString& param, const QVariant& value) const;

    /// Helper to check if a parameter exists
    bool parameterExists(const QString& param) const;

    /// Get parameter value (returns 0 if not exists)
    QVariant parameterValue(const QString& param) const;

    const QString _name;
};
