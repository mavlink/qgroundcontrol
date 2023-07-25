/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef AUTOPILOTPLUGIN_H
#define AUTOPILOTPLUGIN_H

#include <QObject>
#include <QList>
#include <QString>
#include <QQmlContext>

#include "VehicleComponent.h"
#include "FactSystem.h"
#include "Vehicle.h"

class Vehicle;
class FirmwarePlugin;

/// This is the base class for AutoPilot plugins
///
/// The AutoPilotPlugin class is an abstract base class which represent the methods and objects
/// which are specific to a certain AutoPilot. This is the only place where AutoPilot specific
/// code should reside in QGroundControl. The remainder of the QGroundControl source is
/// generic to a common mavlink implementation.

class AutoPilotPlugin : public QObject
{
    Q_OBJECT

public:
    AutoPilotPlugin(Vehicle* vehicle, QObject* parent);
    ~AutoPilotPlugin();

    Q_PROPERTY(QVariantList vehicleComponents   READ vehicleComponents  NOTIFY vehicleComponentsChanged)    ///< List of VehicleComponent objects
    Q_PROPERTY(bool         setupComplete       READ setupComplete      NOTIFY setupCompleteChanged)        ///< false: One or more vehicle components require setup

    /// Called when parameters are ready for the first time. Note that parameters may still be missing.
    /// Overrides must call base class.
    virtual void parametersReadyPreChecks(void);

    // Must be implemented by derived class
    virtual const QVariantList& vehicleComponents(void) = 0;

    /// Returns the name of the vehicle component which must complete setup prior to this one. Empty string for none.
    Q_INVOKABLE virtual QString prerequisiteSetup(VehicleComponent* component) const = 0;

    // Property accessors
    bool setupComplete(void) const;

signals:
    void setupCompleteChanged(bool setupComplete);
    void vehicleComponentsChanged(void);

protected:
    /// All access to AutoPilotPugin objects is through getInstanceForAutoPilotPlugin
    AutoPilotPlugin(QObject* parent = nullptr) : QObject(parent) { }

    Vehicle*        _vehicle;
    FirmwarePlugin* _firmwarePlugin;
    bool            _setupComplete;

private slots:
    void _recalcSetupComplete(void);
};

#endif
