/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>

class FirmwarePlugin;
class Vehicle;
class VehicleComponent;

Q_DECLARE_LOGGING_CATEGORY(AutoPilotPluginLog)

/// The AutoPilotPlugin class is an abstract base class which represent the methods and objects
/// which are specific to a certain AutoPilot. This is the only place where AutoPilot specific
/// code should reside in QGroundControl. The remainder of the QGroundControl source is
/// generic to a common mavlink implementation.
class AutoPilotPlugin : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList vehicleComponents   READ vehicleComponents  NOTIFY vehicleComponentsChanged)    ///< List of VehicleComponent objects
    Q_PROPERTY(bool         setupComplete       READ setupComplete      NOTIFY setupCompleteChanged)        ///< false: One or more vehicle components require setup

public:
    explicit AutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);
    virtual ~AutoPilotPlugin();

    // Vehicle Components which are available for firmware types
    enum KnownVehicleComponent {
        KnownRadioVehicleComponent,
        KnownFlightModesVehicleComponent,
        KnownSensorsVehicleComponent,
        KnownSafetyVehicleComponent,
        KnownPowerVehicleComponent,
        UnknownVehicleComponent // Firmware specific vehicle components
    };
    Q_ENUM(KnownVehicleComponent)

    /// Called when parameters are ready for the first time. Note that parameters may still be missing.
    /// Overrides must call base class.
    virtual void parametersReadyPreChecks();

    // Must be implemented by derived class
    virtual const QVariantList &vehicleComponents() = 0;

    /// Returns the name of the vehicle component which must complete setup prior to this one. Empty string for none.
    Q_INVOKABLE virtual QString prerequisiteSetup(VehicleComponent *component) const = 0;

    /// Returns true if the vehicle component is available for the vehicle. Customs build have different components.
    Q_INVOKABLE bool knownVehicleComponentAvailable(KnownVehicleComponent knownVehicleComponent) { return (findKnownVehicleComponent(knownVehicleComponent) != nullptr); }

    /// Returns the VehicleComponent for the knownVehicleComponent. Returns nullptr if not available.
    Q_INVOKABLE VehicleComponent *findKnownVehicleComponent(KnownVehicleComponent knownVehicleComponent);

    bool setupComplete() const { return _setupComplete; }

signals:
    void setupCompleteChanged();
    void vehicleComponentsChanged();

protected:
    Vehicle *_vehicle = nullptr;
    FirmwarePlugin *_firmwarePlugin = nullptr;
    bool _setupComplete = false;

private slots:
    void _recalcSetupComplete();
};
