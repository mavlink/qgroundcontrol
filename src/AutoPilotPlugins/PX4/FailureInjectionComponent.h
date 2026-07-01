/****************************************************************************
 * FailureInjectionComponent.h
 * Vehicle Setup component wrapper for the Failure Injection QML page.
 ****************************************************************************/
#pragma once

#include "VehicleComponent.h"

class FailureInjectionComponent : public VehicleComponent
{
    Q_OBJECT
public:
    FailureInjectionComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    QString name(void) const override;
    QString description(void) const override;
    QString iconResource(void) const override;

    bool requiresSetup(void) const override { return false; }

    bool setupComplete(void) const override { return true; }

    QStringList setupCompleteChangedTriggerList(void) const override { return QStringList(); }

    QUrl setupSource(void) const override;

    QUrl summaryQmlSource(void) const override { return QUrl(); }

    // Failure injection is meant to be exercised in flight (validate failsafes/EKF), so keep the page usable while
    // armed.
    bool allowSetupWhileArmed(void) const override { return true; }

    bool allowSetupWhileFlying(void) const override { return true; }

private:
    const QString _name;
};
